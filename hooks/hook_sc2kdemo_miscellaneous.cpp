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

extern "C" CSimcityAppDemo *__stdcall Hook_Demo_SimcityAppConstruct() {
	CSimcityAppDemo *pThis;

	__asm mov[pThis], ecx

	CSimcityAppDemo *(__thiscall *H_Demo_SimcityAppConstruct)(void *) = (CSimcityAppDemo *(__thiscall *)(void *))0x475B4C;

	CSimcityAppDemo *ret;

	ret = H_Demo_SimcityAppConstruct(pThis);
	ret->iSCAProgramStep = 1;

	return ret;
}

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

	// Fix the Maxis Presents logo not being shown
	VirtualProtect((LPVOID)0x402A1D, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402A1D, Hook_Demo_SimcityAppConstruct);
	VirtualProtect((LPVOID)0x4D2984, 13, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4D2984, 0, 13);
	memcpy_s((LPVOID)0x4D2984, 13, "presnts.bmp", 13);

	// Experiment with nullifying the timer during the first load.
	//VirtualProtect((LPVOID)0x47685E, 10, PAGE_EXECUTE_READWRITE, &dwDummy);
	//BYTE bTimePatch[10] = { 0xC7, 0x05, 0x68, 0x6A, 0x4B, 0x00, 0xFF, 0xFF, 0x00, 0x00 };
	//memcpy((LPVOID)0x47685E, bTimePatch, 10);
}
