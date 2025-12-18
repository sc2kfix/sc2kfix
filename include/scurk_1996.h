// sc2kfix include/scurk_1996.h: defines specific to the 1996 (Network Edition) version of WinSCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the 1996 (Network Edition) version of WinSCURK - highly experimental.
// Any corrections for WinSCURK are very much "best effort" - good luck.
//
// This version has been confirmed to have been used with the following games:
// 1) SimCity 2000 Network Edition

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMEOFF(type, name, address) \
	type* __ptr__##name##_SCURK1996 = (type*)address; \
	type& ##name##_SCURK1996 = *__ptr__##name##_SCURK1996;
#define SCURK1996_GAMEOFF_ARR(type, name, address) type* ##name##_SCURK1996 = (type*)address;
#else
#define SCURK1996_GAMEOFF(type, name, address) extern type& ##name##_SCURK1996;
#define SCURK1996_GAMEOFF_ARR(type, name, address) extern type* ##name##_SCURK1996;
#endif

#define SCURK1996_GAMEOFF_PTR SCURK1996_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996)(__VA_ARGS__); \
	GameFuncPtr_##name##_SCURK1996 Game_##name##_SCURK1996 = (GameFuncPtr_##name##_SCURK1996)address;
#else
#define SCURK1996_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_SCURK1996 Game_##name##_SCURK1996;
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_SCURK1996 GameMain_##name##_SCURK1996 = (GameMainFuncPtr_##name##_SCURK1996)address;
#else
#define SCURK1996_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_SCURK1996 GameMain_##name##_SCURK1996;
#endif

// Program funcs
SCURK1996_GAMECALL_MAIN(0x4496E8, void, __cdecl, winscurkMDIClient_RotateColors, winscurkMDIClient *, int)
SCURK1996_GAMECALL_MAIN(0x458150, TBC45XPalette *, __cdecl, winscurkApp_GetPalette, winscurkApp *)
SCURK1996_GAMECALL_MAIN(0x45A2F0, winscurkPlaceWindow *, __cdecl, winscurkApp_GetPlaceWindow, winscurkApp *)

// BC and WinAPI
SCURK1996_GAMECALL_MAIN(0x45FF8F, void, __cdecl, BCDC_SelectObjectPalette, TBC45XDC *, TBC45XPalette *, int)
SCURK1996_GAMECALL_MAIN(0x469D4A, unsigned int, __cdecl, BCWindow_DefaultProcessing, TBC45XParWindow *)
SCURK1996_GAMECALL_MAIN(0x46BF48, void, __cdecl, BCWindowDC_Destruct, TBC45XWindowDC *, char)
SCURK1996_GAMECALL_MAIN(0x46BFE5, TBC45XClientDC *, __cdecl, BCClientDC_Construct, TBC45XClientDC *, HWND)
SCURK1996_GAMECALL_MAIN(0x4702B5, void, __cdecl, BCDialog_EvClose, TBC45XParDialog *)
SCURK1996_GAMECALL_MAIN(0x476BF5, int, __cdecl, BCListBox_GetSelIndex, TBC45XParListBox *)
SCURK1996_GAMECALL_MAIN(0x4781E2, LRESULT, __cdecl, BCListBox_GetString, TBC45XParListBox *, char *, int)
SCURK1996_GAMECALL_MAIN(0x47821B, int, __cdecl, BCListBox_SetItemData, TBC45XParListBox *, int, unsigned int)
SCURK1996_GAMECALL_MAIN(0x4782A6, int, __cdecl, BCListBox_AddString, TBC45XParListBox *, const char *)
SCURK1996_GAMECALL_MAIN(0x478CCE, TBC45XMDIChild *, __cdecl, BCMDIClient_GetActiveMDIChild, TBC45XParMDIClient *)

// Vars
SCURK1996_GAMEOFF(WORD,			wColFastCnt,		0x498232)
SCURK1996_GAMEOFF(WORD,			wColSlowCnt,		0x498234)
SCURK1996_GAMEOFF(winscurkApp *,	gScurkApplication,	0x4A6DEC)
