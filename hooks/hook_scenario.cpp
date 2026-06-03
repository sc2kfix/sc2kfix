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

	Game_SimcityApp_GetValueStringA(pSCApp, &strFilePath, "PATHS", "SCENARIOS");
	if (!strFilePath.m_nDataLength)
		GameMain_String_OperatorConcat(&strFilePath, ".\\scenario\\");

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
							ConsoleLog(LOG_DEBUG, "(%d) [%s]\n", nScenCnt, szFileBuf);
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

	Game_SimcityApp_GetValueStringA(pSCApp, &strFilePath, "PATHS", "GRAPHICS");
	if (!strFilePath.m_nDataLength)
		GameMain_String_OperatorConcat(&strFilePath, ".\\graphics");

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

void InstallScenarioHooks_SC2K1996(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		scenario_debug = DEBUG_FLAGS_EVERYTHING;

	// Hook for CScenarioDialog::OnInitDialog
	SafeVirtualProtect((LPVOID)0x4016A4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4016A4, Hook_ScenarioDialog_OnInitDialog);
}
