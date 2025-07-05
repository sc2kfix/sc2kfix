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

void InstallMiscHooks_SC2KDemo(void) {
	InstallRegistryPathingHooks_SC2KDemo();
}
