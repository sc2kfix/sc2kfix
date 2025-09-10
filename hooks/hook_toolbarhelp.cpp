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

extern "C" void __stdcall Hook_CityToolBar_SetSelection(DWORD nIndex, DWORD nSubIndex) {
	CCityToolBar *pThis;

	__asm mov[pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	DWORD nSubTool;
	BOOL bCurrentBudgetSetting;
	int iLayer;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (nIndex < CITYTOOL_BUTTON_SIGNS) {
		nSubTool = nSubIndex;
		if (nIndex == CITYTOOL_BUTTON_REWARDS && nSubIndex > REWARDS_ARCOLOGIES_WAITING)
			nSubTool = REWARDS_ARCOLOGIES_WAITING;
		GameMain_String_OperatorSet(&pThis->dwCTBString[nIndex], cityToolGroupStrings[nIndex*nSubTool].m_pchData);
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
			Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_CENTERINGTOOL, 2u);
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
			if ( pSCView->wSCVZoomLevel )
			{
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, 0);
				if ( pSCView->wSCVZoomLevel == 2 && !pSCView->dwSCVIsZoomed )
					Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, 0);
			}
			else
			{
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMOUT, 0x400u);
			}
			break;
		case CITYTOOL_BUTTON_ZOOMIN:
			Game_SimcityView_ScaleIn(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			if ( pSCView->dwSCVIsZoomed )
			{
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, 0x400u);
			}
			else
			{
				Game_MyToolBar_SetButtonStyle(pThis, CITYTOOL_BUTTON_ZOOMIN, 0);
				if ( pSCView->wSCVZoomLevel == 1 )
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
			if ( nIndex == CITYTOOL_BUTTON_DISPLAYUNDERGROUND )
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
	ConsoleLog(LOG_DEBUG, "CityToolBar_SetSelection(%u, %u): wCurrentCityToolGroup(%u), wSelectedSubtool(%u)\n", nIndex, nSubIndex, wCurrentCityToolGroup, wSelectedSubtool[wCurrentCityToolGroup]);
	Game_SimcityApp_UpdateStatus(pSCApp, FALSE);
	Game_SimcityApp_GetToolSound(pSCApp);
	if (nIndex < CITYTOOL_BUTTON_SIGNS) {
		nSubTool = wSelectedSubtool[nIndex];
		if (nIndex == CITYTOOL_BUTTON_WATER && !wSelectedSubtool[CITYTOOL_GROUP_WATER]
			|| nIndex == CITYTOOL_BUTTON_RAIL && nSubTool == RAILS_SUBWAY) {
			if (!DisplayLayer[0]) {
				DisplayLayer[0] = 1;
				Game_CityToolBar_AdjustLayers(pThis, 1);
				Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
			}
		}
		else if (nIndex) {
			if ((nIndex != CITYTOOL_BUTTON_RAIL || nSubTool != RAILS_SUBSTATION && nSubTool != RAILS_SUBTORAIL)
				&& (nIndex != CITYTOOL_BUTTON_WATER || nSubTool != WATER_PUMP)) {
				if (DisplayLayer[0]) {
					DisplayLayer[0] = 0;
					Game_CityToolBar_AdjustLayers(pThis, 1);
					Game_SimcityView_UpdateAreaCompleteColorFill(pSCView);
				}
			}
		}
	}
}

void InstallToolBarHelpHooks_SC2K1996(void) {
	// Hook CCityToolBar::SetSelection
	VirtualProtect((LPVOID)0x402A1D, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402A1D, Hook_CityToolBar_SetSelection);
}
