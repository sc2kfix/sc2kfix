// sc2kfix utility.cpp: utility functions to save me from reinventing the wheel
// (c) 2025 github.com/araxestroy - released under the MIT license

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

static HMODULE hMod_SMK = 0;

void GetSMKFuncs() {
	char szSMKLibPath[MAX_PATH] = { 0 };

	GetModuleFileNameEx(GetCurrentProcess(), NULL, szSMKLibPath, MAX_PATH);
	PathRemoveFileSpecA(szSMKLibPath);
	strcat_s(szSMKLibPath, MAX_PATH, "\\smackw32.dll");

	hMod_SMK = LoadLibraryA(szSMKLibPath);
	if ( ((UINT)hMod_SMK) < ((UINT)HINSTANCE_ERROR) ) {
		ConsoleLog(LOG_ERROR, "Failed to load smacker library, related hooks will be disabled.\n");
		return;
	}

	SMKOpenProc = (SMKOpenPtr) GetProcAddress(hMod_SMK, "_SmackOpen");
	if (!SMKOpenProc) {
		ConsoleLog(LOG_INFO, "Failed to load smacker open function.\n");

		FreeLibrary(hMod_SMK);
		hMod_SMK = 0;
		return;
	}

	ConsoleLog(LOG_INFO, "Loaded smacker functions.\n");
}

void ReleaseSMKFuncs() {
	if (hMod_SMK) {
		ConsoleLog(LOG_INFO, "Releasing smacker functions.\n");

		FreeLibrary(hMod_SMK);
		hMod_SMK = 0;
	}
}
