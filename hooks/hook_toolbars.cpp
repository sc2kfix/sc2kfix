// sc2kfix hooks/hook_toolbars.cpp: toolbar handling.
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

// The "RCI" Indicator criteria.
#define RCI_DEFRCI_TOP    21

// If 'RCI_DEFRCI_TOP' is adjusted, the following
// offset may also need a tweak in order to keep
// the 'RCI' badge widget centred between the
// demand and surplus graph bars.
#define RCI_DEFWDG_OFFSET 2

// The height of the 'RCI' widget badge.
#define RCI_DEFWDG_HEIGHT 16

#define RCI_RECT_LEFT     66
#define RCI_RECT_TOP      257
#define RCI_RECT_RIGHT    92

// Offset values for drawing the +/_ indicators
#define RCI_PLUS_OFFSET   12
#define RCI_MINUS_OFFSET  8

// Offset value for hitting the RCI area
#define RCI_AREA_VERTOFFSET 5

#define TOOLBAR_DEBUG_OTHER 1

#define TOOLBAR_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TOOLBAR_DEBUG
#define TOOLBAR_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT toolbar_debug = TOOLBAR_DEBUG;

extern "C" void __stdcall Hook_CityToolBar_ToolMenuDisable() {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	ToggleFloatingStatusDialog(FALSE);

	GameMain_CityToolBar_ToolMenuDisable(pThis);
}

extern "C" void __stdcall Hook_CityToolBar_ToolMenuEnable() {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	ToggleFloatingStatusDialog(TRUE);

	GameMain_CityToolBar_ToolMenuEnable(pThis);
}

// This call is used for getting the 'top' of the bottom extent
// for the demand graph rectangle, and is also used for determining
// the top of the 'RCI' widget badge rectangle.
static int getSurplusTopExtent(int nExtent, int nTop, int nOffset) {
	return nExtent + nTop + nOffset;
}

static bool GetRCIWidgetCursorArea(CCityToolBar *pCCTB, CMFC3XPoint pt) {
	RECT RCIRect;

	RCIRect.left    = RCI_RECT_LEFT;
	RCIRect.top     = RCI_RECT_TOP; // Maximum top extent for when there's demand.
	RCIRect.right   = RCI_RECT_RIGHT;
	RCIRect.bottom  = getSurplusTopExtent(RCI_DEFRCI_TOP, RCIRect.top, RCI_DEFWDG_OFFSET) + RCI_DEFWDG_HEIGHT + RCI_DEFWDG_OFFSET + RCI_DEFRCI_TOP; // Maximum bottom extent for when there's a surplus.

	RCIRect.top    -= RCI_AREA_VERTOFFSET;
	RCIRect.bottom += RCI_AREA_VERTOFFSET;

	return (pt.x >= RCIRect.left && pt.x <= RCIRect.right) &&
		(pt.y >= RCIRect.top && pt.y <= RCIRect.bottom) ? true : false;
}

extern "C" void __stdcall Hook_CityToolBar_OnLButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	int iStoredMenuButtonPos;
	int iHitMenuButton;
	HMENU hSubMenu;
	int iCursorMoving;
	DWORD nTargetTicks;
	MSG Msg;
	UINT uMenuItem;
	UINT uItemCount;
	CMFC3XString cStr;
	CHAR *pBuffer;
	int iTrackedMenu;
	HWND mainhWnd;

	pSCApp = &pCSimcityAppThis;
	dwCityToolBarArcologyDialogCancel = 0;
	iStoredMenuButtonPos = pThis->iMyTBMenuButtonPos;
	if (pThis->m_cyTopBorder < pt.y) {
		iHitMenuButton = (GetRCIWidgetCursorArea(pThis, pt)) ? CITYTOOL_BUTTON_RCI : Game_CityToolBar_HitTestFromPoint(pThis, pt);
		pThis->iMyTBMenuButtonPos = (iHitMenuButton != CITYTOOL_BUTTON_RCI) ? iHitMenuButton : -1;
		if (iHitMenuButton < 0)
			return;
		// Added - 'Shift + Click' help messages that replaces the now non-functional
		// help file in Windows.
		if (iHitMenuButton != CITYTOOL_BUTTON_HELP && (nFlags & MK_SHIFT)) {
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
			DisplayItemHelp(pSCApp->m_pMainWnd->m_hWnd, HELPTYPE_CITYTOOLBAR, iHitMenuButton, true);
			return;
		}
		if (pThis->iMyTBMenuButtonPos > -1) {
			if (!Game_CityToolBar_PressButton(pThis, iHitMenuButton)) {
				pThis->iMyTBMenuButtonPos = iStoredMenuButtonPos;
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_ERROR);
				return;
			}
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
		}
		hSubMenu = (pThis->iMyTBMenuButtonPos > -1) ? GetSubMenu(pThis->dwCTBMenuOne.m_hMenu, pThis->iMyTBMenuButtonPos) : NULL;
		if (hSubMenu) {
			iCursorMoving = 1;
			nTargetTicks = GetTickCount32() + 500;
			pThis->dwMyTBButtonMenu = 1;
			while (TRUE) {
				if (GetTickCount32() < nTargetTicks) {
					if (PeekMessageA(&Msg, pThis->m_hWnd, 0, 0, WM_MOVE)) {
						if (!Game_SimcityApp_PreTranslateMessage(pSCApp, &Msg)) {
							TranslateMessage(&Msg);
							DispatchMessage(&Msg);
						}
					}
				}
				else
					iCursorMoving = 0;
				if (Msg.message == WM_LBUTTONUP)
					break;
				if (iCursorMoving != 1) {
					ClientToScreen(pThis->m_hWnd, &pt);
					uMenuItem = 0;
					if (GetMenuItemCount(hSubMenu)) {
						do {
							GameMain_String_Cons(&cStr);
							pBuffer = GameMain_String_GetBuffer(&cStr, 32);
							GetMenuStringA(hSubMenu, uMenuItem, pBuffer, 32, MF_BYPOSITION);
							GameMain_String_ReleaseBuffer(&cStr, -1);
							if (strcmp(pThis->dwCTBString[iHitMenuButton].m_pchData, cStr.m_pchData) == 0)
								CheckMenuItem(hSubMenu, uMenuItem, MF_BYPOSITION|MF_CHECKED);
							else
								CheckMenuItem(hSubMenu, uMenuItem, MF_BYPOSITION);
							GameMain_String_Dest(&cStr);
							++uMenuItem;
							uItemCount = GetMenuItemCount(hSubMenu);
						} while (uItemCount > uMenuItem);
					}
					iTrackedMenu = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x + 3, pt.y + 3, 0, pThis->m_hWnd, 0);
					if (iTrackedMenu > 0)
						PostMessageA(pThis->m_hWnd, WM_COMMAND, iTrackedMenu, 0);
					else
						Game_CityToolBar_SetSelection(pThis, iHitMenuButton, pThis->dwCTToolSelection[iHitMenuButton]);
					if (dwCityToolBarArcologyDialogCancel) {
						Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_REWARDS, TBBS_CHECKBOX);
						iHitMenuButton = CITYTOOL_BUTTON_CENTERINGTOOL;
					}
					Game_MyToolBar_SetButtonStyle(pThis, iHitMenuButton, (TBBS_CHECKED|TBBS_CHECKBOX));
					Game_MyToolBar_InvalidateButton(pThis, iHitMenuButton);
					Game_CityToolBar_OnCancelMode(pThis);
					goto MENUOUT;
				}
			}
			if (iHitMenuButton < CITYTOOL_BUTTON_RESIDENTIAL ||
				iHitMenuButton > CITYTOOL_BUTTON_INDUSTRIAL) {
				if (!iHitMenuButton || iHitMenuButton == CITYTOOL_BUTTON_POWER)
					Game_CityToolBar_SetSelection(pThis, iHitMenuButton, 0);
				else if (iHitMenuButton == CITYTOOL_BUTTON_REWARDS) {
					Game_MyToolBar_SetButtonStyle(pThis, iHitMenuButton, (TBBS_CHECKED|TBBS_CHECKBOX));
					Game_CityToolBar_SetSelection(pThis, iHitMenuButton, pThis->dwCTToolSelection[iHitMenuButton]);
				}
				else
					Game_CityToolBar_SetSelection(pThis, iHitMenuButton, wSelectedSubtool[iHitMenuButton]);
			}
			else
				Game_CityToolBar_SetSelection(pThis, iHitMenuButton, ZONES_HIGH);
			MENUOUT:
			pThis->dwMyTBButtonMenu = 0;
		}
		else {
			Game_CityToolBar_SetSelection(pThis, iHitMenuButton, 0);
			if (iHitMenuButton < CITYTOOL_BUTTON_ROTATEANTICLOCKWISE || iHitMenuButton == CITYTOOL_BUTTON_CENTERINGTOOL) {
				Game_MyToolBar_SetButtonStyle(pThis, iHitMenuButton, (TBBS_CHECKED|TBBS_CHECKBOX));
				Game_MyToolBar_InvalidateButton(pThis, iHitMenuButton);
				Game_CityToolBar_OnCancelMode(pThis);
			}
		}
	}
	else {
		pSCApp->dwSCADragSuspendSim = 1;
		pThis->dwCTBToolBarTitleDrag = 1;
		pThis->CTBGrabPoint.x = pt.x;
		pThis->CTBGrabPoint.y = pt.y;
		mainhWnd = SetCapture(pThis->m_hWnd);
		GameMain_Wnd_FromHandle(mainhWnd);
		ClientToScreen(pThis->m_hWnd, &pt);
		Game_CityToolBar_MoveAndBlitToolBar(pThis, pt.x, pt.y);
		pThis->CTBBorderPoint.x = pt.x;
		pThis->CTBBorderPoint.y = pt.y;
	}
	dwCityToolBarArcologyDialogCancel = 0;
}

extern "C" void __stdcall Hook_CityToolBar_SetSelection(DWORD nIndex, DWORD nSubIndex) {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	DWORD nSubTool;
	CMFC3XString *citySubToolStrings;
	BOOL bCurrentBudgetSetting;
	int iLayer;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (nIndex < CITYTOOL_BUTTON_SIGNS) {
		nSubTool = nSubIndex;
		if (nIndex == CITYTOOL_BUTTON_REWARDS && nSubIndex > REWARDS_ARCOLOGIES)
			nSubTool = REWARDS_ARCOLOGIES;
		citySubToolStrings = &cityToolGroupStrings[nIndex*MAX_CITY_MENUTOOLS];
		GameMain_String_OperatorCopy(&pThis->dwCTBString[nIndex], &citySubToolStrings[nSubTool]);
		pThis->dwCTToolSelection[nIndex] = nSubTool;
	}
	switch (nIndex) {
		case CITYTOOL_BUTTON_BULLDOZER:
			pSCApp->iSCAActiveCursor = GAMECURSOR_BULLDOZER;
			wCurrentCityToolGroup = CITYTOOL_GROUP_BULLDOZER;
			wSelectedSubtool[CITYTOOL_GROUP_BULLDOZER] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_NATURE:
			pSCApp->iSCAActiveCursor = (nSubIndex) ? GAMECURSOR_POND : GAMECURSOR_TREE;
			wCurrentCityToolGroup = CITYTOOL_GROUP_NATURE;
			wSelectedSubtool[CITYTOOL_GROUP_NATURE] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_DISPATCH:
			pSCApp->iSCAActiveCursor = GAMECURSOR_DISPATCH;
			wCurrentCityToolGroup = CITYTOOL_GROUP_DISPATCH;
			wSelectedSubtool[CITYTOOL_GROUP_DISPATCH] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_POWER:
			pSCApp->iSCAActiveCursor = GAMECURSOR_POWER;
			wCurrentCityToolGroup = CITYTOOL_GROUP_POWER;
			wSelectedSubtool[CITYTOOL_GROUP_POWER] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_WATER:
			pSCApp->iSCAActiveCursor = GAMECURSOR_WATER;
			wCurrentCityToolGroup = CITYTOOL_GROUP_WATER;
			wSelectedSubtool[CITYTOOL_GROUP_WATER] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_REWARDS:
			wSelectedSubtool[CITYTOOL_GROUP_REWARDS] = (WORD)nSubIndex;
			pSCApp->iSCAActiveCursor = GAMECURSOR_REWARDS;
			wCurrentCityToolGroup = CITYTOOL_GROUP_REWARDS;
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_CENTERINGTOOL, TBBS_CHECKBOX);
			break;
		case CITYTOOL_BUTTON_ROAD:
			pSCApp->iSCAActiveCursor = GAMECURSOR_ROAD;
			wCurrentCityToolGroup = CITYTOOL_GROUP_ROADS;
			wSelectedSubtool[CITYTOOL_GROUP_ROADS] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_RAIL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_RAIL;
			wCurrentCityToolGroup = CITYTOOL_GROUP_RAIL;
			wSelectedSubtool[CITYTOOL_GROUP_RAIL] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_PORTS:
			pSCApp->iSCAActiveCursor = GAMECURSOR_PORTS;
			wCurrentCityToolGroup = CITYTOOL_GROUP_PORTS;
			wSelectedSubtool[CITYTOOL_GROUP_PORTS] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_RESIDENTIAL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_RESIDENTIAL;
			wCurrentCityToolGroup = CITYTOOL_GROUP_RESIDENTIAL;
			wSelectedSubtool[CITYTOOL_GROUP_RESIDENTIAL] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_COMMERCIAL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_COMMERCIAL;
			wCurrentCityToolGroup = CITYTOOL_GROUP_COMMERCIAL;
			wSelectedSubtool[CITYTOOL_GROUP_COMMERCIAL] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_INDUSTRIAL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_INDUSTRIAL;
			wCurrentCityToolGroup = CITYTOOL_GROUP_INDUSTRIAL;
			wSelectedSubtool[CITYTOOL_GROUP_INDUSTRIAL] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_EDUCATION:
			pSCApp->iSCAActiveCursor = GAMECURSOR_EDUCATION;
			wCurrentCityToolGroup = CITYTOOL_GROUP_EDUCATION;
			wSelectedSubtool[CITYTOOL_GROUP_EDUCATION] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_SERVICES:
			pSCApp->iSCAActiveCursor = GAMECURSOR_SERVICES;
			wCurrentCityToolGroup = CITYTOOL_GROUP_SERVICES;
			wSelectedSubtool[CITYTOOL_GROUP_SERVICES] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_PARKS:
			pSCApp->iSCAActiveCursor = GAMECURSOR_PARKS;
			wCurrentCityToolGroup = CITYTOOL_GROUP_PARKS;
			wSelectedSubtool[CITYTOOL_GROUP_PARKS] = (WORD)nSubIndex;
			break;
		case CITYTOOL_BUTTON_SIGNS:
			pSCApp->iSCAActiveCursor = GAMECURSOR_SIGNS;
			wCurrentCityToolGroup = CITYTOOL_GROUP_SIGNS;
			break;
		case CITYTOOL_BUTTON_QUERY:
			pSCApp->iSCAActiveCursor = GAMECURSOR_QUERY;
			wCurrentCityToolGroup = CITYTOOL_GROUP_QUERY;
			break;
		case CITYTOOL_BUTTON_ROTATEANTICLOCKWISE:
			Game_SimcityView_RotateAntiClockwise(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ROTATEANTICLOCKWISE, 0);
			break;
		case CITYTOOL_BUTTON_ROTATECLOCKWISE:
			Game_SimcityView_RotateClockwise(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ROTATECLOCKWISE, 0);
			break;
		case CITYTOOL_BUTTON_ZOOMOUT:
			Game_SimcityView_ScaleOut(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			if (pSCView->wSCVZoomLevel) {
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, 0);
				if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_LARGE && !pSCView->dwSCVIsZoomed)
					Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, 0);
			}
			else
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, TBBS_DISABLED);
			break;
		case CITYTOOL_BUTTON_ZOOMIN:
			Game_SimcityView_ScaleIn(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			if (pSCView->dwSCVIsZoomed)
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, TBBS_DISABLED);
			else {
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, 0);
				if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_SMALL)
					Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, 0);
			}
			break;
		case CITYTOOL_BUTTON_CENTERINGTOOL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_CENTER;
			wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
			break;
		case CITYTOOL_BUTTON_CITYMAP:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, SC2K_DIALOG_CITYMAP);
			break;
		case CITYTOOL_BUTTON_CITYPOPULATION:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, SC2K_DIALOG_POPULATION);
			break;
		case CITYTOOL_BUTTON_CITYNEIGHBOURS:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, SC2K_DIALOG_NEIGHBORS);
			break;
		case CITYTOOL_BUTTON_CITYGRAPHS:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, SC2K_DIALOG_GRAPH);
			break;
		case CITYTOOL_BUTTON_CITYINDUSTRY:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, SC2K_DIALOG_INDUSTRY);
			break;
		case CITYTOOL_BUTTON_BUDGET:
			bCurrentBudgetSetting = bOptionsAutoBudget;
			bOptionsAutoBudget = 0;
			Game_SimulationPrepareBudgetDialog(0);
			bOptionsAutoBudget = bCurrentBudgetSetting;
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_BUDGET, 0);
			break;
		case CITYTOOL_BUTTON_DISPLAYBUILDINGS:
		case CITYTOOL_BUTTON_DISPLAYSIGNS:
		case CITYTOOL_BUTTON_DISPLAYINFRA:
		case CITYTOOL_BUTTON_DISPLAYZONES:
		case CITYTOOL_BUTTON_DISPLAYUNDERGROUND:
			iLayer = LAYER_BUILDINGS - (nIndex - CITYTOOL_BUTTON_DISPLAYBUILDINGS);
			DisplayLayer[iLayer] = !DisplayLayer[iLayer];
			if (nIndex == CITYTOOL_BUTTON_DISPLAYUNDERGROUND)
				Game_CityToolBar_AdjustLayers(pThis, 0);
			Game_SimcityView_DrawHouse(pSCView);
			break;
		case CITYTOOL_BUTTON_HELP:
			DisplayItemHelp(pSCApp->m_pMainWnd->m_hWnd, HELPTYPE_CITYTOOLBAR, nIndex, true);
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_HELP, 0);
			break;
		case CITYTOOL_BUTTON_RCI:
			// Some functionality perhaps.
			return;
		default:
			break;
	}
	Game_SimcityApp_UpdateStatus(pSCApp, FALSE);
	Game_SimcityApp_GetToolSound(pSCApp);
	if (nIndex < CITYTOOL_BUTTON_SIGNS) {
		nSubTool = wSelectedSubtool[nIndex];
		if (nIndex == CITYTOOL_BUTTON_WATER && !wSelectedSubtool[CITYTOOL_GROUP_WATER] ||
			nIndex == CITYTOOL_BUTTON_RAIL && nSubTool == RAILS_SUBWAY) {
			if (!DisplayLayer[LAYER_UNDERGROUND]) {
				DisplayLayer[LAYER_UNDERGROUND] = 1;
				Game_CityToolBar_AdjustLayers(pThis, 1);
				Game_SimcityView_DrawHouse(pSCView);
			}
		}
		else if (nIndex) {
			if ((nIndex != CITYTOOL_BUTTON_RAIL || nSubTool != RAILS_SUBSTATION && nSubTool != RAILS_SUBTORAIL) &&
				(nIndex != CITYTOOL_BUTTON_WATER || nSubTool != WATER_PUMP)) {
				if (DisplayLayer[LAYER_UNDERGROUND]) {
					DisplayLayer[LAYER_UNDERGROUND] = 0;
					Game_CityToolBar_AdjustLayers(pThis, 1);
					Game_SimcityView_DrawHouse(pSCView);
				}
			}
		}
	}
}

extern "C" void __stdcall Hook_MapToolBar_OnLButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CMapToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	int iHitMenuButton;
	HWND mainhWnd;

	pSCApp = &pCSimcityAppThis;
	if (pThis->m_cyTopBorder < pt.y) {
		iHitMenuButton = Game_MapToolBar_HitTestFromPoint(pThis, pt);
		pThis->iMyTBMenuButtonPos = iHitMenuButton;
		if (iHitMenuButton >= 0) {
			// Added - 'Shift + Click' help messages that replaces the now non-functional
			// help file in Windows.
			if (iHitMenuButton != MAPTOOL_BUTTON_HELP && (nFlags & MK_SHIFT)) {
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
				DisplayItemHelp(pSCApp->m_pMainWnd->m_hWnd, HELPTYPE_MAPTOOLBAR, iHitMenuButton, true);
				return;
			}
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
			Game_MapToolBar_PressButton(pThis, iHitMenuButton);
			Game_MapToolBar_SetSelection(pThis, iHitMenuButton, 0, &pt);
		}
	}
	else {
		pThis->dwMTBToolBarTitleDrag = 1;
		pThis->MTBGrabPoint.x = pt.x;
		pThis->MTBGrabPoint.y = pt.y;
		mainhWnd = SetCapture(pThis->m_hWnd);
		GameMain_Wnd_FromHandle(mainhWnd);
		ClientToScreen(pThis->m_hWnd, &pt);
		Game_MapToolBar_MoveAndBlitToolBar(pThis, pt.x, pt.y);
		pThis->MTBBorderPoint.x = pt.x;
		pThis->MTBBorderPoint.y = pt.y;
	}
}

extern "C" void __stdcall Hook_MapToolBar_SetSelection(UINT nIndex, UINT nSubIndex, CMFC3XPoint *pt) {
	CMapToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	switch (nIndex) {
		case MAPTOOL_BUTTON_RAISETERRAIN:
			pSCApp->iSCAActiveCursor = GAMECURSOR_RAISETERRAIN;
			wCurrentMapToolGroup = MAPTOOL_GROUP_RAISETERRAIN;
			break;
		case MAPTOOL_BUTTON_LOWERTERRAIN:
			pSCApp->iSCAActiveCursor = GAMECURSOR_LOWERTERRAIN;
			wCurrentMapToolGroup = MAPTOOL_GROUP_LOWERTERRAIN;
			break;
		case MAPTOOL_BUTTON_STRETCHTERRAIN:
			pSCApp->iSCAActiveCursor = GAMECURSOR_STRETCHTERRAIN;
			wCurrentMapToolGroup = MAPTOOL_GROUP_STRETCHTERRAIN;
			break;
		case MAPTOOL_BUTTON_LEVELTERRAIN:
			pSCApp->iSCAActiveCursor = GAMECURSOR_LEVELTERRAIN;
			wCurrentMapToolGroup = MAPTOOL_GROUP_LEVELTERRAIN;
			break;
		case MAPTOOL_BUTTON_INCREASEWATERLEVEL:
			Game_IncreaseWaterLevel();
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_FLOOD);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_INCREASEWATERLEVEL, 0);
			break;
		case MAPTOOL_BUTTON_DECREASEWATERLEVEL:
			Game_DecreaseWaterLevel();
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_FLOOD);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_DECREASEWATERLEVEL, 0);
			break;
		case MAPTOOL_BUTTON_WATER:
			pSCApp->iSCAActiveCursor = GAMECURSOR_POND;
			wCurrentMapToolGroup = MAPTOOL_GROUP_WATER;
			break;
		case MAPTOOL_BUTTON_STREAM:
			pSCApp->iSCAActiveCursor = GAMECURSOR_STREAM;
			wCurrentMapToolGroup = MAPTOOL_GROUP_STREAM;
			break;
		case MAPTOOL_BUTTON_TREES:
			pSCApp->iSCAActiveCursor = GAMECURSOR_TREE;
			wCurrentMapToolGroup = MAPTOOL_GROUP_TREES;
			break;
		case MAPTOOL_BUTTON_FOREST:
			pSCApp->iSCAActiveCursor = GAMECURSOR_FOREST;
			wCurrentMapToolGroup = MAPTOOL_GROUP_FOREST;
			break;
		case MAPTOOL_BUTTON_CENTERINGTOOL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_CENTER;
			wCurrentMapToolGroup = MAPTOOL_GROUP_CENTERINGTOOL;
			break;
		case MAPTOOL_BUTTON_ZOOMOUT:
			Game_SimcityView_ScaleOut(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			if (pSCView->wSCVZoomLevel) {
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMOUT, 0);
				if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_LARGE && !pSCView->dwSCVIsZoomed)
					Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMIN, 0);
			}
			else
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMOUT, TBBS_DISABLED);
			break;
		case MAPTOOL_BUTTON_ZOOMIN:
			Game_SimcityView_ScaleIn(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			if (pSCView->dwSCVIsZoomed)
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMIN, TBBS_DISABLED);
			else {
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMIN, 0);
				if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_SMALL)
					Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ZOOMOUT, 0);
			}
			break;
		case MAPTOOL_BUTTON_ROTATEANTICLOCKWISE:
			Game_SimcityView_RotateAntiClockwise(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ROTATEANTICLOCKWISE, 0);
			break;
		case MAPTOOL_BUTTON_ROTATECLOCKWISE:
			Game_SimcityView_RotateClockwise(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_ROTATECLOCKWISE, 0);
			break;
		case MAPTOOL_BUTTON_HELP:
			DisplayItemHelp(pSCApp->m_pMainWnd->m_hWnd, HELPTYPE_MAPTOOLBAR, nIndex, true);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_HELP, 0);
			break;
		case MAPTOOL_BUTTON_TERRAINHILLS:
		case MAPTOOL_BUTTON_TERRAINWATER:
		case MAPTOOL_BUTTON_TERRAINTREES:
			Game_MapToolBar_AdjustSlider(pThis, nIndex, pt);
			break;
		case MAPTOOL_BUTTON_TOGGLEOCEAN:
			if (bCityHasOcean) {
				bCityHasOcean = 0;
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_TOGGLEOCEAN, (TBBS_CHECKED|TBBS_CHECKBOX));
			}
			else {
				bCityHasOcean = 1;
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_TOGGLEOCEAN, TBBS_CHECKBOX);
			}
			break;
		case MAPTOOL_BUTTON_TOGGLERIVER:
			if (bCityHasRiver) {
				bCityHasRiver = 0;
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_TOGGLERIVER, (TBBS_CHECKED|TBBS_CHECKBOX));
			}
			else {
				bCityHasRiver = 1;
				Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_TOGGLERIVER, TBBS_CHECKBOX);
			}
			break;
		case MAPTOOL_BUTTON_MAKE:
			Game_SimcityView_MakeTerrain(
				pSCView,
				bCityHasOcean,
				bCityHasRiver,
				wCityTerrainSliderHills,
				wCityTerrainSliderWater,
				wCityTerrainSliderTrees);
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_MAKE, 0);
			Game_MapToolBar_PressButton(pThis, MAPTOOL_BUTTON_CENTERINGTOOL);
			break;
		case MAPTOOL_BUTTON_DONE:
			// Rotate the map 90 degrees at a time until wViewRotation is north. We need to do
			// this because SimCity 2000 was programmed by madmen and rotating the viewport is
			// actually accomplished by rotating all the map data in memory.
			if (wViewRotation != VIEWROTATION_NORTH) {
				do
					Game_SimcityView_RotateAntiClockwise(pSCView);
				while (wViewRotation != VIEWROTATION_NORTH);
			}
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_DONE, 0);
			Game_SimcityApp_NewCity(pSCApp);
			break;
		default:
			break;
	}
	Game_SimcityApp_GetToolSound(pSCApp);
}

static void CityToolBar_RCIPlusMinus(CCityToolBar *pCCTB, HDC hDC, LONG x, LONG y, const char *pStr) {
	if (!pStr)
		return;

	HFONT hOldFont = SelectFont(hDC, hFontMSSansSerifRegular8);
	SetTextColor(hDC, pCCTB->dwMyTBButtonText);
	SetTextAlign(hDC, TA_CENTER);
	SetBkMode(hDC, TRANSPARENT);

	TextOutA(hDC, x, y, pStr, strlen(pStr));
	SelectFont(hDC, hOldFont);
}

extern "C" void __stdcall Hook_CityToolBar_DrawRCIIndicator(CMFC3XDC *pDC) {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	RECT RCIRect, RCIWidgRect, r;
	COLORREF BkColor;
	int left, top, cx, cy;
	int nIndicatorHorzOffset, nRCI, nColDemand, nColSepSpace, nColPos;
	RECT *pRectClear;
	HFONT hOldFont;
	const char *RCIStr;

	RCIRect.left   = RCI_RECT_LEFT;
	RCIRect.top    = RCI_RECT_TOP; // Maximum top extent for when there's demand.
	RCIRect.right  = RCI_RECT_RIGHT;
	RCIRect.bottom = getSurplusTopExtent(RCI_DEFRCI_TOP, RCIRect.top, RCI_DEFWDG_OFFSET) + RCI_DEFWDG_HEIGHT + RCI_DEFWDG_OFFSET + RCI_DEFRCI_TOP; // Maximum bottom extent for when there's a surplus.

	// Horizontal offset for the +/_ indicator.
	nIndicatorHorzOffset = 2 * (RCI_RECT_RIGHT - RCI_RECT_LEFT) / 4;

	BkColor = GetBkColor(pDC->m_hAttribDC);

	// +
	CityToolBar_RCIPlusMinus(pThis, pDC->m_hDC, RCI_RECT_LEFT + nIndicatorHorzOffset, RCIRect.top - RCI_PLUS_OFFSET, "+");

	// Annoying case here, if the painted graph
	// area goes beyond a total height (top to bottom)
	// of 64px, it'll leave bleed artifacts behind depending
	// on whether the top or bottom exceeded that given
	// height measurement.
	nColSepSpace = 2;
	for (nRCI = 0; nRCI < DEMAND_COUNT; ++nRCI) {
		nColDemand = RCI_DEFRCI_TOP * wCityDemand[nRCI] / 2000;
		if (nColDemand) {
			nColPos = (nRCI + 1) * (RCI_RECT_RIGHT - RCI_RECT_LEFT) / 4;
			if (nRCI == DEMAND_IND)
				nColPos++;

			r.left = RCI_RECT_LEFT + nColPos - nColSepSpace;
			r.right = RCI_RECT_LEFT + nColSepSpace + nColPos;

			pRectClear = &RCIRect;

			if (nColDemand <= 0) {
				r.top = pRectClear->bottom - RCI_DEFRCI_TOP;
				r.bottom = pRectClear->bottom - RCI_DEFRCI_TOP - nColDemand;
			}
			else {
				r.bottom = pRectClear->top + RCI_DEFRCI_TOP;
				r.top = pRectClear->top + RCI_DEFRCI_TOP - nColDemand;
			}

			pRectClear->left = r.left;
			pRectClear->right = r.right;

			// Clear the entire column.
			InflateRect(pRectClear, 1, 1);
			SetBkColor(pDC->m_hDC, pThis->dwMyTBButtonFace);
			ExtTextOutA(pDC->m_hDC, pRectClear->left, pRectClear->top, ETO_OPAQUE, pRectClear, 0, 0, 0);
			InflateRect(pRectClear, -1, -1);

			// Border
			InflateRect(&r, 1, 1);
			SetBkColor(pDC->m_hDC, PALETTEINDEX(164));
			ExtTextOutA(pDC->m_hDC, r.left, r.top, ETO_OPAQUE, &r, 0, 0, 0);

			// Graph bar
			InflateRect(&r, -1, -1);
			SetBkColor(pDC->m_hDC, colRCI[nRCI]);
			ExtTextOutA(pDC->m_hDC, r.left, r.top, ETO_OPAQUE, &r, 0, 0, 0);
		}
	}

	// The "RCI" middle widget.
	RCIWidgRect.left = RCI_RECT_LEFT;
	RCIWidgRect.top = RCIRect.top;
	RCIWidgRect.right = RCI_RECT_RIGHT;
	RCIWidgRect.bottom = RCIRect.bottom;

	left = RCIWidgRect.left;
	top = getSurplusTopExtent(RCI_DEFRCI_TOP, RCIWidgRect.top, RCI_DEFWDG_OFFSET);
	cx = RCIWidgRect.right - left;
	cy = RCI_DEFWDG_HEIGHT;

	hOldFont = SelectFont(pDC->m_hDC, MainFontsArl[0]->m_hObject);
	SetBkColor(pDC->m_hDC, pThis->dwMyTBButtonFace);
	SetTextColor(pDC->m_hDC, pThis->dwMyTBButtonText);
	SetTextAlign(pDC->m_hDC, TA_CENTER);
	SetBkMode(pDC->m_hDC, TRANSPARENT);

	RCIStr = "RCI";
	RCIWidgRect.top = top;
	RCIWidgRect.bottom = top + cy;
	TextOutA(pDC->m_hDC, (cx / 2 + left) - 1, top + 2, RCIStr, strlen(RCIStr));

	SelectFont(pDC->m_hDC, hOldFont);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left, top, 1, cy - 1, pThis->dwMyTBButtonHighlighted);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left, top, cx - 1, 1, pThis->dwMyTBButtonHighlighted);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left + cx - 1, top, 1, cy, pThis->dwMyTBButtonShadow);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left, top + cy - 1, cx, 1, pThis->dwMyTBButtonShadow);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left + cx - 2, top + 1, 1, cy - 2, pThis->dwMyTBButtonShadow);
	GameMain_CityToolBarSetBgdAndText(pDC->m_hDC, left + 1, top + cy - 2, cx - 2, 1, pThis->dwMyTBButtonShadow);

	// _ (An underscore stands out more than a dash)
	CityToolBar_RCIPlusMinus(pThis, pDC->m_hDC, RCI_RECT_LEFT + nIndicatorHorzOffset, RCIRect.bottom - RCI_MINUS_OFFSET, "_");

	SetBkColor(pDC->m_hAttribDC, BkColor);
}

extern "C" void __cdecl Hook_CityToolMenuAction(UINT nFlags, CMFC3XPoint pt) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iBulldozerTool, iCurrCityToolGroupWithHotKey;
	__int16 iTileCoords;
	coords_w_t tileCoords;
	int ret;
	BYTE bTextOverlay;
	WORD wNewScreenPointX, wNewScreenPointY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	iBulldozerTool = wSelectedSubtool[CITYTOOL_GROUP_BULLDOZER];
	if ((nFlags & MK_SHIFT) != 0)
		iCurrCityToolGroupWithHotKey = CITYTOOL_GROUP_QUERY;
	else if ((nFlags & MK_CONTROL) != 0) {
		iCurrCityToolGroupWithHotKey = CITYTOOL_GROUP_BULLDOZER;
		wSelectedSubtool[CITYTOOL_GROUP_BULLDOZER] = BULLDOZER_DEMOLISH;
	}
	else
		iCurrCityToolGroupWithHotKey = wCurrentCityToolGroup;
	iTileCoords = Game_PointToTile((__int16)pt.x, (__int16)pt.y);
	if (iTileCoords >= 0) {
		tileCoords.x = LOBYTE(iTileCoords);
		tileCoords.y = HIBYTE(iTileCoords);
		if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y >= 0) {
			switch (iCurrCityToolGroupWithHotKey) {
			case CITYTOOL_GROUP_BULLDOZER:
				Game_UseBulldozer(tileCoords.x, tileCoords.y);
				// This was UpdateHouse(), however ghost
				// inverted tiles were left in underground
				// mode when clicking single clear tiles.
				Game_SimcityView_DrawHouse(pSCView);
				break;
			case CITYTOOL_GROUP_NATURE:
				Game_CityToolPlaceNature(pt);
				return;
			case CITYTOOL_GROUP_DISPATCH:
				if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == DISPATCH_FIRE)
					Game_PlaceFireDispatchUnit(tileCoords.x, tileCoords.y);
				else if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == DISPATCH_MILITARY)
					Game_PlaceMilitaryDispatchUnit(tileCoords.x, tileCoords.y);
				else
					Game_PlacePoliceDispatchUnit(tileCoords.x, tileCoords.y);
				// With wZoomLevel set to maximum and dwSCVIsZoomed set to
				// true, there's a bug while paused that prevents a change
				// to the tile from being displayed immediately (likely the
				// dirtyRect calculation).
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_DrawHouse(pSCView);
				else
					Game_SimcityView_UpdateHouse(pSCView);
				break;
			case CITYTOOL_GROUP_POWER:
				if (wSelectedSubtool[iCurrCityToolGroupWithHotKey]) {
					if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == POWER_PLANTS_HYDRO)
						ret = Game_CityToolPlacePowerHydroDam(tileCoords.x, tileCoords.y);
					else
						ret = Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]);
				}
				else
					ret = Game_SimcityView_CityToolPlacePowerLine(pSCView, tileCoords.x, tileCoords.y);
				if (ret) {
					L_PlayToolSound_SC2K1996(pSCApp);
					// Commented out section is to do with the power/water grid updates slowdown case.
					if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == POWER_WIRES/* && dwCityPopulation < 50000*/)
						Game_SimulationUpdatePowerConsumption();
				}
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				// With wZoomLevel set to maximum and dwSCVIsZoomed set to
				// true, there's a bug while paused that prevents a change
				// to the tile from being displayed immediately (likely the
				// dirtyRect calculation).
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_DrawHouse(pSCView);
				else
					Game_SimcityView_UpdateHouse(pSCView);
				// Interesting case.. why return for anything that's not wind?
				if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] != POWER_PLANTS_WIND)
					return;
				break;
			case CITYTOOL_GROUP_WATER:
				if (wSelectedSubtool[CITYTOOL_GROUP_WATER])
					ret = Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]);
				else
					ret = Game_SimcityView_CityToolPlaceWaterPipe(pSCView, tileCoords.x, tileCoords.y);
				if (ret) {
					L_PlayToolSound_SC2K1996(pSCApp);
					// Commented out section is to do with the power/water grid updates slowdown case.
					if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == WATER_PIPES/* && dwCityPopulation < 50000*/)
						Game_SimulationUpdateWaterConsumption();
				}
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				// With wZoomLevel set to maximum and dwSCVIsZoomed set to
				// true, there's a bug while paused that prevents a change
				// to the tile from being displayed immediately (likely the
				// dirtyRect calculation).
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_DrawHouse(pSCView);
				else
					Game_SimcityView_UpdateHouse(pSCView);
				// Interesting case.. why return for anything that's not a pump?
				if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] != WATER_PUMP)
					return;
				break;
			case CITYTOOL_GROUP_REWARDS:
				if (Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]))
				{
					if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] < REWARDS_ARCOLOGIES) {
						Game_SimulationToggleGrantReward(wSelectedSubtool[iCurrCityToolGroupWithHotKey], FALSE);
						wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
					}
					L_PlayToolSound_SC2K1996(pSCApp);
				}
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				// With wZoomLevel set to maximum and dwSCVIsZoomed set to
				// true, there's a bug while paused that prevents a change
				// to the tile from being displayed immediately (likely the
				// dirtyRect calculation).
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_DrawHouse(pSCView);
				else
					Game_SimcityView_UpdateHouse(pSCView);
				break;
			case CITYTOOL_GROUP_ROADS:
				ret = iBulldozerTool;
				switch (wSelectedSubtool[iCurrCityToolGroupWithHotKey]) {
				case ROADS_ROAD:
					ret = Game_SimcityView_CityToolPlaceRoad(pSCView, tileCoords.x, tileCoords.y);
					break;
				case ROADS_HIGHWAY:
					ret = Game_SimcityView_CityToolPlaceHighway(pSCView, tileCoords.x, tileCoords.y);
					break;
				case ROADS_TUNNEL:
					ret = Game_CityToolPlaceTunnel(tileCoords.x, tileCoords.y);
					break;
				case ROADS_ONRAMP:
					ret = Game_CityToolPlaceOnRamp(tileCoords.x, tileCoords.y);
					break;
				case ROADS_BUSSTATION:
					ret = Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]);
					break;
				}
				if (ret)
					L_PlayToolSound_SC2K1996(pSCApp);
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				Game_SimcityView_DrawHouse(pSCView);
				return;
			case CITYTOOL_GROUP_RAIL:
				ret = iBulldozerTool;
				switch (wSelectedSubtool[iCurrCityToolGroupWithHotKey]) {
				case RAILS_RAIL:
					ret = Game_SimcityView_CityToolPlaceRail(pSCView, tileCoords.x, tileCoords.y);
					break;
				case RAILS_SUBWAY:
					ret = Game_SimcityView_CityToolPlaceSubway(pSCView, tileCoords.x, tileCoords.y);
					break;
				case RAILS_DEPOT:
				case RAILS_SUBSTATION:
					ret = Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]);
					break;
				case RAILS_SUBTORAIL:
					ret = Game_CityToolPlaceSubToRail(tileCoords.x, tileCoords.y);
					break;
				}
				if (ret) {
					if (wSelectedSubtool[iCurrCityToolGroupWithHotKey] == RAILS_DEPOT)
						Game_SpawnTrain(tileCoords.x, tileCoords.y);
					L_PlayToolSound_SC2K1996(pSCApp);
				}
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				Game_SimcityView_DrawHouse(pSCView);
				return;
			case CITYTOOL_GROUP_PORTS:
			case CITYTOOL_GROUP_RESIDENTIAL:
			case CITYTOOL_GROUP_COMMERCIAL:
			case CITYTOOL_GROUP_INDUSTRIAL:
				if (Game_SimcityView_CityToolSetSelectedZone(pSCView, tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey]))
					L_PlayToolSound_SC2K1996(pSCApp);
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				// With wZoomLevel set to maximum and dwSCVIsZoomed set to
				// true, there's a bug while paused that prevents a change
				// to the tile from being displayed immediately (likely the
				// dirtyRect calculation).
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_DrawHouse(pSCView);
				else
					Game_SimcityView_UpdateHouse(pSCView);
				return;
			case CITYTOOL_GROUP_EDUCATION:
			case CITYTOOL_GROUP_SERVICES:
			case CITYTOOL_GROUP_PARKS:
				if (Game_CityToolPlaceSelectedBuilding(tileCoords.x, tileCoords.y, iCurrCityToolGroupWithHotKey, wSelectedSubtool[iCurrCityToolGroupWithHotKey])) {
					L_PlayToolSound_SC2K1996(pSCApp);
					if (iCurrCityToolGroupWithHotKey == CITYTOOL_GROUP_PARKS)
						Game_SimcityApp_MusicPlay(pSCApp, 10010);
				}
				else
					L_PlayToolSound_SC2K1996(pSCApp, SOUND_ERROR);
				Game_SimcityView_DrawHouse(pSCView);
				break;
			case CITYTOOL_GROUP_SIGNS:
				bTextOverlay = XTXTGetTextOverlayID(tileCoords.x, tileCoords.y);
				if (bTextOverlay < MAX_LABEL_TEXT_ENTRY_RANGE)
					Game_CityToolSetSign(tileCoords.x, tileCoords.y);
				Game_SimcityView_DrawHouse(pSCView);
				break;
			case CITYTOOL_GROUP_QUERY:
				bTextOverlay = XTXTGetTextOverlayID(tileCoords.x, tileCoords.y);
				if (bTextOverlay < MIN_SIM_TEXT_ENTRIES || bTextOverlay >= MIN_XTHG_TEXT_ENTRIES)
					Game_QueryGeneralItem(tileCoords.x, tileCoords.y);
				else
					Game_QuerySpecificItem(tileCoords.x, tileCoords.y);
				pSCView->dwSCVLeftMouseDownInGameArea = FALSE;
				pSCView->dwSCVLeftMouseButtonDown = FALSE;
				break;
			case CITYTOOL_GROUP_CENTERINGTOOL:
				Game_CityToolTargetHelicopter(tileCoords.x, tileCoords.y);
				Game_GetScreenCoordsFromTileCoords(tileCoords.x, tileCoords.y, &wNewScreenPointX, &wNewScreenPointY);
				L_PlayToolSound_SC2K1996(pSCApp, SOUND_CLICK);
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_CenterOnNewScreenCoordinates(pSCView, iScreenPointX - HALVECOORD(wNewScreenPointX), iScreenPointY - HALVECOORD(wNewScreenPointY));
				else
					Game_SimcityView_CenterOnNewScreenCoordinates(pSCView, iScreenPointX - wNewScreenPointX, iScreenPointY - wNewScreenPointY);
				Game_SimcityView_DrawHouse(pSCView);
				break;
			default:
				break;
			}
		}
		UpdateWindow(pSCView->m_hWnd);
		wSelectedSubtool[CITYTOOL_GROUP_BULLDOZER] = iBulldozerTool;
	}
}

extern "C" void __cdecl Hook_MapToolMenuAction(UINT nFlags, CMFC3XPoint pt) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iCurrMapToolGroupWithHotKey, iCurrMapToolGroupNoHotKey;
	CMFC3XPoint screenCoords;
	__int16 iTileCoords;
	coords_w_t tileCoords;
	WORD wNewScreenPointX, wNewScreenPointY;
	BOOL bUpdate, bNoLoop;

	// pSCView[62] = dwSCVLeftMouseButtonDown
	// *(DWORD *)((char *)pSCView + 322) = SCVIsZoomed (This is referenced as a DWORD internally - structure alignment between it and the prior WORD at (WORD)pSCView[160])

	// pSCView[62] - When this is set to 0, you remain within the do/while loop until you
	//             release the left mouse button.
	//             If it is set to 1 while the left mouse button is pressed (Shift key is
	//             pressed and the iCurrToolGroupA is not 7 or 8 (trees or forest respectively)
	//             it will break out of the loop and then you end up within the WM_MOUSEMOVE
	//             call (if mouse movement is taking place).
	//
	// The change in this case is to only set pSCView[62] to 0 when the iCurrToolGroupA is not
	// 'Center Tool', this will then allow it to pass-through to the WM_MOUSEMOVE call.

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);	// TODO: is this necessary or can we just dereference pCSimcityView?
	Game_SimcityView_KillCursor(pSCView);
	screenCoords.x = 400;
	screenCoords.y = 400;
	iCurrMapToolGroupNoHotKey = wCurrentMapToolGroup;
	iCurrMapToolGroupWithHotKey = iCurrMapToolGroupNoHotKey;
	if ((nFlags & MK_CONTROL) != 0)
		iCurrMapToolGroupWithHotKey = MAPTOOL_GROUP_CENTERINGTOOL;
	if (iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_CENTERINGTOOL)
		pSCView->dwSCVLeftMouseButtonDown = 0;
	do {
		iTileCoords = Game_PointToTile((__int16)pt.x, (__int16)pt.y);
		if (iTileCoords < 0)
			break;
		tileCoords.x = LOBYTE(iTileCoords);
		tileCoords.y = HIBYTE(iTileCoords);
		if (tileCoords.x >= GAME_MAP_SIZE || tileCoords.y < 0)
			break;
		if ((nFlags & MK_SHIFT) != 0 && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_TREES && iCurrMapToolGroupWithHotKey != MAPTOOL_GROUP_FOREST) {
			pSCView->dwSCVLeftMouseButtonDown = 1;
			break;
		}
		if (screenCoords.x != tileCoords.x || screenCoords.y != tileCoords.y) {
			switch (iCurrMapToolGroupWithHotKey) {
			case MAPTOOL_GROUP_BULLDOZER:
				Game_UseBulldozer(tileCoords.x, tileCoords.y);
				Game_SimcityView_UpdateHouse(pSCView);
				break;
			case MAPTOOL_GROUP_RAISETERRAIN:
				Game_MapToolRaiseTerrain(tileCoords.x, tileCoords.y);
				break;
			case MAPTOOL_GROUP_LOWERTERRAIN:
				Game_MapToolLowerTerrain(tileCoords.x, tileCoords.y);
				break;
			case MAPTOOL_GROUP_STRETCHTERRAIN:
				Game_MapToolStretchTerrain(tileCoords.x, tileCoords.y, (__int16)pt.y);
				break;
			case MAPTOOL_GROUP_LEVELTERRAIN:
				Game_MapToolLevelTerrain(tileCoords.x, tileCoords.y);
				break;
			case MAPTOOL_GROUP_WATER:
			case MAPTOOL_GROUP_STREAM:
				if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_WATER) {
					if (!Game_MapToolPlaceWater(tileCoords.x, tileCoords.y) || Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
						break;
				}
				else {
					Game_MapToolPlaceStream(tileCoords.x, tileCoords.y, 100);
					if (Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
						break;
				}
				L_PlayToolSound_SC2K1996(pSCApp);
				break;
			case MAPTOOL_GROUP_TREES:
			case MAPTOOL_GROUP_FOREST:
				if (!Game_Sound_MapToolSoundTrigger(pSCApp->SCASNDLayer))
					L_PlayToolSound_SC2K1996(pSCApp);
				if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_TREES)
					Game_MapToolPlaceTree(tileCoords.x, tileCoords.y);
				else
					Game_MapToolPlaceForest(tileCoords.x, tileCoords.y);
				break;
			case MAPTOOL_GROUP_CENTERINGTOOL:
				Game_GetScreenCoordsFromTileCoords(tileCoords.x, tileCoords.y, &wNewScreenPointX, &wNewScreenPointY);
				L_PlayToolSound_SC2K1996(pSCApp, SOUND_CLICK);
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_CenterOnNewScreenCoordinates(pSCView, iScreenPointX - HALVECOORD(wNewScreenPointX), iScreenPointY - HALVECOORD(wNewScreenPointY));
				else
					Game_SimcityView_CenterOnNewScreenCoordinates(pSCView, iScreenPointX - wNewScreenPointX, iScreenPointY - wNewScreenPointY);
				break;
			default:
				break;
			}
		}
		bUpdate = TRUE;  // Update window (excluding terrain tools)
		bNoLoop = FALSE; // Break out of loop (terrain and centering tools - latter excluded while alt is pressed)
		if ((iCurrMapToolGroupWithHotKey >= MAPTOOL_GROUP_RAISETERRAIN && iCurrMapToolGroupWithHotKey <= MAPTOOL_GROUP_LEVELTERRAIN) ||
			iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_CENTERINGTOOL) {
			if (iCurrMapToolGroupWithHotKey == MAPTOOL_GROUP_CENTERINGTOOL)
				Game_SimcityView_DrawHouse(pSCView);
			else
				bUpdate = FALSE;
			bNoLoop = TRUE;
		}
		else {
			Game_SimcityView_UpdateHouse(pSCView);
			screenCoords.x = tileCoords.x;
			screenCoords.y = tileCoords.y;
		}
		if (bUpdate)
			UpdateWindow(pSCView->m_hWnd);
		if (bNoLoop)
			break;
	} while (Game_GetGameAreaMouseActivity(pSCView, &pt));
	if (iCurrMapToolGroupNoHotKey != iCurrMapToolGroupWithHotKey)
		wCurrentCityToolGroup = iCurrMapToolGroupNoHotKey;
}

void InstallToolBarHooks_SC2K1996(void) {
	// Hooks for CCityToolBar::ToolMenuDisable and CCityToolBar::ToolMenuEnable
	// Both of which are called when a modal CGameDialog is opened.
	// The purpose in this case will be to temporarily alter the parent
	// of the status widget (if it is in floating mode) so interaction is disabled.
	SafeVirtualProtect((LPVOID)0x402937, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402937, Hook_CityToolBar_ToolMenuDisable);
	SafeVirtualProtect((LPVOID)0x401519, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401519, Hook_CityToolBar_ToolMenuEnable);

	// Hook CCityToolBar::OnLButtonDown
	SafeVirtualProtect((LPVOID)0x4026AD, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4026AD, Hook_CityToolBar_OnLButtonDown);

	// Hook CCityToolBar::SetSelection
	SafeVirtualProtect((LPVOID)0x402A1D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402A1D, Hook_CityToolBar_SetSelection);

	// Hook CMapToolBar::OnLButtonDown
	SafeVirtualProtect((LPVOID)0x401406, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401406, Hook_MapToolBar_OnLButtonDown);

	// Hook CMapToolBar::SetSelection
	SafeVirtualProtect((LPVOID)0x402A5E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402A5E, Hook_MapToolBar_SetSelection);

	// Hook for CCityToolBar::DrawRCIIndicator
	SafeVirtualProtect((LPVOID)0x402EAF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402EAF, Hook_CityToolBar_DrawRCIIndicator);

	// Hook for the CityToolMenuAction call.
	SafeVirtualProtect((LPVOID)0x402C25, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402C25, Hook_CityToolMenuAction);

	// Hook for the MapToolMenuAction call.
	SafeVirtualProtect((LPVOID)0x402B44, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402B44, Hook_MapToolMenuAction);
}
