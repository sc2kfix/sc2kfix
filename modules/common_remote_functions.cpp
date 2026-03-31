// sc2kfix modules/common_remote_functions.cpp: wrapper remote functions
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE: The defined remote functions only account for certain cases.
// If you attempt to use any of these remote functions out-of-context
// (ie, they don't have a targeted remote call) then they will fail.
//
// Be careful.

// Keys for remote framework cases:
// BOR - Borland wrapper functions.
// MFC - MFC wrapper/replacement functions.
// SCURK - SCURK-only.
// SC2K - SC2K-only.

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>
#include "../resource.h"

// Borland function wrappers.

void *__cdecl L_BOR_WRP_gAllocBlock(size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gAllocBlock_SCURKPrimary(nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gAllocBlock_SCURK1996(nSz);
	}
	return NULL;
}

void *__cdecl L_BOR_WRP_gResizeBlock(BYTE *pBlock, size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gResizeBlock_SCURKPrimary(pBlock, nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gResizeBlock_SCURK1996(pBlock, nSz);
	}
	return NULL;
}

void __cdecl L_BOR_WRP_gFreeBlock(void *pBlock) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_gFreeBlock_SCURKPrimary(pBlock);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_gFreeBlock_SCURK1996(pBlock);
	}
}

void __stdcall L_BOR_WRP_gUpdateWaitWindow() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_gUpdateWaitWindow_SCURKPrimary();
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_gUpdateWaitWindow_SCURK1996();
	}
}

void L_BOR_WRP_DC_SelectObjectPalette(TBC45XDC *pThis, TBC45XPalette *pPal, int nVal) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDC_SelectObjectPalette_SCURKPrimary(pThis, pPal, nVal);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDC_SelectObjectPalette_SCURK1996(pThis, pPal, nVal);
	}
}

unsigned int L_BOR_WRP_Window_DefaultProcessing(TBC45XParWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCWindow_DefaultProcessing_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCWindow_DefaultProcessing_SCURK1996(pThis);
	}
	return 0;
}

LRESULT L_BOR_WRP_Window_HandleMessage(TBC45XParWindow *pThis, unsigned int msg, WPARAM wParam, LPARAM lParam) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCWindow_HandleMessage_SCURKPrimary(pThis, msg, wParam, lParam);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCWindow_HandleMessage_SCURK1996(pThis, msg, wParam, lParam);
	}
	return 0;
}

void L_BOR_WRP_Window_SetCursor(TBC45XParWindow *pThis, TBC45XModule *pModule, const char *pResID) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCWindow_SetCursor_SCURKPrimary(pThis, pModule, pResID);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCWindow_SetCursor_SCURK1996(pThis, pModule, pResID);
	}
}

void L_BOR_WRP_WindowDC_Destruct(TBC45XWindowDC *pThis, char c) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCWindowDC_Destruct_SCURKPrimary(pThis, c);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCWindowDC_Destruct_SCURK1996(pThis, c);
	}
}

TBC45XClientDC *L_BOR_WRP_ClientDC_Construct(TBC45XClientDC *pThis, HWND hWnd) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCClientDC_Construct_SCURKPrimary(pThis, hWnd);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCClientDC_Construct_SCURK1996(pThis, hWnd);
	}
	return NULL;
}

void L_BOR_WRP_Dialog_SetupWindow(TBC45XParDialog *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_SetupWindow_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_SetupWindow_SCURK1996(pThis);
	}
}

void L_BOR_WRP_Dialog_SetCaption(TBC45XParDialog *pThis, char *pCaption) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_SetCaption_SCURKPrimary(pThis, pCaption);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_SetCaption_SCURK1996(pThis, pCaption);
	}
}

void L_BOR_WRP_Dialog_EvClose(TBC45XParDialog *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_EvClose_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_EvClose_SCURK1996(pThis);
	}
}

int L_BOR_WRP_ListBox_GetSelIndex(TBC45XParListBox *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_GetSelIndex_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_GetSelIndex_SCURK1996(pThis);
	}
	return 0;
}

LRESULT L_BOR_WRP_ListBox_GetString(TBC45XParListBox *pThis, char *pString, int nIndex) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_GetString_SCURKPrimary(pThis, pString, nIndex);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_GetString_SCURK1996(pThis, pString, nIndex);
	}
	return 0;
}

int L_BOR_WRP_ListBox_SetItemData(TBC45XParListBox *pThis, int nIndex, unsigned int uItemData) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_SetItemData_SCURKPrimary(pThis, nIndex, uItemData);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_SetItemData_SCURK1996(pThis, nIndex, uItemData);
	}
	return 0;
}

int L_BOR_WRP_ListBox_AddString(TBC45XParListBox *pThis, const char *pString) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_AddString_SCURKPrimary(pThis, pString);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_AddString_SCURK1996(pThis, pString);
	}
	return 0;
}

TBC45XMDIChild *L_BOR_WRP_MDIClient_GetActiveMDIChild(TBC45XParMDIClient *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCMDIClient_GetActiveMDIChild_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCMDIClient_GetActiveMDIChild_SCURK1996(pThis);
	}
	return NULL;
}

// SCURK global variable wrappers

__int16 *L_SCURK_WRP_GetwTileObjects() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return wTileObjects_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return wTileObjects_SCURK1996;
	}
	return NULL;
}

WORD *L_SCURK_WRP_GetwColFastCnt() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wColFastCnt_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wColFastCnt_SCURK1996;
	}
	return NULL;
}

WORD *L_SCURK_WRP_GetwColSlowCnt() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wColSlowCnt_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wColSlowCnt_SCURK1996;
	}
	return NULL;
}

__int16 *L_SCURK_WRP_GetwToolNum() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return wtoolNum_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return wtoolNum_SCURK1996;
	}
	return NULL;
}

__int16 *L_SCURK_WRP_GetwToolValue() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wtoolValue_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wtoolValue_SCURK1996;
	}
	return 0;
}

winscurkApp *L_SCURK_WRP_winscurkApp_GetPointerToClass() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return gScurkApplication_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return gScurkApplication_SCURK1996;
	}
	return NULL;
}

BC45Xstring *L_SCURK_WRP_GetTAppInitCmdLine() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return TAppInitCmdLine_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return TAppInitCmdLine_SCURK1996;
	}
	return NULL;
}

// cEditableTileSet function wrappers

char *L_SCURK_WRP_EditableTileSet_GetLongName(cEditableTileSet *pThis, int nEdNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditableTileSet_GetLongName_SCURKPrimary(pThis, nEdNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditableTileSet_GetLongName_SCURK1996(pThis, nEdNum);
	}
	return NULL;
}

// winscurkPlaceWindow function wrappers

void L_SCURK_WRP_winscurkPlaceWindow_ClearCurrentTool(winscurkPlaceWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_winscurkPlaceWindow_ClearCurrentTool_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_winscurkPlaceWindow_ClearCurrentTool_SCURK1996(pThis);
	}
}

// winscurkMDIClient function wrappers

void L_SCURK_WRP_winscurkMDIClient_RotateColors(winscurkMDIClient *pThis, BOOL bFast) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_winscurkMDIClient_RotateColors_SCURKPrimary(pThis, bFast);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_winscurkMDIClient_RotateColors_SCURK1996(pThis, bFast);
	}
}

// winscurkApp function wrappers

TBC45XPalette *L_SCURK_WRP_winscurkApp_GetPalette(winscurkApp *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_winscurkApp_GetPalette_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_winscurkApp_GetPalette_SCURK1996(pThis);
	}
	return NULL;
}

void L_SCURK_WRP_winscurkApp_ScurkSound(winscurkApp *pThis, int nSoundID) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_winscurkApp_ScurkSound_SCURKPrimary(pThis, nSoundID);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_winscurkApp_ScurkSound_SCURK1996(pThis, nSoundID);
	}
}

winscurkPlaceWindow *L_SCURK_WRP_winscurkApp_GetPlaceWindow(winscurkApp *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_winscurkApp_GetPlaceWindow_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_winscurkApp_GetPlaceWindow_SCURK1996(pThis);
	}
	return NULL;
}
