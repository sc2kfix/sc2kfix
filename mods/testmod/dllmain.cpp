// sc2kfix/testmod dllmain.cpp: test mod
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#include <windows.h>
#include "../sc2kfix.h"
#include "../../include/sc2k_1996.h"

sc2kfix_mod_info_t stModInfo = {
    /* .szModName = */ "Test Native Code Mod",
    /* .szModShortName = */ "testmod",
    /* .szModAuthor = */ "sc2kfix Project",
    /* .szModDescription = */ "A test mod to demonstrate sc2kfix native code mod loading, linking against DLL exports from sc2kfix, and manipulating game data from native code."
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

HOOKCB sc2kfix_mod_info_t* HookCb_GetModInfo(void) {
    return &stModInfo;
}