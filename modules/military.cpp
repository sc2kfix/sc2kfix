// sc2kfix modules/military.cpp: hooks and restored simulations for military bases
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>

#pragma intrinsic(_ReturnAddress)

#define MILITARY_DEBUG_PLACEMENT 1

#define MILITARY_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MILITARY_DEBUG
#define MILITARY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT military_debug = MILITARY_DEBUG;

static DWORD dwDummy;

UINT iMilitaryBaseTries = 0;
WORD wMilitaryBaseX = 0, wMilitaryBaseY = 0;

// Fix military bases not growing.
// XXX - This could use a few extra lines as it's currently possible for a few placeable buildings
// to overwrite and effectively erase military zoned tiles, and I don't know what that will do to
// the simulation engine since it keeps meticulous track of things like that.
//
// We also might want to optionally add in a few more buildings to the growth algorithm for Army
// bases, as currently Army bases only ever build 0xE8 Small Hangar and 0xEF Military Parking Lot.
// Maybe add in 0xE3 Warehouse or 0xF1 Top Secret, since those seem to only grow on naval bases?
extern "C" void _declspec(naked) Hook_FixMilitaryBaseGrowth(void) {
	__asm {
		cmp bp, 0xDD
		jb bail
		cmp bp, 0xF9
		ja bail

		push 0x440D55					// Maxim 43:
		retn							// "If it's stupid and it works...

	bail:
		push 0x440E00					// ...it's still stupid and you're *lucky*."
		retn							//    - The Seventy Maxims of Maximally Effective Mercenaries
	}
}

// Hook to reset iMilitaryBaseTries if needed (new/loaded game, gilmartin)
extern "C" void _declspec(naked) Hook_SimulationProposeMilitaryBase(void) {
	if (military_debug & MILITARY_DEBUG_PLACEMENT)
		ConsoleLog(LOG_DEBUG, "MIL:  SimulationProposeMilitaryBase called, resetting iMilitaryBaseTries.\n");
	iMilitaryBaseTries = 0;
	__asm {
		push 0x4142C0
		retn
	}
}

// Fix the game giving up after one attempt at placing a military base.
// 10 tries was enough to get an army base to spawn in the smallest crags of a map with a maxed-
// out mountain slider, so that's what we're going with here.
extern "C" void _declspec(naked) Hook_AttemptMultipleMilitaryBases(void) {
	if (iMilitaryBaseTries++ <= 10) {
		if (military_debug & MILITARY_DEBUG_PLACEMENT)
			ConsoleLog(LOG_DEBUG, "MIL:  Military base placement attempt %i.\n", iMilitaryBaseTries);
		__asm {
			push 0x4142E9
			retn
		}
	} else {
		__asm {
			push 0x4147AF
			retn
		}
	}
}

// Quick detour to pull the top-left corner coordinates of a spawned military base.
extern "C" void _declspec(naked) Hook_41442E(void) {
	__asm {
		mov edx, 0x4B234F				// AfxMessageBox
		call edx

		mov edx, [esp + 0x5C - 0x38]
		mov word ptr [wMilitaryBaseX], dx
		mov edx, [esp + 0x5C - 0x34]
		mov word ptr [wMilitaryBaseY], dx
	}

	if (military_debug & MILITARY_DEBUG_PLACEMENT)
		ConsoleLog(LOG_DEBUG, "MIL:  Military base placed at %i, %i.\n", wMilitaryBaseX, wMilitaryBaseY);

	__asm{
		push 0x414433
		retn
	}
}

void InstallMilitaryHooks(void) {
	// Fix military bases not growing
	VirtualProtect((LPVOID)0x440D4F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJZ((LPVOID)0x440D4F, Hook_FixMilitaryBaseGrowth);

	// Make multiple attempts at building a military base before giving up
	VirtualProtect((LPVOID)0x4142D8, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJNZ((LPVOID)0x4142D8, Hook_AttemptMultipleMilitaryBases);
	VirtualProtect((LPVOID)0x4146B5, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJNZ((LPVOID)0x4146B5, Hook_AttemptMultipleMilitaryBases);
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);

	// Store the coordinates of the military base
	VirtualProtect((LPVOID)0x41442E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x41442E, Hook_41442E);
}