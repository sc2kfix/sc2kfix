// sc2kfix modules/scurkfix_1996.cpp: fixes for SCURK - Network Edition (1996) version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

#define MISCHOOK_SCURK1996_DEBUG_INTERNAL 1
#define MISCHOOK_SCURK1996_DEBUG_PICKANDPLACE 2
#define MISCHOOK_SCURK1996_DEBUG_PLACEANDCOPY 4

#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURK1996_DEBUG
#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_scurk1996_debug = MISCHOOK_SCURK1996_DEBUG;

void InstallFixes_SCURK1996(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk1996_debug = DEBUG_FLAGS_EVERYTHING;

	InstallRegistryPathingHooks_SCURK1996();
}
