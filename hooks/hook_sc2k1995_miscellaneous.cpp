// sc2kfix hooks/hook_sc2k1995_miscellaneous.cpp: 1995 CD Collection hooks to be injected
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Hooks for the 1995 CD Collection - these are minimal cases to avoid certain
// interface-breaking conditions.

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
