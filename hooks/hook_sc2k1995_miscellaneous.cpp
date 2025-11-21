// sc2kfix hooks/hook_sc2k1995_miscellaneous.cpp: 1995 CD Collection hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Hooks for the 1995 CD Collection - these are minimal cases to avoid certain
// interface-breaking conditions and other chosen cases.
//
// The 1995 CD Collection is still deprecated, we'd strongly recommend the 1996 Special Edition!

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define MISCHOOK_1995_DEBUG_OTHER 1
#define MISCHOOK_1995_DEBUG_MENU 2

#define MISCHOOK_1995_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_1995_DEBUG
#define MISCHOOK_1995_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_1995_debug = MISCHOOK_1995_DEBUG;

static DWORD dwDummy;

extern "C" void __stdcall Hook_1995_SimcityApp_OnQuit(void) {
	CSimcityAppPrimary *pThis;

	__asm mov [pThis], ecx

	// pThis[63] = dwSCAOnQuitSuspendSim
	// pThis[64] = dwSCAMainFrameDestroyVar
	// pThis[206] = dwSCASysCmdOnQuitVar
	//
	// While 'dwSCAMainFrameDestroyVar' is set to 1 various simulation and update
	// aspects are suspended. Originally when "Cancel" was clicked in order to avoid
	// quitting it wouldn't unset the var and consequently the game simulation would
	// remain suspended.

	int iReqRet;

	pThis->dwSCAMainFrameDestroyVar = 1;
	pThis->dwSCAOnQuitSuspendSim = 0;
	iReqRet = Game_SimcityApp_ExitRequester_1995(pThis, pThis->dwSCASysCmdOnQuitVar);
	if (iReqRet != IDCANCEL) {
		if (iReqRet == IDYES)
			Game_SimcityApp_SaveCity_1995(pThis);
		Game_MainFrame_ToggleToolBars_1995((CMainFrame *)pThis->m_pMainWnd, 0);
		GameMain_WinApp_OnAppExit_1995(pThis);
		return;
	}
	pThis->dwSCAMainFrameDestroyVar = 0;
	pThis->dwSCAOnQuitSuspendSim = 0;
}

void InstallMiscHooks_SC2K1995(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_1995_debug = DEBUG_FLAGS_EVERYTHING;

	// Install critical Windows API hooks

	InstallRegistryPathingHooks_SC2K1995();

	// Fix the 'Arial" font
	VirtualProtect((LPVOID)0x4E6234, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E6234, 0, 6);
	memcpy_s((LPVOID)0x4E6234, 6, "Arial", 6);
	VirtualProtect((LPVOID)0x44D5C2, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44D5C2 = 5;
	VirtualProtect((LPVOID)0x44D5CF, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x44D5CF = 10;

	// Hook CSimcityApp::OnQuit
	VirtualProtect((LPVOID)0x401749, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401749, Hook_1995_SimcityApp_OnQuit);

	// Set the initial program state to ONIDLE_STATE_DISPLAYMAXIS
	VirtualProtect((LPVOID)0x4051DD, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4051DD = ONIDLE_STATE_DISPLAYMAXIS;

	// Fix the Maxis Presents logo not being shown
	VirtualProtect((LPVOID)0x4E5120, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E5120, 0, 13);
	memcpy_s((LPVOID)0x4E5120, 13, "presnts.bmp", 13);
}
