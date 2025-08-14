// sc2kfix modules/scenario_status.cpp: scenario progress/goals dialog
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define SCENSTAT_DEBUG_WHOKNOWS 1

#define SCENSTAT_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SCENSTAT_DEBUG
#define SCENSTAT_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT scenstat_debug = SCENSTAT_DEBUG;

static DWORD dwDummy;

extern const char* szCurrentScenarioDescription;
extern DWORD dwScenarioStartDays;

BOOL CALLBACK ScenarioStatusDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	CMFC3XString* SCApCStringArrLongMonths = (CMFC3XString*)0x4C71F8;
	void(__cdecl * H_CStringFormat)(CMFC3XString*, char const* Ptr, ...) = (void(__cdecl*)(CMFC3XString*, char const* Ptr, ...))0x49EBD3;
	CMFC3XString* (__thiscall * H_CStringCons)(CMFC3XString*) = (CMFC3XString * (__thiscall*)(CMFC3XString*))0x4A2C28;

	DWORD dwCityDueDate = wScenarioTimeLimitMonths * 25 + dwCityDays;
	dwCityDueDate = dwCityDueDate / 25 * 25 + 22;
	int iDueDateDay = dwCityDueDate % 25 + 1;
	int iDueDateMonth = dwCityDueDate / 25 % 12;
	int iDueDateYear = wCityStartYear + dwCityDueDate / 300;

	BOOL bScenarioGoalsMet = FALSE;

	std::string strScenarioGoals, strScenarioCurrent, strScenarioGoalsHeader;
	CMFC3XString cDueDateString;
	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		
		// Scenario goals header
		strScenarioGoalsHeader = "Scenario Goals - ";
		strScenarioGoalsHeader += pszCityName.m_pchData;

		// Compile the scenario goal strings
		if (wScenarioDisasterID) {
			strScenarioGoals += "Survive the disaster afflicting the city!\n\n";
			strScenarioCurrent += "TBD...\n\n";
		}

		if (dwScenarioCitySize) {
			strScenarioGoals += "Attain a total city population of " + std::to_string(dwScenarioCitySize) + ".\n\n";
			strScenarioCurrent += std::to_string(dwCityPopulation) + "\n\n";
		}

		if (dwScenarioResPopulation) {
			strScenarioGoals += "Attain a residential population of " + std::to_string(dwScenarioResPopulation) + ".\n\n";
			strScenarioCurrent += std::to_string(pBudgetArr[0].iCurrentCosts) + "\n\n";
		}

		if (dwScenarioComPopulation) {
			strScenarioGoals += "Attain a commercial population of " + std::to_string(dwScenarioComPopulation) + ".\n\n";
			strScenarioCurrent += std::to_string(pBudgetArr[1].iCurrentCosts) + "\n\n";
		}

		if (dwScenarioIndPopulation) {
			strScenarioGoals += "Attain an industrial population of " + std::to_string(dwScenarioIndPopulation) + ".\n\n";
			strScenarioCurrent += std::to_string(pBudgetArr[2].iCurrentCosts) + "\n\n";
		}

		if (dwScenarioCashGoal) {
			strScenarioGoals += "Have at least $" + std::to_string(dwScenarioCashGoal) + " in the city reserves (minus bonds).\n\n";
			strScenarioCurrent += "$" + std::to_string(dwCityFunds - dwCityBonds) + "\n\n";
		}

		if (dwScenarioLandValueGoal) {
			strScenarioGoals += "Have at least $" + std::to_string(dwScenarioLandValueGoal) + "000 total city land value.\n\n";
			strScenarioCurrent += "$" + std::to_string(dwCityLandValue) + "000\n\n";
		}

		if (wScenarioLEGoal) {
			strScenarioGoals += "Attain an average Life Expectancy of " + std::to_string(wScenarioLEGoal) + " or higher.\n\n";
			strScenarioCurrent += std::to_string(dwCityWorkforceLE) + " years\n\n";
		}

		if (wScenarioEQGoal) {
			strScenarioGoals += "Attain an average Education Quotient of " + std::to_string(wScenarioEQGoal) + " or higher.\n\n";
			strScenarioCurrent += std::to_string(dwCityWorkforceEQ) + " EQ\n\n";
		}

		if (dwScenarioPollutionLimit) {
			strScenarioGoals += "Maintain a pollution rating of " + std::to_string(dwScenarioPollutionLimit) + "% or lower.\n\n";
			strScenarioCurrent += std::to_string(dwCityPollution) + "%\n\n";
		}

		if (dwScenarioCrimeLimit) {
			strScenarioGoals += "Maintain a crime rate of " + std::to_string(dwScenarioCrimeLimit) + "% or lower.\n\n";
			strScenarioCurrent += std::to_string(dwCityCrime) + "%\n\n";
		}

		if (dwScenarioTrafficLimit) {
			strScenarioGoals += "Maintain a traffic rating of " + std::to_string(dwScenarioTrafficLimit) + "% or lower.\n\n";
			strScenarioCurrent += std::to_string(dwCityTrafficUnknown) + "%\n\n";
		}

		// TODO: Building goals
		// TODO: Modding hooks

		H_CStringCons(&cDueDateString);
		H_CStringFormat(&cDueDateString, "%s %d %4d", SCApCStringArrLongMonths[iDueDateMonth].m_pchData, iDueDateDay, iDueDateYear);

		// Load the compiled strings into the dialog
		SetDlgItemText(hwndDlg, IDC_STATIC_SCENGOALSHEADER, strScenarioGoalsHeader.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_SCENGOALS, strScenarioGoals.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_SCENCURRENT, strScenarioCurrent.c_str());
		SetDlgItemText(hwndDlg, IDC_STATIC_SCENDUEDATE, cDueDateString.m_pchData);

		// Send a WM_SETFONT to set the due date font
		SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_SCENDUEDATE), WM_SETFONT, (WPARAM)hFontArialBold16, TRUE);

		// Center the dialog box
		CenterDialogBox(hwndDlg);

		// Cheer if the goals are met
		if (bScenarioGoalsMet)
			Game_SoundPlaySound(&pCSimcityAppThis, SOUND_CHEERS);

		return TRUE;

	// Set the color of the due date
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_SCENDUEDATE)) {
			SetBkMode((HDC)wParam, TRANSPARENT);
			if (!bScenarioGoalsMet)
				SetTextColor((HDC)wParam, RGB(255, 0, 0));
			else
				SetTextColor((HDC)wParam, RGB(0, 192, 0));
			return (BOOL)GetSysColorBrush(COLOR_MENU);
		}
		if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_STATIC_FORFEIT)) {
			SetBkMode((HDC)wParam, TRANSPARENT);
			SetTextColor((HDC)wParam, RGB(0, 0, 0));
			return (BOOL)GetStockObject(HOLLOW_BRUSH);
		}
		break;

	// Close without saving if the dialog is closed via the menu bar close button or Alt+F4
	case WM_CLOSE:
		EndDialog(hwndDlg, wParam);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			break;
		}
		return TRUE;
	}
	return FALSE;
}

void ShowScenarioStatusDialog(void) {
	ToggleFloatingStatusDialog(FALSE);
	
	if (!bInScenario)
		MessageBox(GameGetRootWindowHandle(), "You can't do that, we're not in a scenario!", "whoa nelly", MB_OK | MB_ICONINFORMATION);
	else
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_SCENARIOSTATUS), GameGetRootWindowHandle(), ScenarioStatusDialogProc);

	ToggleFloatingStatusDialog(TRUE);
}