// sc2kfix/testmod dllmain.cpp: test mod
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#include <windows.h>
#include "../sc2kfix.h"

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

