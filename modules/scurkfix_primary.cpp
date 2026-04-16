// sc2kfix modules/scurkfix_primary.cpp: fixes for SCURK - primary (1995) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// This source file only contains hooks that by their very nature are unique
// to the primary version of SCURK (partial cases or those that only apply
// against this version), the rest are under scurkfix_common.cpp

#undef UNICODE
#include <windows.h>
#include <direct.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

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

extern "C" void __cdecl Hook_SCURKPrimary_DebugOut(char const *fmt, ...) {
	va_list args;

	if ((mischook_scurk_debug & MISCHOOK_SCURK_DEBUG_INTERNAL) == 0)
		return;

	va_start(args, fmt);
	L_SCURK_gDebugOut(fmt, args);
	va_end(args);
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

extern "C" void __declspec(naked) __cdecl Hook_SCURKPrimary_PaletteWindow_Construct_PalRes(void) {
	cPaletteWindow *pThis;
	
	__asm {
		mov ebx, [ebp + 0x8]
		mov [pThis], ebx
	}

	pThis = L_SCURK_LoadOwnPaletteResources(pThis);

	__asm {
		mov ebx, [pThis]
		mov [ebp + 0x8], ebx
	}

	GAMEJMP(0x450F7E);
}

void InstallFixes_SCURKPrimary(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk_debug = DEBUG_FLAGS_EVERYTHING;

	InstallRegistryPathingHooks_SCURKPrimary();

	L_SCURK_InitDOSMacPaletteIdxTable();

	// Hook for palette animation fix
	VirtualProtect((LPVOID)0x4496D4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4496D4, Hook_SCURK_winscurkMDIClient_CycleColors);
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
	NEWJMP((LPVOID)0x4104B8, Hook_SCURK_PlaceTileListDlg_SetupWindow);
	VirtualProtect((LPVOID)0x410D94, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410D94, Hook_SCURK_PlaceTileListDlg_EvLButtonDblClk);
	VirtualProtect((LPVOID)0x410ED0, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x410ED0, Hook_SCURK_PlaceTileListDlg_EvLBNSelChange);

	// TEncodeDib::mFillAt
	// TEncodeDib::mFillLine
	// Tweaks concerning various off-by-one and alignment
	// situations when it came to using the fill tool.
	// (There is some crossover with changes made to the
	// cEditableTileSet::mRenderDBShapeToDIB calls that
	// only apply when you switch back and forth between
	// tiles). #1
	VirtualProtect((LPVOID)0x4140D0, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4140D0, Hook_SCURK_EncodeDib_mFillAt);
	VirtualProtect((LPVOID)0x4141C8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4141C8, Hook_SCURK_EncodeDib_mFillLine);

	// TEncodeDib::mDetermineShapeHeight
	// Tweaks concerning height off-by-one cases.
	VirtualProtect((LPVOID)0x414314, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x414314, Hook_SCURK_EncodeDib_mDetermineShapeHeight);

	// TEncodeDib::mShrink
	// Account for an ancient issue concerning the
	// left-most column of pixels being missed
	// (This was the most visible when it came to the
	// Plymouth Arcology).
	VirtualProtect((LPVOID)0x41435C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x41435C, Hook_SCURK_EncodeDib_mShrink);

	// TEncodeDib::mEncodeShape
	// Fixes concerning off-by-one situation
	// as well as ensuring that once the bottom row
	// has been processed it always uses the "End of Shape"
	// mode.
	VirtualProtect((LPVOID)0x414450, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x414450, Hook_SCURK_EncodeDib_mEncodeShape);

	// Hook cEditableTileSet::mReadFromFile
	// This call is used to load the TILES.DB.
	VirtualProtect((LPVOID)0x4150EC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4150EC, Hook_SCURK_EditableTileSet_mReadFromFile);

	// cEditableTileSet::mWriteToMIFFFile
	// Backup functionality added.
	VirtualProtect((LPVOID)0x415404, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x415404, Hook_SCURK_EditableTileSet_mWriteToMIFFFile);

	// cEditableTileSet::mReadFromMIFFFile
	// Macintosh-type MIF detection added
	// for proper palette processing.
	VirtualProtect((LPVOID)0x415D78, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x415D78, Hook_SCURK_EditableTileSet_mReadFromMIFFFile);

	// cEditableTileSet::mReadShapeFromDib
	// PostBuild: This one concerns the reading of Shap information
	// from the processed Dib during TIL Large -> Small/Med building.
	// Paint: This one concerns the reading of Shap information
	// from the PaintWindow's EncodedDib during EditWindow tile
	// selection in-order to update the Small/Med objects based on
	// the large object.
	VirtualProtect((LPVOID)0x41692C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x41692C, Hook_SCURK_EditableTileSet_mReadShapeFromDib_PostBuild);
	VirtualProtect((LPVOID)0x416A18, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x416A18, Hook_SCURK_EditableTileSet_mReadShapeFromDib_Paint);

	// cEditableTileSet::mRenderDBShapeToDIB
	// Dib: Encoded Shap to PaintWindow Dib
	// Graphic: Encoded Shap PaintWindow WinGBitmap
	// Fixes/adjustments concerning both vertical off-by-one
	// and the omission of portions of the
	// right-most column of pixels on 4x4 objects
	// (Plymouth Arcology being a prime example).
	VirtualProtect((LPVOID)0x416C7C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x416C7C, Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Dib);
	VirtualProtect((LPVOID)0x416DFC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x416DFC, Hook_SCURK_EditableTileSet_mRenderDBShapeToDIB_Graphic);

	// cEditableTileSet::mRenderShapeToTile
	// Shap to the tile selection on the following windows:
	// - Place & Print object selection dialogue
	// - Pick & Copy tiles for source and working sets
	// - Paint the Town for the current working set
	// Fix/adjustment concerning a vertical off-by-one case.
	VirtualProtect((LPVOID)0x416F84, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x416F84, Hook_SCURK_EditableTileSet_mRenderShapeToTile);

	// cEditableTileSet::mReadFromDOSFile
	// Included a more comprehensive conversion
	// of the TIL set so it accounts for the
	// original small and tiny tiles
	// (At the moment processing only occurs
	// for Shap objects that are within the
	// bounds of the nEdNum range for WinSCURK).
	VirtualProtect((LPVOID)0x417150, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x417150, Hook_SCURK_EditableTileSet_mReadFromDOSFile);

	// cPaintWindow::mEncodeShape
	VirtualProtect((LPVOID)0x447200, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x447200, Hook_SCURK_PaintWindow_mEncodeShape);

	// 'nop' out the -1 case in the following functions in-regards to the
	// nMaxHorzScrollPos maximum extent:
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

	// cPaintWindow::mFill
	// Tweaks concerning various off-by-one and alignment
	// situations when it came to using the fill tool.
	// (There is some crossover with changes made to the
	// cEditableTileSet::mRenderDBShapeToDIB calls that
	// only apply when you switch back and forth between
	// tiles). #2
	VirtualProtect((LPVOID)0x444700, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x444700, Hook_SCURK_PaintWindow_mFill);

	// Hook cPaintWindow::mClipDrawing
	// Fixes/Adjustments to prevent certain
	// over/under clipping situations
	// (Originally even with a properly aligned
	// shape you'd see at least one pixel on each
	// row being clipped on the right-most side
	// of the tile base).
	VirtualProtect((LPVOID)0x443F34, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x443F34, Hook_SCURK_PaintWindow_mClipDrawing);

	// winscurkMDIFrame::AssignMenu
	VirtualProtect((LPVOID)0x4482F8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4482F8, Hook_SCURK_winscurkMDIFrame_AssignMenu);

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
	NEWJMP((LPVOID)0x450080, Hook_SCURK_MoverWindow_EvGetMinMaxInfo);

	// Re-route the loading of palette-related resources
	// in order for it use the adjusted set here.
	VirtualProtect((LPVOID)0x450E3F, 324, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x450E3F, 0x90, 324);
	NEWJMP((LPVOID)0x450E3F, Hook_SCURKPrimary_PaletteWindow_Construct_PalRes);

	// For both Left and Right Button Down it has now been adjusted so it
	// can access entries that are on row 10.
	VirtualProtect((LPVOID)0x4514E8, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4514E8, Hook_SCURK_PaletteWindow_EvLButtonDown);
	VirtualProtect((LPVOID)0x4515B4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4515B4, Hook_SCURK_PaletteWindow_EvRButtonDown);

	// OwlMain() command line fix.
	VirtualProtect((LPVOID)0x45A0B9, 7, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x45A0B9, 0x90, 7);
	NEWJMP((LPVOID)0x45A0B9, Hook_SCURKPrimary_OwlMainCommandLineFix);

	// TMenuItemEnabler::Enable
	VirtualProtect((LPVOID)0x47316A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x47316A, Hook_SCURK_MenuItemEnabler_Enable);

	// This hook is to prevent the Place&Pick selection dialogue
	// from being unintentionally closed; it catches and ignores
	// the cancel (esc) action.
	VirtualProtect((LPVOID)0x46FB26, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x46FB26, Hook_SCURK_BCDialog_CmCancel);

	// TFrameWindow::EvCommand
	VirtualProtect((LPVOID)0x473733, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x473733, Hook_SCURK_FrameWindow_EvCommand);
}
