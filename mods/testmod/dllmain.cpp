// sc2kfix/testmod dllmain.cpp: test mod
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#define GAMEOFF_IMPL

#include <windows.h>
#include "../sc2kfix.h"
#include "../../include/sc2k_1996.h"

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
    /* .szModDescription = */ "A test mod to demonstrate sc2kfix native code mod loading, linking against DLL exports from sc2kfix, and manipulating game data from native code."
};

sc2kfix_mod_hooklist_t stModHooks = {
    1,
    {
        { "Hook_SimulationProcessTickDaySwitch_Before", 0 }
    }
};

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

HOOKCB sc2kfix_mod_info_t* HookCb_GetModInfo(void) {
    return &stModInfo;
}

HOOKCB sc2kfix_mod_hooklist_t* HookCb_GetHookList(void) {
    return &stModHooks;
}