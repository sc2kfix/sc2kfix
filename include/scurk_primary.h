// sc2kfix include/scurk_primary.h: defines specific to the primary (1995) version of WinSCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Header for the primary (1995) version of WinSCURK - highly experimental.
// Any corrections for WinSCURK are very much "best effort" - good luck.
//
// This version has been confirmed to have been used with the following games:
// 1) 1995 CD Collection
// 2) 1996 Special Edition
// 3) Streets of SimCity

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
SCURKPRIMARY_GAMECALL_MAIN(0x412134, int, __cdecl, WinGBitmap_Width, CWinGBitmap *)
SCURKPRIMARY_GAMECALL_MAIN(0x412154, int, __cdecl, WinGBitmap_Height, CWinGBitmap *)
SCURKPRIMARY_GAMECALL_MAIN(0x4132EC, void, __cdecl, DebugOut, const char *, ...)
SCURKPRIMARY_GAMECALL_MAIN(0x41378C, void *, __cdecl, gAllocBlock, size_t)
SCURKPRIMARY_GAMECALL_MAIN(0x4137EC, void *, __cdecl, gResizeBlock, BYTE *, size_t)
SCURKPRIMARY_GAMECALL_MAIN(0x413810, void, __cdecl, gFreeBlock, void *)
SCURKPRIMARY_GAMECALL_MAIN(0x4139B4, void, __stdcall, gUpdateWaitWindow)
SCURKPRIMARY_GAMECALL_MAIN(0x413DB4, TEncodeDib *, __cdecl, EncodeDib_Construct_Dimens, TEncodeDib *, LONG, LONG, UINT, uint16_t)
SCURKPRIMARY_GAMECALL_MAIN(0x413E8C, void, __cdecl, EncodeDib_Destruct, TEncodeDib *, char)
SCURKPRIMARY_GAMECALL_MAIN(0x4140D0, void, __cdecl, EncodeDib_mFillAt, TEncodeDib *, TBC45XPoint *)
SCURKPRIMARY_GAMECALL_MAIN(0x4141C8, void, __cdecl, EncodeDib_mFillLine, TEncodeDib *, TBC45XPoint *, BYTE)
SCURKPRIMARY_GAMECALL_MAIN(0x414314, int, __cdecl, EncodeDib_mDetermineShapeHeight, TEncodeDib *)
SCURKPRIMARY_GAMECALL_MAIN(0x41435C, void, __cdecl, EncodeDib_mShrink, TEncodeDib *, TBC45XDib *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x41441C, void, __cdecl, EncodeDib_mAcquireEncodedShapeData, TEncodeDib *, TEncodeDib *)
SCURKPRIMARY_GAMECALL_MAIN(0x414450, void, __cdecl, EncodeDib_mEncodeShape, TEncodeDib *, WORD, WORD, WORD)
SCURKPRIMARY_GAMECALL_MAIN(0x4148E8, int, __cdecl, EditableTileSet_mShapeNumToEditableNum, cEditableTileSet *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x414D78, char *, __cdecl, EditableTileSet_GetLongName, cEditableTileSet *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x4168B0, int, __cdecl, EditableTileSet_mGetShapeWidth, cEditableTileSet *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x416C0C, void, __cdecl, EditableTileSet_mRenderEditableShapeToDIB_Dib, cEditableTileSet *, TBC45XDib *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x416C44, void, __cdecl, EditableTileSet_mRenderEditableShapeToDIB_Graphic, cEditableTileSet *, CWinGBitmap *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x4173EC, void, __cdecl, EditableTileSet_mBuildSmallMedTiles, cEditableTileSet *)
SCURKPRIMARY_GAMECALL_MAIN(0x43DB38, TBC45XDib *, __cdecl, EditWindow_mGetForegroundPattern, winscurkEditWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x43F57C, int, __cdecl, EditWindow_mGetShapeWidth, winscurkEditWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x44015C, BYTE, __cdecl, EditWindow_mGetForegroundColor, winscurkEditWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x440170, BYTE, __cdecl, EditWindow_mGetBackgroundColor, winscurkEditWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x440F80, void, __cdecl, PlaceWindow_ClearCurrentTool, winscurkPlaceWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x4438B8, void, __cdecl, PaintWindow_mApplyTileToScreen, cPaintWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x443950, void, __cdecl, PaintWindow_mPreserveToUndoBuffer, cPaintWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x44398C, void, __cdecl, PaintWindow_mClearTile, cPaintWindow *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x443F34, void, __cdecl, PaintWindow_mClipDrawing, cPaintWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x4440D0, void, __cdecl, PaintWindow_mScreenToDib, TBC45XPoint *, cPaintWindow *, TBC45XPoint *)
SCURKPRIMARY_GAMECALL_MAIN(0x444210, void, __cdecl, PaintWindow_mPutPixel, cPaintWindow *, TBC45XRect *, BYTE, BYTE)
SCURKPRIMARY_GAMECALL_MAIN(0x446964, void, __cdecl, PaintWindow_mDraw, cPaintWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x447200, void, __cdecl, PaintWindow_mEncodeShape, cPaintWindow *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x4495BC, void, __cdecl, winscurkMDIClient_RotateColors, winscurkMDIClient *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x457AF4, TBC45XPalette *, __cdecl, winscurkApp_GetPalette, winscurkApp *)
SCURKPRIMARY_GAMECALL_MAIN(0x459604, void, __cdecl, winscurkApp_ScurkSound, winscurkApp *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x459CA0, winscurkPlaceWindow *, __cdecl, winscurkApp_GetPlaceWindow, winscurkApp *)
SCURKPRIMARY_GAMECALL_MAIN(0x459CB8, winscurkEditWindow *, __cdecl, winscurkApp_GetEditWindow, winscurkApp *)

// BC and WinAPI
SCURKPRIMARY_GAMECALL_MAIN(0x45F80F, void, __cdecl, BCDC_SelectObjectPalette, TBC45XDC *, TBC45XPalette *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x4695CA, unsigned int, __cdecl, BCWindow_DefaultProcessing, TBC45XParWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x46975C, LRESULT, __cdecl, BCWindow_EvCommand, TBC45XWindow *, DWORD, HWND, DWORD)
SCURKPRIMARY_GAMECALL_MAIN(0x469C64, LRESULT, __cdecl, BCWindow_HandleMessage, TBC45XParWindow *, unsigned int msg, WPARAM wParam, LPARAM lParam)
SCURKPRIMARY_GAMECALL_MAIN(0x469E27, TBC45XWindow *, __cdecl, GetWindowPtr, HWND, TBC45XApplication *)
SCURKPRIMARY_GAMECALL_MAIN(0x46B108, void, __cdecl, BCWindow_SetCursor, TBC45XParWindow *, TBC45XModule *, const char *)
SCURKPRIMARY_GAMECALL_MAIN(0x46B7C8, void, __cdecl, BCWindowDC_Destruct, TBC45XWindowDC *, char)
SCURKPRIMARY_GAMECALL_MAIN(0x46B865, TBC45XClientDC *, __cdecl, BCClientDC_Construct, TBC45XClientDC *, HWND)
SCURKPRIMARY_GAMECALL_MAIN(0x46F9DB, void, __cdecl, BCDialog_SetupWindow, TBC45XParDialog *)
SCURKPRIMARY_GAMECALL_MAIN(0x46FABD, void, __cdecl, BCDialog_SetCaption, TBC45XParDialog *, char *)
SCURKPRIMARY_GAMECALL_MAIN(0x46FB35, void, __cdecl, BCDialog_EvClose, TBC45XParDialog *)
SCURKPRIMARY_GAMECALL_MAIN(0x4736E2, HWND, __cdecl, BCFrameWindow_GetCommandTarget, TBC45XFrameWindow *)
SCURKPRIMARY_GAMECALL_MAIN(0x476475, int, __cdecl, BCListBox_GetSelIndex, TBC45XParListBox *)
SCURKPRIMARY_GAMECALL_MAIN(0x477A62, LRESULT, __cdecl, BCListBox_GetString, TBC45XParListBox *, char *, int)
SCURKPRIMARY_GAMECALL_MAIN(0x477A9B, int, __cdecl, BCListBox_SetItemData, TBC45XParListBox *, int, unsigned int)
SCURKPRIMARY_GAMECALL_MAIN(0x477B26, int, __cdecl, BCListBox_AddString, TBC45XParListBox *, const char *)
SCURKPRIMARY_GAMECALL_MAIN(0x47854E, TBC45XMDIChild *, __cdecl, BCMDIClient_GetActiveMDIChild, TBC45XParMDIClient *)
SCURKPRIMARY_GAMECALL_MAIN(0x4790F2, BOOL, __cdecl, BCMDIFrame_SetMenu, TBC45XMDIFrame *, HMENU)
SCURKPRIMARY_GAMECALL_MAIN(0x480E97, char *, __cdecl, BC_strnewdup, char *, size_t)
SCURKPRIMARY_GAMECALL_MAIN(0x488028, void *, __cdecl, Op_New, size_t)

// Vars
SCURKPRIMARY_GAMEOFF_PTR(__int16,	wTileObjects,			0x491BAA)
SCURKPRIMARY_GAMEOFF(WORD,			wColFastCnt,			0x498506)
SCURKPRIMARY_GAMEOFF(WORD,			wColSlowCnt,			0x498508)
SCURKPRIMARY_GAMEOFF_PTR(__int16,	wtoolNum,				0x4A482E)
SCURKPRIMARY_GAMEOFF(__int16,		wtoolValue,				0x4A4866)
SCURKPRIMARY_GAMEOFF(TBC45XDib *,	mTileBack,				0x4A56A0)
SCURKPRIMARY_GAMEOFF(winscurkApp *,	gScurkApplication,		0x4A6D88)
SCURKPRIMARY_GAMEOFF_PTR(BC45Xstring,	TAppInitCmdLine,	0x4A73D0)
