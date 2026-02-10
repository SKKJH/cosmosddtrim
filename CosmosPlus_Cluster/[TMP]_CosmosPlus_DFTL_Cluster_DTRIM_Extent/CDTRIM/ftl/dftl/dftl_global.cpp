/*******************************************************
*
* Copyright (C) 2018-2019 
* Embedded Software Laboratory(ESLab), SUNG KYUN KWAN UNIVERSITY
* * This file is part of ESLab's Flash memory firmware
* * This source can not be copied and/or distributed without the express
* permission of ESLab
*
* Author: DongYoung Seo (dongyoung.seo@gmail.com)
* Kyuhwa Han (hgh6877@gmail.com)
* ESLab: http://nyx.skku.ac.kr
*
*******************************************************/

#include "hil.h"
#include "dftl_internal.h"

// [Helper] 메모리 초기화 및 주소 계산 헬퍼
static inline void memset_volatile_u8(volatile UINT8* dst, UINT8 value, size_t len)
{
	for (size_t i = 0; i < len; ++i) dst[i] = value;
}

static inline UINT32 seg_base_lpn(UINT32 seg_idx) { return seg_idx * 1024; }
static inline UINT32 seg_end_lpn (UINT32 seg_idx) { return seg_base_lpn(seg_idx) + 1024 - 1; }
static inline UINT32 byte_idx_from_lpn(UINT32 lpn) { return lpn / 8; }
static inline UINT32 bit_off_from_lpn (UINT32 lpn) { return lpn % 8; }

#define GET_TRIM_BIT(lpn) ((g_trim_bitmap[(lpn)/8] >> ((lpn)%8)) & 0x1)

// 전역 TRIM 변수 정의
volatile UINT8* g_trim_bitmap 		= NULL;
volatile UINT16* g_trim_seg_count	= NULL;
volatile UINT32 g_trim_pending 		= 0;
volatile UINT32 g_trim_going 		= 0;

#define TRIM_BITMAP_BYTES ((TOTAL_LPN_COUNT + 7ull) / 8ull)
#define SEGMENT_COUNT     (TOTAL_LPN_COUNT / LPNS_PER_SEGMENT)
#define LPNS_PER_SEGMENT  (1024)

// 비교 함수에서 참조하기 위해 static 전역으로 선언
static UINT32 g_clusterUtils[USER_CLUSTERS];

// ----------------------------------------------------------------------------
// [DFTL_GLOBAL] 통합 TRIM 노드 할당/해제 및 교체 정책 구현
// ----------------------------------------------------------------------------

TRIM_NODE* DFTL_GLOBAL::AllocTrimNode(UINT32 nLength, UINT32 nRequestClusterID)
{
	// 1. 전역 프리 리스트에 여유가 있는 경우 (O(1))
	if (m_pGlobalFreeListHead != NULL) {
		TRIM_NODE* pNode = m_pGlobalFreeListHead;
		m_pGlobalFreeListHead = pNode->m_pNextLPN;
		pNode->Reset();
		m_nGlobalUsedNodeCount++;
		return pNode;
	}

	// 2. 노드 고갈 시 교체 대상(Victim) 선정
	TRIM_NODE* pVictim = NULL;
	UINT32 nVictimClusterID = 0xFFFFFFFF;

	// [우선순위 1] 현재 요청한 클러스터 내에 파편화된 노드(Length 1)가 있는지 우선 확인
	for (int i = 0; i < TRIM_HASH_SIZE; i++) {
		TRIM_NODE* pTail = m_stTrimMgr[nRequestClusterID].m_RangeHashTableTail[i];
		if (pTail != NULL) {
			if (pTail->m_nLength == 1) {
				pVictim = pTail;
				nVictimClusterID = nRequestClusterID;
			}
			break; // 해당 클러스터의 가장 작은 단위 확인 완료
		}
	}

	// [우선순위 2] 현재 클러스터에 Len 1이 없다면, 전체 클러스터를 뒤져서 전역에서 가장 작은 노드 검색
	if (pVictim == NULL) {
		for (int i = 0; i < TRIM_HASH_SIZE; i++) {
			TRIM_NODE* pBestInTier = NULL;
			UINT32 nBestClusterInTier = 0;

			for (int c = 0; c < USER_CLUSTERS; c++) {
				TRIM_NODE* pTail = m_stTrimMgr[c].m_RangeHashTableTail[i];
				if (pTail != NULL) {
					if (pBestInTier == NULL || pTail->m_nLength < pBestInTier->m_nLength) {
						pBestInTier = pTail;
						nBestClusterInTier = c;
					}
				}
			}

			if (pBestInTier != NULL) {
				// [최적화] 발견된 최소 노드가 현재 요청보다 크거나 같으면, 교체 불가 -> 즉시 종료
				if (pBestInTier->m_nLength >= nLength) {
					return NULL;
				}

				// 교체 대상 선정 성공
				pVictim = pBestInTier;
				nVictimClusterID = nBestClusterInTier;
				break;
			}
		}
	}

	// 3. 교체 실행 결정
	if (pVictim != NULL) {
		// (최적화 로직 덕분에 여기서는 무조건 nLength > pVictim->m_nLength 임이 보장됨)

		// [1] Victim 클러스터 리스트에서 제거
		m_stTrimMgr[nVictimClusterID]._RemoveFromLists(pVictim);

		// [2] [버그 수정] Victim 클러스터의 사용량 감소 (뺏기는 쪽)
		if(m_stTrimMgr[nVictimClusterID].m_nUsedNodeCount > 0)
			m_stTrimMgr[nVictimClusterID].m_nUsedNodeCount--;

		pVictim->Reset();

		// m_nGlobalUsedNodeCount는 유지됨 (전체 개수는 -1 +1 = 0 변동 없음)
		return pVictim;
	}

	return NULL; // 할당 실패
}

void DFTL_GLOBAL::FreeTrimNode(TRIM_NODE* pNode)
{
	if (pNode == NULL) return;

	pNode->Reset();
	// 전역 프리 리스트 반납
	pNode->m_pNextLPN = m_pGlobalFreeListHead;
	m_pGlobalFreeListHead = pNode;
	m_nGlobalUsedNodeCount--;
}

// ----------------------------------------------------------------------------
// TRIM_MGR Implementation
// ----------------------------------------------------------------------------

TRIM_MGR::TRIM_MGR()
{
	m_nUsedNodeCount = 0;
	for (int i = 0; i < TRIM_HASH_SIZE; i++) {
		m_LPNHashTable[i] = NULL;
		m_RangeHashTable[i] = NULL;
		m_RangeHashTableTail[i] = NULL;
	}
}

void TRIM_MGR::InvalidateOverlap(UINT32 nWriteStart, UINT32 nWriteLength)
{
    UINT32 nWriteEnd = nWriteStart + nWriteLength;
    UINT32 nEndBucketIdx = _GetLPNHash(nWriteEnd - 1);

    // [최적화 1] 검색 시작 버킷 계산
    UINT32 nSafeStartLPN = (nWriteStart > m_nMaxTrimLength) ? (nWriteStart - m_nMaxTrimLength) : 0;
    UINT32 nStartBucketIdx = _GetLPNHash(nSafeStartLPN);

    for (UINT32 i = nStartBucketIdx; i <= nEndBucketIdx; i++)
    {
        TRIM_NODE* pCurr = m_LPNHashTable[i];

        while (pCurr != NULL)
        {
            // [최적화 2] 조기 종료
            if (pCurr->m_nStartLPN >= nWriteEnd) break;

            TRIM_NODE* pNext = pCurr->m_pNextLPN;
            UINT32 nTrimStart = pCurr->m_nStartLPN;
            UINT32 nTrimEnd = nTrimStart + pCurr->m_nLength;

            if (nTrimEnd <= nWriteStart) {
                pCurr = pNext;
                continue;
            }

            // Case 1: Delete
            if (nWriteStart <= nTrimStart && nWriteEnd >= nTrimEnd)
            {
                _RemoveFromLists(pCurr);
                FreeNode(pCurr); // 내부에서 DecreaseCMTCount 호출됨
            }
            // Case 2: Split
            else if (nWriteStart > nTrimStart && nWriteEnd < nTrimEnd)
            {
                // [수정] 잘려나가는 길이(WriteLength) 만큼 Pending 양 감소
                if (m_nPendingTrimPages >= nWriteLength) m_nPendingTrimPages -= nWriteLength;
                else m_nPendingTrimPages = 0;

                // [수정] CMT Count 차감
                DecreaseCMTCount(pCurr->m_nStartLPN, nWriteLength);

                UINT32 nOldTrimEnd = nTrimEnd;

                // 2-1. 기존 노드 수정
                _RemoveFromLists(pCurr);
                pCurr->m_nLength = nWriteStart - nTrimStart;
                _AddToLPNList(_GetLPNHash(pCurr->m_nStartLPN), pCurr);
                _AddToRangeList(pCurr);

                // 2-2. 새 노드 할당
                TRIM_NODE* pSplitNode = AllocNode(nOldTrimEnd - nWriteEnd);
                if (pSplitNode) {
                    pSplitNode->m_nStartLPN = nWriteEnd;
                    pSplitNode->m_nLength = nOldTrimEnd - nWriteEnd;
                    _AddToLPNList(_GetLPNHash(pSplitNode->m_nStartLPN), pSplitNode);
                    _AddToRangeList(pSplitNode);
                    // [중요] AllocNode는 단순히 메모리 할당만 하므로,
                    // 잘려나간 뒷부분에 대해 IncreaseCMTCount는 호출할 필요 없음.
                    // (원래 pCurr에 포함되어 있던 영역이므로)
                }
                else {
                    // 할당 실패 시: 뒷부분 유실 -> 추가 차감
                    UINT32 nLostLen = nOldTrimEnd - nWriteEnd;
                    if (m_nPendingTrimPages >= nLostLen) m_nPendingTrimPages -= nLostLen;
                    else m_nPendingTrimPages = 0;

                    DecreaseCMTCount(nWriteEnd, nLostLen);
                }
            }
            // Case 3: Shrink Tail
            else if (nWriteStart > nTrimStart)
            {
                UINT32 nRemovedLen = nTrimEnd - nWriteStart;
                if (m_nPendingTrimPages >= nRemovedLen) m_nPendingTrimPages -= nRemovedLen;
                else m_nPendingTrimPages = 0;

                DecreaseCMTCount(pCurr->m_nStartLPN, nRemovedLen);

                _RemoveFromLists(pCurr);
                pCurr->m_nLength = nWriteStart - nTrimStart;
                _AddToLPNList(_GetLPNHash(pCurr->m_nStartLPN), pCurr);
                _AddToRangeList(pCurr);
            }
            // Case 4: Shrink Head
            else
            {
                UINT32 nRemovedLen = nWriteEnd - nTrimStart;
                if (m_nPendingTrimPages >= nRemovedLen) m_nPendingTrimPages -= nRemovedLen;
                else m_nPendingTrimPages = 0;

                DecreaseCMTCount(pCurr->m_nStartLPN, nRemovedLen);

                _RemoveFromLists(pCurr);
                pCurr->m_nStartLPN = nWriteEnd;
                pCurr->m_nLength = nTrimEnd - nWriteEnd;
                _AddToLPNList(_GetLPNHash(pCurr->m_nStartLPN), pCurr);
                _AddToRangeList(pCurr);
            }

            pCurr = pNext;
        }
    }
}

VOID TRIM_MGR::Initialize(UINT32 clusterID)
{
	m_nUsedNodeCount = 0;
	m_nClusterID = clusterID;
	m_nMaxTrimLength = 0;
	m_nPendingTrimPages = 0;

	m_pCMTListHead = NULL;
	m_nCMTGroupCount = 0;

	// 개별 메모리 할당 제거 (DFTL_GLOBAL 통합 관리)

	for (int i = 0; i < TRIM_HASH_SIZE; i++)
	{
		m_LPNHashTable[i] = NULL;
		m_RangeHashTable[i] = NULL;
		m_RangeHashTableTail[i] = NULL;
	}

	PrintInfo();
}

#ifndef GET_META_LPN
#define GET_META_LPN(lpn) ((lpn) / L2V_PER_META_PAGE)
#endif

void TRIM_MGR::IncreaseCMTCount(UINT32 nStartLPN, UINT32 nLength)
{
    UINT32 nMetaLPN = GET_META_LPN(nStartLPN);

    // 1. 리스트 검색 (이미 존재하는 그룹인지)
    CMT_GROUP_NODE* pCurr = m_pCMTListHead;
    while (pCurr) {
        if (pCurr->m_nMetaLPN == nMetaLPN) {
            pCurr->m_nTotalTrim += nLength;
            return;
        }
        pCurr = pCurr->m_pNext;
    }

    // 2. 없으면 신규 할당 (전역 풀 사용)
    CMT_GROUP_NODE* pNew = DFTL_GLOBAL::GetInstance()->AllocCMTGroupNode();
    if (pNew) {
        pNew->m_nMetaLPN = nMetaLPN;
        pNew->m_nTotalTrim = nLength;

        // 리스트 헤드에 삽입
        pNew->m_pNext = m_pCMTListHead;
        if (m_pCMTListHead) m_pCMTListHead->m_pPrev = pNew;
        m_pCMTListHead = pNew;
        m_nCMTGroupCount++;
    }
}

void TRIM_MGR::DecreaseCMTCount(UINT32 nStartLPN, UINT32 nLength)
{
    UINT32 nMetaLPN = GET_META_LPN(nStartLPN);
    CMT_GROUP_NODE* pCurr = m_pCMTListHead;

    while (pCurr) {
        if (pCurr->m_nMetaLPN == nMetaLPN) {
            if (pCurr->m_nTotalTrim > nLength) {
                pCurr->m_nTotalTrim -= nLength;
            } else {
                // 0이 되면 리스트에서 제거 및 반납
                if (pCurr->m_pPrev) pCurr->m_pPrev->m_pNext = pCurr->m_pNext;
                else m_pCMTListHead = pCurr->m_pNext;

                if (pCurr->m_pNext) pCurr->m_pNext->m_pPrev = pCurr->m_pPrev;

                DFTL_GLOBAL::GetInstance()->FreeCMTGroupNode(pCurr);
                if (m_nCMTGroupCount > 0) m_nCMTGroupCount--;
            }
            return;
        }
        pCurr = pCurr->m_pNext;
    }
    // 못 찾은 경우: 이미 GC가 Pop해서 처리 중인 상태이므로 무시 (정상 동작)
}

CMT_GROUP_NODE* TRIM_MGR::PopBestCMTGroup()
{
    if (m_pCMTListHead == NULL) return NULL;

    CMT_GROUP_NODE* pBest = m_pCMTListHead;
    CMT_GROUP_NODE* pCurr = m_pCMTListHead->m_pNext;

    // Max 값 탐색 (선형 탐색)
    while (pCurr) {
        if (pCurr->m_nTotalTrim > pBest->m_nTotalTrim) {
            pBest = pCurr;
        }
        pCurr = pCurr->m_pNext;
    }

    // 리스트에서 분리 (Detach)
    if (pBest->m_pPrev) pBest->m_pPrev->m_pNext = pBest->m_pNext;
    else m_pCMTListHead = pBest->m_pNext;

    if (pBest->m_pNext) pBest->m_pNext->m_pPrev = pBest->m_pPrev;

    pBest->m_pNext = NULL;
    pBest->m_pPrev = NULL;
    if (m_nCMTGroupCount > 0) m_nCMTGroupCount--;

    return pBest;
}

// ----------------------------------------------------------------------------
// TRIM_MGR::PrintInfo
// ----------------------------------------------------------------------------
VOID TRIM_MGR::PrintInfo()
{
	UINT32 nClusterID = m_nClusterID;
	xil_printf("============================================================\r\n");
	xil_printf("[TRIM_MGR] Cluster %u Configured (Using Global Pool)\r\n", nClusterID);
	xil_printf("------------------------------------------------------------\r\n");
	xil_printf("============================================================\r\n\r\n");
}

VOID TRIM_MGR::VisualPrintBuckets()
{
	xil_printf("--- [Cluster %u] Trim Extent Visualization ---\r\n", m_nClusterID);
	UINT32 nActiveBuckets = 0;

//	for (UINT32 i = 0; i < TRIM_HASH_SIZE; i++)
//	{
//		TRIM_NODE* pCurr = m_LPNHashTable[i];
//
//		if (pCurr != NULL)
//		{
//			nActiveBuckets++;
//			xil_printf(" [Bucket %4d]: ", i);
//
//			while (pCurr != NULL)
//			{
//				xil_printf("[%u, %u]%s", pCurr->m_nStartLPN, pCurr->m_nLength, (pCurr->m_pNextLPN ? " <-> " : ""));
//				pCurr = pCurr->m_pNextLPN;
//			}
//			xil_printf("\r\n");
//		}
//	}

	if (nActiveBuckets == 0)
	{
		xil_printf(" (No pending Trim nodes in this cluster)\r\n");
	}

	UINT32 nGlobalUsed = DFTL_GLOBAL::GetInstance()->m_nGlobalUsedNodeCount;
	UINT32 nGlobalFree = GLOBAL_TRIM_POOL_SIZE - nGlobalUsed;

	xil_printf(" >> [Local Active]: %u | [Global Free]: %u / %d\r\n",
			m_nUsedNodeCount, nGlobalFree, GLOBAL_TRIM_POOL_SIZE);
	xil_printf("----------------------------------------------\r\n");
}

VOID DFTL_GLOBAL::VisualPrintAllTrimNodes()
{
	xil_printf("\r\n############################################################\r\n");
	xil_printf("   GLOBAL TRIM EXTENT STATE VISUALIZATION\r\n");
	xil_printf("   (LPN Hash Bucket-wise Distribution)\r\n");
	xil_printf("############################################################\r\n");

	for (UINT32 i = 0; i < USER_CLUSTERS; i++)
	{
		m_stTrimMgr[i].VisualPrintBuckets();
	}

	xil_printf("Total Used Nodes Across All Clusters: %d\r\n", m_nGlobalUsedNodeCount);
	xil_printf("############################################################\r\n\r\n");
}

TRIM_NODE* TRIM_MGR::AllocNode(UINT32 nLength)
{
	TRIM_NODE* pNode = DFTL_GLOBAL::GetInstance()->AllocTrimNode(nLength, m_nClusterID);

	if (pNode != NULL) {
		m_nUsedNodeCount++;
	}

	TrimPending_Set();
	return pNode;
}

void TRIM_MGR::FreeNode(TRIM_NODE* pNode)
{
	if (pNode == NULL) {
		return;
	}
	DecreaseCMTCount(pNode->m_nStartLPN, pNode->m_nLength);

	if (m_nPendingTrimPages >= pNode->m_nLength) {
		m_nPendingTrimPages -= pNode->m_nLength;
	} else {
		m_nPendingTrimPages = 0; // Underflow 방지
	}

	DFTL_GLOBAL::GetInstance()->FreeTrimNode(pNode);
	if(m_nUsedNodeCount > 0)
		m_nUsedNodeCount--;
}

UINT32 TRIM_MGR::_GetLPNHash(UINT32 nLPN)
{
	DFTL_GLOBAL* pGlobal = DFTL_GLOBAL::GetInstance();
	UINT32 nTotalLPN = pGlobal->GetLPNCount();
	UINT32 nLPNsPerCluster = nTotalLPN / USER_CLUSTERS;
	UINT32 nClusterBaseLPN = m_nClusterID * nLPNsPerCluster;
	UINT32 nLPNsPerBucket = nLPNsPerCluster / TRIM_HASH_SIZE;

	if (nLPNsPerBucket == 0) return 0;

	UINT32 nRelativeLPN = (nLPN >= nClusterBaseLPN) ? (nLPN - nClusterBaseLPN) : 0;
	UINT32 nHashIdx = nRelativeLPN / nLPNsPerBucket;

	return (nHashIdx >= TRIM_HASH_SIZE) ? (TRIM_HASH_SIZE - 1) : nHashIdx;
}

/* TRIM_MGR 클래스에 추가 (dftl_global.h / cpp) */

// [최적화] StartLPN이 변하지 않을 때, Range 리스트만 갱신하는 함수
void TRIM_MGR::_UpdateNodeLengthOnly(TRIM_NODE* pNode, UINT32 nNewLength)
{
    // 1. 기존 Range List에서 제거
    UINT32 nOldRangeHash = _GetRangeHash(pNode->m_nLength);

    if (pNode->m_pPrevRange) pNode->m_pPrevRange->m_pNextRange = pNode->m_pNextRange;
    else m_RangeHashTable[nOldRangeHash] = pNode->m_pNextRange;

    if (pNode->m_pNextRange) pNode->m_pNextRange->m_pPrevRange = pNode->m_pPrevRange;
    else m_RangeHashTableTail[nOldRangeHash] = pNode->m_pPrevRange;

    // 2. 길이 변경
    pNode->m_nLength = nNewLength;

    // 3. 새 Range List에 삽입 (기존 _AddToRangeList 재사용)
    _AddToRangeList(pNode);
}

// 모든 해시 리스트에서 노드 연결 해제
void TRIM_MGR::_RemoveFromLists(TRIM_NODE* pNode)
{
	// 1. LPN Hash List에서 제거
	UINT32 nLPNHash = _GetLPNHash(pNode->m_nStartLPN);
	if (pNode->m_pPrevLPN) pNode->m_pPrevLPN->m_pNextLPN = pNode->m_pNextLPN;
	else m_LPNHashTable[nLPNHash] = pNode->m_pNextLPN;

	if (pNode->m_pNextLPN) pNode->m_pNextLPN->m_pPrevLPN = pNode->m_pPrevLPN;

	// 2. Range Hash List에서 제거
	UINT32 nRangeHash = _GetRangeHash(pNode->m_nLength);
	if (pNode->m_pPrevRange) pNode->m_pPrevRange->m_pNextRange = pNode->m_pNextRange;
	else m_RangeHashTable[nRangeHash] = pNode->m_pNextRange;

	if (pNode->m_pNextRange) pNode->m_pNextRange->m_pPrevRange = pNode->m_pPrevRange;
	else m_RangeHashTableTail[nRangeHash] = pNode->m_pPrevRange;

	// 포인터 초기화
	pNode->m_pPrevLPN = pNode->m_pNextLPN = NULL;
	pNode->m_pPrevRange = pNode->m_pNextRange = NULL;
}

// LPN Hash List 정렬 삽입 (오름차순)
void TRIM_MGR::_AddToLPNList(UINT32 nHashIdx, TRIM_NODE* pNew)
{
	TRIM_NODE* pPrev = NULL;
	TRIM_NODE* pCurr = m_LPNHashTable[nHashIdx];

	while (pCurr != NULL && pCurr->m_nStartLPN < pNew->m_nStartLPN) {
		pPrev = pCurr;
		pCurr = pCurr->m_pNextLPN;
	}

	pNew->m_pNextLPN = pCurr;
	pNew->m_pPrevLPN = pPrev;

	if (pPrev) pPrev->m_pNextLPN = pNew;
	else m_LPNHashTable[nHashIdx] = pNew;

	if (pCurr) pCurr->m_pPrevLPN = pNew;
}

// Range Hash List 정렬 삽입 (내림차순: 큰 순서)
void TRIM_MGR::_AddToRangeList(TRIM_NODE* pNew)
{
	UINT32 nRangeHash = _GetRangeHash(pNew->m_nLength);
	TRIM_NODE* pPrev = NULL;
	TRIM_NODE* pCurr = m_RangeHashTable[nRangeHash];

	// 큰 순서대로 탐색 (같거나 작으면 정지)
	while (pCurr != NULL && pCurr->m_nLength > pNew->m_nLength) {
		pPrev = pCurr;
		pCurr = pCurr->m_pNextRange;
	}

	pNew->m_pNextRange = pCurr;
	pNew->m_pPrevRange = pPrev;

	if (pPrev) pPrev->m_pNextRange = pNew;
	else m_RangeHashTable[nRangeHash] = pNew;

	if (pCurr) pCurr->m_pPrevRange = pNew;
	else m_RangeHashTableTail[nRangeHash] = pNew;
}

void TRIM_MGR::InsertTrim(UINT32 nStartLPN, UINT32 nLength)
{
	// Fast Drop: 1개짜리 요청인데 풀이 꽉 찼으면 병합 시도조차 하지 않음
	if (nLength == 1 && DFTL_GLOBAL::GetInstance()->m_pGlobalFreeListHead == NULL) {
		return;
	}

	if (nLength > m_nMaxTrimLength) {
		m_nMaxTrimLength = nLength;
	}
	m_nPendingTrimPages += nLength;

	UINT32 nHashIdx = _GetLPNHash(nStartLPN);
	TRIM_NODE* pCurr = m_LPNHashTable[nHashIdx];
	TRIM_NODE* pPrev = NULL;

	// 삽입 위치 탐색
	while (pCurr != NULL && pCurr->m_nStartLPN < nStartLPN) {
		pPrev = pCurr;
		pCurr = pCurr->m_pNextLPN;
	}

	TRIM_NODE* pNext = pCurr;
	BOOL bMerged = FALSE;

	// [Case 1] Prev 노드와 병합 (Overlap/Adjacent)
	if (pPrev != NULL && (pPrev->m_nStartLPN + pPrev->m_nLength) >= nStartLPN) {
		_RemoveFromLists(pPrev);

		UINT32 nNewEnd = (nStartLPN + nLength > pPrev->m_nStartLPN + pPrev->m_nLength)
						 ? (nStartLPN + nLength) : (pPrev->m_nStartLPN + pPrev->m_nLength);
		pPrev->m_nLength = nNewEnd - pPrev->m_nStartLPN;

		_AddToLPNList(nHashIdx, pPrev);
		_AddToRangeList(pPrev);
		bMerged = TRUE;

		// 연쇄 병합: 확장된 Prev가 Next와도 닿는 경우
		if (pNext != NULL && (pPrev->m_nStartLPN + pPrev->m_nLength) >= pNext->m_nStartLPN) {
			_RemoveFromLists(pPrev);
			_RemoveFromLists(pNext);

			UINT32 nFinalEnd = (pNext->m_nStartLPN + pNext->m_nLength > pPrev->m_nStartLPN + pPrev->m_nLength)
							   ? (pNext->m_nStartLPN + pNext->m_nLength) : (pPrev->m_nStartLPN + pPrev->m_nLength);
			pPrev->m_nLength = nFinalEnd - pPrev->m_nStartLPN;

			_AddToLPNList(nHashIdx, pPrev);
			_AddToRangeList(pPrev);
			FreeNode(pNext);
		}
	}
	// [Case 2] Next 노드와 병합
	else if (pNext != NULL && (nStartLPN + nLength) >= pNext->m_nStartLPN) {
		_RemoveFromLists(pNext);

		UINT32 nNewEnd = (pNext->m_nStartLPN + pNext->m_nLength > nStartLPN + nLength)
						 ? (pNext->m_nStartLPN + pNext->m_nLength) : (nStartLPN + nLength);
		pNext->m_nStartLPN = nStartLPN;
		pNext->m_nLength = nNewEnd - nStartLPN;

		_AddToLPNList(nHashIdx, pNext);
		_AddToRangeList(pNext);
		bMerged = TRUE;
	}

	// [Case 3] 병합되지 않은 경우 신규 삽입
	if (!bMerged) {
		TRIM_NODE* pNew = AllocNode(nLength);
		if (pNew == NULL)
		{
			// 교체(Replacement) 실패 또는 Drop
			return;
		}
		if (pNew) {
			pNew->m_nStartLPN = nStartLPN;
			pNew->m_nLength = nLength;
			_AddToLPNList(nHashIdx, pNew);
			_AddToRangeList(pNew);
		}
	}
	IncreaseCMTCount(nStartLPN, nLength);
}

DFTL_GLOBAL* DFTL_GLOBAL::m_pstInstance;

// ----------------------------------------------------------------------------
//  DFTL_GLOBAL Member Functions
// ----------------------------------------------------------------------------

VOID DFTL_GLOBAL::WritePtr_GetAndAdvance(UINT32 clusterID)
{
	m_page_cnt[clusterID]++;
	if (m_page_cnt[clusterID] >= LPN_PER_PHYSICAL_PAGE) {
		m_page_cnt[clusterID] = 0;
		m_wp_ch[clusterID]++;
		if (m_wp_ch[clusterID] >= USER_CHANNELS) {
			m_wp_ch[clusterID] = 0;
			m_wp_wy[clusterID]++;
			if (m_wp_wy[clusterID] >= USER_WAYS) {
				m_wp_wy[clusterID] = 0;
			}
		}
	}
}

VOID DFTL_GLOBAL::GC_WritePtr_GetAndAdvance(UINT32 clusterID)
{
	m_gc_page_cnt[clusterID]++;
	if (m_gc_page_cnt[clusterID] >= LPN_PER_PHYSICAL_PAGE) {
		m_gc_page_cnt[clusterID] = 0;
		m_gc_wp_ch[clusterID]++;
		if (m_gc_wp_ch[clusterID] >= USER_CHANNELS) {
			m_gc_wp_ch[clusterID] = 0;
			m_gc_wp_wy[clusterID]++;
			if (m_gc_wp_wy[clusterID] >= USER_WAYS) {
				m_gc_wp_wy[clusterID] = 0;
			}
		}
	}
}

VOID DFTL_GLOBAL::DebugBlockPrint(UINT32 FLAG)
{
	GetUserBlockMgr()->DebugPrintAllByVBN(FLAG);
}

UINT32 DFTL_GLOBAL::GetClusterID(UINT32 LPN)
{
	UINT64 num	= (UINT64)LPN * (UINT64)USER_CLUSTERS;
	UINT32 cid	= (UINT32)(num / (UINT64)m_nLPNCount);

	if (cid >= USER_CLUSTERS)
	{
		xil_printf("Wrong CID\r\n");
		assert(0);
	}
	return cid;
}

VOID DFTL_GLOBAL::SB_INIT()
{
	SBINFO_MGR* sbm = GetSBInfoMgr();
	INT32 c = DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();

	INIT_LIST_HEAD(&sbm->m_dlFreeList);
	sbm->m_nFreeCount = 0;

	for (INT32 vbn = 0; vbn < c; vbn++)
	{
		SBINFO* sb = &sbm->m_pastSBInfo[vbn];
		INIT_LIST_HEAD(&sb->m_dlList);

		if (sb->m_bBad != 0)	continue;
		if (sb->m_bMeta)		continue;
		if (sb->m_nUSED != 0) continue;

		list_add_tail(&sb->m_dlList, &sbm->m_dlFreeList);
		sbm->m_nFreeCount++;
	}
	SB_INIT_FLAG = TRUE;
}

VIRTUAL VOID DFTL_GLOBAL::StartGCMonitor(UINT32 nTotalReq, UINT32 nWindowSize) {
	m_bMonitorOn = TRUE;
	m_nMonitorTotalReq = nTotalReq;
	m_nWindowSize = nWindowSize;
	m_nMonitorCurReq = 0;
	m_nWindowHit = 0;
	xil_printf("[MON] GC Done. Start Monitoring (Total: %d, Window: %d)\r\n", nTotalReq, nWindowSize);
}

VOID DFTL_GLOBAL::RecordHostAccess(BOOL bHit) {
	if (m_bMonitorOn == FALSE) return;

	m_nMonitorCurReq++;
	if (bHit) m_nWindowHit++;

	// 윈도우 단위(예: 100개)가 찰 때마다 로그 출력
	if (m_nMonitorCurReq % m_nWindowSize == 0) {
		float rate = (float)m_nWindowHit / (float)m_nWindowSize * 100.0f;
		xil_printf("[MON] Req %4d ~ %4d | HitRate: %3d%% (%d/%d)\r\n",
			m_nMonitorCurReq - m_nWindowSize + 1,
			m_nMonitorCurReq,
			(int)rate, m_nWindowHit, m_nWindowSize);

		// 윈도우 초기화
		m_nWindowHit = 0;
	}

	// 설정한 횟수만큼 다 찍었으면 모니터링 종료
	if (m_nMonitorCurReq >= m_nMonitorTotalReq) {
		m_bMonitorOn = FALSE;
		xil_printf("[MON] Monitoring End.\r\n");
	}
}

VIRTUAL VOID DFTL_GLOBAL::Initialize(VOID)
{
	m_bFormatCheck = TRUE;
	m_bMonitorOn = FALSE;
	m_nMonitorTotalReq = 0;
	m_nMonitorCurReq = 0;
	m_nWindowSize = 100; // 100개 단위로 로그 출력
	m_nWindowHit = 0;

	m_nHostReqCount = 0;
	m_nLastHostHit = 0;
	m_nLastHostMiss = 0;

	DS_CNT = 0;
	DS_Length = 0;
	m_nTotalPendingTrimCount = 0;
	SB_INIT_FLAG = FALSE;
	SB_PRINT_FLAG = FALSE;
	META_CNT = 0;

	nand_write_cnt = 0;
	host_write_cnt = 0;

	for (int i=0; i<USER_CLUSTERS; i++)
	{
		m_util_pages[i] = 0;
		m_util_blks[i] = 0;
		m_nTrimSize[i] = 0;

		m_wp_ch[i] = 0;
		m_wp_wy[i] = 0;
		m_page_cnt[i] = 0;

		m_gc_wp_ch[i] = 0;
		m_gc_wp_wy[i] = 0;
		m_gc_page_cnt[i] = 0;
	}

	m_pstInstance = this;
	_Initialize();
	GetVNandMgr()->Initialize();
	GetMetaMgr()->Initialize();
	GetMetaL2VMgr()->Initialize();

	TrimPending_Clear();

	for (int channel = 0; channel < USER_CHANNELS; channel++) {
		for (int way = 0; way < USER_WAYS; way++) {
			GetVBInfoMgr(channel, way)->Initialize();
		}
	}
	GetUserBlockMgr()->Initialize(USER_BLOCK_MGR);
#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaBlockMgr()->Initialize(META_BLOCK_MGR);
#endif

	for (int cluster = 0; cluster < USER_CLUSTERS; cluster++)
	{
		m_stTrimMgr[cluster].Initialize(cluster);
	}

	GetRequestMgr()->Initialize();
	GetBufferMgr()->Initialize();
	for (int channel = 0; channel < USER_CHANNELS; channel++) {
		for (int way = 0; way < USER_WAYS; way++) {
			for (int cluster = 0; cluster < USER_CLUSTERS; cluster++)
			{
				GetActiveBlockMgr(cluster, channel, way)->Initialize(cluster, channel, way);
			}
			GetGCMgr(channel, way)->Initialize(GetGCTh(), IOTYPE_GC, channel, way);
#if (SUPPORT_META_DEMAND_LOADING == 1)
			GetMetaGCMgr(channel, way)->Initialize(META_GC_THRESHOLD, IOTYPE_META, channel, way);
#endif
		}
	}
	_PrintInfo();
	m_MetaGCing = FALSE;

	GetReadCacheMgr()->Initialize();
	GetSBInfoMgr()->Initialize();
}

VIRTUAL BOOL
DFTL_GLOBAL::Format(VOID)
{
	BOOL	bRet;

	GetUserBlockMgr()->Format();

#if (SUPPORT_META_DEMAND_LOADING == 1)
	GetMetaBlockMgr()->Format();
	bRet = GetMetaL2VMgr()->Format();
#endif

	bRet = GetMetaMgr()->Format();

	return bRet;
}

VIRTUAL VOID
DFTL_GLOBAL::Run(VOID)
{
	FIL_Run();
	GetRequestMgr()->Run();

#if (SUPPORT_META_DEMAND_LOADING == 1)
	for (UINT32 channel = 0; channel < USER_CHANNELS; channel++) {
		for (UINT32 way = 0; way < USER_WAYS; way++) {
			GetMetaGCMgr(channel, way)->CheckAndStartGC();
		}
	}
#endif
	if (GetSuperGCMgr()->CheckAndStartGC())
	{
		for (UINT32 channel = 0; channel < USER_CHANNELS; channel++) {
			for (UINT32 way = 0; way < USER_WAYS; way++) {
				GetGCMgr(channel, way)->CheckAndStartGC();
			}
		}
	}

#if (SUPPORT_META_DEMAND_LOADING == 1)
	for (UINT32 channel = 0; channel < USER_CHANNELS; channel++) {
		for (UINT32 way = 0; way < USER_WAYS; way++) {
			GetMetaGCMgr(channel, way)->Run();
		}
	}
#endif
	for (UINT32 channel = 0; channel < USER_CHANNELS; channel++) {
		for (UINT32 way = 0; way < USER_WAYS; way++) {
			GetGCMgr(channel, way)->Run();
		}
	}
}

VIRTUAL VOID
DFTL_GLOBAL::ReadPage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	HIL_REQUEST*	pstRequest;

	if (nLPN == 77)
	{
		if ((nCount == 77) || (nCount == 78))
			DFTL_PrintProfile(0);
		else
			DFTL_PrintProfile(1);
		m_bEnable += 1;
	}

	do
	{
		// allocate request
		pstRequest = m_stRequestMgr.AllocateHILRequest();
		if (pstRequest == NULL)
		{
			Run();
		}
	} while (pstRequest == NULL);

	pstRequest->Initialize(HIL_REQUEST_READ_WAIT, NVME_CMD_OPCODE_READ,
						nLPN, nCmdSlotTag, nCount);
#ifndef WIN32
	//xil_printf("1	%u	%u \r\n", nLPN, nCount);
#endif
	m_stRequestMgr.AddToHILRequestWaitQ(pstRequest);

	DFTL_IncreaseProfile(Prof_Host_read, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_READ, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_READ_REQ);
}

VIRTUAL VOID
DFTL_GLOBAL::WritePage(UINT32 nCmdSlotTag, UINT32 nLPN, UINT32 nCount)
{
	HIL_REQUEST*	pstRequest;
	do
	{
		// allocate request
		pstRequest = m_stRequestMgr.AllocateHILRequest();
		if (pstRequest == NULL)
		{
			Run();
		}
	} while (pstRequest == NULL);

	pstRequest->Initialize(HIL_REQUEST_WRITE_WAIT, NVME_CMD_OPCODE_WRITE,
		nLPN, nCmdSlotTag, nCount);

#ifndef WIN32
	//xil_printf("7	%u	%u \r\n", nLPN, nCount);
#endif
	m_stRequestMgr.AddToHILRequestWaitQ(pstRequest);

	DFTL_IncreaseProfile(Prof_Host_write, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_WRITE, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_WRITE_REQ);
}

VIRTUAL VOID
DFTL_GLOBAL::DatasetManagement(UINT32 nCmdSlotTag, UINT32 nr, UINT32 ad)
{
	HIL_REQUEST*	pstRequest;

	do
	{
		// allocate request
		pstRequest = m_stRequestMgr.AllocateHILRequest();
		if (pstRequest == NULL)
		{
			Run();
		}
	} while (pstRequest == NULL);

	pstRequest->Initialize(HIL_REQUEST_DSM_WAIT, NVME_CMD_OPCODE_DSM,
		ad, nCmdSlotTag, nr);

	m_stRequestMgr.AddToHILRequestWaitQ(pstRequest);

	DFTL_IncreaseProfile(Prof_Host_Discard);
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_DSM, pstRequest->GetLPNCount());
	DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_DSM_REQ);
}

VIRTUAL VOID
DFTL_GLOBAL::CallBack(FTL_REQUEST_ID stReqID)
{
#if (UNIT_TEST_FIL_PERF == 1)
	return;
#endif

	switch (stReqID.stCommon.nType)
	{
	case FTL_REQUEST_ID_TYPE_HIL_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		HIL_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetHILRequestInfo();
		HIL_REQUEST_PER_WAY * pstRequest = pstRequestInfo->GetRequest_per_way(stReqID.stHIL.nRequestIndex);

		pstRequest->IncreaseDoneCount();
		pstRequest->pBufEntry[stReqID.stHIL.bufOffset]->readDone = 1;

		if (pstRequest->GetDoneCount() == pstRequest->GetLPNCount())
		{
			pstRequestInfo->RemoveFromIssuedQ_per_way(pstRequest);
			pstRequest->HDMAIssue();
			pstRequestInfo->AddToDoneQ_per_way(pstRequest);
			pstRequest->GoToNextStatus();
		}
		break;
	}
	case FTL_REQUEST_ID_TYPE_WRITE:
	{
		INT32	nIndex = stReqID.stProgram.nActiveBlockIndex;
		IOTYPE	eIOType = static_cast<IOTYPE>(stReqID.stProgram.nIOType);

		ACTIVE_BLOCK* pstActiveBlock = DFTL_GLOBAL::GetActiveBlockMgr(stReqID.cluster, stReqID.channel, stReqID.way)->GetActiveBlock(nIndex, eIOType);
		pstActiveBlock->ProgramDone(stReqID.stProgram.nBufferingIndex);
		break;
	}
	case FTL_REQUEST_ID_TYPE_GC_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		GC_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetGCRequestInfo();
		GC_REQUEST * pstRequest = pstRequestInfo->GetRequest(stReqID.stGC.nRequestIndex);
		pstRequest->GCReadDone();
		break;
	}
#if (SUPPORT_META_DEMAND_LOADING == 1)
	case FTL_REQUEST_ID_TYPE_META_READ:
	{
		REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
		META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
		META_REQUEST * pstRequest = pstRequestInfo->GetRequest(stReqID.stMeta.nRequestIndex);

		pstRequestInfo->RemoveFromIssuedQ(pstRequest);
		pstRequestInfo->AddToDoneQ(pstRequest);
		pstRequest->GoToNextStatus();

		break;
	}
#endif
	default:
		ASSERT(0);
		break;
	}
}

VIRTUAL VOID
DFTL_GLOBAL::IOCtl(IOCTL_TYPE eType)
{
	switch (eType)
	{
	case IOCTL_INIT_PROFILE_COUNT:
		m_stProfile.Initialize();
		break;

	case IOCTL_PRINT_PROFILE_COUNT:
		m_stProfile.Print();
		break;

	default:
		ASSERT(0);
		break;
	}
}

VOID
DFTL_GLOBAL::SetStatus(DFTL_STATUS eStatus)
{
	m_eStatus = static_cast<DFTL_STATUS>(m_eStatus | eStatus);
}

BOOL
DFTL_GLOBAL::CheckStatus(DFTL_STATUS eStatus)
{
	return (m_eStatus & eStatus) ? TRUE : FALSE;
}

VOID
DFTL_GLOBAL::_Initialize(VOID)
{
	UINT32 nPPagesPerVBlock = m_stVNand.GetPPagesPerVBlock();
	m_nPhysicalFlashSizeKB = USER_CHANNELS * USER_WAYS * m_stVNand.GetVBlockCount() * nPPagesPerVBlock * (PHYSICAL_PAGE_SIZE / KB);

	m_nVBlockSizeKB			= nPPagesPerVBlock * PHYSICAL_PAGE_SIZE;
	m_nVPagesPerVBlock		= m_stVNand.GetVPagesPerVBlock();
	m_nLPagesPerVBlockBits	= UTIL_GetBitCount(m_nVPagesPerVBlock);
	m_nLPagesPerVBlockMask	= (1 << m_nLPagesPerVBlockBits) - 1;

	m_fOverProvisionRatio = (float)OVERPROVISION_RATIO_DEFAULT;
	m_nOverprovisionSizeKB = (INT64)(m_nPhysicalFlashSizeKB * m_fOverProvisionRatio);
	m_nLogicalFlashSizeKB = m_nPhysicalFlashSizeKB - m_nOverprovisionSizeKB;

#if (SUPPORT_STATIC_DENSITY != 0)
	UINT32 nLogicalFlashSizeKB = SUPPORT_STATIC_DENSITY * (GB / KB);
	ASSERT(m_nLogicalFlashSizeKB >= nLogicalFlashSizeKB);
	m_nLogicalFlashSizeKB = nLogicalFlashSizeKB;
	ASSERT(m_nLogicalFlashSizeKB >= nLogicalFlashSizeKB);
#endif

	m_nLPNCount			= m_nLogicalFlashSizeKB / LOGICAL_PAGE_SIZE_KB;
//	xil_printf(" BEF LPN CNT: %u\r\n", m_nLPNCount);
	UINT32 nAlignment = USER_CLUSTERS * LPNS_PER_SEGMENT;
	m_nLPNCount = (m_nLPNCount / nAlignment) * nAlignment;
//	xil_printf(" AFT LPN CNT: %u\r\n", m_nLPNCount);

	m_nVBlockCount		= m_stVNand.GetVBlockCount();

#if (SUPPORT_META_BLOCK == 1)
	m_bEnableMetaBlock = TRUE;
#else
	m_bEnableMetaBlock = FALSE;
#endif

	m_nGCTh = FREE_BLOCK_GC_THRESHOLD_DEFAULT;

	// [수정] 통합 TRIM 노드 풀 할당 (10,000개 고정)
	UINT32 nPoolSize = sizeof(TRIM_NODE) * GLOBAL_TRIM_POOL_SIZE;
	m_pstTrimNodePool = (TRIM_NODE*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nPoolSize, OSAL_MEMALLOC_FW_ALIGNMENT);

	if (m_pstTrimNodePool != NULL) {
		memset((void*)m_pstTrimNodePool, 0, nPoolSize);
		// 전역 프리 리스트 연결
		for (int i = 0; i < GLOBAL_TRIM_POOL_SIZE; i++) {
			m_pstTrimNodePool[i].Reset();
			m_pstTrimNodePool[i].m_pNextLPN = (i < GLOBAL_TRIM_POOL_SIZE - 1) ? &m_pstTrimNodePool[i+1] : NULL;
		}
		m_pGlobalFreeListHead = &m_pstTrimNodePool[0];
		m_nGlobalUsedNodeCount = 0;
	}

	UINT32 nCMTPoolSize = sizeof(CMT_GROUP_NODE) * GLOBAL_TRIM_POOL_SIZE;
	m_pstCMTGroupPool = (CMT_GROUP_NODE*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nCMTPoolSize, OSAL_MEMALLOC_FW_ALIGNMENT);

	if (m_pstCMTGroupPool != NULL) {
		memset((void*)m_pstCMTGroupPool, 0, nCMTPoolSize);
		// 프리 리스트 연결
		for (int i = 0; i < GLOBAL_TRIM_POOL_SIZE; i++) {
			m_pstCMTGroupPool[i].Reset();
			m_pstCMTGroupPool[i].m_pNext = (i < GLOBAL_TRIM_POOL_SIZE - 1) ? &m_pstCMTGroupPool[i+1] : NULL;
		}
		m_pCMTGroupFreeListHead = &m_pstCMTGroupPool[0];
	}

	HIL_SetStorageBlocks(m_nLPNCount);
	m_stProfile.Initialize();
}

CMT_GROUP_NODE* DFTL_GLOBAL::AllocCMTGroupNode()
{
    if (m_pCMTGroupFreeListHead == NULL) return NULL; // 풀 고갈

    CMT_GROUP_NODE* pNode = m_pCMTGroupFreeListHead;
    m_pCMTGroupFreeListHead = pNode->m_pNext;
    pNode->Reset();
    return pNode;
}

void DFTL_GLOBAL::FreeCMTGroupNode(CMT_GROUP_NODE* pNode)
{
    if (pNode == NULL) return;
    pNode->Reset();
    // 프리 리스트 반납 (Head에 삽입)
    pNode->m_pNext = m_pCMTGroupFreeListHead;
    m_pCMTGroupFreeListHead = pNode;
}

VOID
DFTL_GLOBAL::_PrintInfo(VOID)
{
#if defined(FPM_FTL)
	char	psFTL[] = "FPMFTL";
#elif defined(DFTL)
	char	psFTL[] = "DFTL";
#else
#error check config
#endif
	PRINTF("[%s] Physical Density: %d MB \n\r", psFTL, m_nPhysicalFlashSizeKB / KB);
	PRINTF("[%s] Logical Density: %d MB \n\r", psFTL, m_nLogicalFlashSizeKB / KB);
}

VOID SBINFO_MGR::Initialize()
{
	INT32 nSize = sizeof(SBINFO) * DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();
	m_pastSBInfo = (SBINFO *)OSAL_MemAlloc(MEM_TYPE_FW_DATA, nSize, OSAL_MEMALLOC_FW_ALIGNMENT);
	m_nFreeCount = 0;

	int c = DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();
	for (int i=0; i<c; i++)
	{
		m_pastSBInfo[i].SetFree();
		m_pastSBInfo[i].m_nVBN = i;
		m_pastSBInfo[i].m_nUSED = 0;
		m_pastSBInfo[i].m_bBad = 0;
		m_pastSBInfo[i].m_bCID = USER_CLUSTERS;
		if (i<20)
		{
			m_pastSBInfo[i].m_bMeta = 1;
		}
		else
		{
			m_pastSBInfo[i].m_bMeta = 0;
		}
	}
}

VOID Read_Cache::Initialize()
{
	for (int i = 0; i < MAX_READ_CACHE_ENTRY; i++)
	{
		source_lpn[i] = 0xffffffff;
		nVPPN[i] = 0xffffffff;
		Buf[i] = DFTL_GLOBAL::GetBufferMgr()->Allocate();
		if (Buf[i] == NULL)
			ASSERT(0);
		Buf[i]->readDone = 1;
	}
}

BUFFER_ENTRY * Read_Cache::change_next_buffer(UINT32 src_lpn, BUFFER_ENTRY * input_buf, UINT32 channel, UINT32 way)
{
	BUFFER_ENTRY * ret;
	UINT32 iter;
	UINT32 start_offset, end_offset;
	way += channel << NUM_BIT_WAY;
	start_offset = way << READ_CAHCE_PER_WAY_BIT;
	end_offset = (way + 1) << READ_CAHCE_PER_WAY_BIT;

	for (iter = start_offset; iter < end_offset; iter++)
	{
		if (Buf[iter]->refCount == 0)
		{
			break;
		}
	}
	if (iter == end_offset)
		return NULL;

	ret = Buf[iter];
	source_lpn[iter] = src_lpn;
	Buf[iter] = input_buf;
	nVPPN[iter] = input_buf->nVPPN >> NUM_BIT_LPN_PER_PAGE;
	return ret;
}

BUFFER_ENTRY * Read_Cache::get_buffer_by_VPPN(UINT32 nVPPN_input)
{
	UINT32 channel, way;
	channel = CHANNEL_FROM_VPPN(nVPPN_input << NUM_BIT_LPN_PER_PAGE);
	way = WAY_FROM_VPPN(nVPPN_input << NUM_BIT_LPN_PER_PAGE);
	UINT32 start_offset, end_offset;
	way += channel << NUM_BIT_WAY;
	start_offset = way << READ_CAHCE_PER_WAY_BIT;
	end_offset = (way + 1) << READ_CAHCE_PER_WAY_BIT;
	for (int iter = start_offset; iter < end_offset; iter++)
	{
		if (nVPPN_input == nVPPN[iter])
			return Buf[iter];
	}
	return NULL;
}

VOID Read_Cache::free_buffer_by_VPPN(UINT32 nVPPN_input)
{
	UINT32 channel, way;
	channel = CHANNEL_FROM_VPPN(nVPPN_input << NUM_BIT_LPN_PER_PAGE);
	way = WAY_FROM_VPPN(nVPPN_input << NUM_BIT_LPN_PER_PAGE);
	UINT32 start_offset, end_offset;
	way += channel << NUM_BIT_WAY;
	start_offset = way << READ_CAHCE_PER_WAY_BIT;
	end_offset = (way + 1) << READ_CAHCE_PER_WAY_BIT;
	for (int iter = start_offset; iter < end_offset; iter++)
	{
		if (nVPPN_input == nVPPN[iter])
		{
			Buf[iter]->nVPPN = 0xffffffff;
			nVPPN[iter] = 0xffffffff;
			return;
		}
	}
}
