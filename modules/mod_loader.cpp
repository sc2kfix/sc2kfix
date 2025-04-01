// sc2kfix modules/mod_loader.cpp: native code mod loader
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>

#include <sc2kfix.h>
#include "../resource.h"

#define MODLOADER_DEBUG_MODULES 1
#define MODLOADER_DEBUG_HOOKS 2

#define MODLOADER_DEBUG DEBUG_FLAGS_EVERYTHING

#ifdef DEBUGALL
#undef MODLOADER_DEBUG
#define MODLOADER_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT modloader_debug = MODLOADER_DEBUG;

std::map<HMODULE, sc2kfix_mod_info_t> mapLoadedNativeMods;
std::map<HMODULE, std::vector<sc2kfix_mod_hook_t>> mapLoadedNativeModHooks;

int LoadNativeCodeHooks(HMODULE hModule) {
	int iHooksLoaded = 0;
	std::vector<sc2kfix_mod_hook_t> vecHooksLoaded;

	// Get the mod's HookCb_GetHookList function or return -1 on failure
	sc2kfix_mod_hooklist_t* (*HookCb_GetHookList)(void) = (sc2kfix_mod_hooklist_t* (*)(void))GetProcAddress(hModule, "HookCb_GetHookList");
	if (!HookCb_GetHookList)
		return -1;

	// Read the hook list and iterate through it
	sc2kfix_mod_hooklist_t* stHookList = HookCb_GetHookList();
	for (int i = 0; i < stHookList->iHookCount; i++) {
		hook_function_t stHookFn;
		stHookFn.iPriority = stHookList->stHooks[i].iHookPriority;
		stHookFn.pFunction = (void*)GetProcAddress(hModule, stHookList->stHooks[i].szHookName);
		if (!stHookFn.pFunction) {
			ConsoleLog(LOG_WARNING, "MODS: Couldn't load hook %s from native code mod %s.\n", stHookList->stHooks[i].szHookName, mapLoadedNativeMods[hModule].szModShortName);
			continue;
		}
		if (!strcmp(stHookList->stHooks[i].szHookName, "Hook_SimulationProcessTickDaySwitch_Before"))
			stHooks_Hook_SimulationProcessTickDaySwitch_Before.push_back(stHookFn);

		vecHooksLoaded.push_back({ stHookList->stHooks[i].szHookName, stHookFn.iPriority });
		if (modloader_debug & MODLOADER_DEBUG_HOOKS)
			ConsoleLog(LOG_DEBUG, "MODS: Loaded hook %s at address 0x%08X (pri %d) from native code mod %s.\n",
				stHookList->stHooks[i].szHookName, stHookFn.pFunction, stHookFn.iPriority, mapLoadedNativeMods[hModule].szModShortName);
		iHooksLoaded++;
	}

	mapLoadedNativeModHooks[hModule] = vecHooksLoaded;
	return iHooksLoaded;
}

bool operator<(const hook_function_t& a, const hook_function_t& b) {
	return a.iPriority < b.iPriority;
}

void SortHookLists(void) {
	std::sort(stHooks_Hook_SimulationProcessTickDaySwitch_Before.begin(), stHooks_Hook_SimulationProcessTickDaySwitch_Before.end());

	if (modloader_debug & MODLOADER_DEBUG_HOOKS)
		ConsoleLog(LOG_DEBUG, "MODS: Sorted all hooks.\n");
}

void LoadNativeCodeMods(void) {
	// Create the mods folder if it doesn't already exist
	DWORD dwModsFolderAttribs = GetFileAttributes(GetModsFolderPath());
	if (dwModsFolderAttribs == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(GetModsFolderPath(), NULL);

	// Iterate through all the .dll files in the mods folder
	char szModFilePath[MAX_PATH];
	WIN32_FIND_DATA ffdModFile;
	sprintf_s(szModFilePath, MAX_PATH, "%s\\*.dll", GetModsFolderPath());
	HANDLE hModFile = FindFirstFile(szModFilePath, &ffdModFile);
	if (hModFile != INVALID_HANDLE_VALUE) {
		do {
			// Attempt to load the DLL
			if (modloader_debug & MODLOADER_DEBUG_MODULES)
				ConsoleLog(LOG_DEBUG, "MODS: Loader found native code mod \"%s\".\n", ffdModFile.cFileName);
			sprintf_s(szModFilePath, MAX_PATH, "%s\\%s", GetModsFolderPath(), ffdModFile.cFileName);
			HMODULE hModLoaded = LoadLibrary(szModFilePath);
			if (!hModLoaded) {
				ConsoleLog(LOG_ERROR, "MODS: Failed to load mod \"%s\", error 0x%08X.\n", ffdModFile.cFileName, GetLastError());
				continue;
			}

			// Load the mod's info and parse it if LoadLibrary was successful
			sc2kfix_mod_info_t* (*HookCb_GetModInfo)(void) = (sc2kfix_mod_info_t* (*)(void))GetProcAddress(hModLoaded, "HookCb_GetModInfo");
			if (!HookCb_GetModInfo) {
				ConsoleLog(LOG_ERROR, "MODS: Failed to load mod \"%s\". DLL does not export HookCb_GetModInfo.\n", ffdModFile.cFileName);
				FreeLibrary(hModLoaded);
			}
			else {
				mapLoadedNativeMods[hModLoaded] = *HookCb_GetModInfo();
				DWORD dwModMinVersion = mapLoadedNativeMods[hModLoaded].iMinimumVersionMajor << 24 |
					mapLoadedNativeMods[hModLoaded].iMinimumVersionMinor << 16 |
					mapLoadedNativeMods[hModLoaded].iMinimumVersionPatch << 8;
				if (dwSC2KFixVersion < dwModMinVersion) {
					ConsoleLog(LOG_ERROR, "MODS: Failed to load mod \"%s\". sc2kfix version is lower than the required version (%s).\n", ffdModFile.cFileName,
						FormatVersion(mapLoadedNativeMods[hModLoaded].iMinimumVersionMajor, mapLoadedNativeMods[hModLoaded].iMinimumVersionMinor, mapLoadedNativeMods[hModLoaded].iMinimumVersionPatch));
					FreeLibrary(hModLoaded);
					mapLoadedNativeMods.erase(hModLoaded);
					continue;
				}

				if (LoadNativeCodeHooks(hModLoaded) < 0) {
					ConsoleLog(LOG_ERROR, "MODS: Failed to load mod \"%s\". DLL does not export HookCb_GetHookList.\n", ffdModFile.cFileName);
					FreeLibrary(hModLoaded);
					mapLoadedNativeMods.erase(hModLoaded);
					continue;
				}
			}

			// Log that we were successful in loading the mod
			ConsoleLog(LOG_INFO, "MODS: Loaded native code mod \"%s\" (%s) version %s.\n", mapLoadedNativeMods[hModLoaded].szModName, mapLoadedNativeMods[hModLoaded].szModShortName,
				FormatVersion(mapLoadedNativeMods[hModLoaded].iModVersionMajor, mapLoadedNativeMods[hModLoaded].iModVersionMinor, mapLoadedNativeMods[hModLoaded].iModVersionPatch));
		} while (FindNextFile(hModFile, &ffdModFile) != NULL);

		// Sort all loaded hooks by priority. Lower priorities get run first.
		SortHookLists();
	}
}