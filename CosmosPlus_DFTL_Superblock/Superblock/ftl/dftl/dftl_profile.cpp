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
* Author: Kyuhwa Han (hgh6877@gmail.com)
* ESLab: http://nyx.skku.ac.kr
*
*******************************************************/

#include "dftl_internal.h"


VOID DFTL_Profile_Initialize() {
	for (int iter = 0; iter < Prof_Total_Num; iter++) {
		DFTL_profile[iter] = 0;
	}
}

VOID DFTL_IncreaseProfile(DFTL_PROFILE offset) {
	DFTL_profile[(UINT32)offset]++;
}

VOID DFTL_IncreaseProfile(DFTL_PROFILE offset, UINT32 count) {
	DFTL_profile[(UINT32)offset] += count;
}

UINT32 DFTL_GetProfile(DFTL_PROFILE offset) {
	return DFTL_profile[(UINT32)offset];
}

#include "dftl_profile.h"
#include "xil_printf.h"

VOID DFTL_PrintProfile(UINT32 FLAG)
{
	const char* profile_names[] = {
			"Prof_Host_read",
			"Prof_Host_write",
			"Prof_Host_Discard",

			"Prof_CMT_read",
			"Prof_CMT_write",

			"Prof_CMTGC_read",
			"Prof_CMTGC_write",
			"Prof_CMTGC_count",

			"Prof_GC_read",
			"Prof_GC_write",
			"Prof_GC_read_set",
			"Prof_GC_write_set",
			"Prof_GC_count",	//2MB block

			"Prof_NAND_read",	//16KB
			"Prof_NAND_write",	//16KB

			"Prof_NAND_HOST_write",
			"Prof_NAND_META_write",
			"Prof_NAND_GC_write",

			"Prof_NAND_CMT_write",
			"Prof_NAND_HG_write",
			"Prof_NAND_erase",

			"Prof_Discard_Range_Num",
			"Prof_Discard_Total_Page_Num",

			"Prof_Discard_Page_Num",
			"Prof_Discard_Pass_Num",
			"Prof_Discard_Load_Num",

			"Prof_Discard_Page_Miss",
			"Prof_Discard_Page_Hit",
	};

	for (UINT32 i = 0; i < Prof_Total_Num; i++) {
		xil_printf("[%-35s] %u\r\n", profile_names[i], DFTL_profile[i]);
		DFTL_profile[i] = 0;
	}
	DFTL_GLOBAL::GetInstance()->DebugBlockPrint(0);

	if (FLAG == 1)
	{
		SBINFO* sb = DFTL_GLOBAL::GetSBInfoMgr()->m_pastSBInfo;
		int c = DFTL_GLOBAL::GetVNandMgr()->GetVBlockCount();
		for (int i=0; i<c; i++)
		{
			if (!sb[i].IsBad())
			{
//				if (sb[i].m_bFree)
//				{
//					if (sb[i].IsMeta())
//						xil_printf("[M] %u is FREE, %u blks\r\n",sb[i].m_nVBN, sb[i].m_nUSED);
//					else
//						xil_printf("[H] %u is FREE, %u blks\r\n",sb[i].m_nVBN, sb[i].m_nUSED);
//				}
//				else
//				{
//					if (sb[i].IsMeta())
//						xil_printf("[M] %u is USED, %u blks\r\n",sb[i].m_nVBN, sb[i].m_nUSED);
//					else
//						xil_printf("[H] %u is USED, %u blks\r\n",sb[i].m_nVBN, sb[i].m_nUSED);
//				}
			}
//			else
//				xil_printf("[BAD] %u is BAD Block\r\n",sb[i].m_nVBN);
		}
	}
	 {
	        SBINFO_MGR* sbm = DFTL_GLOBAL::GetSBInfoMgr();
	        UINT32 freeWalk = 0, badInFree = 0, metaInFree = 0, notFreeFlag = 0, usedInFree = 0;

	        xil_printf("[SB][FREE-LIST] ===== START =====\r\n");
	        if (list_empty(&sbm->m_dlFreeList)) {
	            xil_printf("[SB][FREE-LIST] (empty)\r\n");
	        } else {
	            SBINFO* pos;
	            // 프로젝트 매크로 스타일 유지: (TYPE, var, head, member)
	            list_for_each_entry(SBINFO, pos, &sbm->m_dlFreeList, m_dlList) {
//	                xil_printf("[%c] %u is FREE, %u blks\r\n",
//	                           pos->IsMeta() ? 'M' : 'H', pos->m_nVBN, pos->m_nUSED);
	                freeWalk++;

	                // 일관성 체크(경고용)
	                if (pos->IsBad())      badInFree++;
	                if (pos->IsMeta())     metaInFree++;   // SB_INIT에서 meta는 FREE에 안 올려서 비정상
	                if (!pos->m_bFree)     notFreeFlag++;  // FREE 리스트인데 m_bFree=0이면 비정상
	                if (pos->m_nUSED != 0) usedInFree++;   // FREE인데 USED>0이면 비정상(정책에 따라 경고)
	            }
	        }
	        xil_printf("[SB][FREE-LIST] COUNT(walked): %u, COUNT(tracked): %u\r\n",
	                   freeWalk, sbm->m_nFreeCount);
	        if (badInFree || metaInFree || notFreeFlag || usedInFree) {
	            xil_printf("[SB][FREE-LIST][WARN] bad:%u meta:%u notFree:%u used:%u\r\n",
	                       badInFree, metaInFree, notFreeFlag, usedInFree);
	        }
	        xil_printf("[SB][FREE-LIST] ===== END =====\r\n");
	    }
}
