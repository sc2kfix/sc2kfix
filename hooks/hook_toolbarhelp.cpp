// sc2kfix hooks/hook_toolbarhelp.cpp: help-replacement handling for both the city
//                                     and map toolbars.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

#define TOOLBARHELP_DEBUG_OTHER 1

#define TOOLBARHELP_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TOOLBARHELP_DEBUG
#define TOOLBARHELP_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT toolbarhelp_debug = TOOLBARHELP_DEBUG;

static DWORD dwDummy;

extern "C" void __stdcall Hook_CityToolBar_OnLButtonDown(UINT nFlags, CMFC3XPoint pt) {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	int iStoredMenuButtonPos;
	int iHitMenuButton;
	HMENU hSubMenu;
	CMFC3XMenu *pSubMenu;
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
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	dwCityToolBarArcologyDialogCancel = 0;
	iStoredMenuButtonPos = pThis->iMyTBMenuButtonPos;
	if (pThis->m_cyTopBorder < pt.y) {
		iHitMenuButton = Game_CityToolBar_HitTestFromPoint(pThis, pt);
		pThis->iMyTBMenuButtonPos = iHitMenuButton;
		if (iHitMenuButton < 0)
			return;
		// Added - 'Shift + Click' help messages that replaces the now non-functional
		// help file in Windows.
		if (iHitMenuButton != CITYTOOL_BUTTON_HELP && (nFlags & MK_SHIFT)) {
			char temp[64+1];

			sprintf_s(temp, sizeof(temp)-1, "Tool Help (%d)\n", iHitMenuButton);
			if (pSCView)
				L_MessageBoxA(pSCView->m_hWnd, temp, gamePrimaryKey, MB_ICONINFORMATION|MB_TOPMOST);
			return;
		}
		if (!Game_CityToolBar_PressButton(pThis, iHitMenuButton)) {
			pThis->iMyTBMenuButtonPos = iStoredMenuButtonPos;
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_ERROR);
			return;
		}
		Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
		hSubMenu = GetSubMenu(pThis->dwCTBMenuOne.m_hMenu, pThis->iMyTBMenuButtonPos);
		pSubMenu = GameMain_Menu_FromHandle(hSubMenu);
		if (pSubMenu) {
			iCursorMoving = 1;
			nTargetTicks = GetTickCount() + 500;
			pThis->dwMyTBButtonMenu = 1;
			while (TRUE) {
				if (GetTickCount() < nTargetTicks) {
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
					if (GetMenuItemCount(pSubMenu->m_hMenu)) {
						do {
							GameMain_String_Cons(&cStr);
							pBuffer = GameMain_String_GetBuffer(&cStr, 32);
							GetMenuStringA(pSubMenu->m_hMenu, uMenuItem, pBuffer, 32, MF_BYPOSITION);
							GameMain_String_ReleaseBuffer(&cStr, -1);
							if (strcmp(pThis->dwCTBString[iHitMenuButton].m_pchData, cStr.m_pchData) == 0)
								CheckMenuItem(pSubMenu->m_hMenu, uMenuItem, MF_BYPOSITION|MF_CHECKED);
							else
								CheckMenuItem(pSubMenu->m_hMenu, uMenuItem, MF_BYPOSITION);
							GameMain_String_Dest(&cStr);
							++uMenuItem;
							uItemCount = GetMenuItemCount(pSubMenu->m_hMenu);
						} while (uItemCount > uMenuItem);
					}
					iTrackedMenu = GameMain_Menu_TrackPopupMenu(pSubMenu, TPM_RETURNCMD, pt.x + 3, pt.y + 3, pThis, 0);
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
				Game_CityToolBar_SetSelection(pThis, iHitMenuButton, CITYTOOL_BUTTON_NATURE);
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
		// Yes.. it is unfortunately like this.. alignment headaches.
		pSCApp->dwSCADragSuspendSim = 1;
		pThis->dwMyTBToolBarTitleDrag = 1;
		pThis->dwMyTBPointOne.y = pt.x;
		pThis->dwMyTBPointTwo.x = pt.y;
		mainhWnd = SetCapture(pThis->m_hWnd);
		GameMain_Wnd_FromHandle(mainhWnd);
		ClientToScreen(pThis->m_hWnd, &pt);
		Game_CityToolBar_MoveAndBlitToolBar(pThis, pt.x, pt.y);
		pThis->dwMyTBPointTwo.y = pt.x;
		pThis->dwCTBPointThree.x = pt.y;
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
		if (nIndex == CITYTOOL_BUTTON_REWARDS && nSubIndex > REWARDS_ARCOLOGIES_WAITING)
			nSubTool = REWARDS_ARCOLOGIES_WAITING;
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
			pSCApp->iSCAActiveCursor = GAMECURSOR_TREE;
			if (nSubIndex)
				pSCApp->iSCAActiveCursor = GAMECURSOR_POND;
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
				if (pSCView->wSCVZoomLevel == 2 && !pSCView->dwSCVIsZoomed)
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
				if (pSCView->wSCVZoomLevel == 1)
					Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, 0);
			}
			break;
		case CITYTOOL_BUTTON_CENTERINGTOOL:
			pSCApp->iSCAActiveCursor = GAMECURSOR_CENTER;
			wCurrentCityToolGroup = CITYTOOL_GROUP_CENTERINGTOOL;
			break;
		case CITYTOOL_BUTTON_CITYMAP:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, 246);
			break;
		case CITYTOOL_BUTTON_CITYPOPULATION:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, 128);
			break;
		case CITYTOOL_BUTTON_CITYNEIGHBOURS:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, 157);
			break;
		case CITYTOOL_BUTTON_CITYGRAPHS:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, 152);
			break;
		case CITYTOOL_BUTTON_CITYINDUSTRY:
			Game_MainFrame_ToggleNonModalDialog((CMainFrame *)pSCApp->m_pMainWnd, 160);
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
			Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
			break;
		case CITYTOOL_BUTTON_HELP:
			if (pSCView)
				L_MessageBoxA(pSCView->m_hWnd, "Insert help message here", gamePrimaryKey, MB_ICONINFORMATION|MB_TOPMOST);
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_HELP, 0);
			break;
		default:
			return;
	}
	Game_SimcityApp_UpdateStatus(pSCApp, FALSE);
	Game_SimcityApp_GetToolSound(pSCApp);
	if (nIndex < CITYTOOL_BUTTON_SIGNS) {
		nSubTool = wSelectedSubtool[nIndex];
		if (nIndex == CITYTOOL_BUTTON_WATER && !wSelectedSubtool[CITYTOOL_GROUP_WATER] ||
			nIndex == CITYTOOL_BUTTON_RAIL && nSubTool == RAILS_SUBWAY) {
			if (!DisplayLayer[0]) {
				DisplayLayer[0] = 1;
				Game_CityToolBar_AdjustLayers(pThis, 1);
				Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
			}
		}
		else if (nIndex) {
			if ((nIndex != CITYTOOL_BUTTON_RAIL || nSubTool != RAILS_SUBSTATION && nSubTool != RAILS_SUBTORAIL) &&
				(nIndex != CITYTOOL_BUTTON_WATER || nSubTool != WATER_PUMP)) {
				if (DisplayLayer[0]) {
					DisplayLayer[0] = 0;
					Game_CityToolBar_AdjustLayers(pThis, 1);
					Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
				}
			}
		}
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
				if (pSCView->wSCVZoomLevel == 2 && !pSCView->dwSCVIsZoomed)
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
				if (pSCView->wSCVZoomLevel == 1)
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
			if (pSCView)
				L_MessageBoxA(pSCView->m_hWnd, "Insert help message here", gamePrimaryKey, MB_ICONINFORMATION|MB_TOPMOST);
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
			Game_MyToolBar_SetButtonStyle(pThis, MAPTOOL_BUTTON_DONE, 0);
			Game_SimcityApp_NewCity(pSCApp);
			break;
		default:
			break;
	}
	Game_SimcityApp_GetToolSound(pSCApp);
}

void InstallToolBarHelpHooks_SC2K1996(void) {
	// Hook CCityToolBar::OnLButtonDown
	VirtualProtect((LPVOID)0x4026AD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026AD, Hook_CityToolBar_OnLButtonDown);

	// Hook CCityToolBar::SetSelection
	VirtualProtect((LPVOID)0x402A1D, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402A1D, Hook_CityToolBar_SetSelection);

	// Hook CMapToolBar::SetSelection
	VirtualProtect((LPVOID)0x402A5E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402A5E, Hook_MapToolBar_SetSelection);
}
