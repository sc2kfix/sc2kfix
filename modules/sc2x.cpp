// sc2kfix modules/sc2x.cpp: JSON-based extensible save game file format
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <regex>

#include <sc2kfix.h>
#include "../resource.h"

#define SC2X_DEBUG_LOAD 1
#define SC2X_DEBUG_SAVE 2

#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING

#ifdef DEBUGALL
#undef SC2X_DEBUG
#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT sc2x_debug = SC2X_DEBUG;

static DWORD dwDummy;

std::vector<hook_function_t> stHooks_Hook_LoadGame_Before;
std::vector<hook_function_t> stHooks_Hook_LoadGame_After;

extern "C" DWORD __stdcall Hook_LoadGame(int* a2, char* src) {
	DWORD pThis;
	__asm mov [pThis], ecx

	for (const auto& hook : stHooks_Hook_LoadGame_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*, int*, char*) = (void(*)(void*, int*, char*))hook.pFunction;
			fnHook((void*)pThis, a2, src);
		}
	}

	__asm {
		push src
		push a2
		mov ecx, [pThis]
		mov edi, 0x4302E0
		call edi
	}

	for (const auto& hook : stHooks_Hook_LoadGame_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*, int*, char*) = (void(*)(void*, int*, char*))hook.pFunction;
			fnHook((void*)pThis, a2, src);
		}
	}
	
	__asm mov ecx, [pThis]
}

std::vector<hook_function_t> stHooks_Hook_SaveGame_Before;
std::vector<hook_function_t> stHooks_Hook_SaveGame_After;

extern "C" DWORD __stdcall Hook_SaveGame(int* a2) {
	DWORD pThis;
	__asm mov[pThis], ecx

	for (const auto& hook : stHooks_Hook_SaveGame_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*, int*) = (void(*)(void*, int*))hook.pFunction;
			fnHook((void*)pThis, a2);
		}
	}

	__asm {
		push a2
		mov ecx, [pThis]
		mov edi, 0x432180
		call edi
	}

	for (const auto& hook : stHooks_Hook_SaveGame_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE) {
			void (*fnHook)(void*, int*) = (void(*)(void*, int*))hook.pFunction;
			fnHook((void*)pThis, a2);
		}
	}

	__asm mov ecx, [pThis]
}

void InstallSaveHooks(void) {
	// Load game hook
	VirtualProtect((LPVOID)0x4025A4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4025A4, Hook_LoadGame);

	// Save game hook
	VirtualProtect((LPVOID)0x401870, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401870, Hook_SaveGame);
}
