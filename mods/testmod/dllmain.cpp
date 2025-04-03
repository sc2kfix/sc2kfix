// sc2kfix/testmod dllmain.cpp: test/template mod with inline documentation
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// These two lines are mandatory for sc2kfix mods. The first one ensures that all Windows API
// calls use the ANSI calls instead of the Unicode calls, which aligns with the way that SimCity
// 2000 was compiled. It should be the first preprocessor line of each source file in each mod.
// The second is required in **EXACTLY** one file of each mod and no more, and lets sc2k_1996.h
// define all of its GAMECALLs and GAMEOFFs so the mod can interact with the game engine in the
// same way that the sc2kfix core does.
#undef UNICODE
#define GAMEOFF_IMPL

// Include files go here. The standard order used in the sc2kfix project is to have bracketed C
// header files first, then any bracketed C++ header files, and then quoted header files.
#include <windows.h>
#include "../sc2kfix.h"
#include "../../include/sc2k_1996.h"

// The stModHooks array of structs define each hook that the mod exports. Each hook is defined
// with a string of its full name followed by a signed integer of its priority level. Lower
// priorities are called first, so eg. a mod that exports Hook_OnNewCity_After with a priority
// of 0 will be called after a mod that exports Hook_OnNewCity_After with a priority of -1 and
// before a mod that exports Hook_OnNewCity_After with a priority of 1.
sc2kfix_mod_hook_t stModHooks[] = {
	{ "Hook_SimulationProcessTickDaySwitch_Before", 0 },
	{ "Hook_SimulationProcessTickDaySwitch_After", 0 }
};

// The stModInfo structure tells the sc2kfix mod loader about the mod and how to load it. It
// contains the version info for the mod itself as well as what version of sc2kfix it requires, as
// well as some basic textual descriptions of the mod and info about the stModHooks struct so that
// sc2kfix can load the mod's exported hooks.
sc2kfix_mod_info_t stModInfo = {
	/* .iModInfoVersion = */ 1,
	/* .iModVersionMajor = */ 0,
	/* .iModVersionMinor = */ 1,
	/* .iModVersionPatch = */ 2,
	/* .iMinimumVersionMajor = */ 0,
	/* .iMinimumVersionMinor = */ 10,
	/* .iMinimumVersionPatch = */ 0,
	/* .szModName = */ "Test Native Code Mod",
	/* .szModShortName = */ "testmod",
	/* .szModAuthor = */ "sc2kfix Project",
	/* .szModDescription = */ "A test mod to demonstrate sc2kfix native code mod loading, linking against DLL exports from sc2kfix, and manipulating game data from native code.",
	/* .iHookCount = */ HOOKS_COUNT(stModHooks),
	/* .pstHooks = */ stModHooks
};

// sc2kfix looks for the HookCb_GetModInfo function when loading each native code mod. You should
// never need to do anything in this function other than return a pointer to the stModInfo struct,
// as the entire purpose of this function is to initially validate that the DLL that sc2kfix is
// loading is actually a legitimate sc2kfix mod.
HOOKCB sc2kfix_mod_info_t* HookCb_GetModInfo(void) {
	return &stModInfo;
}

// The DllMain function in each hook should be used sparingly and follow the best practices
// conventions of the Windows API documentation for DLL entry points, as the DllMain function
// is executed while under the global loader lock. See the following link for details:
// https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		ConsoleLog(LOG_INFO, "MODS: testmod says hello!\n");
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		ConsoleLog(LOG_INFO, "MODS: testmod says goodbye!\n");
		break;
	}
	return TRUE;
}

HOOKCB void Hook_SimulationProcessTickDaySwitch_Before(void) {
	if (dwCityDays % 300 == 0)
		ConsoleLog(LOG_NOTICE, ":toot: Happy new year!\n");
}

HOOKCB void Hook_SimulationProcessTickDaySwitch_After(void) {
	ConsoleLog(LOG_NOTICE, "Today was day %d in the glorious history of %s.\n", dwCityDays + 1, *pszCityName);
}