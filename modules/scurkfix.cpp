// sc2kfix modules/scurkfix.cpp: fixes for SCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

#define MISCHOOK_SCURK1996_DEBUG_OTHER 1
#define MISCHOOK_SCURK1996_DEBUG_INTERNAL 2

#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURK1996_DEBUG
#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_scurk1996_debug = MISCHOOK_SCURK1996_DEBUG;

DWORD dwSCURKAppTimestamp = 0;
DWORD dwSCURKAppVersion = SC2KVERSION_UNKNOWN;
HMODULE hSCURKAppModule = NULL;

DWORD *pPlaceTileDlgThis = NULL;

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

extern "C" __declspec(naked) void __cdecl Hook_SCURK1996SE_AnimationFix(void) {
	__asm {
		push 0x81
		push 0
		push 0
		mov eax, [ebx]					// this
		mov eax, [eax + 0x10]
		push eax						// hWnd
		call [RedrawWindow]
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp
		retn
	}
}

extern "C" void __cdecl Hook_SCURK1996SE_DebugOut(char const *fmt, ...) {
	va_list args;
	int len;
	char* buf;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_INTERNAL) == 0)
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

extern "C" void __cdecl Hook_SCURK1996SE_PlaceTileListDlg_SetupWindow(DWORD *pThis) {
	char szTileStr[80 + 1];
	int nItem, nMax;
	int nIdx;
	int iCXHScroll, imainRight, imainBottom, ilbCX, ilbCY;
	TBC45XRect mainRect, lbRect;

	pPlaceTileDlgThis = pThis;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_SetupWindow(0x%06X)\n", _ReturnAddress(), pThis);

	strcpy_s(szTileStr, sizeof(szTileStr) - 1, "Tile");
	GameMain_BCDialog_SetupWindow_SCURK1996SE(pThis);

	iCXHScroll = GetSystemMetrics(SM_CXHSCROLL);

	// First resize the dialogue.
	GetClientRect(*(HWND *)(*pThis + 16), &mainRect);
	imainRight = pThis[14] + iCXHScroll - mainRect.right;
	imainBottom = pThis[11] - mainRect.bottom;
	GetWindowRect(*(HWND *)(*pThis + 16), &mainRect);
	mainRect.right += imainRight + 8;
	mainRect.bottom += imainBottom + 8;
	SetWindowPos(*(HWND *)(*pThis + 16), HWND_TOP, mainRect.left, mainRect.top, mainRect.right - mainRect.left, mainRect.bottom - mainRect.top, SWP_NOZORDER | SWP_NOMOVE);
	
	// Then resize the listbox control.
	// If it is done in the wrong order it will fail "hard"
	// on Windows 11 24H2+.
	// Adjust the width and height slightly as well...
	// otherwise it will still fail "hard".
	GetWindowRect(*(HWND *)(pThis[10] + 16), &lbRect);
	ilbCX = (mainRect.right - mainRect.left) - 8;
	ilbCY = (mainRect.bottom - mainRect.top) - 8;
	SetWindowPos(*(HWND *)(pThis[10] + 16), HWND_TOP, lbRect.left, lbRect.top, ilbCX + 2, ilbCY + 2, SWP_NOZORDER | SWP_NOMOVE);

	GameMain_BCWindow_HandleMessage_SCURK1996SE((DWORD *)pThis[10], LB_SETCOLUMNWIDTH, pThis[14], 0);

	nMax = wTileObjects_SCURK1996SE[3 * pThis[17]] + wTileObjects_SCURK1996SE[3 * pThis[17] + 1] - 1;
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "pThis[17](%d), nMax(%d), pThis[13](%d)\n", pThis[17], nMax, pThis[13]);
	for (nItem = wTileObjects_SCURK1996SE[3 * pThis[17]]; nMax > nItem; nItem += pThis[13]) {
		sprintf_s(szTileStr, sizeof(szTileStr) - 1, "Tile%04d%04d", nItem, nItem + pThis[13] - 1);
		nIdx = GameMain_BCListBox_AddString_SCURK1996SE((DWORD *)pThis[10], szTileStr);
		if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
			ConsoleLog(LOG_DEBUG, "nItem(%d), szTileStr[%s], nIdx(%d)\n", nItem, szTileStr, nIdx);
		GameMain_BCListBox_SetItemData_SCURK1996SE((DWORD *)pThis[10], nIdx, nItem);
	}
}

extern "C" void __cdecl Hook_SCURK1996SE_PlaceTileListDlg_EvLButtonDblClk(DWORD *pThis, unsigned int nFlags, TBC45XPoint *pt) {
	int nCurSel;
	int nPosOne, nPosTwo;
	char szBuf[80 + 1];
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLButtonDblClk(0x%06X, %u, %d/%d)\n", _ReturnAddress(), pThis, nFlags, pt->x, pt->y);

	nCurSel = GameMain_BCListBox_GetSelIndex_SCURK1996SE((DWORD *)pThis[10]);
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSel(%d)\n", nCurSel);

	GetCursorPos(&curPt);
	GetWindowRect(*(HWND *)(pThis[10] + 16), &lbRect);
	pThis[16] = (curPt.x - lbRect.left) / (int)pThis[12];
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "pThis[16](%d)\n", pThis[16]);

	GameMain_BCListBox_GetString_SCURK1996SE((DWORD *)pThis[10], szBuf, nCurSel);
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d)\n", nPosOne, nPosTwo);
	pThis[15] = pThis[16] + nPosOne;
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "pThis[15](%d)\n", pThis[15]);
}

extern "C" void __cdecl Hook_SCURK1996SE_PlaceTileListDlg_EvLBNSelChange(DWORD *pThis) {
	int nCurSel;
	int nPosOne, nPosTwo;
	int nValOne, nValTwo;
	char szBuf[80 + 1];
	char *pLongTileName;
	DWORD *pWindow, *pCursorModule;
	TBC45XPoint curPt;
	TBC45XRect lbRect;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "0x%06X -> PlaceTileListDlg_EvLBNSelChange(0x%06X)\n", _ReturnAddress(), pThis);

	nCurSel = GameMain_BCListBox_GetSelIndex_SCURK1996SE((DWORD *)pThis[10]);
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "nCurSel(%d)\n", nCurSel);

	GameMain_BCListBox_GetString_SCURK1996SE((DWORD *)pThis[10], szBuf, nCurSel);
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "szBuf(%s)\n", szBuf);

	// These 3 lines have been added since in Windows 11 24H2-onwards
	// it seems as if pThis[18] is not being set correctly.
	// The following code is partially from the EvLButtonDblClk() call.
	GetCursorPos(&curPt);
	GetWindowRect(*(HWND *)(pThis[10] + 16), &lbRect);
	pThis[18] = (curPt.x - lbRect.left);

	nValOne = (int)pThis[18] / (int)pThis[12];
	sscanf_s(szBuf, "Tile%04d%04d", &nPosOne, &nPosTwo);
	nValTwo = nValOne + nPosOne;
	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) != 0)
		ConsoleLog(LOG_DEBUG, "nPosOne(%d), nPosTwo(%d), nValOne(%d), nValTwo(%d)\n", nPosOne, nPosTwo, nValOne, nValTwo);

	if (nValTwo >= wTileObjects_SCURK1996SE[3 * pThis[17] + 1] + wTileObjects_SCURK1996SE[3 * pThis[17]]) {
		GameMain_winscurkApp_ScurkSound_SCURK1996SE(gScurkApplication_SCURK1996SE, 3);
		pThis[20] = 0;
	}
	else {
		pThis[16] = nValOne;
		pThis[15] = nValTwo;
		pThis[20] = 1;
		pLongTileName = GameMain_EditableTileSet_GetLongName_SCURK1996SE((DWORD *)gScurkApplication_SCURK1996SE[32], pThis[15]);
		GameMain_BCDialog_SetCaption_SCURK1996SE(pThis, pLongTileName);
		wtoolValue_SCURK1996SE = 8;
		*(&wtoolNum_SCURK1996SE + 8) = *((WORD *)pThis + 30);
		InvalidateRect(*(HWND *)(*pThis + 16), 0, 0);
		pWindow = GameMain_winscurkApp_GetPlaceWindow_SCURK1996SE(gScurkApplication_SCURK1996SE);
		GameMain_winscurkPlaceWindow_ClearCurrentTool_SCURK1996SE(pWindow);
		pCursorModule = *(DWORD **)(*pThis + 108);
		GameMain_BCWindow_SetCursor_SCURK1996SE((DWORD *)pWindow[1], pCursorModule, (const char *)30006);
		GameMain_winscurkApp_ScurkSound_SCURK1996SE(gScurkApplication_SCURK1996SE, 1);
	}
}

extern "C" void __cdecl Hook_SCURK1996SE_BCDialog_CmCancel(DWORD *pThis) {
	// We really don't want to close the Place&Pick object selection
	// dialogue by pressing escape...
	if (pPlaceTileDlgThis && pPlaceTileDlgThis == pThis)
		return;
	GameMain_BCDialog_EvClose_SCURK1996SE(pThis);
}

BOOL InjectSCURKFix(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk1996_debug = DEBUG_FLAGS_EVERYTHING;

	ConsoleLog(LOG_INFO, "CORE: Injecting SCURK fixes...\n");
	hSCURKAppModule = GetModuleHandle(NULL);
	dwSCURKAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSCURKAppModule)->e_lfanew + (UINT_PTR)hSCURKAppModule))->FileHeader.TimeDateStamp;
	switch (dwSCURKAppTimestamp) {
	case 0xBC7B1F0E:							// Yes, for some reason the timestamp is set to 2070.
		dwSCURKAppVersion = SC2KVERSION_1996;
		ConsoleLog(LOG_DEBUG, "CORE: SCURK version 1996SE detected.\n");
		break;
	default:
		ConsoleLog(LOG_ERROR, "CORE: Could not detect SCURK version (got timestamp 0x%08X). Not injecting fixes.\n", dwSCURKAppTimestamp);
		return TRUE;
	}

	if (dwSCURKAppVersion == SC2KVERSION_1996)
		InstallRegistryPathingHooks_SCURK1996SE();

	// Tell the rest of the plugin we're in SCURK
	bInSCURK = TRUE;
	// return TRUE; 
	// Hook for palette animation fix
	// Intercept call to 0x480140 at 0x48A683
	VirtualProtect((LPVOID)0x4497F5, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4497F5, Hook_SCURK1996SE_AnimationFix);
	ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix for SCURK.\n");

	// Add back the internal debug notices for tracing purposes.
	VirtualProtect((LPVOID)0x4132EC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4132EC, Hook_SCURK1996SE_DebugOut);

	// These hooks are to account for the Place&Pick selection dialogue
	// malfunctions that were occurring under Win11 24H2+:
	// 1) The Listbox was no longer displayed
	// 2) Mouse selection was no longer recognised - or rather
	//    the stored point within the window wasn't recorded.
	VirtualProtect((LPVOID)0x4104B8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4104B8, Hook_SCURK1996SE_PlaceTileListDlg_SetupWindow);
	VirtualProtect((LPVOID)0x410D94, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410D94, Hook_SCURK1996SE_PlaceTileListDlg_EvLButtonDblClk);
	VirtualProtect((LPVOID)0x410ED0, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410ED0, Hook_SCURK1996SE_PlaceTileListDlg_EvLBNSelChange);

	// This hook is to prevent the Place&Pick selection dialogue
	// from being unintentionally closed; it catches and ignores
	// the cancel (esc) action.
	VirtualProtect((LPVOID)0x46FB26, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x46FB26, Hook_SCURK1996SE_BCDialog_CmCancel);
	
	return TRUE;
}