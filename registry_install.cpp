// sc2kfix registry_install.cpp: faux installer to create missing registry entries
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>

#include <sc2kfix.h>
#include <resource.h>

char szSC2KPath[MAX_PATH];
char szSC2KGoodiesPath[MAX_PATH];
char szMayorName[64];
char szCompanyName[64];

HWND hwndDesktop;
RECT rcTemp, rcDlg, rcDesktop;

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// These both come from the game themselves.
		// I don't know if they're used anywhere, but they're there.
		SetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, "Marvin Maxis");
		SetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, "Q37 Space Modulator Mfg.");

		// Center the dialog box
		hwndDesktop = GetDesktopWindow();
		GetWindowRect(hwndDesktop, &rcDesktop);
		GetWindowRect(hwndDesktop, &rcTemp);
		GetWindowRect(hwndDlg, &rcDlg);
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rcTemp, -rcDesktop.left, -rcDesktop.top);
		OffsetRect(&rcTemp, -rcDlg.right, -rcDlg.bottom);
		SetWindowPos(hwndDlg, HWND_TOP, rcDesktop.left + (rcTemp.right / 2), rcDesktop.top + (rcTemp.bottom / 2), 0, 0, SWP_NOSIZE);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, szMayorName, 63))
				strcpy_s(szMayorName, 64, "Marvin Maxis");
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, szCompanyName, 63))
				strcpy_s(szCompanyName, 64, "Q37 Space Modulator Mfg.");
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL DoRegistryCheckAndInstall(void) {
	HKEY hkeySC2KRegistration;
	LSTATUS lResultRegistration = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Registration", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KRegistration, NULL);
	if (lResultRegistration != ERROR_SUCCESS) {
		MessageBox(NULL, "Couldn't open registry keys for editing", "sc2kfix error", MB_OK | MB_ICONEXCLAMATION);
		ConsoleLog(LOG_ERROR, "Couldn't open registry keys for registry check, error = 0x%08X\n", lResultRegistration);
		return FALSE;
	}

	if (RegQueryValueEx(hkeySC2KRegistration, "Mayor Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND ||
		RegQueryValueEx(hkeySC2KRegistration, "Company Name", NULL, NULL, NULL, NULL) == ERROR_FILE_NOT_FOUND) {

		// Fake an install.
		
		// Prompt the user for the mayor and company names
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_INSTALL), NULL, InstallDialogProc);

		// Write registration strings
		RegSetValueEx(hkeySC2KRegistration, "Mayor Name", NULL, REG_SZ, (BYTE*)szMayorName, strlen(szMayorName) + 1);
		RegSetValueEx(hkeySC2KRegistration, "Company Name", NULL, REG_SZ, (BYTE*)szCompanyName, strlen(szCompanyName) + 1);

		// Generate paths
		char szSC2KExePath[MAX_PATH] = { 0 };
		char szSC2KPaths[9][MAX_PATH];
		GetModuleFileNameEx(GetCurrentProcess(), NULL, szSC2KExePath, MAX_PATH);
		PathRemoveFileSpecA(szSC2KExePath);
		
		for (int i = 0; i < 9; i++)
			strcpy_s(szSC2KPaths[i], MAX_PATH, szSC2KExePath);

		strcat_s(szSC2KPaths[0], MAX_PATH, "\\CITIES");
		strcat_s(szSC2KPaths[1], MAX_PATH, "\\DATA");
		strcat_s(szSC2KPaths[2], MAX_PATH, "\\GOODIES");
		strcat_s(szSC2KPaths[3], MAX_PATH, "\\BITMAPS");
		strcat_s(szSC2KPaths[4], MAX_PATH, "");
		strcat_s(szSC2KPaths[5], MAX_PATH, "\\SOUNDS");
		strcat_s(szSC2KPaths[6], MAX_PATH, "\\CITIES");
		strcat_s(szSC2KPaths[7], MAX_PATH, "\\SCENARIO");
		strcat_s(szSC2KPaths[8], MAX_PATH, "\\SCURKART");

		// Write paths
		HKEY hkeySC2KPaths;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Paths", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KPaths, NULL);
		RegSetValueEx(hkeySC2KPaths, "Cities", NULL, REG_SZ, (BYTE*)szSC2KPaths[0], strlen(szSC2KPaths[0]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Data", NULL, REG_SZ, (BYTE*)szSC2KPaths[1], strlen(szSC2KPaths[1]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Goodies", NULL, REG_SZ, (BYTE*)szSC2KPaths[2], strlen(szSC2KPaths[2]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Graphics", NULL, REG_SZ, (BYTE*)szSC2KPaths[3], strlen(szSC2KPaths[3]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Home", NULL, REG_SZ, (BYTE*)szSC2KPaths[4], strlen(szSC2KPaths[4]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Music", NULL, REG_SZ, (BYTE*)szSC2KPaths[5], strlen(szSC2KPaths[5]) + 1);
		RegSetValueEx(hkeySC2KPaths, "SaveGame", NULL, REG_SZ, (BYTE*)szSC2KPaths[6], strlen(szSC2KPaths[6]) + 1);
		RegSetValueEx(hkeySC2KPaths, "Scenarios", NULL, REG_SZ, (BYTE*)szSC2KPaths[7], strlen(szSC2KPaths[7]) + 1);
		RegSetValueEx(hkeySC2KPaths, "TileSets", NULL, REG_SZ, (BYTE*)szSC2KPaths[8], strlen(szSC2KPaths[8]) + 1);

		// Write version info
		HKEY hkeySC2KVersion;
		DWORD dwSC2KVersion = 0x00000100;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Version", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KVersion, NULL);
		RegSetValueEx(hkeySC2KVersion, "SCURK", NULL, REG_DWORD, ((BYTE*)&dwSC2KVersion), sizeof(DWORD));
		RegSetValueEx(hkeySC2KVersion, "SimCity 2000", NULL, REG_DWORD, ((BYTE*)&dwSC2KVersion), sizeof(DWORD));

		// Write language info
		HKEY hkeySC2KLocalize;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Localize", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KLocalize, NULL);
		RegSetValueEx(hkeySC2KLocalize, "Language", NULL, REG_SZ, ((BYTE*)"USA"), sizeof("USA"));

		// Write default settings
		HKEY hkeySC2KOptions;
		DWORD dwTrue = 0x00000001;
		DWORD dwFalse = 0x00000000;
		RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Maxis\\SimCity 2000\\Options", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeySC2KOptions, NULL);
		RegSetValueEx(hkeySC2KOptions, "Disasters", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Music", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Sound", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoGoto", NULL, REG_DWORD, ((BYTE*)&dwTrue), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoBudget", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "AutoSave", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));
		RegSetValueEx(hkeySC2KOptions, "Speed", NULL, REG_DWORD, ((BYTE*)&dwFalse), sizeof(DWORD));

		// Signal that we had to fake an install.
		return TRUE;
	}
	return FALSE;
}
