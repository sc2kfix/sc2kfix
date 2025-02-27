// sc2kfix scurkfix.cpp: fixes for SCURK
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;
DWORD dwSCURKAppTimestamp = 0;
DWORD dwSCURKAppVersion = SC2KVERSION_UNKNOWN;
HMODULE hSCURKAppModule = NULL;

extern "C" __declspec(naked) void __cdecl Hook_SCURK1996AnimationFix(void) {
	__asm {
		push 0x81
		push 0
		push 0
		mov eax, [ebx]					// this
		mov eax, [eax + 0x10]
		push eax						// hWnd
		call [RedrawWindow]
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp
		retn
	}
}

BOOL InjectSCURKFix(void) {
	ConsoleLog(LOG_INFO, "Injecting SCURK fixes...\n");
	hSCURKAppModule = GetModuleHandle(NULL);
	dwSCURKAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSCURKAppModule)->e_lfanew + (UINT_PTR)hSCURKAppModule))->FileHeader.TimeDateStamp;
	switch (dwSCURKAppTimestamp) {
	case 0xBC7B1F0E:							// Yes, for some reason the timestamp is set to 2070.
		dwSCURKAppVersion = SC2KVERSION_1996;
		ConsoleLog(LOG_DEBUG, "SCURK version 1996\n");
		break;
	default:
		ConsoleLog(LOG_ERROR, "Could not detect SCURK version. Not injecting animation fix.\n");
		return TRUE;
	}

	// Tell the rest of the plugin we're in SCURK
	bInSCURK = TRUE;
	// return TRUE; 
	// Hook for palette animation fix
	// Intercept call to 0x480140 at 0x48A683
	VirtualProtect((LPVOID)0x4497F5, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4497F5, Hook_SCURK1996AnimationFix);
	ConsoleLog(LOG_INFO, "Patched palette animation fix for SCURK.\n");
	return TRUE;
}