// sc2kfix modules/scurkfix_primary.cpp: fixes for SCURK - primary (1995) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

#define MISCHOOK_SCURKPRIMARY_DEBUG_INTERNAL 1
#define MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE 2
#define MISCHOOK_SCURKPRIMARY_DEBUG_PLACEANDCOPY 4

#define MISCHOOK_SCURKPRIMARY_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURKPRIMARY_DEBUG
#define MISCHOOK_SCURKPRIMARY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_scurkprimary_debug = MISCHOOK_SCURKPRIMARY_DEBUG;

// Commented out but retained, just in case any manual VTable entry
// confirmation checks are needed.
/*
static void SCURK_VTable_Check(DWORD *pThis, const char *s) {
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::SetSelIndex\n", s, (*(DWORD *)(pThis[10] + 8) + 196)); // TListBox::SetSelIndex
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::GetSelIndex\n", s, (*(DWORD *)(pThis[10] + 8) + 192)); // TListBox::GetSelIndex
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::AddString\n", s, (*(DWORD *)(pThis[10] + 8) + 180)); // TListBox::AddString
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::ClearList\n", s, (*(DWORD *)(pThis[10] + 8) + 172)); // TListBox::ClearList
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::SetItemData\n", s, (*(DWORD *)(pThis[10] + 8) + 160)); // TListBox::SetItemData
ConsoleLog(LOG_DEBUG, "SCURK_VTable_Check[class path - %s] - 0x%06X - TListBox::GetString\n", s, (*(DWORD *)(pThis[10] + 8) + 152)); // TListBox::GetString
}
*/

extern "C" void Hook_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	TBC45XPalette *pPal;
	TBC45XClientDC clDC;
	TBC45XMDIChild *pMDIChild;
	HWND hWndChild, hWndObjSelect;

	if (!IsIconic(pThis->pWnd->HWindow)) {
		pPal = GameMain_winscurkApp_GetPalette_SCURKPrimary(gScurkApplication_SCURKPrimary);
		GameMain_BCClientDC_Construct_SCURKPrimary(&clDC, pThis->pWnd->HWindow);
		GameMain_BCDC_SelectObjectPalette_SCURKPrimary(&clDC, pPal, 0);
		if (wColFastCnt_SCURKPrimary == 5) {
			GameMain_winscurkMDIClient_RotateColors_SCURKPrimary(pThis, 1);
			AnimatePalette((HPALETTE)pPal->Handle, 0xAB, 0x31, pThis->mFastColors);
			wColFastCnt_SCURKPrimary = 0;
		}
		if (wColSlowCnt_SCURKPrimary == 30) {
			GameMain_winscurkMDIClient_RotateColors_SCURKPrimary(pThis, 0);
			AnimatePalette((HPALETTE)pPal->Handle, 0xE0, 0x10, pThis->mSlowColors);
			wColSlowCnt_SCURKPrimary = 0;
		}
		++wColFastCnt_SCURKPrimary;
		++wColSlowCnt_SCURKPrimary;
		GameMain_BCWindowDC_Destruct_SCURKPrimary(&clDC, 0);

		// Only call redraw if the given MDIChild is active, rather than
		// refreshing all windows from pThis->pWnd->HWindow downwards.
		//
		// This reduces "a bit" of the flickering that was otherwise occurring
		// across all windows; at this stage it is only limited to the active
		// MDI Child.
		pMDIChild = GameMain_BCMDIClient_GetActiveMDIChild_SCURKPrimary(pThis);
		if (pMDIChild) {
			hWndChild = 0;
			hWndObjSelect = 0;
			if (pMDIChild == (TBC45XMDIChild *)pThis->mPlaceWindow) {
				hWndChild = pThis->mPlaceWindow->__wndHead.pWnd->HWindow;
				hWndObjSelect = pThis->mPlaceWindow->pPlaceTileListDlg->pWnd->HWindow;
			}
			else if (pMDIChild == (TBC45XMDIChild *)pThis->mMoverWindow)
				hWndChild = pThis->mMoverWindow->__wndHead.pWnd->HWindow;
			else if (pMDIChild == (TBC45XMDIChild *)pThis->mEditWindow)
				hWndChild = ((winscurkParMDIChild *)pThis->mEditWindow)->__wndHead.pWnd->HWindow;

			if (hWndChild)
				RedrawWindow(hWndChild, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
			if (hWndObjSelect)
				RedrawWindow(hWndObjSelect, 0, 0, RDW_ALLCHILDREN | RDW_INVALIDATE);
		}
	}
}

extern "C" void __cdecl Hook_SCURKPrimary_DebugOut(char const *fmt, ...) {
	va_list args;
	int len;
	char* buf;

	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_INTERNAL) == 0)
		return;

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		ConsoleLog(LOG_DEBUG, "0x%06X -> gDebugOut(): %s", _ReturnAddress(), buf);

		free(buf);
	}

	va_end(args);
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis) {
	char szTileStr[80 + 1];
	int nItem, nMax;
	int nIdx;
	int iCXHScroll, imainRight, imainBottom, ilbCX, ilbCY;
	TBC45XRect mainRect, lbRect;

	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_SetupWindow(0x%06X)\n", _ReturnAddress(), pThis);

	strcpy_s(szTileStr, sizeof(szTileStr) - 1, "Tile");
	GameMain_BCDialog_SetupWindow_SCURKPrimary(pThis);

	iCXHScroll = GetSystemMetrics(SM_CXHSCROLL);

	// First resize the dialogue.
	GetClientRect(pThis->pWnd->HWindow, &mainRect);
	imainRight = pThis->nMaxHitArea + iCXHScroll - mainRect.right;
	imainBottom = pThis->nLBButtonWidth - mainRect.bottom;
	GetWindowRect(pThis->pWnd->HWindow, &mainRect);
	mainRect.right += imainRight + 8;
	mainRect.bottom += imainBottom + 8;
	SetWindowPos(pThis->pWnd->HWindow, HWND_TOP, mainRect.left, mainRect.top, mainRect.right - mainRect.left, mainRect.bottom - mainRect.top, SWP_NOZORDER | SWP_NOMOVE);
	
	// Then resize the listbox control.
	// If it is done in the wrong order it will fail "hard"
	// on Windows 11 24H2+.
	// Adjust the width and height slightly as well...
	// otherwise it will still fail "hard".
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	ilbCX = (mainRect.right - mainRect.left) - 8;
	ilbCY = (mainRect.bottom - mainRect.top) - 8;
	SetWindowPos(pThis->pListBox->HWindow, HWND_TOP, lbRect.left, lbRect.top, ilbCX + 2, ilbCY + 2, SWP_NOZORDER | SWP_NOMOVE);

	GameMain_BCWindow_HandleMessage_SCURKPrimary(pThis->pListBox, LB_SETCOLUMNWIDTH, pThis->nMaxHitArea, 0);

	nMax = wTileObjects_SCURKPrimary[3 * pThis->mNumTiles] + wTileObjects_SCURKPrimary[3 * pThis->mNumTiles + 1] - 1;
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->mNumTiles(%d), nMax(%d), pThis->nTileRow(%d)\n", pThis[17], nMax, pThis->nTileRow);
	for (nItem = wTileObjects_SCURKPrimary[3 * pThis->mNumTiles]; nMax > nItem; nItem += pThis->nTileRow) {
		sprintf_s(szTileStr, sizeof(szTileStr) - 1, "Tile%04d%04d", nItem, nItem + pThis->nTileRow - 1);
		nIdx = GameMain_BCListBox_AddString_SCURKPrimary(pThis->pListBox, szTileStr);
		if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
			ConsoleLog(LOG_DEBUG, "nItem(%d), szTileStr[%s], nIdx(%d)\n", nItem, szTileStr, nIdx);
		GameMain_BCListBox_SetItemData_SCURKPrimary(pThis->pListBox, nIdx, nItem);
	}
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis) {
	int nCurSelRowIdx;
	int nPosOne, nPosTwo;
	char szBuf[80 + 1];
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLButtonDblClk(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = GameMain_BCListBox_GetSelIndex_SCURKPrimary(pThis->pListBox);
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	GetCursorPos(&curPt);
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	pThis->nXPos = (curPt.x - lbRect.left) / pThis->nPosWidth;
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nXPos(%d)\n", pThis->nXPos);

	GameMain_BCListBox_GetString_SCURKPrimary(pThis->pListBox, szBuf, nCurSelRowIdx);
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d)\n", nPosOne, nPosTwo);
	pThis->nCurPos = pThis->nXPos + nPosOne;
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "pThis->nCurPos(%d)\n", pThis->nCurPos);
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis) {
	int nCurSelRowIdx;
	int nPosOne, nPosTwo;
	int nValOne, nValTwo;
	char szBuf[80 + 1];
	char *pLongTileName;
	winscurkPlaceWindow *pWindow;
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLBNSelChange(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSelRowIdx = GameMain_BCListBox_GetSelIndex_SCURKPrimary(pThis->pListBox);
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSelRowIdx(%d)\n", nCurSelRowIdx);

	GameMain_BCListBox_GetString_SCURKPrimary(pThis->pListBox, szBuf, nCurSelRowIdx);
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	// These 3 lines have been added since in Windows 11 24H2-onwards
	// it seems as if pThis[18] is not being set correctly.
	// The following code is partially from the EvLButtonDblClk() call.
	GetCursorPos(&curPt);
	GetWindowRect(pThis->pListBox->HWindow, &lbRect);
	pThis->nChldHndlorX = (curPt.x - lbRect.left);

	nValOne = pThis->nChldHndlorX / pThis->nPosWidth;
	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	nValTwo = nValOne + nPosOne;
	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PICKANDPLACE) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d), nValOne(%d), nValTwo(%d)\n", nPosOne, nPosTwo, nValOne, nValTwo);

	if (nValTwo >= wTileObjects_SCURKPrimary[3 * pThis->mNumTiles + 1] + wTileObjects_SCURKPrimary[3 * pThis->mNumTiles]) {
		GameMain_winscurkApp_ScurkSound_SCURKPrimary(gScurkApplication_SCURKPrimary, 3);
		pThis->nSelected = 0;
	}
	else {
		pThis->nXPos = nValOne;
		pThis->nCurPos = nValTwo;
		pThis->nSelected = 1;
		pLongTileName = GameMain_EditableTileSet_GetLongName_SCURKPrimary(gScurkApplication_SCURKPrimary->mWorkingTiles, pThis->nCurPos);
		GameMain_BCDialog_SetCaption_SCURKPrimary(pThis, pLongTileName);
		wtoolValue_SCURKPrimary = 8;
		*(&wtoolNum_SCURKPrimary + 8) = pThis->nCurPos;
		InvalidateRect(pThis->pWnd->HWindow, 0, 0);
		pWindow = GameMain_winscurkApp_GetPlaceWindow_SCURKPrimary(gScurkApplication_SCURKPrimary);
		GameMain_winscurkPlaceWindow_ClearCurrentTool_SCURKPrimary(pWindow);
		GameMain_BCWindow_SetCursor_SCURKPrimary(pWindow->__wndHead.pWnd, pThis->pWnd->Module, (const char *)30006);
		GameMain_winscurkApp_ScurkSound_SCURKPrimary(gScurkApplication_SCURKPrimary, 1);
	}
}

extern "C" void __declspec(naked) Hook_SCURKPrimary_MoverWindow_DisableMaximizeBox(void) {
	TBC45XWindow *pWnd;

	__asm {
		mov eax, [ebx + 0x4]
		mov [pWnd], eax
	}

	if ((mischook_scurkprimary_debug & MISCHOOK_SCURKPRIMARY_DEBUG_PLACEANDCOPY) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> DisableMaximizeBox()\n", _ReturnAddress());

	if (GetSystemMetrics(SM_CXSCREEN) > 700)
		pWnd->Attr.Style &= ~WS_MAXIMIZEBOX;
	else
		pWnd->Attr.Style |= WS_MAXIMIZE;

	__asm {
		mov eax, pWnd
		mov [ebx + 0x4], eax
	}
	GAMEJMP(0x44E2EF);
}

extern "C" void __cdecl Hook_SCURKPrimary_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi) {
	LONG nCXScreen, x, y;

	GameMain_BCWindow_DefaultProcessing_SCURKPrimary(pThis->__wndHead.pWnd);
	nCXScreen = GetSystemMetrics(SM_CXSCREEN);
	if (nCXScreen <= 640) {
		x = 512;
		y = 256;
	}
	else {
		x = 640;
		y = 480;
	}

	pMmi->ptMinTrackSize.x = x;
	pMmi->ptMinTrackSize.y = y;

	pMmi->ptMaxPosition.x = 0;
	pMmi->ptMaxPosition.y = 0;
	pMmi->ptMaxSize.x = x;
	pMmi->ptMaxSize.y = y;

	pMmi->ptMaxTrackSize.x = x;
	pMmi->ptMaxTrackSize.y = y;
}

extern "C" void __cdecl Hook_SCURKPrimary_BCDialog_CmCancel(TBC45XDialog *pThis) {
	winscurkPlaceWindow *pWindow;

	// We really don't want to close the Place&Pick object selection
	// dialogue by pressing escape...
	pWindow = GameMain_winscurkApp_GetPlaceWindow_SCURKPrimary(gScurkApplication_SCURKPrimary);
	if (pWindow && pWindow->pPlaceTileListDlg && pWindow->pPlaceTileListDlg == (TPlaceTileListDlg *)pThis)
		return;

	GameMain_BCDialog_EvClose_SCURKPrimary(pThis);
}

void InstallFixes_SCURKPrimary(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurkprimary_debug = DEBUG_FLAGS_EVERYTHING;
	
	InstallRegistryPathingHooks_SCURKPrimary();

	// Hook for palette animation fix
	// Intercept call to 0x480140 at 0x48A683
	VirtualProtect((LPVOID)0x4496D4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4496D4, Hook_winscurkMDIClient_CycleColors);
	ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix for SCURK.\n");

	// Add back the internal debug notices for tracing purposes.
	VirtualProtect((LPVOID)0x4132EC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4132EC, Hook_SCURKPrimary_DebugOut);

	// These hooks are to account for the Place&Pick selection dialogue
	// malfunctions that were occurring under Win11 24H2+:
	// 1) The Listbox was no longer displayed
	// 2) Mouse selection was no longer recognised - or rather
	//    the stored point within the window wasn't recorded.
	VirtualProtect((LPVOID)0x4104B8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4104B8, Hook_SCURKPrimary_PlaceTileListDlg_SetupWindow);
	VirtualProtect((LPVOID)0x410D94, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410D94, Hook_SCURKPrimary_PlaceTileListDlg_EvLButtonDblClk);
	VirtualProtect((LPVOID)0x410ED0, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410ED0, Hook_SCURKPrimary_PlaceTileListDlg_EvLBNSelChange);

	// Temporarily remove the TFrameWindow::EvSize call.
	// This avoids some redrawing strangeness that otherwise occurs
	// if the Pick&Copy window is in-focus and you then restore
	// the Place&Pick window to its non-maximized state.
	VirtualProtect((LPVOID)0x44FE19, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x44FE19, 0x90, 13);

	// Temporarily disable the maximizebox style if SM_CXSCREEN is above 700.
	VirtualProtect((LPVOID)0x44E2D7, 24, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x44E2D7, 0x90, 24);
	NEWJMP((LPVOID)0x44E2D7, Hook_SCURKPrimary_MoverWindow_DisableMaximizeBox);

	// Temporarily lock the Min/Max size of the Pick&Copy window
	// to avoid rendering the area non-functional.
	VirtualProtect((LPVOID)0x450080, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x450080, Hook_SCURKPrimary_MoverWindow_EvGetMinMaxInfo);

	// This hook is to prevent the Place&Pick selection dialogue
	// from being unintentionally closed; it catches and ignores
	// the cancel (esc) action.
	VirtualProtect((LPVOID)0x46FB26, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x46FB26, Hook_SCURKPrimary_BCDialog_CmCancel);
}
