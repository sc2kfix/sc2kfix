// sc2kfix include/scurk_primary.h: defines specific to the primary (1995) version of WinSCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the primary (1995) version of WinSCURK - highly experimental.
// Any corrections for WinSCURK are very much "best effort" - good luck.

#pragma once

#ifndef HOOKEXT
#define HOOKEXT extern "C" __declspec(dllexport)
#endif

#ifdef GAMEOFF_IMPL
#define SCURKPRIMARY_GAMEOFF(type, name, address) \
	type* __ptr__##name##_SCURKPrimary = (type*)address; \
	type& ##name##_SCURKPrimary = *__ptr__##name##_SCURKPrimary;
#define SCURKPRIMARY_GAMEOFF_ARR(type, name, address) type* ##name##_SCURKPrimary = (type*)address;
#else
#define SCURKPRIMARY_GAMEOFF(type, name, address) extern type& ##name##_SCURKPrimary;
#define SCURKPRIMARY_GAMEOFF_ARR(type, name, address) extern type* ##name##_SCURKPrimary;
#endif

#define SCURKPRIMARY_GAMEOFF_PTR SCURKPRIMARY_GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define SCURKPRIMARY_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURKPrimary)(__VA_ARGS__); \
	GameFuncPtr_##name##_SCURKPrimary Game_##name##_SCURKPrimary = (GameFuncPtr_##name##_SCURKPrimary)address;
#else
#define SCURKPRIMARY_GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name##_SCURKPrimary)(__VA_ARGS__);\
	extern GameFuncPtr_##name##_SCURKPrimary Game_##name##_SCURKPrimary;
#endif

#ifdef GAMEOFF_IMPL
#define SCURKPRIMARY_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURKPrimary)(__VA_ARGS__); \
	GameMainFuncPtr_##name##_SCURKPrimary GameMain_##name##_SCURKPrimary = (GameMainFuncPtr_##name##_SCURKPrimary)address;
#else
#define SCURKPRIMARY_GAMECALL_MAIN(address, type, conv, name, ...) \
	typedef type (conv *GameMainFuncPtr_##name##_SCURKPrimary)(__VA_ARGS__);\
	extern GameMainFuncPtr_##name##_SCURKPrimary GameMain_##name##_SCURKPrimary;
#endif

// Program funcs
SCURKPRIMARY_GAMECALL_MAIN(0x41083C, void, __cdecl, PlaceTileListDlg_DrawTile, TPlaceTileListDlg *, DRAWITEMSTRUCT *)
SCURKPRIMARY_GAMECALL_MAIN(0x410A80, void, __cdecl, PlaceTileListDlg_ChangeHiLight, TPlaceTileListDlg *, DRAWITEMSTRUCT *)
SCURKPRIMARY_GAMECALL_MAIN(0x4132EC, void, __cdecl, DebugOut, const char *, ...)
SCURKPRIMARY_GAMECALL_MAIN(0x414D78, char *, __cdecl, EditableTileSet_GetLongName, cEditableTileSet *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x416F84, void, __cdecl, EditableTileSet_RenderShapeToTile, cEditableTileSet *, TBC45XDib *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x440F80, void, __cdecl, winscurkPlaceWindow_ClearCurrentTool, DWORD *)
SCURKPRIMARY_GAMECALL_MAIN(0x457AF4, TBC45XPalette *, __cdecl, winscurkApp_GetPalette, DWORD *)
SCURKPRIMARY_GAMECALL_MAIN(0x459604, void, __cdecl, winscurkApp_ScurkSound, DWORD *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x459CA0, DWORD *, __cdecl, winscurkApp_GetPlaceWindow, DWORD *)

// BC and WinAPI
SCURKPRIMARY_GAMECALL_MAIN(0x45F691, TBC45XDC *, __cdecl, BCDC_Cons, TBC45XDC *, HDC)
SCURKPRIMARY_GAMECALL_MAIN(0x45F718, void, __cdecl, BCDC_Dest, TBC45XDC *, char)
SCURKPRIMARY_GAMECALL_MAIN(0x45F80F, void, __cdecl, BCDC_SelectObjectPalette, TBC45XDC *, TBC45XPalette *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x469C64, LRESULT, __cdecl, BCWindow_HandleMessage, TBC45XParWindow *, unsigned int msg, WPARAM wParam, LPARAM lParam)
SCURKPRIMARY_GAMECALL_MAIN(0x46B108, void, __cdecl, BCWindow_SetCursor, TBC45XParWindow *, TBC45XModule *, const char *)
SCURKPRIMARY_GAMECALL_MAIN(0x46F9DB, void, __cdecl, BCDialog_SetupWindow, TBC45XParDialog *)
SCURKPRIMARY_GAMECALL_MAIN(0x46FABD, void, __cdecl, BCDialog_SetCaption, TBC45XParDialog *, char *)
SCURKPRIMARY_GAMECALL_MAIN(0x46FB35, void, __cdecl, BCDialog_EvClose, TBC45XParDialog *)
SCURKPRIMARY_GAMECALL_MAIN(0x476475, int, __cdecl, BCListBox_GetSelIndex, TBC45XParListBox *)
SCURKPRIMARY_GAMECALL_MAIN(0x477A62, LRESULT, __cdecl, BCListBox_GetString, TBC45XParListBox *, char *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x477A9B, int, __cdecl, BCListBox_SetItemData, TBC45XParListBox *, int, unsigned int)
SCURKPRIMARY_GAMECALL_MAIN(0x477B26, int, __cdecl, BCListBox_AddString, TBC45XParListBox *, const char *)

// Vars
SCURKPRIMARY_GAMEOFF_PTR(__int16,	wTileObjects,	0x491BAA)
SCURKPRIMARY_GAMEOFF(__int16,		wtoolNum,		0x4A482E)
SCURKPRIMARY_GAMEOFF(__int16,	wtoolValue,			0x4A4866)
SCURKPRIMARY_GAMEOFF(DWORD *,	gScurkApplication,	0x4A6D88)
