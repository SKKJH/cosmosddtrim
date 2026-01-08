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

#ifndef __DFTL_META_H__
#define __DFTL_META_H__

#define SUPPORT_META_DEBUG		0

#if (SUPPORT_META_DEBUG == 0)
	#define META_DEBUG_PRINTF(...)	((void)0)
#else
	#define META_DEBUG_PRINTF	PRINTF
#endif

#define SIZE_OF_L2V							(sizeof(UINT32))		// 4 byte
#define L2V_PER_META_PAGE					(META_VPAGE_SIZE / SIZE_OF_L2V)		//L2V_PER_META_PAGE = 1024
#define META_CACHE_ENTRY_COUNT				(META_CACHE_SIZE / META_VPAGE_SIZE)
#define PIN_LIMIT							(25)

// Hash KJH
#define META_HASH_BITS   8
#define META_HASH_SIZE   (1u << META_HASH_BITS)   //256(8)
#define META_HASH_MASK   (META_HASH_SIZE - 1)
// Hash KJH

class META_CACHE_ENTRY
{
public:
	VOID	Initialize(VOID);

	UINT32				m_nMetaLPN;

	unsigned int		m_bValid : 1;
	unsigned int		m_bDirty : 1;
	unsigned int		m_bIORunning : 1;
	unsigned int		m_bConfined : 1;

	struct list_head	m_dlList;       // 전체 LRU 리스트용
	struct list_head 	m_dlHash;       // 해시 테이블용

    // [추가] Confined(Pinned)된 엔트리끼리 연결할 리스트 노드
	struct list_head    m_dlConfinedList;

	UINT32				m_anL2V[L2V_PER_META_PAGE];
};

class META_CACHE
{
public:
	VOID				Initialize(VOID);
	BOOL				Format(VOID);
	BOOL				IsMetaAvailable(UINT32 nLPN, BOOL bIsMETA);
	BOOL				IsMetaWritable(UINT32 nLPN, BOOL bIsMETA);
	META_CACHE_ENTRY*	GetMetaEntry(UINT32 nLPN, BOOL bIsMETA);
	VOID				LoadMeta(UINT32 nLPN, UINT32 DS_FLAG, BOOL bIsMETA);
	VOID                ReleaseConfinedEntries(VOID);

	UINT32	GetL2V(UINT32 nLPN, BOOL bIsMETA);
	UINT32	SetL2V(UINT32 nLPN, UINT32 nVPPN, BOOL bIsMETA);
	UINT32  get_mod_lpn(UINT32 nLPN);

private:
	META_CACHE_ENTRY*	_Allocate(VOID);
	VOID				_Release(META_CACHE_ENTRY* pstEntry);
	UINT32				_GetMetaLPN(UINT32 nLPN);

	META_CACHE_ENTRY	m_astCacheEntry[META_CACHE_ENTRY_COUNT];

	struct list_head	m_dlLRU;		// LRU for victim Meta cache entry selection, MRU:@Head, LRU:@tail
	struct list_head	m_dlFree;		// Free cache entry list
	INT32				m_nFreeCount;

	UINT32				m_nHit;			// L2P Hit Count
	UINT32				m_nMiss;		// L2P Miss Count
	struct list_head    m_dlConfinedLRU;
	UINT32              m_nConfinedCount;

	// Hash kjh

	struct list_head    m_ahHash[META_HASH_SIZE];
	static inline UINT32 _HashIdx(UINT32 meta) { return (meta & META_HASH_MASK); }
	META_CACHE_ENTRY*   _HashFind(UINT32 nMetaLPN);
	VOID                _HashInsert(META_CACHE_ENTRY* e);
	VOID                _HashRemove(META_CACHE_ENTRY* e);
	// Hash kjh

	// For metadata format
	UINT32				m_nFormatMetaLPN;
};

class META_MGR
{
public:
	VOID	Initialize(VOID);
	BOOL	Format(VOID);
	UINT32	GetL2V(UINT32 nLPN, BOOL bIsMETA);
	VOID	SetL2V(UINT32 nLPN, UINT32 nVPPN, UINT32 DS_FLAG, BOOL bIsMETA);

	BOOL	IsMetaAvailable(UINT32 nLPN, BOOL bIsMETA);
	BOOL	IsMetaWritable(UINT32 nLPN, BOOL bIsMETA);
	VOID	LoadMeta(UINT32 nLPN, UINT32 DS_FLAG, BOOL bIsMETA);
	VOID 	ApplyTrimOnLoadedEntry_OPT(META_CACHE_ENTRY* pstMetaEntry, UINT32 metaLPN);
	BOOL    ApplyTrimIfCached(UINT32 nLPN);

	VOID	LoadDone(META_CACHE_ENTRY* pstMetaEntry, VOID* pBuf);
	VOID	StoreDone(META_CACHE_ENTRY* pstMetaEntry);		// NAND IO Done
	VOID    ReleaseConfinedEntries(VOID) { m_stMetaCache.ReleaseConfinedEntries(); }


protected:
	INT32	_GetL2PSize(VOID);

private:
	META_CACHE	m_stMetaCache;
#if (SUPPORT_META_DEMAND_LOADING == 1)
#else
	UINT32		*m_panL2V;			// L2VPpn Array
#endif

protected:
	UINT32		m_nVPC;				// FTL Valid Page Count
	unsigned int	m_bFormatted : 1;	// format status
};

class META_L2V_MGR : public META_MGR
{
public:
	VOID	Initialize(VOID);
	BOOL	Format(VOID);
	UINT32	GetL2V(UINT32 nLPN);
	VOID	SetL2V(UINT32 nLPN, UINT32 nVPPN);

	UINT32	GetMetaLPNCount(VOID);

private:
	INT32	_GetL2PSize(VOID);

	UINT32		*m_panL2V;
};

#endif
