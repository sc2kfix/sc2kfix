// sc2kfix hook/hook_scenario.cpp: hooks for Scenario functionality
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <io.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SCENARIO_DEBUG_OTHER 1
#define SCENARIO_DEBUG_LISTINIT 2

#define SCENARIO_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SCENARIO_DEBUG
#define SCENARIO_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT scenario_debug = SCENARIO_DEBUG;

extern "C" void __stdcall Hook_ScenarioDialog_OnInitDialog() {
	CScenarioDialog *pThis;

	__asm mov[pThis], ecx

	CMFC3XString strFilePath;
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	char szPathBuf[MAX_PATH + 1], szFileBuf[MAX_PATH + 1], *pExt;
	int nScenCnt;
	long lSrch;
	_finddata_t fdat;
	CGraphics *pGraphic;
	CMFC3XWnd *pWnd;

	pThis->pGraphPict = 0;
	GameMain_Dialog_OnInitDialog(pThis);

	pThis->nIdx = -1;
	pWnd = GameMain_Wnd_FromHandle(GetParent(pThis->m_hWnd));
	Game_GameDialog_RepositionSubDialog(pThis, pWnd);

	Game_SimcityApp_GetValueStringA(pSCApp, &strFilePath, aPaths, aScenarios);
	if (!strFilePath.m_nDataLength)
		GameMain_String_OperatorConcat(&strFilePath, aScenarioDir);

	strcpy_s(szPathBuf, strFilePath.m_pchData);
	strcat_s(szPathBuf, "*.scn");

	nScenCnt = 0;

	lSrch = _findfirst(szPathBuf, &fdat);
	if (lSrch != -1L) {
		do {
			if (fdat.name[0] != '.') {
				pExt = strchr(fdat.name, '.');
				if (pExt) {
					if (_stricmp(pExt + 1, "SCN") == 0) {
						++nScenCnt;
						strcpy_s(szFileBuf, strFilePath.m_pchData);
						strcat_s(szFileBuf, fdat.name);
						if (scenario_debug & SCENARIO_DEBUG_LISTINIT)
							ConsoleLog(LOG_DEBUG, "ScenarioDialog::OnInitDialog(): (%d) [%s]\n", nScenCnt, szFileBuf);
						SendMessageA(pThis->listBox.m_hWnd, LB_ADDSTRING, 0, (LPARAM)szFileBuf);
					}
				}
			}
		} while (_findnext(lSrch, &fdat) != -1);
		_findclose(lSrch);
	}

	GameMain_String_Dest(&strFilePath);

	Game_SimcityApp_SetGameCursor(pSCApp, 0, 0);

	if (!nScenCnt) {
		GameMain_AfxMessageBoxID(75, 0, 0xFFFFFFFF);
		GameMain_Dialog_OnCancel(pThis);
		return;
	}

	pGraphic = new CGraphics();
	if (pGraphic)
		pGraphic = Game_Graphics_Cons(pGraphic);

	Game_SimcityApp_GetValueStringA(pSCApp, &strFilePath, aPaths, aGraphics);
	if (!strFilePath.m_nDataLength)
		GameMain_String_OperatorConcat(&strFilePath, aGraphicsDir);

	sprintf_s(szPathBuf, "%s\\pal_mac.bmp", strFilePath.m_pchData);

	Game_Graphics_Load(pGraphic, szPathBuf, 0);
	pThis->hPictPal = Game_Graphics_MakeUnmappedPalette(pGraphic);
	if (pGraphic) {
		Game_Graphics_DeleteStored(pGraphic);
		delete pGraphic;
		pGraphic = 0;
	}
	pThis->pGraphPict = new CGraphics();
	if (pThis->pGraphPict)
		pThis->pGraphPict = Game_Graphics_Cons(pThis->pGraphPict);
	Game_Graphics_DeleteObject(pThis->pGraphPict);
	pThis->pGraphPict->CreateWithPalette_SC2K1996(65, 65);

	GameMain_String_Dest(&strFilePath);
}

extern "C" BOOL __stdcall Hook_ScenarioDialog_SetCursorAndDeleteGraphics() {
	CScenarioDialog *pThis;

	__asm mov[pThis], ecx

	Game_GameDialog_SetCursor(pThis);
	if (pThis->pGraphPict) {
		pThis->pGraphPict->DeleteStored_SC2K1996();
		delete pThis->pGraphPict;
		pThis->pGraphPict = 0;
	}
	return DeleteObject(pThis->hPictPal);
}

static void *L_LoadFileChunkAndInitVar(FILE *f, char *pName, __int16 nMaxSize, void *pBuf) {
	fpos_t pos;
	DWORD dwSize;
	char szEnt[4];
	DWORD dwEntSize, dwOffset, dwTargetEntSize, dwCurrEntSize;

	fgetpos(f, &pos);
	dwSize = (DWORD)pos;
	if (dwSize == -1)
		return 0;
	if (fread(szEnt, 1, sizeof(szEnt), f) == 4) {
		if (memcmp(szEnt, "FORM", 4) == 0) {
			if (fread(&dwEntSize, 1, sizeof(dwEntSize), f) == 4) {
				if (fread(szEnt, 1, sizeof(szEnt), f) == 4) {
					if (memcmp(szEnt, "SCDH", 4) == 0) {
						dwEntSize = _byteswap_ulong(dwEntSize);
						dwOffset = 12;
						while (TRUE) {
							while (TRUE) {
								while (TRUE) {
									if (fread(szEnt, 1, sizeof(szEnt), f) != 4)
										goto BREAKOUT;
									dwOffset += 4;
									if (strncmp(szEnt, pName, 4) == 0)
										break;
									if (fread(&dwSize, 1, sizeof(dwSize), f) != 4)
										goto BREAKOUT;
									dwOffset += 4;
									dwSize = _byteswap_ulong(dwSize);
									dwOffset += dwSize;
									fseek(f, dwOffset, SEEK_SET);
								}
								if (fread(&dwSize, 1, sizeof(dwSize), f) != 4)
									goto BREAKOUT;
								dwOffset += 4;
								dwSize = _byteswap_ulong(dwSize);
								if (fread(&dwTargetEntSize, 1, sizeof(dwTargetEntSize), f) != 4)
									goto BREAKOUT;
								dwOffset += 4;
								dwSize -= 4;
								if (nMaxSize == dwTargetEntSize)
									break;
								fseek(f, dwSize, SEEK_CUR);
							}
							if (memcmp(pName, "TEXT", 4) == 0) {
								if (dwSize > 768)
									dwSize = 768;
								dwCurrEntSize = fread(pBuf, 1, dwSize, f);
								if (dwCurrEntSize != dwSize)
									goto BREAKOUT;
								((BYTE *)pBuf)[dwSize] = 0;
								fseek(f, 0, SEEK_SET);
								return pBuf;
							}
							if (memcmp(pName, "SCEN", 4) == 0) {
								dwCurrEntSize = fread(pBuf, 1, dwSize, f);
								if (dwCurrEntSize != dwSize)
									goto BREAKOUT;
								fseek(f, 0, SEEK_SET);
								return pBuf;
							}
							if (memcmp(pName, "PICT", 4) == 0) {
								if (dwSize > 4300)
									dwSize = 4300;
								dwCurrEntSize = fread(pBuf, 1, dwSize, f);
								if (dwCurrEntSize != dwSize)
									goto BREAKOUT;
								fseek(f, 0, SEEK_SET);
								return pBuf;
							}
						}
					}
				}
			}
		}
	}
BREAKOUT:
	fseek(f, 0, SEEK_SET);
	return NULL;
}

static void L_ByteSwapScenarioAttribute(__int16 *pBuf) {
	__int16 v1;
	__int16 v2;

	v1 = *pBuf << 8;
	v2 = *pBuf >> 8;
	*pBuf = v2;
	*pBuf = (unsigned __int8)v2;
	*pBuf = v1 + (unsigned __int8)v2;
}

static int L_SimcityApp_OpenScenario(CSimcityAppPrimary *pSCApp, char *lpFileName) {
	int ret;
	FILE *f;

	ret = 0;
	GameMain_CmdTarget_BeginWaitCursor(pSCApp);
	bInScenario = L_SimcityApp_DoLoad(pSCApp, lpFileName);
	if (!bInScenario)
		goto FAILOUT;
	f = old_fopen(lpFileName, "rb");
	if (!f) {
		Game_FailRadio(0x2F);
		goto FAILOUT;
	}
	if (!L_LoadFileChunkAndInitVar(f, "SCEN", 128, &scenarioAttrib)) {
		Game_FailRadio(0xEF);
		goto ABORTOUT;
	}
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wDisasterID);
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wTimeLimit);
	scenarioAttrib.dwCitySize = _byteswap_ulong(scenarioAttrib.dwCitySize);
	scenarioAttrib.dwResPop = _byteswap_ulong(scenarioAttrib.dwResPop);
	scenarioAttrib.dwComPop = _byteswap_ulong(scenarioAttrib.dwComPop);
	scenarioAttrib.dwIndPop = _byteswap_ulong(scenarioAttrib.dwIndPop);
	scenarioAttrib.dwCashGoal = _byteswap_ulong(scenarioAttrib.dwCashGoal);
	scenarioAttrib.dwLandValueGoal = _byteswap_ulong(scenarioAttrib.dwLandValueGoal);
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wLEGoal);
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wEQGoal);
	scenarioAttrib.dwPollutionLimit = _byteswap_ulong(scenarioAttrib.dwPollutionLimit);
	scenarioAttrib.dwCrimeLimit = _byteswap_ulong(scenarioAttrib.dwCrimeLimit);
	scenarioAttrib.dwTrafficLimit = _byteswap_ulong(scenarioAttrib.dwTrafficLimit);
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wFirstBuildTileCnt);
	L_ByteSwapScenarioAttribute(&scenarioAttrib.wSecondBuildTileCnt);
	wSetTriggerDisasterType = scenarioAttrib.wDisasterID;
	Game_SetCPoint(&disasterPoint, scenarioAttrib.bDisasterX, scenarioAttrib.bDisasterY);
	ret = 1;
ABORTOUT:
	fclose(f);
FAILOUT:
	GameMain_CmdTarget_EndWaitCursor(pSCApp);
	return ret;
}

extern "C" int __stdcall Hook_SimcityApp_OpenScenario(char *lpFileName) {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	return L_SimcityApp_OpenScenario(pThis, lpFileName);
}

static void L_CacheScenarioDetails(const char *szText) {
	// Save the scenario starting state in order to be used later in the scenario status dialog
	if (szText && strlen(szText) > 0)
		scScenarioDescription = szText;
	dwScenarioStartDays = dwCityDays;
	dwScenarioStartPopulation = dwCityPopulation;
	wScenarioStartXVALTiles = wCityDevelopedTiles;
	dwScenarioStartTrafficDivisor = pBudgetArr[10].iCurrentCosts + pBudgetArr[11].iCurrentCosts + pBudgetArr[12].iCurrentCosts + 1;		// XXX - this should be a descriptive macro
}

void L_ClearScenarioDetails() {
	scScenarioDescription = NULL;
	dwScenarioStartDays = 0;
	dwScenarioStartPopulation = 0;
	wScenarioStartXVALTiles = 0;
	dwScenarioStartTrafficDivisor = 0;
}

extern "C" void __stdcall Hook_SimcityApp_LoadScenario() {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	CSimcityView *pSCView;
	CScenarioDialog scenDlg;
	FILE *f;
	char szText[768];

	Game_ScenarioDialog_Cons(&scenDlg, 0);
	
	memset(szText, 0, sizeof(szText));

	pThis->dwSCAOnQuitSuspendSim = 0;
	pThis->dwSCABackgroundColourCyclingActive = 1;
	Game_GameDialog_DoModal(&scenDlg);
	pThis->dwSCABackgroundColourCyclingActive = 0;
	if (scenDlg.nIdx == -1) {
		if (!pThis->dwSCAOnInitToggleToolBar) {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
		}
		goto SCENFAIL;
	}
	if (Game_SimcityApp_CheckActiveGame(pThis) == IDCANCEL) {
		if (!pThis->dwSCAOnInitToggleToolBar) {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
		}
		goto SCENFAIL;
	}
	Game_StartCleanGame();
	Game_PrepareGame();
	if (!L_SimcityApp_OpenScenario(pThis, scenDlg.szScenFilePath)) {
		if (!pThis->dwSCAOnInitToggleToolBar) {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
		}
		goto SCENFAIL;
	}
	GameMain_Document_UpdateAllViews(pCSimcityDoc, 0, SCD_UPDATE_VIEW_UPDATE, 0);
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pThis);
	if (pSCView)
		Game_SimcityView_UpdateHouse(pSCView);
	Game_ShowViewControls();
	pThis->iSCAProgramStep = ONIDLE_STATE_INGAME;
	pThis->dwSCASetNextStep = TRUE;
	Game_SimcityApp_AdjustMenus(pThis, GAME_MODE_CITY);
	f = old_fopen(scenDlg.szScenFilePath, "rb");
	if (!f) {
		Game_FailRadio(0x2F);
		goto SCENFAIL;
	}
	if (!L_LoadFileChunkAndInitVar(f, "TEXT", 129, szText)) {
		Game_FailRadio(0xEF);
		if (!pThis->dwSCAOnInitToggleToolBar) {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
		}
		fclose(f);
		goto SCENFAIL;
	}
	fclose(f);
	int nLen = strlen(szText) + 1;
	for (int nPos = 0; (nLen - 1) > nPos; ++nPos) {
		char c = szText[nPos];
		if (c == '\r' || c == '\t')
			szText[nPos] = ' ';
	}
	Game_AdjustScenarioTextCharacters(szText, szText);
	L_CacheScenarioDetails(szText);
	Game_DisplayInformationMessageBox(szText, 0, 0);
	Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
	dwMapEditingMode = 0;
	pThis->dwSCAGameStarted = 1;
	pThis->dwSCAMapModeVarCheck = 0;
SCENFAIL:
	Game_ScenarioDialog_Dest(&scenDlg);
}

void L_SimcityApp_LoadScenarioFromCMDLine(CSimcityAppPrimary *pSCApp, const char *lpFileNameFromCMDLine) {
	char szFileName[MAX_PATH + 1], szText[768];
	int nLen;
	FILE *f;
	bool bLoadSuccess;

	memset(szFileName, 0, sizeof(szFileName));
	memset(szText, 0, sizeof(szText));
	bLoadSuccess = false;

	pSCApp->dwSCAOnQuitSuspendSim = 0;
	if (Game_SimcityApp_CheckActiveGame(pSCApp) == IDCANCEL) {
		pSCApp->dwSCACMDLineLoadMode = 0;
		if (!pSCApp->dwSCAOnInitToggleToolBar) {
			pSCApp->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pSCApp->dwSCASetNextStep = TRUE;
			return;
		}
	}
	Game_StartCleanGame();
	Game_PrepareGame();
	strncpy_s(szFileName, lpFileNameFromCMDLine, MAX_PATH);
	nLen = strlen(szFileName);
	szFileName[nLen] = 0;
	if (!L_SimcityApp_OpenScenario(pSCApp, szFileName)) {
		GameMain_AfxMessageBoxID(412, 0, 0xFFFFFFFF);
		pSCApp->dwSCACMDLineLoadMode = 0;
		if (!pSCApp->dwSCAOnInitToggleToolBar) {
			pSCApp->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pSCApp->dwSCASetNextStep = TRUE;
			return;
		}
	}
	GameMain_Document_UpdateAllViews(pCSimcityDoc, 0, SCD_UPDATE_VIEW_UPDATE, 0);
	Game_ShowViewControls();
	pSCApp->iSCAProgramStep = ONIDLE_STATE_INGAME;
	pSCApp->dwSCASetNextStep = TRUE;
	Game_SimcityApp_AdjustMenus(pSCApp, GAME_MODE_CITY);
	f = old_fopen(szFileName, "rb");
	if (!f) {
		Game_FailRadio(0x2F);
		pSCApp->dwSCACMDLineLoadMode = 0;
		return;
	}
	if (L_LoadFileChunkAndInitVar(f, "TEXT", 129, szText)) {
		int nLen = strlen(szText) + 1;
		for (int nPos = 0; (nLen - 1) > nPos; ++nPos) {
			char c = szText[nPos];
			if (c == '\r' || c == '\t')
				szText[nPos] = ' ';
		}
		Game_AdjustScenarioTextCharacters(szText, szText);
		L_CacheScenarioDetails(szText);
		Game_DisplayInformationMessageBox(szText, 0, 0);
		Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
		Game_MainFrame_ToggleToolBars((CMainFrame *)pSCApp->m_pMainWnd, TRUE);
		dwMapEditingMode = 0;
		pSCApp->dwSCAGameStarted = 1;
		pSCApp->dwSCAMapModeVarCheck = 0;
		bLoadSuccess = true;
	}
	fclose(f);
	if (!bLoadSuccess) {
		Game_FailRadio(0xEF);
		pSCApp->dwSCACMDLineLoadMode = 0;
		if (!pSCApp->dwSCAOnInitToggleToolBar) {
			pSCApp->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pSCApp->dwSCASetNextStep = TRUE;
		}
	}
}

void InstallScenarioHooks_SC2K1996(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		scenario_debug = DEBUG_FLAGS_EVERYTHING;

	// Hook for CScenarioDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x4016A4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4016A4, Hook_ScenarioDialog_OnInitDialog);

	// Hook for CScenarioDialog::SetCursorAndDeleteGraphics
	SafeVirtualProtect((LPVOID)0x402806, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402806, Hook_ScenarioDialog_SetCursorAndDeleteGraphics);

	// Hook for CSimcityApp::OpenScenario
	SafeVirtualProtect((LPVOID)0x402806, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402040, Hook_SimcityApp_OpenScenario);

	// Hook for CSimcityApp::LoadScenario
	SafeVirtualProtect((LPVOID)0x4023D8, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4023D8, Hook_SimcityApp_LoadScenario);
}
