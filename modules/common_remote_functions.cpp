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

void *__cdecl R_BOR_WRP_gAllocBlock(size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gAllocBlock_SCURKPrimary(nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gAllocBlock_SCURK1996(nSz);
	}
	return NULL;
}

void *__cdecl R_BOR_WRP_gResizeBlock(BYTE *pBlock, size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_gResizeBlock_SCURKPrimary(pBlock, nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_gResizeBlock_SCURK1996(pBlock, nSz);
	}
	return NULL;
}

void __cdecl R_BOR_WRP_gFreeBlock(void *pBlock) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_gFreeBlock_SCURKPrimary(pBlock);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_gFreeBlock_SCURK1996(pBlock);
	}
}

void __stdcall R_BOR_WRP_gUpdateWaitWindow() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_gUpdateWaitWindow_SCURKPrimary();
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_gUpdateWaitWindow_SCURK1996();
	}
}

void R_BOR_WRP_DC_SelectObjectPalette(TBC45XDC *pThis, TBC45XPalette *pPal, int nVal) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDC_SelectObjectPalette_SCURKPrimary(pThis, pPal, nVal);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDC_SelectObjectPalette_SCURK1996(pThis, pPal, nVal);
	}
}

LRESULT R_BOR_WRP_Window_EvCommand(TBC45XWindow *pThis, DWORD dwID, HWND hWndCtl, DWORD dwNotifyCode) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCWindow_EvCommand_SCURKPrimary(pThis, dwID, hWndCtl, dwNotifyCode);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCWindow_EvCommand_SCURK1996(pThis, dwID, hWndCtl, dwNotifyCode);
	}
	return -1;
}

unsigned int R_BOR_WRP_Window_DefaultProcessing(TBC45XParWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCWindow_DefaultProcessing_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCWindow_DefaultProcessing_SCURK1996(pThis);
	}
	return 0;
}

LRESULT R_BOR_WRP_Window_HandleMessage(TBC45XParWindow *pThis, unsigned int msg, WPARAM wParam, LPARAM lParam) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCWindow_HandleMessage_SCURKPrimary(pThis, msg, wParam, lParam);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCWindow_HandleMessage_SCURK1996(pThis, msg, wParam, lParam);
	}
	return 0;
}

TBC45XWindow *R_BOR_WRP_GetWindowPtr(HWND hWndTarget, TBC45XApplication *pApp) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_GetWindowPtr_SCURKPrimary(hWndTarget, pApp);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_GetWindowPtr_SCURK1996(hWndTarget, pApp);
	}
	return NULL;
}

void R_BOR_WRP_Window_SetCursor(TBC45XParWindow *pThis, TBC45XModule *pModule, const char *pResID) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCWindow_SetCursor_SCURKPrimary(pThis, pModule, pResID);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCWindow_SetCursor_SCURK1996(pThis, pModule, pResID);
	}
}

void R_BOR_WRP_WindowDC_Destruct(TBC45XWindowDC *pThis, char c) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCWindowDC_Destruct_SCURKPrimary(pThis, c);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCWindowDC_Destruct_SCURK1996(pThis, c);
	}
}

TBC45XClientDC *R_BOR_WRP_ClientDC_Construct(TBC45XClientDC *pThis, HWND hWnd) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCClientDC_Construct_SCURKPrimary(pThis, hWnd);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCClientDC_Construct_SCURK1996(pThis, hWnd);
	}
	return NULL;
}

void R_BOR_WRP_Dialog_SetupWindow(TBC45XParDialog *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_SetupWindow_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_SetupWindow_SCURK1996(pThis);
	}
}

void R_BOR_WRP_Dialog_SetCaption(TBC45XParDialog *pThis, char *pCaption) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_SetCaption_SCURKPrimary(pThis, pCaption);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_SetCaption_SCURK1996(pThis, pCaption);
	}
}

void R_BOR_WRP_Dialog_EvClose(TBC45XParDialog *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_BCDialog_EvClose_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_BCDialog_EvClose_SCURK1996(pThis);
	}
}

HWND R_BOR_WRP_FrameWindow_GetCommandTarget(TBC45XFrameWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCFrameWindow_GetCommandTarget_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCFrameWindow_GetCommandTarget_SCURK1996(pThis);
	}
	return NULL;
}

int R_BOR_WRP_ListBox_GetSelIndex(TBC45XParListBox *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_GetSelIndex_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_GetSelIndex_SCURK1996(pThis);
	}
	return 0;
}

LRESULT R_BOR_WRP_ListBox_GetString(TBC45XParListBox *pThis, char *pString, int nIndex) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_GetString_SCURKPrimary(pThis, pString, nIndex);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_GetString_SCURK1996(pThis, pString, nIndex);
	}
	return 0;
}

int R_BOR_WRP_ListBox_SetItemData(TBC45XParListBox *pThis, int nIndex, unsigned int uItemData) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_SetItemData_SCURKPrimary(pThis, nIndex, uItemData);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_SetItemData_SCURK1996(pThis, nIndex, uItemData);
	}
	return 0;
}

int R_BOR_WRP_ListBox_AddString(TBC45XParListBox *pThis, const char *pString) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCListBox_AddString_SCURKPrimary(pThis, pString);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCListBox_AddString_SCURK1996(pThis, pString);
	}
	return 0;
}

TBC45XMDIChild *R_BOR_WRP_MDIClient_GetActiveMDIChild(TBC45XParMDIClient *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCMDIClient_GetActiveMDIChild_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCMDIClient_GetActiveMDIChild_SCURK1996(pThis);
	}
	return NULL;
}

BOOL R_BOR_MDIFrame_SetMenu(TBC45XMDIFrame *pThis, HMENU hMenu) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BCMDIFrame_SetMenu_SCURKPrimary(pThis, hMenu);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BCMDIFrame_SetMenu_SCURK1996(pThis, hMenu);
	}
	return FALSE;
}

char *R_BOR_strnewdup(char *pStr, size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_BC_strnewdup_SCURKPrimary(pStr, nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_BC_strnewdup_SCURK1996(pStr, nSz);
	}
	return NULL;
}

void *R_BOR_Op_New(size_t nSz) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_Op_New_SCURKPrimary(nSz);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_Op_New_SCURK1996(nSz);
	}
	return NULL;
}

// SCURK global variable wrappers

__int16 *R_SCURK_WRP_GetwTileObjects() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return wTileObjects_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return wTileObjects_SCURK1996;
	}
	return NULL;
}

WORD *R_SCURK_WRP_GetwColFastCnt() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wColFastCnt_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wColFastCnt_SCURK1996;
	}
	return NULL;
}

WORD *R_SCURK_WRP_GetwColSlowCnt() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wColSlowCnt_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wColSlowCnt_SCURK1996;
	}
	return NULL;
}

__int16 *R_SCURK_WRP_GetwToolNum() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return wtoolNum_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return wtoolNum_SCURK1996;
	}
	return NULL;
}

__int16 *R_SCURK_WRP_GetwToolValue() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return &wtoolValue_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return &wtoolValue_SCURK1996;
	}
	return 0;
}

TBC45XDib *R_SCURK_WRP_mTileBack() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return mTileBack_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return mTileBack_SCURK1996;
	}
	return NULL;
}

winscurkApp *R_SCURK_WRP_winscurkApp_GetPointerToClass() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return gScurkApplication_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return gScurkApplication_SCURK1996;
	}
	return NULL;
}

BC45Xstring *R_SCURK_WRP_GetTAppInitCmdLine() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return TAppInitCmdLine_SCURKPrimary;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return TAppInitCmdLine_SCURK1996;
	}
	return NULL;
}

// SCURK address wrappers

DWORD R_SCURK_ADDR_FrameWindow_EvCommand_To_TDecoratedFrame_EvCommand() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return 0x46DF05;
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return 0x46E685;
	}
	return 0;
}

// CWinGBitmap function wrappers

int R_SCURK_WRP_WinGBitmap_Width(CWinGBitmap *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_WinGBitmap_Width_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_WinGBitmap_Width_SCURK1996(pThis);
	}
	return -1;
}

int R_SCURK_WRP_WinGBitmap_Height(CWinGBitmap *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_WinGBitmap_Height_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_WinGBitmap_Height_SCURK1996(pThis);
	}
	return -1;
}

// TEncode function wrappers

TEncodeDib *R_SCURK_WRP_EncodeDib_Construct_Dimens(TEncodeDib *pThis, LONG nWidth, LONG nHeight, DWORD dwColors, WORD wMode) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EncodeDib_Construct_Dimens_SCURKPrimary(pThis, nWidth, nHeight, dwColors, wMode);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EncodeDib_Construct_Dimens_SCURK1996(pThis, nWidth, nHeight, dwColors, wMode);
	}
	return NULL;
}

void R_SCURK_WRP_EncodeDib_Destruct(TEncodeDib *pThis, char c) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_Destruct_SCURKPrimary(pThis, c);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_Destruct_SCURK1996(pThis, c);
	}
}

void R_SCURK_WRP_EncodeDib_mFillAt(TEncodeDib *pThis, TBC45XPoint *pt) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_mFillAt_SCURKPrimary(pThis, pt);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_mFillAt_SCURK1996(pThis, pt);
	}
}

void R_SCURK_WRP_EncodeDib_mFillLine(TEncodeDib *pThis, TBC45XPoint *pt, BYTE Bit) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_mFillLine_SCURKPrimary(pThis, pt, Bit);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_mFillLine_SCURK1996(pThis, pt, Bit);
	}
}

int R_SCURK_WRP_EncodeDib_mDetermineShapeHeight(TEncodeDib *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EncodeDib_mDetermineShapeHeight_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EncodeDib_mDetermineShapeHeight_SCURK1996(pThis);
	}
	return -1;
}

void R_SCURK_WRP_EncodeDib_mShrink(TEncodeDib *pThis, TBC45XDib *pInDib, int nScaleBy) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_mShrink_SCURKPrimary(pThis, pInDib, nScaleBy);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_mShrink_SCURK1996(pThis, pInDib, nScaleBy);
	}
}

void R_SCURK_WRP_EncodeDib_mAcquireEncodedShapeData(TEncodeDib *pThis, TEncodeDib *pEncDib) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_mAcquireEncodedShapeData_SCURKPrimary(pThis, pEncDib);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_mAcquireEncodedShapeData_SCURK1996(pThis, pEncDib);
	}
}

void R_SCURK_WRP_EncodeDib_mEncodeShape(TEncodeDib *pThis, WORD shapeHeight, WORD shapeWidth, WORD nOffSet) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EncodeDib_mEncodeShape_SCURKPrimary(pThis, shapeHeight, shapeWidth, nOffSet);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EncodeDib_mEncodeShape_SCURK1996(pThis, shapeHeight, shapeWidth, nOffSet);
	}
}

// cEditableTileSet function wrappers

int R_SCURK_WRP_EditableTileSet_mShapeNumToEditableNum(cEditableTileSet *pThis, int nShapNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditableTileSet_mShapeNumToEditableNum_SCURKPrimary(pThis, nShapNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditableTileSet_mShapeNumToEditableNum_SCURK1996(pThis, nShapNum);
	}
	return 0;
}

char *R_SCURK_WRP_EditableTileSet_GetLongName(cEditableTileSet *pThis, int nEdNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditableTileSet_GetLongName_SCURKPrimary(pThis, nEdNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditableTileSet_GetLongName_SCURK1996(pThis, nEdNum);
	}
	return NULL;
}

int R_SCURK_WRP_EditableTileSet_mGetShapeWidth(cEditableTileSet *pThis, int nEdNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditableTileSet_mGetShapeWidth_SCURKPrimary(pThis, nEdNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditableTileSet_mGetShapeWidth_SCURK1996(pThis, nEdNum);
	}
	return 0;
}

void R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Dib(cEditableTileSet *pThis, TBC45XDib *pDib, int nEdNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EditableTileSet_mRenderEditableShapeToDIB_Dib_SCURKPrimary(pThis, pDib, nEdNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EditableTileSet_mRenderEditableShapeToDIB_Dib_SCURK1996(pThis, pDib, nEdNum);
	}
}

void R_SCURK_WRP_EditableTileSet_mRenderEditableShapeToDIB_Graphic(cEditableTileSet *pThis, CWinGBitmap *pGraphic, int nEdNum) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EditableTileSet_mRenderEditableShapeToDIB_Graphic_SCURKPrimary(pThis, pGraphic, nEdNum);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EditableTileSet_mRenderEditableShapeToDIB_Graphic_SCURK1996(pThis, pGraphic, nEdNum);
	}
}

void R_SCURK_WRP_EditableTileSet_mBuildSmallMedTiles(cEditableTileSet *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_EditableTileSet_mBuildSmallMedTiles_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_EditableTileSet_mBuildSmallMedTiles_SCURK1996(pThis);
	}
}

// winscurkEditWindow function wrappers

TBC45XDib *R_SCURK_WRP_EditWindow_mGetForegroundPattern(winscurkEditWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditWindow_mGetForegroundPattern_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditWindow_mGetForegroundPattern_SCURK1996(pThis);
	}
	return NULL;
}

int R_SCURK_WRP_EditWindow_mGetShapeWidth(winscurkEditWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditWindow_mGetShapeWidth_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditWindow_mGetShapeWidth_SCURK1996(pThis);
	}
	return 0;
}

BYTE R_SCURK_WRP_EditWindow_mGetForegroundColor(winscurkEditWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditWindow_mGetForegroundColor_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditWindow_mGetForegroundColor_SCURK1996(pThis);
	}
	return 0;
}

BYTE R_SCURK_WRP_EditWindow_mGetBackgroundColor(winscurkEditWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_EditWindow_mGetBackgroundColor_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_EditWindow_mGetBackgroundColor_SCURK1996(pThis);
	}
	return 0;
}

// winscurkPlaceWindow function wrappers

void R_SCURK_WRP_PlaceWindow_ClearCurrentTool(winscurkPlaceWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PlaceWindow_ClearCurrentTool_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PlaceWindow_ClearCurrentTool_SCURK1996(pThis);
	}
}

// cPaintWindow function wrappers

void R_SCURK_WRP_PaintWindow_mPreserveToUndoBuffer(cPaintWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mPreserveToUndoBuffer_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mPreserveToUndoBuffer_SCURK1996(pThis);
	}
}

void R_SCURK_WRP_PaintWindow_mApplyTileToScreen(cPaintWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mApplyTileToScreen_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mApplyTileToScreen_SCURK1996(pThis);
	}
}

void R_SCURK_WRP_PaintWindow_mClearTile(cPaintWindow *pThis, int nTileBase) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mClearTile_SCURKPrimary(pThis, nTileBase);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mClearTile_SCURK1996(pThis, nTileBase);
	}
}

void R_SCURK_WRP_PaintWindow_mClipDrawing(cPaintWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mClipDrawing_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mClipDrawing_SCURK1996(pThis);
	}
}

void R_SCURK_WRP_PaintWindow_mScreenToDib(TBC45XPoint *pt, cPaintWindow *pThis, TBC45XPoint *pPoint) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mScreenToDib_SCURKPrimary(pt, pThis, pPoint);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mScreenToDib_SCURK1996(pt, pThis, pPoint);
	}
}

void R_SCURK_WRP_PaintWindow_mPutPixel(cPaintWindow *pThis, TBC45XRect *pRect, BYTE foreColor, BYTE backColor) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mPutPixel_SCURKPrimary(pThis, pRect, foreColor, backColor);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mPutPixel_SCURK1996(pThis, pRect, foreColor, backColor);
	}
}

void R_SCURK_WRP_PaintWindow_mDraw(cPaintWindow *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mDraw_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mDraw_SCURK1996(pThis);
	}
}

void R_SCURK_WRP_PaintWindow_mEncodeShape(cPaintWindow *pThis, int nZoomLevel) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_PaintWindow_mEncodeShape_SCURKPrimary(pThis, nZoomLevel);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_PaintWindow_mEncodeShape_SCURK1996(pThis, nZoomLevel);
	}
}

// winscurkMDIClient function wrappers

void R_SCURK_WRP_winscurkMDIClient_RotateColors(winscurkMDIClient *pThis, BOOL bFast) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_winscurkMDIClient_RotateColors_SCURKPrimary(pThis, bFast);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_winscurkMDIClient_RotateColors_SCURK1996(pThis, bFast);
	}
}

// winscurkApp function wrappers

TBC45XPalette *R_SCURK_WRP_winscurkApp_GetPalette(winscurkApp *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_winscurkApp_GetPalette_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_winscurkApp_GetPalette_SCURK1996(pThis);
	}
	return NULL;
}

void R_SCURK_WRP_winscurkApp_ScurkSound(winscurkApp *pThis, int nSoundID) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			GameMain_winscurkApp_ScurkSound_SCURKPrimary(pThis, nSoundID);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			GameMain_winscurkApp_ScurkSound_SCURK1996(pThis, nSoundID);
	}
}

winscurkPlaceWindow *R_SCURK_WRP_winscurkApp_GetPlaceWindow(winscurkApp *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_winscurkApp_GetPlaceWindow_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_winscurkApp_GetPlaceWindow_SCURK1996(pThis);
	}
	return NULL;
}

winscurkEditWindow *R_SCURK_WRP_winscurkApp_GetEditWindow(winscurkApp *pThis) {
	if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
		if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
			return GameMain_winscurkApp_GetEditWindow_SCURKPrimary(pThis);
		else if (dwDetectedVersion == VERSION_SCURK_1996)
			return GameMain_winscurkApp_GetEditWindow_SCURK1996(pThis);
	}
	return NULL;
}
