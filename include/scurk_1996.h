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
SCURK1996_GAMECALL_MAIN(0x412134, int, __cdecl, WinGBitmap_Width, CWinGBitmap *)
SCURK1996_GAMECALL_MAIN(0x412154, int, __cdecl, WinGBitmap_Height, CWinGBitmap *)
SCURK1996_GAMECALL_MAIN(0x4132E8, void, __cdecl, DebugOut, const char *, ...)
SCURK1996_GAMECALL_MAIN(0x4132F0, int, __cdecl, gScurkMessage, DWORD, DWORD, DWORD)
SCURK1996_GAMECALL_MAIN(0x413448, int, __cdecl, gScurkMessage_Str, char *, DWORD, DWORD)
SCURK1996_GAMECALL_MAIN(0x413668, BC45Xstring *, __cdecl, gScurkLoadString, BC45Xstring *, DWORD)
SCURK1996_GAMECALL_MAIN(0x413788, void *, __cdecl, gAllocBlock, size_t)
SCURK1996_GAMECALL_MAIN(0x4137E8, void *, __cdecl, gResizeBlock, BYTE *, size_t)
SCURK1996_GAMECALL_MAIN(0x41380C, void, __cdecl, gFreeBlock, void *)
SCURK1996_GAMECALL_MAIN(0x413878, void, __stdcall, BeginWaitCursor)
SCURK1996_GAMECALL_MAIN(0x413890, void, __stdcall, EndWaitCursor)
SCURK1996_GAMECALL_MAIN(0x4138A8, void, __cdecl, gBeginWaitWindow, int, char *, TBC45XModule *)
SCURK1996_GAMECALL_MAIN(0x4139B0, void, __stdcall, gUpdateWaitWindow)
SCURK1996_GAMECALL_MAIN(0x4139F0, void, __stdcall, gEndWaitWindow)
SCURK1996_GAMECALL_MAIN(0x413DB0, TEncodeDib *, __cdecl, EncodeDib_Construct_Dimens, TEncodeDib *, LONG, LONG, UINT, uint16_t)
SCURK1996_GAMECALL_MAIN(0x413E88, void, __cdecl, EncodeDib_Destruct, TEncodeDib *, char)
SCURK1996_GAMECALL_MAIN(0x4140F0, void, __cdecl, EncodeDib_mFillAt, TEncodeDib *, TBC45XPoint *)
SCURK1996_GAMECALL_MAIN(0x4141E8, void, __cdecl, EncodeDib_mFillLine, TEncodeDib *, TBC45XPoint *, BYTE)
SCURK1996_GAMECALL_MAIN(0x414334, int, __cdecl, EncodeDib_mDetermineShapeHeight, TEncodeDib *)
SCURK1996_GAMECALL_MAIN(0x41437C, void, __cdecl, EncodeDib_mShrink, TEncodeDib *, TBC45XDib *, int)
SCURK1996_GAMECALL_MAIN(0x41443C, void, __cdecl, EncodeDib_mAcquireEncodedShapeData, TEncodeDib *, TEncodeDib *)
SCURK1996_GAMECALL_MAIN(0x414470, void, __cdecl, EncodeDib_mEncodeShape, TEncodeDib *, WORD, WORD, WORD)
SCURK1996_GAMECALL_MAIN(0x414908, int, __cdecl, EditableTileSet_mShapeNumToEditableNum, cEditableTileSet *, int)
SCURK1996_GAMECALL_MAIN(0x414D98, char *, __cdecl, EditableTileSet_GetLongName, cEditableTileSet *, int)
SCURK1996_GAMECALL_MAIN(0x41690C, int, __cdecl, EditableTileSet_mGetShapeWidth, cEditableTileSet *, int)
SCURK1996_GAMECALL_MAIN(0x416C68, void, __cdecl, EditableTileSet_mRenderEditableShapeToDIB_Dib, cEditableTileSet *, TBC45XDib *, int)
SCURK1996_GAMECALL_MAIN(0x416CA0, void, __cdecl, EditableTileSet_mRenderEditableShapeToDIB_Graphic, cEditableTileSet *, CWinGBitmap *, int)
SCURK1996_GAMECALL_MAIN(0x417448, void, __cdecl, EditableTileSet_mBuildSmallMedTiles, cEditableTileSet *)
SCURK1996_GAMECALL_MAIN(0x429538, void, __cdecl, PlaceWindow_DrawHouse, winscurkPlaceWindow *, char *)
SCURK1996_GAMECALL_MAIN(0x436C58, void, __cdecl, CheckExtension, char *, char *)
SCURK1996_GAMECALL_MAIN(0x43DC78, TBC45XDib *, __cdecl, EditWindow_mGetForegroundPattern, winscurkEditWindow *)
SCURK1996_GAMECALL_MAIN(0x43F4D0, void, __cdecl, EditWindow_mDoCurrentPatternDib, winscurkEditWindow *)
SCURK1996_GAMECALL_MAIN(0x43F6BC, int, __cdecl, EditWindow_mGetShapeWidth, winscurkEditWindow *)
SCURK1996_GAMECALL_MAIN(0x4401DC, BYTE, __cdecl, EditWindow_mGetForegroundColor, winscurkEditWindow *)
SCURK1996_GAMECALL_MAIN(0x4401F0, BYTE, __cdecl, EditWindow_mGetBackgroundColor, winscurkEditWindow *)
SCURK1996_GAMECALL_MAIN(0x440FEC, void, __cdecl, PlaceWindow_ClearCurrentTool, winscurkPlaceWindow *)
SCURK1996_GAMECALL_MAIN(0x443888, void, __cdecl, PaintWindow_mApplyTileToScreen, cPaintWindow *)
SCURK1996_GAMECALL_MAIN(0x443920, void, __cdecl, PaintWindow_mPreserveToUndoBuffer, cPaintWindow *)
SCURK1996_GAMECALL_MAIN(0x44395C, void, __cdecl, PaintWindow_mClearTile, cPaintWindow *, int)
SCURK1996_GAMECALL_MAIN(0x443F04, void, __cdecl, PaintWindow_mClipDrawing, cPaintWindow *)
SCURK1996_GAMECALL_MAIN(0x4440A0, void, __cdecl, PaintWindow_mScreenToDib, TBC45XPoint *, cPaintWindow *, TBC45XPoint *)
SCURK1996_GAMECALL_MAIN(0x4441E0, void, __cdecl, PaintWindow_mPutPixel, cPaintWindow *, TBC45XRect *, BYTE, BYTE)
SCURK1996_GAMECALL_MAIN(0x446894, void, __cdecl, PaintWindow_mDraw, cPaintWindow *)
SCURK1996_GAMECALL_MAIN(0x447138, void, __cdecl, PaintWindow_mEncodeShape, cPaintWindow *, int)
SCURK1996_GAMECALL_MAIN(0x4496E8, void, __cdecl, winscurkMDIClient_RotateColors, winscurkMDIClient *, int)
SCURK1996_GAMECALL_MAIN(0x44A338, void, __cdecl, winscurkMDIClient_mReadFromMIFFile, winscurkMDIClient *, cEditableTileSet *, const char *)
SCURK1996_GAMECALL_MAIN(0x44A50C, OPENFILENAMEA *, __cdecl, winscurkMDIClient_mGetOpenFileName, winscurkMDIClient *)
SCURK1996_GAMECALL_MAIN(0x44AC44, void, __cdecl, winscurkMDIClient_CmFileSaveWorking, winscurkMDIClient *)
SCURK1996_GAMECALL_MAIN(0x458150, TBC45XPalette *, __cdecl, winscurkApp_GetPalette, winscurkApp *)
SCURK1996_GAMECALL_MAIN(0x458484, char *, __cdecl, winscurkApp_mGetMiffPath, winscurkApp *)
SCURK1996_GAMECALL_MAIN(0x4584CC, void, __cdecl, winscurkApp_mSetMiffPath, winscurkApp *, char *)
SCURK1996_GAMECALL_MAIN(0x459C54, void, __cdecl, winscurkApp_ScurkSound, winscurkApp *, int)
SCURK1996_GAMECALL_MAIN(0x45A2F0, winscurkPlaceWindow *, __cdecl, winscurkApp_GetPlaceWindow, winscurkApp *)
SCURK1996_GAMECALL_MAIN(0x45A308, winscurkEditWindow *, __cdecl, winscurkApp_GetEditWindow, winscurkApp *)
SCURK1996_GAMECALL_MAIN(0x45A554, int, __cdecl, winscurkApp_mGetFileType, winscurkApp *, char *)

// BC and WinAPI
SCURK1996_GAMECALL_MAIN(0x45FF8F, void, __cdecl, BCDC_SelectObjectPalette, TBC45XDC *, TBC45XPalette *, int)
SCURK1996_GAMECALL_MAIN(0x469598, void, __cdecl, BCCommandEnabler_Enable, TBC45XCommandEnabler *)
SCURK1996_GAMECALL_MAIN(0x469D4A, unsigned int, __cdecl, BCWindow_DefaultProcessing, TBC45XParWindow *)
SCURK1996_GAMECALL_MAIN(0x469EDC, LRESULT, __cdecl, BCWindow_EvCommand, TBC45XWindow *, DWORD, HWND, DWORD)
SCURK1996_GAMECALL_MAIN(0x46A3E4, LRESULT, __cdecl, BCWindow_HandleMessage, TBC45XParWindow *, unsigned int msg, WPARAM wParam, LPARAM lParam)
SCURK1996_GAMECALL_MAIN(0x46A5A7, TBC45XWindow *, __cdecl, GetWindowPtr, HWND, TBC45XApplication *)
SCURK1996_GAMECALL_MAIN(0x46AB71, void, __cdecl, BCWindow_EvLButtonDown, TBC45XWindow *, DWORD, TBC45XPoint *)
SCURK1996_GAMECALL_MAIN(0x46B888, void, __cdecl, BCWindow_SetCursor, TBC45XParWindow *, TBC45XModule *, const char *)
SCURK1996_GAMECALL_MAIN(0x46BAB9, int, __cdecl, BCWindow_MessageBox, TBC45XWindow *, const char *, const char *, DWORD)
SCURK1996_GAMECALL_MAIN(0x46BF48, void, __cdecl, BCWindowDC_Destruct, TBC45XWindowDC *, char)
SCURK1996_GAMECALL_MAIN(0x46BFE5, TBC45XClientDC *, __cdecl, BCClientDC_Construct, TBC45XClientDC *, HWND)
SCURK1996_GAMECALL_MAIN(0x47015B, void, __cdecl, BCDialog_SetupWindow, TBC45XParDialog *)
SCURK1996_GAMECALL_MAIN(0x47023D, void, __cdecl, BCDialog_SetCaption, TBC45XParDialog *, char *)
SCURK1996_GAMECALL_MAIN(0x4702B5, void, __cdecl, BCDialog_EvClose, TBC45XParDialog *)
SCURK1996_GAMECALL_MAIN(0x473E62, HWND, __cdecl, BCFrameWindow_GetCommandTarget, TBC45XFrameWindow *)
SCURK1996_GAMECALL_MAIN(0x476BF5, int, __cdecl, BCListBox_GetSelIndex, TBC45XParListBox *)
SCURK1996_GAMECALL_MAIN(0x4781E2, LRESULT, __cdecl, BCListBox_GetString, TBC45XParListBox *, char *, int)
SCURK1996_GAMECALL_MAIN(0x47821B, int, __cdecl, BCListBox_SetItemData, TBC45XParListBox *, int, unsigned int)
SCURK1996_GAMECALL_MAIN(0x4782A6, int, __cdecl, BCListBox_AddString, TBC45XParListBox *, const char *)
SCURK1996_GAMECALL_MAIN(0x478CCE, TBC45XMDIChild *, __cdecl, BCMDIClient_GetActiveMDIChild, TBC45XParMDIClient *)
SCURK1996_GAMECALL_MAIN(0x479872, BOOL, __cdecl, BCMDIFrame_SetMenu, TBC45XMDIFrame *, HMENU)
SCURK1996_GAMECALL_MAIN(0x481617, char *, __cdecl, BC_strnewdup, char *, size_t)
SCURK1996_GAMECALL_MAIN(0x4887A8, void *, __cdecl, Op_New, size_t)
SCURK1996_GAMECALL_MAIN(0x489D40, void, __cdecl, BCString_Destruct, BC45Xstring *, char)

// Vars
SCURK1996_GAMEOFF_PTR(__int16,	wTileObjects,			0x491BAE)
SCURK1996_GAMEOFF(WORD,			wColFastCnt,			0x498232)
SCURK1996_GAMEOFF(WORD,			wColSlowCnt,			0x498234)
SCURK1996_GAMEOFF_PTR(__int16,	wtoolNum,				0x4A4892)
SCURK1996_GAMEOFF(__int16,		wtoolValue,				0x4A48CA)
SCURK1996_GAMEOFF(TBC45XDib *,	mTileBack,				0x4A5704)
SCURK1996_GAMEOFF(int,			gSaveSucceeded,			0x4A5EA4)
SCURK1996_GAMEOFF(winscurkApp *,gScurkApplication,		0x4A6DEC)
SCURK1996_GAMEOFF_PTR(BC45Xstring,	TAppInitCmdLine,	0x4A7434)
