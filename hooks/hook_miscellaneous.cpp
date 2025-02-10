// sc2kfix hook_miscellaneous.cpp: miscellaneous hooks to be injected
// (c) 2025 github.com/araxestroy - released under the MIT license

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

#define MISCHOOK_DEBUG 0

#ifdef DEBUGALL
#undef MISCHOOK_DEBUG
#define MISCHOOK_DEBUG 1
#endif

UINT mischook_debug = MISCHOOK_DEBUG;

extern "C" int __stdcall Hook_LoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int cchBufferMax) {
	// This is mostly just a proof of concept showing we can arbitrarily intercept Win32 API calls.
	// We'll use this for something useful later, I bet.
	return LoadStringA(hInstance, uID, lpBuffer, cchBufferMax);
}

void InstallMiscHooks(void) {
	// Install LoadStringA hook
	*(DWORD*)(0x4EFBE8) = (DWORD)Hook_LoadStringA;
}

