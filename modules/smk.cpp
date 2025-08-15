// sc2kfix modules/smk.cpp: run-time loading/releasing functions for smk.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <time.h>

#include <sc2kfix.h>

SMKOpenPtr SMKOpenProc;

BOOL smk_enabled = FALSE;
BOOL bSkipIntro = FALSE;

static HMODULE hMod_SMK = 0;

void GetSMKFuncs() {
	char szSMKLibPath[MAX_PATH] = { 0 };

	GetModuleFileNameEx(GetCurrentProcess(), NULL, szSMKLibPath, MAX_PATH);
	PathRemoveFileSpecA(szSMKLibPath);
	strcat_s(szSMKLibPath, MAX_PATH, "\\smackw32.dll");

	hMod_SMK = LoadLibraryA(szSMKLibPath);
	if ( ((UINT)hMod_SMK) < ((UINT)HINSTANCE_ERROR) ) {
		ConsoleLog(LOG_ERROR, "SMK:  Failed to load smacker library, related hooks will be disabled.\n");
		return;
	}

	SMKOpenProc = (SMKOpenPtr) GetProcAddress(hMod_SMK, "_SmackOpen");
	if (!SMKOpenProc) {
		ConsoleLog(LOG_ERROR, "SMK:  Failed to load smacker open function. related hooks will be disabled\n");

		FreeLibrary(hMod_SMK);
		hMod_SMK = 0;
		return;
	}

	smk_enabled = TRUE;
	ConsoleLog(LOG_INFO, "SMK:  Loaded smacker functions.\n");
}

void ReleaseSMKFuncs() {
	if (hMod_SMK) {
		ConsoleLog(LOG_INFO, "SMK:  Releasing smacker functions.\n");

		FreeLibrary(hMod_SMK);
		hMod_SMK = 0;
	}
}

extern "C" DWORD __cdecl Hook_MovieCheck(char* sMovStr) {
	if (sMovStr && strncmp(sMovStr, "INTRO", 5) == 0)
		if (!smk_enabled || bSkipIntro || bSettingsAlwaysSkipIntro)
			return 1;

	return Game_Direct_MovieCheck(sMovStr);
}
