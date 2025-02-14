// sc2kfix hooks/hook_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 github.com/araxestroy - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is where I test a bunch of stuff live to cross reference what I think is going on in the
// game engine based on decompiling things in IDA and following the code paths. As a result,
// there's a lot of experimental stuff in here. Comments will probably be unhelpful. Godspeed.

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

#define MISCHOOK_DEBUG 2

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG 0xFFFFFFFF
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

static DWORD dwDummy;

UINT iMilitaryBaseTries = 0;

extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	// This is mostly just a proof of concept showing we can arbitrarily intercept Win32 API calls.
	// We'll use this for something useful later, I bet.
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

// Fix military bases not growing.
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

// Hook to reset iMilitaryBaseTries if needed
extern "C" void _declspec(naked) Hook_SimulationProposeMilitaryBase(void) {
	if (mischook_debug & 2)
		ConsoleLog(LOG_DEBUG, "MISC: SimulationProposeMilitaryBase called, resetting iMilitaryBaseTries.\n");
	iMilitaryBaseTries = 0;
	__asm {
		push 0x4142C0
		retn
	}
}

// Fix the game giving up after one attempt at placing a military base.
extern "C" void _declspec(naked) Hook_AttemptMultipleMilitaryBases(void) {
	if (iMilitaryBaseTries++ < 10) {
		if (mischook_debug & 2)
			ConsoleLog(LOG_DEBUG, "MISC: Failed military base placement, attempting again.\n");
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

void InstallMiscHooks(void) {
	// Install LoadStringA hook
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;

	VirtualProtect((LPVOID)0x440D4F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJZ((LPVOID)0x440D4F, Hook_FixMilitaryBaseGrowth);

	VirtualProtect((LPVOID)0x4146B5, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJNZ((LPVOID)0x4146B5, Hook_AttemptMultipleMilitaryBases);
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);

	// Music in background
	VirtualProtect((LPVOID)0x40BFDA, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x40BFDA, 0x90, 5);
}

