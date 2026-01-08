/*******************************************************
*
* Copyright (C) 2018-2019 
* Embedded Software Laboratory(ESLab), SUNG KYUN KWAN UNIVERSITY
* 
* This file is part of ESLab's Flash memory firmware
* 
* This source can not be copied and/or distributed without the express
* permission of ESLab
*
* Author: DongYoung Seo (dongyoung.seo@gmail.com)
		  Kyuhwa Han (hgh6877@gmail.com)
* ESLab: http://nyx.skku.ac.kr
*
*******************************************************/

#include "dftl_internal.h"

VOID
META_CACHE_ENTRY::Initialize(VOID)
{
	m_nMetaLPN = INVALID_LPN;
	INIT_LIST_HEAD(&m_dlList);
    INIT_LIST_HEAD(&m_dlHash);   //Hash kjh

	m_bValid = FALSE;
	m_bDirty = FALSE;
	m_bIORunning = FALSE;
}

VOID
META_CACHE::Initialize(VOID)
{
	INIT_LIST_HEAD(&m_dlLRU);
	INIT_LIST_HEAD(&m_dlFree);
	m_nFreeCount = 0;

	m_nHit = 0;
	m_nMiss = 0;

// Meta
    for (INT32 i = 0; i < META_HASH_SIZE; ++i)
        INIT_LIST_HEAD(&m_ahHash[i]);
// Hash kjh

	for (INT32 i = 0; i < META_CACHE_ENTRY_COUNT; i++) //META_CACHE_ENTRY_COUNT: 256
	{
		m_astCacheEntry[i].Initialize();

		_Release(&m_astCacheEntry[i]);
	}

	m_nFormatMetaLPN = INVALID_LPN;
}

BOOL
META_CACHE::Format(VOID)
{
	BOOL bRet;

	do
	{
		if (m_nFormatMetaLPN == INVALID_LPN)
		{
			m_nFormatMetaLPN = 0;
		}

		META_L2V_MGR*	pstMetaL2VMgr = DFTL_GLOBAL::GetMetaL2VMgr();
		if (m_nFormatMetaLPN >= pstMetaL2VMgr->GetMetaLPNCount())
		{
			// Format done
			bRet = TRUE;
			break;
		}

		META_CACHE_ENTRY*	pstEntry = _Allocate();
		if (pstEntry == NULL)
		{
			bRet = FALSE;
//			xil_printf("Allocate Fail\r\n");
			break;
		}

		DEBUG_ASSERT(pstEntry->m_bValid == FALSE);
		DEBUG_ASSERT(pstEntry->m_bDirty == FALSE);
		DEBUG_ASSERT(pstEntry->m_bIORunning == FALSE);

		for (INT32 i = 0; i < L2V_PER_META_PAGE; i++)
		{
			pstEntry->m_anL2V[i] = INVALID_LPN; //L2V_PER_META_PAGE = 1024
		}

		pstEntry->m_bDirty = TRUE;
		pstEntry->m_bValid = TRUE;

		pstEntry->m_nMetaLPN = m_nFormatMetaLPN;

		_HashInsert(pstEntry); //Hash KJH

		m_nFormatMetaLPN++;

		bRet = FALSE;

	} while(0);

	return bRet;
}

META_CACHE_ENTRY*
META_CACHE::GetMetaEntry(UINT32 nLPN)
{
	static INT32	nPrevLPN;
	UINT32	nMetaLPN = _GetMetaLPN(nLPN);
    META_CACHE_ENTRY* pstEntry = _HashFind(nMetaLPN);

    if (pstEntry != NULL) {
        list_move_head(&pstEntry->m_dlList, &m_dlLRU);
        DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_L2PCACHE_HIT);
        nPrevLPN = nLPN;
        return pstEntry;
    }

	if (nPrevLPN != nLPN)
	{
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_L2PCACHE_MISS);
		nPrevLPN = nLPN;
	}

	return NULL;
}

BOOL
META_CACHE::IsMetaAvailable(UINT32 nLPN, UINT32 FLAG)
{
#if (META_PER_WAY==1)
	UINT32 mod_lpn = get_mod_lpn(nLPN);
#else
	UINT32 mod_lpn = nLPN;
#endif
	META_CACHE_ENTRY*	pstEntry = GetMetaEntry(mod_lpn);

	if (pstEntry == NULL)
	{
		return FALSE;
	}

	return (pstEntry->m_bValid == TRUE) ? TRUE : FALSE;
}

BOOL
META_CACHE::IsMetaWritable(UINT32 nLPN)
{
#if (META_PER_WAY==1)
	UINT32 mod_lpn = get_mod_lpn(nLPN);
#else
	UINT32 mod_lpn = nLPN;
#endif
	META_CACHE_ENTRY*	pstEntry = GetMetaEntry(mod_lpn);

	if (pstEntry == NULL)
	{
		return FALSE;
	}
	if (pstEntry->m_bValid == TRUE && pstEntry->m_bIORunning == FALSE)
		return TRUE;
	else
		return FALSE;
	
}

// META_CACHE 클래스 안에 (private 쪽에)
VOID META_CACHE::DebugPrintLRUAndPos(META_CACHE_ENTRY* target)
{
    struct list_head *pos;
    UINT32 idx = 0;
    UINT32 len = 0;
    INT32  target_idx = -1;

    // 먼저 전체 길이와 target index를 같이 구한다
    for (pos = m_dlLRU.next; pos != &m_dlLRU; pos = pos->next) {
        META_CACHE_ENTRY* e = list_entry(pos, META_CACHE_ENTRY, m_dlList);
        if (pos == &target->m_dlList) {
            target_idx = (INT32)idx;
        }
        len++;
        idx++;
    }

    xil_printf("[LRU] len=%u, target MetaLPN=%u, idx=%d\r\n",
               len,
               target->m_nMetaLPN,
               target_idx);

//    idx = 0;
//    for (pos = m_dlLRU.next; pos != &m_dlLRU; pos = pos->next) {
//        META_CACHE_ENTRY* e = list_entry(pos, META_CACHE_ENTRY, m_dlList);
//        xil_printf("  [%u] MetaLPN=%u%s\r\n",
//                   idx,
//                   e->m_nMetaLPN,
//                   (e == target) ? " <-- NEW" : "");
//        idx++;
//    }
}

VOID
META_CACHE::LoadMeta(UINT32 nLPN, UINT32 DS_FLAG, UINT32 META_FLAG)
{
#if (META_PER_WAY==1)
	UINT32 mod_lpn = get_mod_lpn(nLPN);
#else
	UINT32 mod_lpn = nLPN;
#endif
#if (SUPPORT_META_DEMAND_LOADING == 1)
	META_CACHE_ENTRY*	pstEntry = GetMetaEntry(mod_lpn);
	if (pstEntry != NULL)
	{
		DEBUG_ASSERT(pstEntry->m_bIORunning == TRUE);
		DEBUG_ASSERT(pstEntry->m_bValid == FALSE);
//		xil_printf("Im Here\r\n");
		return;
	}

	pstEntry = _Allocate();
	if (pstEntry == NULL)
	{
		// busy
		return;
	}

    // Hash KJH
    pstEntry->m_nMetaLPN = _GetMetaLPN(mod_lpn);
    _HashInsert(pstEntry);

    if (META_FLAG == 2u) {
//    	list_move_pos(&pstEntry->m_dlList, &m_dlLRU);
    }
    // Hash KJH

	REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
	META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
	META_REQUEST*	pstRequest = pstRequestInfo->AllocateRequest();

	if (pstRequest == NULL)
	{
		_HashRemove(pstEntry); // Hash KJH
		// there is no free request
		list_del_init(&pstEntry->m_dlList);
		_Release(pstEntry);			// Release Meta entry
		return;
	}

//	pstEntry->m_nMetaLPN = _GetMetaLPN(mod_lpn);

	pstRequest->Initialize(META_REQUEST_READ_WAIT, pstEntry->m_nMetaLPN, pstEntry, IOTYPE_META);
#if (META_PER_WAY==1)
	UINT32 channel, way;
	channel = get_channel_from_lpn(pstEntry->m_nMetaLPN);
	way = get_way_from_lpn(pstEntry->m_nMetaLPN);
	pstRequestInfo->AddToWaitQ(pstRequest, channel, way);
#else
	pstRequestInfo->AddToWaitQ(pstRequest);
#endif

    if (META_FLAG == 1u)
    {
    	DFTL_IncreaseProfile(Prof_CMT_read_meta);
    }
    else if (META_FLAG == 2u)
    {
    	DFTL_IncreaseProfile(Prof_CMT_read_gc);
    }
    else
    {
    	DFTL_IncreaseProfile(Prof_CMT_read_host);
    }
	pstEntry->m_bIORunning = TRUE;

	if (DS_FLAG)
		DFTL_IncreaseProfile(Prof_Discard_Load_Num);

	META_DEBUG_PRINTF("[META] Load MetaLPN: %d \n\r", pstEntry->m_nMetaLPN);

#else
	ASSERT(0);
#endif
}

UINT32
META_CACHE::GetL2V(UINT32 nLPN)
{
	META_CACHE_ENTRY*		pstEntry;
#if (META_PER_WAY==1)
	UINT32 mod_lpn = get_mod_lpn(nLPN);
#else
	UINT32 mod_lpn = nLPN;
#endif

	pstEntry = GetMetaEntry(mod_lpn);
	if (pstEntry == NULL)
		return INVALID_PPN;

	DEBUG_ASSERT(pstEntry->m_bValid == TRUE);

	UINT32 nOffset = mod_lpn % L2V_PER_META_PAGE;

	return pstEntry->m_anL2V[nOffset];
}

/*
	@brief	set a new VPPN,
	@return	OldVPPN
*/
UINT32
META_CACHE::SetL2V(UINT32 nLPN, UINT32 nVPPN)
{
	META_CACHE_ENTRY*		pstEntry;
#if (META_PER_WAY==1)
	UINT32 mod_lpn = get_mod_lpn(nLPN);
#else
	UINT32 mod_lpn = nLPN;
#endif

	pstEntry = GetMetaEntry(mod_lpn);
	ASSERT(pstEntry != NULL);

	DEBUG_ASSERT(pstEntry->m_bValid == TRUE);

	UINT32 nOffset = mod_lpn % L2V_PER_META_PAGE;

	UINT32 nOldVPPN = pstEntry->m_anL2V[nOffset];

	pstEntry->m_anL2V[nOffset] = nVPPN;

	pstEntry->m_bDirty = TRUE;

	return nOldVPPN;
}

UINT32
META_CACHE::get_mod_lpn(UINT32 nLPN) {
//	UINT32 nChannel = get_channel_from_lpn(nLPN);
//	UINT32 nWay = get_way_from_lpn(nLPN);
//	UINT32 nLBN = get_lbn_from_lpn(nLPN);
//	UINT32 nPage = get_page_from_lpn(nLPN);
//	UINT32 page_offset = nLPN % LPN_PER_PHYSICAL_PAGE;
//	UINT32 mod_lpn = get_mod_lpn_from_lpn_lbn(nChannel, nWay, nLBN, nPage) + page_offset;

	return nLPN;
}

UINT32
META_CACHE::_GetMetaLPN(UINT32 nLPN)
{
	return nLPN / L2V_PER_META_PAGE;
}

VOID
META_CACHE::_Release(META_CACHE_ENTRY* pstEntry)
{
	list_add_head(&pstEntry->m_dlList, &m_dlFree);
	m_nFreeCount++;
}

/*
	@brief	allocate a cache entry
*/

META_CACHE_ENTRY*
META_CACHE::_Allocate(VOID)
{
	META_CACHE_ENTRY*	pstEntry;
	if (m_nFreeCount == 0)
	{
		DEBUG_ASSERT(list_empty(&m_dlFree) == TRUE);
		pstEntry = list_last_entry(&m_dlLRU, META_CACHE_ENTRY, m_dlList);

		if (pstEntry->m_bIORunning == TRUE)
		{
			// Programing on going
			return NULL;
		}

		DEBUG_ASSERT(pstEntry->m_bValid == TRUE);


		if (pstEntry->m_bDirty == TRUE)
		{
			// need to write this entry to NAND flash memory
			// Add To Meta Write Queue
			REQUEST_MGR*	pstRequestMgr = DFTL_GLOBAL::GetRequestMgr();
			META_REQUEST_INFO*	pstRequestInfo = pstRequestMgr->GetMetaRequestInfo();
			META_REQUEST*	pstRequest = pstRequestInfo->AllocateRequest();
			if (pstRequest == NULL)
			{
				// there is no free reuqest
				return NULL;
			}

			pstRequest->Initialize(META_REQUEST_WRITE_WAIT, pstEntry->m_nMetaLPN, pstEntry, IOTYPE_META);

			// copy meta to buffer entry
			OSAL_MEMCPY(pstRequest->GetBuffer()->m_pMainBuf, &pstEntry->m_anL2V[0], META_VPAGE_SIZE);
#if (META_PER_WAY==1)
			UINT32 channel, way;
			channel = get_channel_from_lpn(pstEntry->m_nMetaLPN);
			way = get_way_from_lpn(pstEntry->m_nMetaLPN);
			pstRequestInfo->AddToWaitQ(pstRequest, channel, way);
#else
			pstRequestInfo->AddToWaitQ(pstRequest);
#endif

			DFTL_IncreaseProfile(Prof_CMT_write);
			pstEntry->m_bIORunning = TRUE;
			return NULL;
		}

		_HashRemove(pstEntry); //Hash KJH

		META_DEBUG_PRINTF("[META] Evicted MetaLPN: %d \r\n", pstEntry->m_nMetaLPN);

		ASSERT(pstEntry->m_bValid == TRUE);
	}
	else
	{
		pstEntry = list_first_entry(&m_dlFree, META_CACHE_ENTRY, m_dlList);
		m_nFreeCount--;
	}

	list_move_head(&pstEntry->m_dlList, &m_dlLRU);

	pstEntry->m_bValid = FALSE;
	ASSERT(pstEntry->m_bDirty == FALSE);
	ASSERT(pstEntry->m_bIORunning == FALSE);

	return pstEntry;
}

// Hash KJH
META_CACHE_ENTRY* META_CACHE::_HashFind(UINT32 nMetaLPN)
{
    UINT32 idx = _HashIdx(nMetaLPN);
    META_CACHE_ENTRY* e;
    list_for_each_entry(META_CACHE_ENTRY, e, &m_ahHash[idx], m_dlHash) {
        if (e->m_nMetaLPN == nMetaLPN) return e;
    }
    return NULL;
}
VOID META_CACHE::_HashInsert(META_CACHE_ENTRY* e)
{
    UINT32 idx = _HashIdx(e->m_nMetaLPN);
    list_add_head(&e->m_dlHash, &m_ahHash[idx]);
}
VOID META_CACHE::_HashRemove(META_CACHE_ENTRY* e)
{
    list_del_init(&e->m_dlHash);
}
// Hash KJH
///////////////////////////////////////////////////////////////////////////////////
//
//	Meta Manager
//
///////////////////////////////////////////////////////////////////////////////////

VOID
META_MGR::Initialize(VOID)
{
	m_stMetaCache.Initialize();
#if (SUPPORT_META_DEMAND_LOADING == 1)
	
#else
	m_panL2V = (UINT32*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, _GetL2PSize(), OSAL_MEMALLOC_FW_ALIGNMENT);
#endif

	m_bFormatted = FALSE;
}

BOOL
META_MGR::Format(VOID)
{
	if (m_bFormatted == TRUE)
	{
		return TRUE;
	}

	BOOL	bRet;

	bRet = m_stMetaCache.Format();
#if (SUPPORT_META_DEMAND_LOADING == 1)
#else
	OSAL_MEMSET(m_panL2V, 0xFF, _GetL2PSize());		// set invalid LPN
	bRet = TRUE;
#endif

	if (bRet == TRUE)
	{
		m_nVPC = 0;
		m_bFormatted = TRUE;
	}

	return bRet;
}

//BOOL
//META_MGR::IsMetaAvailable(UINT32 nLPN, UINT32 FLAG)
//{
//#if (SUPPORT_META_DEMAND_LOADING == 1)
//	return m_stMetaCache.IsMetaAvailable(nLPN, FLAG);
//#else
//	return TRUE;
//#endif
//}

//BOOL
//META_MGR::IsMetaAvailable(UINT32 nLPN, UINT32 FLAG)
//{
//#if (SUPPORT_META_DEMAND_LOADING == 1)
//	if (m_stMetaCache.IsMetaAvailable(nLPN, FLAG) == FALSE)
//	{
//		if (FLAG == 1)
//			DFTL_IncreaseProfile(Prof_Host_CMT_Miss);
//
//		return FALSE;
//	}
//	else
//	{
//		if (FLAG == 1)
//			DFTL_IncreaseProfile(Prof_Host_CMT_Hit);
//
//		return TRUE;
//	}
//#else
//	return TRUE;
//#endif
//}

BOOL
META_MGR::IsMetaAvailable(UINT32 nLPN, UINT32 FLAG)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
    DFTL_GLOBAL* pGlobal       = DFTL_GLOBAL::GetInstance();
    const BOOL   isHostAccess  = (FLAG == 1u);

    BOOL available = m_stMetaCache.IsMetaAvailable(nLPN, FLAG);

    if ((pGlobal->m_bEnable == 3) && isHostAccess)
    {
        if (available)
        {
            DFTL_IncreaseProfile(Prof_Host_CMT_Hit);
        }
        else
        {
            DFTL_IncreaseProfile(Prof_Host_CMT_Miss);
        }

//        if ((pGlobal->m_nHostReqCount % LOG_INTERVAL) == 0)
//        {
//            UINT32 curHit      = DFTL_GetProfile(Prof_Host_CMT_Hit);
//            UINT32 curMiss     = DFTL_GetProfile(Prof_Host_CMT_Miss);
//            UINT32 cumulTotal  = curHit + curMiss;
//
//            UINT32 deltaHit    = curHit  - pGlobal->m_nLastHostHit;
//            UINT32 deltaMiss   = curMiss - pGlobal->m_nLastHostMiss;
//            UINT32 deltaTotal  = deltaHit + deltaMiss;
//
//            UINT32 intvRatio   = (deltaTotal  > 0u)
//                                   ? (UINT32)((deltaHit * 100u) / deltaTotal)
//                                   : 0u;
//            UINT32 cumulRatio  = (cumulTotal > 0u)
//                                   ? (UINT32)((curHit * 100u) / cumulTotal)
//                                   : 0u;
//
//            xil_printf(
//                "[CMT Tracking] Req: %u | Intv: %u%% (H:%u M:%u) | "
//                "Cumul: %u%% (H:%u M:%u)\r\n",
//                pGlobal->m_nHostReqCount + 1,
//                intvRatio,  deltaHit,  deltaMiss,
//                cumulRatio, curHit,    curMiss
//            );
//
//            pGlobal->m_nLastHostHit  = curHit;
//            pGlobal->m_nLastHostMiss = curMiss;
//        }
//        pGlobal->m_nHostReqCount++;
    }

    return available;
#else
    (void)nLPN;
    (void)FLAG;
    return TRUE;
#endif
}

BOOL
META_MGR::IsMetaWritable(UINT32 nLPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.IsMetaWritable(nLPN);
#else
	return TRUE;
#endif
}

VOID
META_MGR::LoadMeta(UINT32 nLPN, UINT32 DS_FLAG, UINT32 META_FLAG)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.LoadMeta(nLPN, DS_FLAG, META_FLAG);
#else
	return;
#endif
}

VOID
META_MGR::LoadDone(META_CACHE_ENTRY* pstMetaEntry, VOID* pBuf)
{
	DEBUG_ASSERT(FALSE == pstMetaEntry->m_bValid);
	DEBUG_ASSERT(FALSE == pstMetaEntry->m_bDirty);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bIORunning);

	pstMetaEntry->m_bIORunning = FALSE;
	pstMetaEntry->m_bDirty = FALSE;
	pstMetaEntry->m_bValid = TRUE;

	OSAL_MEMCPY(&pstMetaEntry->m_anL2V[0], pBuf, META_VPAGE_SIZE);
}

VOID
META_MGR::StoreDone(META_CACHE_ENTRY* pstMetaEntry)
{
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bValid);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bDirty);
	DEBUG_ASSERT(TRUE == pstMetaEntry->m_bIORunning);

	pstMetaEntry->m_bIORunning = FALSE;
	pstMetaEntry->m_bDirty = FALSE;
}


//VOID
//META_MGR::ApplyTrimOnLoadedEntry(META_CACHE_ENTRY* pEntry, UINT32 metaLPN)
//{
//	UINT32 TRIM_CNT = 0;
//    DEBUG_ASSERT(pEntry != NULL);
//    DEBUG_ASSERT(pEntry->m_bValid == TRUE);
//
//    const UINT32 start_lpn = metaLPN * L2V_PER_META_PAGE;
//    const UINT32 end_lpn   = start_lpn + L2V_PER_META_PAGE - 1;
//
//    for (INT32 i = 0; i < (INT32)L2V_PER_META_PAGE; ++i)
//    {
//        const UINT32 oldVPPN = pEntry->m_anL2V[i];
//        if (oldVPPN == INVALID_PPN) continue;
//
//        const UINT32 lpn  = start_lpn + (UINT32)i;
//        const UINT32 bidx = (lpn >> 3);
//        const UINT8  mask = (UINT8)(1u << (lpn & 7));
//
//        if ((g_trim_bitmap[bidx] & mask) == 0) continue;
//
//        pEntry->m_anL2V[i] = INVALID_PPN;
//        pEntry->m_bDirty = TRUE;
//
//		VNAND*	pstVNand = DFTL_GLOBAL::GetVNandMgr();
//		DEBUG_ASSERT(pstVNand->GetV2L(oldVPPN) == nLPN);
//
//		pstVNand->Invalidate(oldVPPN, 1);
//		DFTL_GLOBAL::GetInstance()->GetUserBlockMgr()->Invalidate(oldVPPN);
//
////    	DFTL_IncreaseProfile(Prof_Discard_Load_Page);
//    	TRIM_CNT += 1;
//    }
//    ClearTrimRange(start_lpn, end_lpn);
//    xil_printf("	INVALID %u Pages\r\n", TRIM_CNT);
//}

#ifndef TRIM_BITMAP_BYTES
#define TRIM_BITMAP_BYTES  ((TOTAL_LPN_COUNT + 7u) >> 3)
#endif

static inline void _safe_store_u32(UINT32* p, UINT32 v) {
    uintptr_t a = (uintptr_t)p;
    if ((a & 3u) == 0) { *p = v; }
    else {
        volatile UINT8* b = (volatile UINT8*)p;
        b[0] = (UINT8)(v      );
        b[1] = (UINT8)(v >>  8);
        b[2] = (UINT8)(v >> 16);
        b[3] = (UINT8)(v >> 24);
    }
}

VOID
META_MGR::ApplyTrimOnLoadedEntry_OPT(META_CACHE_ENTRY* pEntry, UINT32 metaLPN)
{
//	if (g_trim_going == 1)
//		xil_printf("[TRIM] TRIM LOAD ENTRY APPLY METALPN:%u\r\n", metaLPN);
//	else
//		xil_printf(" [HOST] TRIM LOAD ENTRY APPLY METALPN:%u\r\n", metaLPN);
	UINT32 TRIM_CNT = 0;

    VNAND* v = DFTL_GLOBAL::GetVNandMgr();
    BLOCK_MGR* ubm = DFTL_GLOBAL::GetInstance()->GetUserBlockMgr();

    DEBUG_ASSERT(pEntry != NULL);
    DEBUG_ASSERT(pEntry->m_bValid == TRUE);

    const UINT32 start_lpn = metaLPN * L2V_PER_META_PAGE;
    const UINT32 end_lpn   = start_lpn + L2V_PER_META_PAGE - 1;

    // 바이트 단위 스캔 범위 (host LPN 기준)
    UINT32 byte_start  = (start_lpn >> 3);
    UINT32 byte_end_ex = (end_lpn   >> 3) + 1;   // exclusive

    // 비트맵 경계 클램프(OOB 방지)
    if (byte_start >= TRIM_BITMAP_BYTES) {
        ClearTrimRange(start_lpn, end_lpn);
        xil_printf("[ERR] metaLPN:%u cleared_bits:%u (bitmap OOB)\r\n", metaLPN, TRIM_CNT);
        return;
    }
    if (byte_end_ex > TRIM_BITMAP_BYTES) byte_end_ex = TRIM_BITMAP_BYTES;

    UINT32 old_vppns[L2V_PER_META_PAGE];
    UINT32 inv_cnt = 0;

    bool touched = false;

    for (UINT32 b = byte_start; b < byte_end_ex; ++b) {
        UINT8 bits = g_trim_bitmap[b];
        if (!bits) continue;

        const UINT32 lpn_base = (b << 3);
        const UINT32 i_base   = lpn_base - start_lpn; // 엔트리 내 시작 인덱스

        // 엣지 바이트 절단
        const UINT32 bit_lo = (b == byte_start)        ? (start_lpn & 7) : 0;
        const UINT32 bit_hi = (b == (byte_end_ex - 1)) ? (end_lpn   & 7) : 7;

        UINT8 range_mask = (UINT8)((0xFFu << bit_lo) & (0xFFu >> (7u - bit_hi)));
        UINT8 work = (UINT8)(bits & range_mask);
        if (!work) continue;

        for (UINT32 bit = bit_lo; bit <= bit_hi; ++bit) {
        	UINT8 m = (UINT8)(1u << bit);
            if (!(work & m)) continue;

            UINT32 i   = i_base + bit;
            if (i >= (UINT32)L2V_PER_META_PAGE) continue;

            UINT32 oldVPPN = pEntry->m_anL2V[i];
            if (oldVPPN == INVALID_PPN) continue;
            if ((v->GetV2L(oldVPPN))!= (start_lpn + i)) continue;

            UINT32 nVBN = VBN_FROM_VPPN(oldVPPN);
            UINT32 channel = CHANNEL_FROM_VPPN(oldVPPN);
            UINT32 way = WAY_FROM_VPPN(oldVPPN);

            VBINFO*		pstVBInfo;
            pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr(channel, way)->GetVBInfo(nVBN);
            if (pstVBInfo->IsActive()) continue;
            if (DFTL_GLOBAL::GetGCMgr(channel, way)->return_VBN() == nVBN) continue;

            UINT32 nLPN = start_lpn + i;
            UINT32 cID  = DFTL_GLOBAL::GetInstance()->GetClusterID(nLPN);
            DFTL_GLOBAL::GetInstance()->m_util_pages[cID] -= 1;

            _safe_store_u32(&pEntry->m_anL2V[i], INVALID_PPN);
            touched = true;

            if (inv_cnt < (UINT32)L2V_PER_META_PAGE) {
                old_vppns[inv_cnt] = oldVPPN;
                ++inv_cnt;
                TRIM_CNT += 1;
            }
        }
    }
    if (touched) pEntry->m_bDirty = TRUE;

    if (inv_cnt) {
        for (UINT32 k = 0; k < inv_cnt; ++k) {
            v->Invalidate(old_vppns[k], 1);
            ubm->Invalidate(old_vppns[k]);
			DFTL_IncreaseProfile(Prof_Discard_CMT_Num);
        }
    }
    ClearTrimRange(start_lpn, end_lpn);

//	if (g_trim_going == 1)
//		xil_printf("[TRIM-END] TRIM LOAD ENTRY APPLY METALPN:%u\r\n", metaLPN);
//	else
//		xil_printf(" [HOST-END] TRIM LOAD ENTRY APPLY METALPN:%u\r\n", metaLPN);
}

//VOID
//META_MGR::ApplyTrimOnLoadedEntry_OPT(META_CACHE_ENTRY* pEntry, UINT32 metaLPN)
//{
//    UINT32 TRIM_CNT = 0;
//
//    VNAND* v = DFTL_GLOBAL::GetVNandMgr();
//    BLOCK_MGR* ubm = DFTL_GLOBAL::GetInstance()->GetUserBlockMgr();
//
//    DEBUG_ASSERT(pEntry != NULL);
//    DEBUG_ASSERT(pEntry->m_bValid == TRUE);
//
//    const UINT32 start_lpn = metaLPN * L2V_PER_META_PAGE;
//    const UINT32 end_lpn   = start_lpn + L2V_PER_META_PAGE - 1;
//
//    UINT32 byte_start  = (start_lpn >> 3);
//    UINT32 byte_end_ex = (end_lpn   >> 3) + 1;   // exclusive
//
//    // 비트맵 경계 클램프(OOB 방지)
//    if (byte_start >= TRIM_BITMAP_BYTES) {
//        ClearTrimRange(start_lpn, end_lpn);
//        return;
//    }
//    if (byte_end_ex > TRIM_BITMAP_BYTES) byte_end_ex = TRIM_BITMAP_BYTES;
//
//    UINT32 old_vppns[L2V_PER_META_PAGE];
//    UINT32 inv_cnt = 0;
//
//    bool touched = false;
//
//    for (UINT32 b = byte_start; b < byte_end_ex; ++b) {
//        UINT8 bits = g_trim_bitmap[b];
//        if (!bits) continue;
//
//        const UINT32 lpn_base = (b << 3);
//        const UINT32 i_base   = lpn_base - start_lpn; // 엔트리 내 시작 인덱스
//
//        // 엣지 바이트 절단
//        const UINT32 bit_lo = (b == byte_start)        ? (start_lpn & 7) : 0;
//        const UINT32 bit_hi = (b == (byte_end_ex - 1)) ? (end_lpn   & 7) : 7;
//
//        UINT8 range_mask = (UINT8)((0xFFu << bit_lo) & (0xFFu >> (7u - bit_hi)));
//        UINT8 work = (UINT8)(bits & range_mask);
//        if (!work) continue;
//
//        for (UINT32 bit = bit_lo; bit <= bit_hi; ++bit) {
//            UINT8 m = (UINT8)(1u << bit);
//            if (!(work & m)) continue;
//
//            UINT32 i = i_base + bit;
//            if (i >= (UINT32)L2V_PER_META_PAGE) continue;
//
//            UINT32 oldVPPN = pEntry->m_anL2V[i];
//            if (oldVPPN == INVALID_PPN) continue;
//            if ((v->GetV2L(oldVPPN)) != (start_lpn + i)) continue;
//
//            UINT32 nVBN    = VBN_FROM_VPPN(oldVPPN);
//            UINT32 channel = CHANNEL_FROM_VPPN(oldVPPN);
//            UINT32 way     = WAY_FROM_VPPN(oldVPPN);
//
//            VBINFO* pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr(channel, way)->GetVBInfo(nVBN);
//            if (pstVBInfo->IsActive()) continue;
//            if (DFTL_GLOBAL::GetGCMgr(channel, way)->return_VBN() == nVBN) continue;
//
//            UINT32 nLPN = start_lpn + i; // 절대 LPN
//            UINT32 cID  = DFTL_GLOBAL::GetInstance()->GetClusterID(nLPN);
//            DFTL_GLOBAL::GetInstance()->m_util_pages[cID] -= 1;
//
//            _safe_store_u32(&pEntry->m_anL2V[i], INVALID_PPN);
//            touched = true;
//
//            if (inv_cnt < (UINT32)L2V_PER_META_PAGE) {
//                old_vppns[inv_cnt] = oldVPPN;
//                ++inv_cnt;
//                TRIM_CNT += 1;
//            }
//        }
//    }
//    if (touched) pEntry->m_bDirty = TRUE;
//
//    if (inv_cnt) {
//        for (UINT32 k = 0; k < inv_cnt; ++k) {
//            v->Invalidate(old_vppns[k], 1);
//            ubm->Invalidate(old_vppns[k]);
//            DFTL_IncreaseProfile(Prof_Discard_CMT_Num);
//        }
//    }
//    ClearTrimRange(start_lpn, end_lpn);
//}


UINT32
META_MGR::GetL2V(UINT32 nLPN)
{
	DEBUG_ASSERT(nLPN < DFTL_GLOBAL::GetInstance()->GetLPNCount());

#if (SUPPORT_META_DEMAND_LOADING == 1)
	return m_stMetaCache.GetL2V(nLPN);
#else
	return	m_panL2V[nLPN];
#endif
}

VOID
META_MGR::SetL2V(UINT32 nLPN, UINT32 nVPPN, UINT32 DS_FLAG)
{
	DEBUG_ASSERT(nLPN < DFTL_GLOBAL::GetInstance()->GetLPNCount());
		
	UINT32	nOldVPPN;

#if (SUPPORT_META_DEMAND_LOADING == 1)
	nOldVPPN = m_stMetaCache.SetL2V(nLPN, nVPPN);
#else
	nOldVPPN = m_panL2V[nLPN];

	m_panL2V[nLPN] = nVPPN;
#endif

	if (DS_FLAG != 1)
	{
		UINT32 cID = DFTL_GLOBAL::GetInstance()->GetClusterID(nLPN);
		DFTL_GLOBAL::GetInstance()->m_util_pages[cID] += 1;
	}

	if (nOldVPPN == INVALID_PPN)
	{
		UINT32 cID = DFTL_GLOBAL::GetInstance()->GetClusterID(nLPN);

		UINT32 nVBN = VBN_FROM_VPPN(nVPPN);
		UINT32 channel = CHANNEL_FROM_VPPN(nVPPN);
		UINT32 way = WAY_FROM_VPPN(nVPPN);
		VBINFO*		pstVBInfo;

		pstVBInfo = DFTL_GLOBAL::GetVBInfoMgr(channel, way)->GetVBInfo(nVBN);
//		pstVBInfo->IncreaseValidate();
		m_nVPC++;
	}
	else
	{
		UINT32 cID = DFTL_GLOBAL::GetInstance()->GetClusterID(nLPN);
		DFTL_GLOBAL::GetInstance()->m_util_pages[cID] -= 1;

		// Invalidate OLD PPN
		VNAND*	pstVNand = DFTL_GLOBAL::GetVNandMgr();
		DEBUG_ASSERT(pstVNand->GetV2L(nOldVPPN) == nLPN);
		DFTL_GLOBAL::GetInstance()->IncreaseProfileCount(PROFILE_HOST_OVERWRITE);

		pstVNand->Invalidate(nOldVPPN, DS_FLAG);
		DFTL_GLOBAL::GetInstance()->GetUserBlockMgr()->Invalidate(nOldVPPN);

		UINT32 nVBN = VBN_FROM_VPPN(nOldVPPN);
		UINT32 channel = CHANNEL_FROM_VPPN(nOldVPPN);
		UINT32 way = WAY_FROM_VPPN(nOldVPPN);
//		if (nVBN > 19)
//		{
//			int print = DFTL_GLOBAL::GetInstance()->GetUserBlockMgr()->CheckVPC(channel, way, nVBN, 0);
//			if (print == 1)
//				xil_printf("SetL2V Check Result\r\n");
//			xil_printf("[VNAND::ProgramPageSimul] nLPN: %u, nVPPN: %u\r\n", nLPN, nVPPN);
//		}
	}
}

/*
@brief	return L2p SIZE IN BYTE
*/
INT32
META_MGR::_GetL2PSize(VOID)
{
	return sizeof(UINT32) * DFTL_GLOBAL::GetInstance()->GetLPNCount();
}

///////////////////////////////////////////////////////////////////////////////////
//
//	Meta L2V Manager
//
///////////////////////////////////////////////////////////////////////////////////

VOID
META_L2V_MGR::Initialize(VOID)
{
	m_panL2V = (UINT32*)OSAL_MemAlloc(MEM_TYPE_FW_DATA, _GetL2PSize(), OSAL_MEMALLOC_FW_ALIGNMENT);

	m_bFormatted = FALSE;
}

BOOL
META_L2V_MGR::Format(VOID)
{
	if (m_bFormatted == TRUE)
	{
		return TRUE;
	}

	OSAL_MEMSET(m_panL2V, 0xFF, _GetL2PSize());		// set invalid LPN

	m_nVPC = 0;
	m_bFormatted = TRUE;

	return TRUE;
}

UINT32
META_L2V_MGR::GetMetaLPNCount(VOID)
{
	UINT32	nLPNCount	= DFTL_GLOBAL::GetInstance()->GetLPNCount();
	return CEIL(((INT32)nLPNCount), (INT32)L2V_PER_META_PAGE);
}

UINT32
META_L2V_MGR::GetL2V(UINT32 nLPN)
{
	DEBUG_ASSERT(nLPN < GetMetaLPNCount());
	return	m_panL2V[nLPN];
}

VOID
META_L2V_MGR::SetL2V(UINT32 nLPN, UINT32 nVPPN)
{
#if (SUPPORT_META_DEMAND_LOADING == 1)
	DEBUG_ASSERT(nLPN < GetMetaLPNCount());

	BOOL	bOverWrite;

	if (m_panL2V[nLPN] != INVALID_PPN)
	{
		// Invalidate OLD PPN
		UINT32 nOldVPPN = m_panL2V[nLPN];
		VNAND*	pstVNand = DFTL_GLOBAL::GetVNandMgr();
		DEBUG_ASSERT(pstVNand->GetV2L(nOldVPPN) == nLPN);

		pstVNand->Invalidate(nOldVPPN, 2);

		DFTL_GLOBAL::GetInstance()->GetMetaBlockMgr()->Invalidate(nOldVPPN);

		bOverWrite = TRUE;
	}
	else
	{
		bOverWrite = FALSE;
	}

	m_panL2V[nLPN] = nVPPN;

	if (bOverWrite == FALSE)
	{
		m_nVPC++;
	}
#else
	ASSERT(0);		// check option
#endif
}

/*
@brief	return L2p SIZE IN BYTE
*/
INT32
META_L2V_MGR::_GetL2PSize(VOID)
{
	return sizeof(UINT32) * GetMetaLPNCount();
}
