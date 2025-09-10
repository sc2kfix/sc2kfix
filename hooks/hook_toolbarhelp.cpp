// sc2kfix hooks/hook_toolbarhelp.cpp: help-replacement handling for both the city
//                                     and map toolbars.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define TOOLBARHELP_DEBUG_OTHER 1

#define TOOLBARHELP_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TOOLBARHELP_DEBUG
#define TOOLBARHELP_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT toolbarhelp_debug = TOOLBARHELP_DEBUG;

static DWORD dwDummy;

void InstallToolBarHelpHooks_SC2K1996(void) {

}
