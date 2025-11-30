// sc2kfix include/sc2k_1995.h: defines specific to the 1995 CD Collection version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the 1995 CD Collection - these are minimal cases to avoid certain
// interface-breaking conditions.

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define CC1995_GAMEOFF(type, name, address) \
	type* __ptr__##name##_1995 = (type*)address; \
	type& ##name##_1995 = *__ptr__##name##_1995;
#define CC1995_GAMEOFF_ARR(type, name, address) type* ##name##_1995 = (type*)address;
#else
#define CC1995_GAMEOFF(type, name, address) extern type& ##name##_1995;
#define CC1995_GAMEOFF_ARR(type, name, address) extern type* ##name##_1995;
#endif

#define CC1995_GAMEOFF_PTR CC1995_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define CC1995_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_1995)(__VA_ARGS__); \
	GameFuncPtr_##name##_1995 Game_##name##_1995 = (GameFuncPtr_##name##_1995)address;
#else
#define CC1995_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_1995)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_1995 Game_##name##_1995;
#endif

#ifdef GAMEOFF_IMPL
#define CC1995_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_1995)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_1995 GameMain_##name##_1995 = (GameMainFuncPtr_##name##_1995)address;
#else
#define CC1995_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_1995)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_1995 GameMain_##name##_1995;
#endif

// Thunk
CC1995_GAMECALL(0x40103C, void, __thiscall, MainFrame_ToggleToolBars, CMainFrame *, BOOL)
CC1995_GAMECALL(0x4014F1, int, __thiscall, SimcityApp_ExitRequester, CSimcityAppPrimary *, int)
CC1995_GAMECALL(0x401587, void, __thiscall, SimcityApp_SaveCity, CSimcityAppPrimary *)
CC1995_GAMECALL(0x4022C0, int, __stdcall, UpdateSectionsAndResetWindowMenu)
CC1995_GAMECALL(0x4026D0, CSimcityView *, __thiscall, SimcityApp_PointerToCSimcityViewClass, CSimcityAppPrimary *)

// Main
CC1995_GAMECALL_MAIN(0x456A60, void, __cdecl, ToggleColorCycling, CMFC3XPalette *, BOOL)

// MFC and WinAPI
CC1995_GAMECALL_MAIN(0x4A1700, BOOL, __thiscall, CmdTarget_OnCmdMsg, CMFC3XCmdTarget *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo)
CC1995_GAMECALL_MAIN(0x4A1D5E, CMFC3XString *, __thiscall, String_OperatorSet, CMFC3XString *pThis, char *)
CC1995_GAMECALL_MAIN(0x4A2AD3, CMFC3XWnd *, __stdcall, Wnd_FromHandle, HWND hWnd)
CC1995_GAMECALL_MAIN(0x4A2AF1, CMFC3XWnd *, __stdcall, Wnd_FromHandlePermanent, HWND)
CC1995_GAMECALL_MAIN(0x4A4209, CMFC3XTestCmdUI *, __thiscall, TestCmdUI_Construct, CMFC3XTestCmdUI *)
CC1995_GAMECALL_MAIN(0x4A4F85, BOOL, __thiscall, Wnd_SendChildNotifyLastMsg, CMFC3XWnd *, LRESULT *)
CC1995_GAMECALL_MAIN(0x4A9467, void, __thiscall, WinApp_OnAppExit, CMFC3XWinApp *)
CC1995_GAMECALL_MAIN(0x4B8B8E, BOOL, __thiscall, FrameWnd_OnCmdMsg, CMFC3XFrameWnd *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo)
CC1995_GAMECALL_MAIN(0x4BD6EB, void, __thiscall, WinApp_EnableShellOpen, CMFC3XWinApp *)
CC1995_GAMECALL_MAIN(0x4BF624, MFC3X_AFX_THREAD_STATE *, __stdcall, AfxGetThreadState, void)

// Vars

CC1995_GAMEOFF(CSimcityAppPrimary,	pCSimcityAppThis,	0x4C6010)
CC1995_GAMEOFF(BOOL,	bCSimcityDocSC2InUse,		0x4E8734)
CC1995_GAMEOFF(BOOL,	bCSimcityDocSCNInUse,		0x4E8738)
CC1995_GAMEOFF(DWORD,	dwUnknownInitVarOne,		0x4E873C)
CC1995_GAMEOFF(BOOL,	bLoColor,						0x4E903C)
