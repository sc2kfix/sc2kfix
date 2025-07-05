// sc2kfix hooks/hook_sc2kdemo_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Hooks for the Interactive Demo... AAAA!!

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

static DWORD dwDummy;

void InstallMiscHooks_SC2KDemo(void) {
	InstallRegistryPathingHooks_SC2KDemo();

	// Fix the 'Arial" font
	VirtualProtect((LPVOID)0x4CF130, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4CF130, 0, 6);
	memcpy_s((LPVOID)0x4CF130, 6, "Arial", 6);
	VirtualProtect((LPVOID)0x4403A3, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4403A3 = 5;
	VirtualProtect((LPVOID)0x4403AE, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(BYTE*)0x4403AE = 10;
}
