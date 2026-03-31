// sc2kfix modules/scurkfix_1996.cpp: fixes for SCURK - Network Edition (1996) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

extern "C" void Hook_SCURK1996_winscurkMDIClient_CycleColors(winscurkMDIClient *pThis) {
	L_SCURK_winscurkMDIClient_CycleColors(pThis);
}

extern "C" void __cdecl Hook_SCURK1996_DebugOut(char const *fmt, ...) {
	va_list args;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_INTERNAL) == 0)
		return;

	va_start(args, fmt);
	L_SCURK_gDebugOut(fmt, args);
	va_end(args);
}

extern "C" void __cdecl Hook_SCURK1996_PlaceTileListDlg_SetupWindow(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_SetupWindow(pThis);
}

extern "C" void __cdecl Hook_SCURK1996_PlaceTileListDlg_EvLButtonDblClk(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_EvLButtonDblClk(pThis);
}

extern "C" void __cdecl Hook_SCURK1996_PlaceTileListDlg_EvLBNSelChange(TPlaceTileListDlg *pThis) {
	L_SCURK_PlaceTileListDlg_EvLBNSelChange(pThis);
}

extern "C" LONG __cdecl Hook_SCURK1996_EditableTileSet_mReadFromFile(cEditableTileSet *pThis, LPCSTR lpPathName) {
	return L_SCURK_EditableTileSet_mReadFromFile(pThis, lpPathName);
}

extern "C" void __declspec(naked) Hook_SCURK1996_MoverWindow_DisableMaximizeBox(void) {
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
	GAMEJMP(0x44E55A);
}

extern "C" void __cdecl Hook_SCURK1996_MoverWindow_EvGetMinMaxInfo(winscurkMoverWindow *pThis, MINMAXINFO *pMmi) {
	L_SCURK_MoverWindow_EvGetMinMaxInfo(pThis, pMmi);
}

// And we're gritting our teeth...
extern "C" void __declspec(naked) __cdecl Hook_SCURK1996_OwlMainCommandLineFix(void) {
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
	GAMEJMP(0x45A7F6);
}

extern "C" void __cdecl Hook_SCURK1996_BCDialog_CmCancel(TBC45XDialog *pThis) {
	L_SCURK_BCDialog_CmCancel(pThis);
}

void InstallFixes_SCURK1996(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk_debug = DEBUG_FLAGS_EVERYTHING;

	InstallRegistryPathingHooks_SCURK1996();

	// Hook for palette animation fix
	VirtualProtect((LPVOID)0x449800, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x449800, Hook_SCURK1996_winscurkMDIClient_CycleColors);
	ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix for SCURK.\n");

	// Add back the internal debug notices for tracing purposes.
	VirtualProtect((LPVOID)0x4132E8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4132E8, Hook_SCURK1996_DebugOut);

	// These hooks are to account for the Place&Pick selection dialogue
	// malfunctions that were occurring under Win11 24H2+:
	// 1) The Listbox was no longer displayed
	// 2) Mouse selection was no longer recognised - or rather
	//    the stored point within the window wasn't recorded.
	VirtualProtect((LPVOID)0x4104B8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4104B8, Hook_SCURK1996_PlaceTileListDlg_SetupWindow);
	VirtualProtect((LPVOID)0x410D94, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410D94, Hook_SCURK1996_PlaceTileListDlg_EvLButtonDblClk);
	VirtualProtect((LPVOID)0x410ED0, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410ED0, Hook_SCURK1996_PlaceTileListDlg_EvLBNSelChange);

	// Hook cEditableTileSet::mReadFromFile
	// This call is used to load the TILES.DB.
	VirtualProtect((LPVOID)0x41510C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x41510C, Hook_SCURK1996_EditableTileSet_mReadFromFile);

	// 'nop' out the -1 case in the following functions in-regards to the
	// maximum extent:
	// - cPaintWindow::mZoomOut
	// - cPaintWindow::mZoomIn
	// - cPaintWindow::EvHScroll
	VirtualProtect((LPVOID)0x443D28, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443D28) = 0x90;
	VirtualProtect((LPVOID)0x443D72, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443D72) = 0x90;
	VirtualProtect((LPVOID)0x443E58, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443E58) = 0x90;
	VirtualProtect((LPVOID)0x443E9D, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x443E9D) = 0x90;
	VirtualProtect((LPVOID)0x446F76, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x446F76) = 0x90;

	// Increased the maximum extent by 1 to fix the lack of the last
	// right-side column of pixels:
	// cPaintWindow::cPaintWindow
	VirtualProtect((LPVOID)0x4432B4, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE *)(0x4432B4) = 0x01;

	// winscurkMoverWindow::EvSize():
	// Temporarily remove the TFrameWindow::EvSize call.
	// This avoids some redrawing strangeness that otherwise occurs
	// if the Pick&Copy window is in-focus and you then restore
	// the Place&Pick window to its non-maximized state.
	VirtualProtect((LPVOID)0x450095, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x450095, 0x90, 13);

	// Temporarily disable the maximizebox style if SM_CXSCREEN is above 700.
	VirtualProtect((LPVOID)0x44E553, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x44E553, 0x90, 6);
	NEWJMP((LPVOID)0x44E553, Hook_SCURK1996_MoverWindow_DisableMaximizeBox);

	// Temporarily lock the Min/Max size of the Pick&Copy window
	// to avoid rendering the area non-functional.
	VirtualProtect((LPVOID)0x4502E8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4502E8, Hook_SCURK1996_MoverWindow_EvGetMinMaxInfo);

	// OwlMain() command line fix.
	VirtualProtect((LPVOID)0x45A777, 7, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x45A777, 0x90, 7);
	NEWJMP((LPVOID)0x45A777, Hook_SCURK1996_OwlMainCommandLineFix);

	// This hook is to prevent the Place&Pick selection dialogue
	// from being unintentionally closed; it catches and ignores
	// the cancel (esc) action.
	VirtualProtect((LPVOID)0x4702A6, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4702A6, Hook_SCURK1996_BCDialog_CmCancel);
}
