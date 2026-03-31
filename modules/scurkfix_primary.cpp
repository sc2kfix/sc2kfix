// sc2kfix modules/scurkfix_primary.cpp: fixes for SCURK - primary (1995) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>
#include "../resource.h"

static DWORD dwDummy;

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

extern "C" void Hook_SCURKPrimary_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	L_SCURK_winscurkMDIClient_CycleColors(pThis);
}

extern "C" void __cdecl Hook_SCURKPrimary_DebugOut(char const *fmt, ...) {
	va_list args;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_INTERNAL) == 0)
		return;

	va_start(args, fmt);
	L_SCURK_gDebugOut(fmt, args);
	va_end(args);
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_SetupWindow(pThis);
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_EvLButtonDblClk(pThis);
}

extern "C" void __cdecl Hook_SCURKPrimary_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_EvLBNSelChange(pThis);
}

extern "C" LONG __cdecl Hook_SCURKPrimary_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, LPCSTR lpPathName) {
	return L_SCURK_EditableTileSet_mReadFromFile(pThis, lpPathName);
}

extern "C" void __declspec(naked) Hook_SCURKPrimary_MoverWindow_DisableMaximizeBox(void) {
	TBC45XWindow *pWnd;

	__asm {
		mov eax, [ebx + 0x4]
		mov [pWnd], eax
	}

	pWnd = L_SCURK_MoverWindow_DisableMaximizeBox(pWnd);

	__asm {
		mov eax, pWnd
		mov [ebx + 0x4], eax
	}
	GAMEJMP(0x44E2EF);
}

extern "C" void __cdecl Hook_SCURKPrimary_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi) {
	L_SCURK_MoverWindow_EvGetMinMaxInfo(pThis, pMmi);
}

// And we're gritting our teeth...
extern "C" void __declspec(naked) __cdecl Hook_SCURKPrimary_OwlMainCommandLineFix(void) {
	int nArgs;
	char **pArgs;

	__asm {
		mov [nArgs], ebx
		mov eax, [ebp+0xC]
		mov [pArgs], eax
	}

	char *pRet;

	pRet = L_SCURK_OwlMainCommandLineFix(pArgs, nArgs);

	__asm {
		mov esi, [pRet]
	}
	GAMEJMP(0x45A138);
}

extern "C" void __cdecl Hook_SCURKPrimary_BCDialog_CmCancel(TBC45XDialog *pThis) {
	L_SCURK_BCDialog_CmCancel(pThis);
}

void InstallFixes_SCURKPrimary(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk_debug = DEBUG_FLAGS_EVERYTHING;

	InstallRegistryPathingHooks_SCURKPrimary();

	// Hook for palette animation fix
	VirtualProtect((LPVOID)0x4496D4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4496D4, Hook_SCURKPrimary_winscurkMDIClient_CycleColors);
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

	// Hook cEditableTileSet::mReadFromFile
	// This call is used to load the TILES.DB.
	VirtualProtect((LPVOID)0x4150EC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4150EC, Hook_SCURKPrimary_EditableTileSet_mReadFromFile);

	// 'nop' out the -1 case in the following functions in-regards to the
	// maximum extent:
	// - cPaintWindow::mZoomOut
	// - cPaintWindow::mZoomIn
	// - cPaintWindow::EvHScroll
	VirtualProtect((LPVOID)0x443D58, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443D58) = 0x90;
	VirtualProtect((LPVOID)0x443DA2, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443DA2) = 0x90;
	VirtualProtect((LPVOID)0x443E88, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443E88) = 0x90;
	VirtualProtect((LPVOID)0x443ECD, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443ECD) = 0x90;
	VirtualProtect((LPVOID)0x44703E, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x44703E) = 0x90;

	// Increased the maximum extent by 1 to fix the lack of the last
	// right-side column of pixels:
	// cPaintWindow::cPaintWindow
	VirtualProtect((LPVOID)0x4432E4, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x4432E4) = 0x01;

	// winscurkMoverWindow::EvSize():
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

	// OwlMain() command line fix.
	VirtualProtect((LPVOID)0x45A0B9, 7, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x45A0B9, 0x90, 7);
	NEWJMP((LPVOID)0x45A0B9, Hook_SCURKPrimary_OwlMainCommandLineFix);

	// This hook is to prevent the Place&Pick selection dialogue
	// from being unintentionally closed; it catches and ignores
	// the cancel (esc) action.
	VirtualProtect((LPVOID)0x46FB26, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x46FB26, Hook_SCURKPrimary_BCDialog_CmCancel);
}
