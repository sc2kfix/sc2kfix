// sc2kfix include/scurk_1996se.h: defines specific to the 1996SE version of WinSCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for 1996SE WinSCURK - highly experimental.
// Any corrections for WinSCURK are very much "best effort" - good luck.

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996SE_GAMEOFF(type, name, address) \
	type* __ptr__##name##_SCURK1996SE = (type*)address; \
	type& ##name##_SCURK1996SE = *__ptr__##name##_SCURK1996SE;
#define SCURK1996SE_GAMEOFF_ARR(type, name, address) type* ##name##_SCURK1996SE = (type*)address;
#else
#define SCURK1996SE_GAMEOFF(type, name, address) extern type& ##name##_SCURK1996SE;
#define SCURK1996SE_GAMEOFF_ARR(type, name, address) extern type* ##name##_SCURK1996SE;
#endif

#define SCURK1996SE_GAMEOFF_PTR SCURK1996SE_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define SCURK1996SE_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996SE)(__VA_ARGS__); \
	GameFuncPtr_##name##_SCURK1996SE Game_##name##_SCURK1996SE = (GameFuncPtr_##name##_SCURK1996SE)address;
#else
#define SCURK1996SE_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURK1996SE)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_SCURK1996SE Game_##name##_SCURK1996SE;
#endif

#ifdef GAMEOFF_IMPL
#define SCURK1996SE_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996SE)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_SCURK1996SE GameMain_##name##_SCURK1996SE = (GameMainFuncPtr_##name##_SCURK1996SE)address;
#else
#define SCURK1996SE_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURK1996SE)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_SCURK1996SE GameMain_##name##_SCURK1996SE;
#endif

// Program funcs
SCURK1996SE_GAMECALL_MAIN(0x41083C, void, __cdecl, PlaceTileListDlg_DrawTile, DWORD *, DRAWITEMSTRUCT *)
SCURK1996SE_GAMECALL_MAIN(0x410A80, void, __cdecl, PlaceTileListDlg_ChangeHiLight, DWORD *, DRAWITEMSTRUCT *)
SCURK1996SE_GAMECALL_MAIN(0x4132EC, void, __cdecl, DebugOut, const char *, ...)
SCURK1996SE_GAMECALL_MAIN(0x414D78, char *, __cdecl, EditableTileSet_GetLongName, DWORD *, int)
SCURK1996SE_GAMECALL_MAIN(0x440F80, void, __cdecl, winscurkPlaceWindow_ClearCurrentTool, DWORD *)
SCURK1996SE_GAMECALL_MAIN(0x457AF4, TBC45XPalette *, __cdecl, winscurkApp_GetPalette, DWORD *)
SCURK1996SE_GAMECALL_MAIN(0x459604, void, __cdecl, winscurkApp_ScurkSound, DWORD *, int)
SCURK1996SE_GAMECALL_MAIN(0x459CA0, DWORD *, __cdecl, winscurkApp_GetPlaceWindow, DWORD *)

// BC and WinAPI
SCURK1996SE_GAMECALL_MAIN(0x45F691, TBC45XDC *, __cdecl, BCDC_Cons, TBC45XDC *, HDC)
SCURK1996SE_GAMECALL_MAIN(0x45F718, void, __cdecl, BCDC_Dest, TBC45XDC *, char)
SCURK1996SE_GAMECALL_MAIN(0x45F80F, void, __cdecl, BCDC_SelectObjectPalette, TBC45XDC *, TBC45XPalette *, int)
SCURK1996SE_GAMECALL_MAIN(0x469C64, LRESULT, __cdecl, BCWindow_HandleMessage, DWORD *, unsigned int msg, WPARAM wParam, LPARAM lParam)
SCURK1996SE_GAMECALL_MAIN(0x46B108, void, __cdecl, BCWindow_SetCursor, DWORD *, DWORD *, const char *)
SCURK1996SE_GAMECALL_MAIN(0x46F9DB, void, __cdecl, BCDialog_SetupWindow, DWORD *)
SCURK1996SE_GAMECALL_MAIN(0x46FABD, void, __cdecl, BCDialog_SetCaption, DWORD *, char *)
SCURK1996SE_GAMECALL_MAIN(0x46FB35, void, __cdecl, BCDialog_EvClose, DWORD *)
SCURK1996SE_GAMECALL_MAIN(0x476475, int, __cdecl, BCListBox_GetSelIndex, DWORD *)
SCURK1996SE_GAMECALL_MAIN(0x477A62, LRESULT, __cdecl, BCListBox_GetString, DWORD *, char *, int)
SCURK1996SE_GAMECALL_MAIN(0x477A9B, int, __cdecl, BCListBox_SetItemData, DWORD *, int, unsigned int)
SCURK1996SE_GAMECALL_MAIN(0x477B26, int, __cdecl, BCListBox_AddString, DWORD *, const char *)

// Vars
SCURK1996SE_GAMEOFF_PTR(__int16,	wTileObjects,	0x491BAA)
SCURK1996SE_GAMEOFF(__int16,		wtoolNum,		0x4A482E)
SCURK1996SE_GAMEOFF(__int16,	wtoolValue,			0x4A4866)
SCURK1996SE_GAMEOFF(DWORD *,	gScurkApplication,	0x4A6D88)
