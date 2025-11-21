// sc2kfix include/sc2k_demo.h: defines specific to the Interactive Demo version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the Interactive Demo... AAAA!!

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define DEMO_GAMEOFF(type, name, address) \
	type* __ptr__##name##_Demo = (type*)address; \
	type& ##name##_Demo = *__ptr__##name##_Demo;
#define DEMO_GAMEOFF_ARR(type, name, address) type* ##name##_Demo = (type*)address;
#else
#define DEMO_GAMEOFF(type, name, address) extern type& ##name##_Demo;
#define DEMO_GAMEOFF_ARR(type, name, address) extern type* ##name##_Demo;
#endif

#define DEMO_GAMEOFF_PTR DEMO_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define DEMO_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_Demo)(__VA_ARGS__); \
	GameFuncPtr_##name##_Demo Game_##name##_Demo = (GameFuncPtr_##name##_Demo)address;
#else
#define DEMO_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_Demo)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_Demo Game_##name##_Demo;
#endif

#ifdef GAMEOFF_IMPL
#define DEMO_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_Demo)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_Demo GameMain_##name##_Demo = (GameMainFuncPtr_##name##_Demo)address;
#else
#define DEMO_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_Demo)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_Demo GameMain_##name##_Demo;
#endif

// Thunk
DEMO_GAMECALL(0x402725, CSimcityView *, __thiscall, SimcityApp_PointerToCSimcityViewClass, CSimcityAppDemo *)

// Main
DEMO_GAMECALL_MAIN(0x44890F, void, __cdecl, ToggleColorCycling, CMFC3XPalette *, BOOL)

// MFC and WinAPI
DEMO_GAMECALL_MAIN(0x48BF14, CMFC3XWnd *, __stdcall, Wnd_FromHandle, HWND hWnd)
DEMO_GAMECALL_MAIN(0x48BF32, CMFC3XWnd *, __stdcall, Wnd_FromHandlePermanent, HWND)
DEMO_GAMECALL_MAIN(0x48D64A, CMFC3XTestCmdUI *, __thiscall, TestCmdUI_Construct, CMFC3XTestCmdUI *)
DEMO_GAMECALL_MAIN(0x48E3C6, BOOL, __thiscall, Wnd_SendChildNotifyLastMsg, CMFC3XWnd *, LRESULT *)
DEMO_GAMECALL_MAIN(0x48F069, BOOL, __thiscall, CmdTarget_OnCmdMsg, CMFC3XCmdTarget *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo)
DEMO_GAMECALL_MAIN(0x4A2207, BOOL, __thiscall, FrameWnd_OnCmdMsg, CMFC3XFrameWnd *, UINT nID, int nCode, void *pExtra, void *pHandlerInfo)
DEMO_GAMECALL_MAIN(0x4AA2B1, MFC3X_AFX_THREAD_STATE *, __stdcall, AfxGetThreadState, void)

// Vars

DEMO_GAMEOFF(CSimcityAppDemo,	pCSimcityAppThis,			0x4B6A70)
DEMO_GAMEOFF(BOOL,	bLoColor,			0x4D1EDC)
