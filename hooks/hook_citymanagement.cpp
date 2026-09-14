// sc2kfix hooks/hook_citymanagement.cpp: hooks to do with city management: budget
// and ordinances.
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>
#include <stack>

#include <sc2kfix.h>
#include "../resource.h"

// Local destruct function for proper clarity. Since allocation also occurs
// local to the library, a crash would otherwise occur if the remote call
// were used (and it were set to 'delete' pBudgetMainDialog remotely).
static void L_BudgetMainDialog_Dest(CBudgetMainDialog *pBudgetMainDialog) {
	if (pBudgetMainDialog) {
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringToDateExpense);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringYearEndEstimate);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringBudgetNameYearMonth);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBudgetNameYearMonth);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticEndOfYearFunds);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticBudgetSummary);
		GameMain_Static_Dest(&pBudgetMainDialog->dwBDStaticHourGlass);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPropertyTaxTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPropertyTaxYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditOrdinanceTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBondPaymentsTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditEducationTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditFireDepartmentTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditHealthAndWelfareTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPoliceDepartmentTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditBondPaymentsYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditEducationYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditFireDepartmentYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditHealthAndWelfareYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditOrdinanceYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditPoliceDepartmentYEE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditTransitTDE);
		GameMain_Edit_Dest(&pBudgetMainDialog->dwBDEditTransitYEE);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarEducation);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarFireDepartment);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarHealthAndWelfare);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarPoliceDepartment);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarPropertyTax);
		GameMain_ScrollBar_Dest(&pBudgetMainDialog->dwBDScrollBarTransit);
		GameMain_String_Dest(&pBudgetMainDialog->dwBDStringFour);
		for (int i = 8 - 1; i >= 0; --i)
			Game_BitmapButton_Dest(&pBudgetMainDialog->dwBDMBitmapButtonTwo[i]);
		for (int i = 8 - 1; i >= 0; --i)
			Game_BitmapButton_Dest(&pBudgetMainDialog->dwBDMBitmapButtonOne[i]);
		delete pBudgetMainDialog;
		pBudgetMainDialog = 0;
	}
}

extern "C" void __cdecl Hook_SimulationPrepareBudgetDialog(BOOL bNoParent) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CMainFrame *pMainFrm = (CMainFrame *)pSCApp->m_pMainWnd;

	int nCurrentYear;
	char szBudgetStr[255 + 1], szResStr[255 + 1];
	CBudgetMainDialog *pBudgetMainDialog;

	if (!bOptionsAutoBudget && !IsIconic(pMainFrm->m_hWnd)) {
		Game_SimcityApp_SetGameCursor(pSCApp, GAMECURSOR_REWARDS, FALSE);
		// There was originally a "StopSounds" call here, however it's been removed
		// in order to allow for the "click" to be played when the Budget dialogue
		// is executed from the city toolbar.
		switch (Game_RandomWordLFSRMod(4)) {
			case 0:
				Game_SimcityApp_MusicPlay(pSCApp, 10016);
				break;
			case 1:
				Game_SimcityApp_MusicPlay(pSCApp, 10005);
				break;
			case 2:
				Game_SimcityApp_MusicPlay(pSCApp, 10002);
				break;
			case 3:
				Game_SimcityApp_MusicPlay(pSCApp, 10010);
				break;
			default:
				break;
		}
		nCurrentYear = wCityStartYear + dwCityDays / 300;
		sprintf_s(szBudgetStr, "%s\r\n%4d Budget\r\n%s %4d", pszCityName.m_pchData, nCurrentYear, pSCApp->dwSCApCStringLongMonths[(dwCityDays / 25 % 12)].m_pchData, nCurrentYear);
		pBudgetMainDialog = new CBudgetMainDialog();
		if (pBudgetMainDialog)
			pBudgetMainDialog = Game_BudgetMainDialog_Cons(pBudgetMainDialog, (bNoParent) ? NULL : pMainFrm);
		if (pBudgetMainDialog) {
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringBudgetNameYearMonth, szBudgetStr);
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((bYearEndFlag) ? 890 : 891), szResStr, sizeof(szResStr) - 1);
			sprintf_s(szBudgetStr, szResStr, nCurrentYear);
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringToDateExpense, szBudgetStr);
			if (bYearEndFlag)
				++nCurrentYear;
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((bYearEndFlag) ? 892 : 893), szResStr, sizeof(szResStr) - 1);
			sprintf_s(szBudgetStr, szResStr, nCurrentYear);
			GameMain_String_OperatorSet(&pBudgetMainDialog->dwBDStringYearEndEstimate, szBudgetStr);
			bBudgetOpen = true;
			GameMain_GameDialog_DoModal(pBudgetMainDialog);
			bBudgetOpen = false;
			L_BudgetMainDialog_Dest(pBudgetMainDialog);
		}
	}
}

static void OpenOrdinanceDialog(CBudgetOrdinanceDialog *pOrdinanceDialog) {
	bOrdinanceOpen = true;
	Game_GameDialog_DoModal(pOrdinanceDialog);
	bOrdinanceOpen = false;
}

extern "C" void __stdcall Hook_BudgetMainDialog_OpenOrdinanceDialog() {
	CBudgetMainDialog *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	if (pThis->dwDisplayHourGlass == -1) {
		OpenOrdinanceDialog(&ordinanceDlg);
	}
	else {
		KillTimer(pThis->m_hWnd, 32917);
		OpenOrdinanceDialog(&ordinanceDlg);
		SetTimer(pThis->m_hWnd, 32917, 6000, 0);
		pThis->dwDisplayHourGlass = 0;
		Game_BudgetMainDialog_ReleaseObjects(pThis);
	}
	Game_BudgetMainDialog_UpdateInternalInformation(pThis, 0);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
}

extern "C" void __stdcall Hook_SimcityView_DoOrdinance() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CBudgetOrdinanceDialog ordinanceDlg;

	Game_BudgetOrdinanceDialog_Cons(&ordinanceDlg, NULL);
	OpenOrdinanceDialog(&ordinanceDlg);
	Game_BudgetOrdinanceDialog_Dest(&ordinanceDlg);
}

void InstallCityManagementHooks_SC2K1996(void) {
	// Hook for SimulationPrepareBudgetDialog
	SafeVirtualProtect((LPVOID)0x4015E6, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015E6, Hook_SimulationPrepareBudgetDialog);

	// Hook for CBudgetMainDialog::OpenOrdinanceDialog
	SafeVirtualProtect((LPVOID)0x401AFF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401AFF, Hook_BudgetMainDialog_OpenOrdinanceDialog);

	// Hook for CSimcityView::DoOrdinance
	SafeVirtualProtect((LPVOID)0x4013C5, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4013C5, Hook_SimcityView_DoOrdinance);
}
