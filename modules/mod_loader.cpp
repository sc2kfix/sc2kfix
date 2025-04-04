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

#define MODLOADER_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MODLOADER_DEBUG
#define MODLOADER_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define REGISTER_HOOK(name) \
	if (!strcmp(stModInfo->stHooks[i].szHookName, # name)) { \
		stHooks_ ## name.push_back(stHookFn); \
		bHookRegistered = TRUE; \
	}
#define SORT_HOOKS(name) std::sort(stHooks_ ## name.begin(), stHooks_ ## name.end());

UINT modloader_debug = MODLOADER_DEBUG;

std::map<HMODULE, sc2kfix_mod_info_t> mapLoadedNativeMods;
std::map<HMODULE, std::vector<sc2kfix_mod_hook_t>> mapLoadedNativeModHooks;

int LoadNativeCodeHooks(HMODULE hModule) {
	int iHooksLoaded = 0;
	std::vector<sc2kfix_mod_hook_t> vecHooksLoaded;

	// Read the hook list and iterate through it
	sc2kfix_mod_info_t* stModInfo = &mapLoadedNativeMods[hModule];
	for (int i = 0; i < stModInfo->iHookCount; i++) {
		BOOL bHookRegistered = FALSE;
		hook_function_t stHookFn;
		stHookFn.iPriority = stModInfo->stHooks[i].iHookPriority;
		stHookFn.iType = HOOKFN_TYPE_NATIVE;
		stHookFn.pFunction = (void*)GetProcAddress(hModule, stModInfo->stHooks[i].szHookName);
		if (!stHookFn.pFunction) {
			ConsoleLog(LOG_WARNING, "MODS: Couldn't load hook %s from native code mod %s.\n", stModInfo->stHooks[i].szHookName, mapLoadedNativeMods[hModule].szModShortName);
			continue;
		}

		// Compare against each hook that we can register and flag if we register one
		REGISTER_HOOK(Hook_OnNewCity_Before);
		REGISTER_HOOK(Hook_GameDoIdleUpkeep_Before);
		REGISTER_HOOK(Hook_GameDoIdleUpkeep_After);
		REGISTER_HOOK(Hook_SimulationProcessTickDaySwitch_Before);
		REGISTER_HOOK(Hook_SimulationProcessTickDaySwitch_After);

		if (!bHookRegistered) {
			ConsoleLog(LOG_WARNING, "MODS: Native code mod %s presented invalid hook %s; skipping.\n", mapLoadedNativeMods[hModule].szModShortName, stModInfo->stHooks[i].szHookName);
			continue;
		}

		vecHooksLoaded.push_back({ stModInfo->stHooks[i].szHookName, stHookFn.iPriority });
		if (modloader_debug & MODLOADER_DEBUG_HOOKS)
			ConsoleLog(LOG_DEBUG, "MODS: Loaded hook %s at address 0x%08X (pri %d) from native code mod %s.\n",
				stModInfo->stHooks[i].szHookName, stHookFn.pFunction, stHookFn.iPriority, mapLoadedNativeMods[hModule].szModShortName);
		iHooksLoaded++;
	}

	mapLoadedNativeModHooks[hModule] = vecHooksLoaded;
	return iHooksLoaded;
}

bool operator<(const hook_function_t& a, const hook_function_t& b) {
	return a.iPriority < b.iPriority;
}

void SortHookLists(void) {
	SORT_HOOKS(Hook_OnNewCity_Before);
	SORT_HOOKS(Hook_GameDoIdleUpkeep_Before);
	SORT_HOOKS(Hook_GameDoIdleUpkeep_After);
	SORT_HOOKS(Hook_SimulationProcessTickDaySwitch_Before);
	SORT_HOOKS(Hook_SimulationProcessTickDaySwitch_After);

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