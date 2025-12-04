// sc2kfix modules/scurkfix.cpp: fixes for SCURK
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>

#include <sc2kfix.h>

static DWORD dwDummy;

#define MISCHOOK_SCURK1996_DEBUG_OTHER 1

#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MISCHOOK_SCURK1996_DEBUG
#define MISCHOOK_SCURK1996_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mischook_scurk1996_debug = MISCHOOK_SCURK1996_DEBUG;

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

extern "C" void __cdecl Hook_SCURK1996DebugOut(char const *fmt, ...) {
	va_list args;
	int len;
	char* buf;

	if ((mischook_scurk1996_debug & MISCHOOK_SCURK1996_DEBUG_OTHER) == 0)
		return;

	va_start(args, fmt);
	len = _vscprintf(fmt, args) + 1;
	buf = (char*)malloc(len);
	if (buf) {
		vsprintf_s(buf, len, fmt, args);

		ConsoleLog(LOG_DEBUG, "0x%06X -> g_DebugOut(): %s", _ReturnAddress(), buf);

		free(buf);
	}

	va_end(args);
}

BOOL InjectSCURKFix(void) {
	if (mischook_debug == DEBUG_FLAGS_EVERYTHING)
		mischook_scurk1996_debug = DEBUG_FLAGS_EVERYTHING;

	ConsoleLog(LOG_INFO, "CORE: Injecting SCURK fixes...\n");
	hSCURKAppModule = GetModuleHandle(NULL);
	dwSCURKAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSCURKAppModule)->e_lfanew + (UINT_PTR)hSCURKAppModule))->FileHeader.TimeDateStamp;
	switch (dwSCURKAppTimestamp) {
	case 0xBC7B1F0E:							// Yes, for some reason the timestamp is set to 2070.
		dwSCURKAppVersion = SC2KVERSION_1996;
		ConsoleLog(LOG_DEBUG, "CORE: SCURK version 1996 detected.\n");
		break;
	default:
		ConsoleLog(LOG_ERROR, "CORE: Could not detect SCURK version (got timestamp 0x%08X). Not injecting animation fix.\n", dwSCURKAppTimestamp);
		return TRUE;
	}

	if (dwSCURKAppVersion == SC2KVERSION_1996)
		InstallRegistryPathingHooks_SCURK1996();

	// Tell the rest of the plugin we're in SCURK
	bInSCURK = TRUE;
	// return TRUE; 
	// Hook for palette animation fix
	// Intercept call to 0x480140 at 0x48A683
	VirtualProtect((LPVOID)0x4497F5, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4497F5, Hook_SCURK1996AnimationFix);
	ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix for SCURK.\n");

	VirtualProtect((LPVOID)0x4132EC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4132EC, Hook_SCURK1996DebugOut);
	
	return TRUE;
}