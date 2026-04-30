// sc2kfix modules/registry_config.cpp: registry and pathing hooks and configuration file handling
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define REGISTRY_DEBUG_OTHER 1
#define REGISTRY_DEBUG_REGISTRY 2
#define REGISTRY_DEBUG_PATHING 4
#define REGISTRY_DEBUG_FILEASSOCIATIONS 8

#define REGISTRY_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef REGISTRY_DEBUG
#define REGISTRY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT registry_debug = REGISTRY_DEBUG;

#define REG_KEY_BASE 0x80000040UL

enum redirected_keys_t {
	enSoftwareKey,
	enMaxisKey,
	enSC2KKey,
	enPathsKey,
	enWindowsKey,
	enVersionKey,
	enOptionsKey,
	enLocalizeKey,
	enRegistrationKey,
	enSCURKKey,

	enCountKey
};

const char *gamePrimaryKey = "SimCity 2000";

BOOL CALLBACK InstallDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		// These both come from the game themselves.
		// I don't know if they're used anywhere, but they're there.
		SetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, DEF_SIM_REG_MAYOR_NAME);
		SetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, DEF_SIM_REG_COMPANY_NAME);

		// Center the dialog box
		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_INSTALL_OK:
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_MAYOR, szSettingsMayorName, 63))
				strcpy_s(szSettingsMayorName, 64, DEF_SIM_REG_MAYOR_NAME);
			if (!GetDlgItemText(hwndDlg, IDC_EDIT_COMPANY, szSettingsCompanyName, 63))
				strcpy_s(szSettingsCompanyName, 64, DEF_SIM_REG_COMPANY_NAME);

			// Update the settings JSON object
			jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME] = szSettingsMayorName;
			jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME] = szSettingsCompanyName;

			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

void ResetFileAssociations(void) {
	HKEY hkeyClassSC2, hkeyClassSCN, hkeyClassMIF;

	if (dwSC2KFixMode == SC2KFIX_MODE_SC2KDEMO) {
		ConsoleLog(LOG_INFO, "MISC: ResetFileAssociations() called but Interactive Demo detected; not updating file association entries.\n");
		return;
	}

	ConsoleLog(LOG_INFO, "MISC: File association entries do not exist or ResetFileAssociations() called; updating registry.\n");

	char szBinary[MAX_PATH + 1];

	// Craft the path we're going to insert into the registry
	sprintf_s(szBinary, sizeof(szBinary) - 1, "%s\\SIMCITY.EXE", szGamePath);
	std::string strShellCommand = "\"";
	strShellCommand += szBinary;
	strShellCommand += "\" \"%1\"";
	std::string strShellIconSC2 = "\"";
	strShellIconSC2 += szBinary;
	strShellIconSC2 += "\",1";
	std::string strShellIconSCN = "\"";
	strShellIconSCN += szBinary;
	strShellIconSCN += "\",2";

	// Write the class info for .sc2 files (SimCity2000.Document.City)
	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\.sc2", &hkeyClassSC2);
	RegSetValueExA(hkeyClassSC2, "", NULL, REG_SZ, (const BYTE*)"SimCity2000.Document.City", sizeof("SimCity2000.Document.City"));
	RegCloseKey(hkeyClassSC2);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .sc2 file association written (stage 1).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.City", &hkeyClassSC2);
	RegSetValueExA(hkeyClassSC2, "", NULL, REG_SZ, (const BYTE*)"SimCity 2000 City", sizeof("SimCity 2000 City"));
	RegCloseKey(hkeyClassSC2);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .sc2 file association written (stage 2).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.City\\shell\\open\\command", &hkeyClassSC2);
	RegSetValueExA(hkeyClassSC2, "", NULL, REG_SZ, (const BYTE*)strShellCommand.c_str(), strShellCommand.size() + 1);
	RegCloseKey(hkeyClassSC2);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .sc2 file association written (stage 3).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.City\\DefaultIcon", &hkeyClassSC2);
	RegSetValueExA(hkeyClassSC2, "", NULL, REG_SZ, (const BYTE*)strShellIconSC2.c_str(), strShellIconSC2.size() + 1);
	RegCloseKey(hkeyClassSC2);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .sc2 file association written (stage 4).\n");

	// Write the class info for .scn files (SimCity2000.Document.Scenario)
	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\.scn", &hkeyClassSCN);
	RegSetValueExA(hkeyClassSCN, "", NULL, REG_SZ, (const BYTE*)"SimCity2000.Document.Scenario", sizeof("SimCity2000.Document.Scenario"));
	RegCloseKey(hkeyClassSCN);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .scn file association written (stage 1).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.Scenario", &hkeyClassSCN);
	RegSetValueExA(hkeyClassSCN, "", NULL, REG_SZ, (const BYTE*)"SimCity 2000 Scenario", sizeof("SimCity 2000 Scenario"));
	RegCloseKey(hkeyClassSCN);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .scn file association written (stage 2).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.Scenario\\shell\\open\\command", &hkeyClassSCN);
	RegSetValueExA(hkeyClassSCN, "", NULL, REG_SZ, (const BYTE*)strShellCommand.c_str(), strShellCommand.size() + 1);
	RegCloseKey(hkeyClassSCN);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .scn file association written (stage 3).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.Scenario\\DefaultIcon", &hkeyClassSCN);
	RegSetValueExA(hkeyClassSCN, "", NULL, REG_SZ, (const BYTE*)strShellIconSCN.c_str(), strShellIconSCN.size() + 1);
	RegCloseKey(hkeyClassSCN);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .scn file association written (stage 4).\n");

	// Write the class info for .mif files (SimCity2000.Document.TileSet)
	sprintf_s(szBinary, sizeof(szBinary) - 1, "%s\\WinSCURK.EXE", szGamePath);
	strShellCommand = "\"";
	strShellCommand += szBinary;
	strShellCommand += "\" \"%1\"";

	std::string strShellIconMIF = "\"";
	strShellIconMIF += szBinary;
	strShellIconMIF += "\",1";

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\.mif", &hkeyClassMIF);
	RegSetValueExA(hkeyClassMIF, "", NULL, REG_SZ, (const BYTE*)"SimCity2000.Document.TileSet", sizeof("SimCity2000.Document.TileSet"));
	RegCloseKey(hkeyClassMIF);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .mif file association written (stage 1).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.TileSet", &hkeyClassMIF);
	RegSetValueExA(hkeyClassMIF, "", NULL, REG_SZ, (const BYTE*)"SimCity 2000 Graphics Set", sizeof("SimCity 2000 Graphics Set"));
	RegCloseKey(hkeyClassMIF);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .mif file association written (stage 2).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.TileSet\\shell\\open\\command", &hkeyClassMIF);
	RegSetValueExA(hkeyClassMIF, "", NULL, REG_SZ, (const BYTE*)strShellCommand.c_str(), strShellCommand.size() + 1);
	RegCloseKey(hkeyClassMIF);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .mif file association written (stage 3).\n");

	RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Classes\\SimCity2000.Document.TileSet\\DefaultIcon", &hkeyClassMIF);
	RegSetValueExA(hkeyClassMIF, "", NULL, REG_SZ, (const BYTE*)strShellIconMIF.c_str(), strShellIconMIF.size() + 1);
	RegCloseKey(hkeyClassMIF);
	if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
		ConsoleLog(LOG_DEBUG, "MISC: .mif file association written (stage 4).\n");
}

static BOOL InstallSC2KDefaults(void) {
	// Attempt to fix shell registrations. This should happen even if a previous sc2kfix install
	// simulation has occurred.
	HKEY hkeyClassSC2, hkeyClassMIF;
	extern BOOL bFixFileAssociations;
	if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Classes\\.sc2", &hkeyClassSC2) != ERROR_SUCCESS ||
		RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Classes\\.mif", &hkeyClassMIF) != ERROR_SUCCESS ||
		bFixFileAssociations)
		ResetFileAssociations();
	else {
		RegCloseKey(hkeyClassSC2);
		RegCloseKey(hkeyClassMIF);
		if (registry_debug & REGISTRY_DEBUG_FILEASSOCIATIONS)
			ConsoleLog(LOG_DEBUG, "MISC: Skipping shell class registry due to both primary .sc2 and .mif entries already existing.\n");
	}

	// If we haven't got an "installed" flag set to true in our setting structure (ie. there's no
	// existing settings.json or we didn't just convert an sc2kfix.ini), prompt the user for the
	// installer registration info.
	if (!jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_INSTALLED].ToBool()) {
		DialogBox(hSC2KFixModule, MAKEINTRESOURCE(IDD_INSTALL), NULL, InstallDialogProc);
		jsonSettingsCore[C_SC2KFIX][S_FIX_CORE][I_FIX_CORE_INSTALLED] = true;
		SaveJSONSettings();
		return TRUE;
	}
	return FALSE;
}

int DoCheckAndInstall(void) {
	return InstallSC2KDefaults();
}

static BOOL IsRegKey(HKEY hKey, int rkVal) {
	if (rkVal < enSoftwareKey || rkVal >= enCountKey)
		return FALSE;

	if (hKey == (HKEY)(REG_KEY_BASE + (rkVal)))
		return TRUE;

	return FALSE;
}

static BOOL IsFakeRegKey(unsigned long ulKey) {
	if ((ulKey) >= (REG_KEY_BASE + enSoftwareKey) && (ulKey) < (REG_KEY_BASE + enCountKey))
		return TRUE;

	return FALSE;
}

static int GetRedirectKey(HKEY hKey) {
	for (int i = 0; i < enCountKey; i++)
		if (IsRegKey(hKey, i))
			return i;

	return enCountKey;
}

static BOOL RegLookup(const char *lpSubKey, unsigned long *ulKey) {
	BOOL ret;

	ret = FALSE;
	if (_stricmp(lpSubKey, "Software") == 0) {
		*ulKey = enSoftwareKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, "Maxis") == 0) {
		*ulKey = enMaxisKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, gamePrimaryKey) == 0) {
		*ulKey = enSC2KKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_PATHS) == 0) {
		*ulKey = enPathsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_WIN) == 0) {
		*ulKey = enWindowsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_VER) == 0) {
		*ulKey = enVersionKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_OPTIONS) == 0) {
		*ulKey = enOptionsKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_LOCALIZE) == 0) {
		*ulKey = enLocalizeKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_REG) == 0) {
		*ulKey = enRegistrationKey;
		ret = TRUE;
	}
	else if (_stricmp(lpSubKey, S_SIM_SCURK) == 0) {
		*ulKey = enSCURKKey;
		ret = TRUE;
	}
	return ret;
}

static const char *SectionLookup(HKEY hKey) {
	switch (GetRedirectKey(hKey)) {
		case enWindowsKey:
			return S_SIM_WIN;
		case enVersionKey:
			return S_SIM_VER;
		case enOptionsKey:
			return S_SIM_OPTIONS;
		case enLocalizeKey:
			return S_SIM_LOCALIZE;
		case enSCURKKey:
			return S_SIM_SCURK;
		default:
			break;
	}
	return NULL;
}

BOOL L_IsPathValid(const char *pStr) {
	return (pStr && PathFileExistsA(pStr)) ? TRUE : FALSE;
}

BOOL L_IsDirectoryPathValid(const char *pStr) {
	return (L_IsPathValid(pStr) && PathIsDirectoryA(pStr)) ? TRUE : FALSE;
}

static void GetOutString(const char *sString, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL)
		*lpcbData = strlen(sString) + 1;
	else
		memcpy(lpData, sString, *lpcbData);
}

static void GetOutDWORD(DWORD dwValue, LPBYTE lpData, LPDWORD lpcbData) {
	if (lpData == NULL)
		*lpcbData = sizeof(DWORD);
	else
		memcpy(lpData, &dwValue, sizeof(DWORD));
}

static void GamePathAdjust(const char *szBasePath, const char *target, LPBYTE lpData, LPDWORD lpcbData) {
	char szTarget[MAX_PATH];

	sprintf_s(szTarget, MAX_PATH, "%s\\%s", szBasePath, target);
	GetOutString(szTarget, lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD dwReserved, DWORD dwType, const BYTE *lpData, DWORD cbData) {
	const char *section = SectionLookup(hKey);
	std::string strActualValue = lpValueName;

	if (section) {
		// Awful hack to get around SC2K sometimes SHOUTING VALUE NAMES IN ALL CAPS
		if (IsRegKey(hKey, enOptionsKey)) {
			if (!_stricmp(lpValueName, "MUSIC"))
				strActualValue = I_SIM_OPT_MUSIC;
			if (!_stricmp(lpValueName, "SOUND"))
				strActualValue = I_SIM_OPT_SOUND;
			if (!_stricmp(lpValueName, "AUTOSAVE"))
				strActualValue = I_SIM_OPT_AUTOSAVE;
			if (!_stricmp(lpValueName, "AUTOBUDGET"))
				strActualValue = I_SIM_OPT_AUTOBUDGET;
			if (!_stricmp(lpValueName, "AUTOGOTO"))
				strActualValue = I_SIM_OPT_AUTOGOTO;
			if (!_stricmp(lpValueName, "DISASTERS"))
				strActualValue = I_SIM_OPT_DISASTERS;
			if (!_stricmp(lpValueName, "SPEED"))
				strActualValue = I_SIM_OPT_SPEED;
		}

		if (dwType == REG_DWORD || (dwType == REG_BINARY && cbData == sizeof(DWORD)))
			jsonSettingsCore[C_SIMCITY2000][section][strActualValue] = *(const DWORD*)lpData;

		return ERROR_SUCCESS;
	}

	if (registry_debug & REGISTRY_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegSetValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			dwReserved, dwType, *lpData, cbData);

	return RegSetValueExA(hKey, lpValueName, dwReserved, dwType, lpData, cbData);
}

extern "C" LSTATUS __stdcall Hook_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
	if (IsRegKey(hKey, enPathsKey)) {
		char szTargetPath[MAX_PATH];

		strcpy_s(szTargetPath, MAX_PATH, szGamePath);
		if (_stricmp(lpValueName, I_SIM_PATHS_GOODIES) == 0) {
			if (registry_debug & REGISTRY_DEBUG_REGISTRY)
				ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> Query Adjustment - %s -> %s\n", _ReturnAddress(), lpValueName, DEF_SIM_PATHS_MOVIES);
			GamePathAdjust(szTargetPath, DEF_SIM_PATHS_MOVIES, lpData, lpcbData);
		}
		else if (_stricmp(lpValueName, I_SIM_PATHS_CITIES) == 0 ||
			_stricmp(lpValueName, I_SIM_PATHS_SAVEGAME) == 0) {
			if (L_IsDirectoryPathValid(jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES].ToString().c_str()) && dwDetectedVersion == VERSION_SC2K_1996)
				GetOutString(jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES].ToString().c_str(), lpData, lpcbData);
			else
				GamePathAdjust(szTargetPath, DEF_SIM_PATHS_CITIES, lpData, lpcbData);
		}

		else if (_stricmp(lpValueName, I_SIM_PATHS_DATA) == 0)
			GamePathAdjust(szTargetPath, DEF_SIM_PATHS_DATA, lpData, lpcbData);

		else if (_stricmp(lpValueName, I_SIM_PATHS_GRAPHICS) == 0)
			GamePathAdjust(szTargetPath, DEF_SIM_PATHS_GRAPHICS, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_PATHS_HOME) == 0)
			GetOutString(szTargetPath, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_PATHS_MUSIC) == 0)
			GamePathAdjust(szTargetPath, DEF_SIM_PATHS_MUSIC, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_PATHS_SCENARIOS) == 0)
			GamePathAdjust(szTargetPath, DEF_SIM_PATHS_SCENARIOS, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_PATHS_TILESETS) == 0) {
			if (L_IsDirectoryPathValid(jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS].ToString().c_str()) && dwDetectedVersion == VERSION_SC2K_1996)
				GetOutString(jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS].ToString().c_str(), lpData, lpcbData);
			else
				GamePathAdjust(szTargetPath, DEF_SIM_PATHS_TILESETS, lpData, lpcbData);
		}
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enWindowsKey)) {
		if (_stricmp(lpValueName, I_SIM_WIN_DISPLAY) == 0)
			GetOutString(DEF_SIM_WIN_DISPLAY, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_WIN_COLCHECK) == 0)
			GetOutDWORD(DEF_SIM_WIN_COLCHECK, lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_WIN_LASTCOLDEPTH) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_WIN][I_SIM_WIN_LASTCOLDEPTH].ToInt(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enVersionKey)) {
		if (_stricmp(lpValueName, I_SIM_VER_SC2K) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_VER][I_SIM_VER_SC2K].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_VER_SCURK) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_VER][I_SIM_VER_SCURK].ToInt(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enOptionsKey)) {
		if (_stricmp(lpValueName, I_SIM_OPT_DISASTERS) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_DISASTERS].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_MUSIC) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_MUSIC].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_SOUND) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SOUND].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_AUTOGOTO) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOGOTO].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_AUTOBUDGET) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOBUDGET].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_AUTOSAVE) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_AUTOSAVE].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_OPT_SPEED) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_OPTIONS][I_SIM_OPT_SPEED].ToInt(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enLocalizeKey)) {
		if (_stricmp(lpValueName, I_SIM_LOC_LANG) == 0)
			GetOutString(jsonSettingsCore[C_SIMCITY2000][S_SIM_LOCALIZE][I_SIM_LOC_LANG].ToString().c_str(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enRegistrationKey)) {
		if (_stricmp(lpValueName, I_SIM_REG_MAYORNAME) == 0)
			GetOutString(jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_MAYORNAME].ToString().c_str(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_REG_COMPANYNAME) == 0)
			GetOutString(jsonSettingsCore[C_SIMCITY2000][S_SIM_REG][I_SIM_REG_COMPANYNAME].ToString().c_str(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (IsRegKey(hKey, enSCURKKey)) {
		if (_stricmp(lpValueName, I_SIM_SCRK_CYCLECOLORS) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_CYCLECOLORS].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_GRIDHEIGHT) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDHEIGHT].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_GRIDWIDTH) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_GRIDWIDTH].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_SHOWCLIPREG) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWCLIPREG].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_SHOWDRAWGRID) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SHOWDRAWGRID].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_SNAPTOGRID) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SNAPTOGRID].ToInt(), lpData, lpcbData);
		
		else if (_stricmp(lpValueName, I_SIM_SCRK_SOUND) == 0)
			GetOutDWORD(jsonSettingsCore[C_SIMCITY2000][S_SIM_SCURK][I_SIM_SCRK_SOUND].ToInt(), lpData, lpcbData);
		
		return ERROR_SUCCESS;
	}

	if (registry_debug & REGISTRY_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegQueryValueExA(0x%08x, %s, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", _ReturnAddress(), hKey, lpValueName,
			lpReserved, *lpType, lpData, lpcbData);

	return RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" LSTATUS __stdcall Hook_RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD dwReserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
	const LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition) {
	unsigned long ulKey;

	ulKey = 0;
	if (RegLookup(lpSubKey, &ulKey)) {
		*phkResult = (HKEY)(REG_KEY_BASE + ulKey);
		return ERROR_SUCCESS;
	}

	if (registry_debug & REGISTRY_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCreateKeyExA(0x%08x, %s, ...)\n", _ReturnAddress(), hKey, lpSubKey);

	return RegCreateKeyExA(hKey, lpSubKey, dwReserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}

extern "C" LSTATUS __stdcall Hook_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) {
	unsigned long ulKey;
	
	ulKey = 0;
	if (RegLookup(lpSubKey, &ulKey)) {
		*phkResult = (HKEY)(REG_KEY_BASE + ulKey);
		return ERROR_SUCCESS;
	}

	if (registry_debug & REGISTRY_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegOpenKeyExA(0x%08x, %s, ...)\n", _ReturnAddress(), hKey, lpSubKey);

	return RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

extern "C" LSTATUS __stdcall Hook_RegCloseKey(HKEY hKey) {
	if (IsFakeRegKey((unsigned long)hKey))
		return ERROR_SUCCESS;

	if (registry_debug & REGISTRY_DEBUG_REGISTRY)
		ConsoleLog(LOG_DEBUG, "MISC: 0x%08X -> RegCloseKey(0x%08x)\n", _ReturnAddress(), hKey);

	return RegCloseKey(hKey);
}

void InstallRegistryPathingHooks_SC2K1996(void) {
	// Install RegSetValueExA hook
	*(DWORD*)(0X4EF7F8) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EF800) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EF80C) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EF810) = (DWORD)Hook_RegCloseKey;

	// Install RegOpenKeyExA
	*(DWORD*)(0x4EF818) = (DWORD)Hook_RegOpenKeyExA;
}

void InstallRegistryPathingHooks_SC2K1995(void) {
	// Install RegOpenKeyExA
	*(DWORD*)(0x4EE79C) = (DWORD)Hook_RegOpenKeyExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4EE7A4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4EE7A8) = (DWORD)Hook_RegSetValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4EE7A0) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4EE7AC) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SC2KDemo(void) {
	// Install RegQueryValueExA hook
	*(DWORD*)(0x4D7760) = (DWORD)Hook_RegQueryValueExA;

	// Install RegOpenKeyExA
	*(DWORD*)(0x4D7764) = (DWORD)Hook_RegOpenKeyExA;

	// Install RegSetValueExA hook
	*(DWORD*)(0X4D7768) = (DWORD)Hook_RegSetValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4D776C) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4D7770) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SCURKPrimary(void) {
	// Install RegSetValueExA hook
	*(DWORD*)(0X4B05F0) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4B05E4) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4B05E8) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4B05EC) = (DWORD)Hook_RegCloseKey;
}

void InstallRegistryPathingHooks_SCURK1996(void) {
	// Install RegSetValueExA hook
	*(DWORD*)(0X4B05F4) = (DWORD)Hook_RegSetValueExA;

	// Install RegQueryValueExA hook
	*(DWORD*)(0x4B05E8) = (DWORD)Hook_RegQueryValueExA;

	// Install RegCreateKeyExA hook
	*(DWORD*)(0x4B05EC) = (DWORD)Hook_RegCreateKeyExA;

	// Install RegCloseKey hook
	*(DWORD*)(0x4B05F0) = (DWORD)Hook_RegCloseKey;
}
