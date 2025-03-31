// sc2kfix dllmain.cpp: all the magic happens here
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#define GETPROC(i, name) fpWinMMHookList[i] = GetProcAddress(hRealWinMM, #name);
#define DEFPROC(i, name) extern "C" __declspec(naked) void __stdcall _##name() { __asm { jmp fpWinMMHookList[i*4] }};

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <time.h>

#include <sc2kfix.h>
#include <winmm_exports.h>
#include "resource.h"

#include <kuroko/kuroko.h>
#include <kuroko/util.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma intrinsic(_ReturnAddress)

// Global variables that we need to keep handy
BOOL bGameDead = FALSE;
HMODULE hRealWinMM = NULL;
HMODULE hSC2KAppModule = NULL;
HMODULE hSC2KFixModule = NULL;
HMENU hGameMenu = NULL;
FARPROC fpWinMMHookList[180] = { NULL };
DWORD dwDetectedVersion = SC2KVERSION_UNKNOWN;
DWORD dwSC2KAppTimestamp = 0;
const char* szSC2KFixVersion = SC2KFIX_VERSION;
const char* szSC2KFixReleaseTag = SC2KFIX_RELEASE_TAG;
const char* szSC2KFixBuildInfo = __DATE__ " " __TIME__;
FILE* fdLog = NULL;
BOOL bInSCURK = FALSE;
BOOL bKurokoVMInitialized = FALSE;

std::random_device rdRandomDevice;
std::mt19937 mtMersenneTwister(rdRandomDevice());

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Statics
static DWORD dwDummy;

// TODO: Bring this bit of code up to standard with the rest of the project. It's literally the
// oldest hook in sc2kfix and its the kind of quick-and-dirty thing I'd rather rewrite to be more
// digestible. We can hook anything in the game, we don't need to jam hand-assembled code into
// code segments anymore.
// 
// This code replaces the original stack cleanup and return after the engine
// cycles the animation palette.
// 
// 6881000000      push dword 0x81              ; flags = RDW_INVALIDATE | RDW_ALLCHILDREN
// 6A00            push 0                       ; hrgnUpdate = NULL
// 6A00            push 0                       ; lprcUpdate = NULL
// 8B0D2C704C00    mov ecx, [pCWndRootWindow]
// 8B511C          mov edx, [ecx+0x1C]
// 52              push edx                     ; hWnd
// FF155CFD4E00    call [RedrawWindow]
// 5D              pop ebp                      ; Clean up stack and return
// 5F              pop edi
// 5E              pop esi
// 5B              pop ebx
// C3              retn
BYTE bAnimationPatch1996[30] = {
	0x68, 0x81, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x8B, 0x0D, 0x2C,
	0x70, 0x4C, 0x00, 0x8B, 0x51, 0x1C, 0x52, 0xFF, 0x15, 0x5C, 0xFD, 0x4E,
	0x00, 0x5D, 0x5F, 0x5E, 0x5B, 0xC3
};

// Same as above, but with the offsets adjusted for the 1995 EXE
BYTE bAnimationPatch1995[30] = {
	0x68, 0x81, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x8B, 0x0D, 0x2C,
	0x60, 0x4C, 0x00, 0x8B, 0x51, 0x1C, 0x52, 0xFF, 0x15, 0xE8, 0xEC, 0x4E,
	0x00, 0x5D, 0x5F, 0x5E, 0x5B, 0xC3
};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	int argc = 0;
	LPWSTR* argv = NULL;
	BOOL bSkipLoadSettings = FALSE;
	INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		char szModuleBaseName[200];
		// Save our own module handle
		hSC2KFixModule = hModule;

		// Find the actual WinMM library
		char buf[200];
		GetSystemDirectory(buf, 200);
		strcat_s(buf, "\\winmm.dll");

		// Load the actual WinMM library
		if (!(hRealWinMM = LoadLibrary(buf))) {
			MessageBox(GetActiveWindow(), "Could not load winmm.dll (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
			return FALSE;
		}

		// Retrieve the list of functions we need to hook or pass through to WinMM
		ALLEXPORTS_HOOKED(GETPROC);
		ALLEXPORTS_PASSTHROUGH(GETPROC);

		// Before we do anything, check to see whether we're attaching against a valid binary
		// (Based on the filename). Otherwise breakout.
		GetModuleBaseName(GetCurrentProcess(), NULL, szModuleBaseName, 200);
		if (!(_stricmp(szModuleBaseName, "winscurk.exe") == 0 ||
			_stricmp(szModuleBaseName, "simcity.exe") == 0)) {
			break;
		}

		// Save the SimCity 2000 EXE's module handle
		if (!(hSC2KAppModule = GetModuleHandle(NULL))) {
			MessageBox(GetActiveWindow(), "Could not GetModuleHandle(NULL) (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
			return FALSE;
		}

		// Ensure that the common controls library is loaded
		InitCommonControlsEx(&icc);

		// Get our command line. WARNING: This uses WIDE STRINGS.
		argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		if (argv) {
			for (int i = 0; i < argc; i++) {
				if (!lstrcmpiW(argv[i], L"-console"))
					bConsoleEnabled = TRUE;
				if (!lstrcmpiW(argv[i], L"-defaults"))
					bSkipLoadSettings = TRUE;
				if (!lstrcmpiW(argv[i], L"-skipintro"))
					bSkipIntro = TRUE;
				// TODO - put some debug options here
			}
		}

		// Open a log file. If it fails, we handle that safely elsewhere
		// AF - Relocated so it will record any messages that occur
		//      prior to the console itself being enabled.
		fopen_s(&fdLog, "sc2kfix.log", "w");

		SetGamePath();

		// Load settings
		if (!bSkipLoadSettings)
			LoadSettings();
		else
			ConsoleLog(LOG_INFO, "CORE: -default passed, skipping LoadSettings()\n");

		// Force the console to be enabled if bSettingsAlwaysConsole is set
		if (bSettingsAlwaysConsole)
			bConsoleEnabled = true;

		// Force the console to be enabled if DEBUGALL is defined
#ifdef DEBUGALL
		bConsoleEnabled = true;
#endif

		// Allocate ourselves a console and redirect libc stdio to it
		if (bConsoleEnabled) {
			AllocConsole();
			SetConsoleOutputCP(65001);
			SetConsoleCP(65001);
			SetConsoleTitle("sc2kfix console");
			FILE* fdDummy = NULL;
			freopen_s(&fdDummy, "CONIN$", "r", stdin);
			freopen_s(&fdDummy, "CONOUT$", "w", stdout);
			freopen_s(&fdDummy, "CONOUT$", "w", stderr);

			// Enable VT100 codes
			DWORD dwConsoleOutMode;
			GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwConsoleOutMode);
			dwConsoleOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), dwConsoleOutMode);

			// Set the console window icon
			HWND hConsoleWindow = GetConsoleWindow();
			SendMessage(hConsoleWindow, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
			SendMessage(hConsoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		}

		// Print the version banner
		// Yes, I know, there's no CORE: prefix here. That's intentional. I promise.
		ConsoleLog(LOG_INFO, "sc2kfix version %s started - https://sc2kfix.net\n", szSC2KFixVersion);
#ifdef DEBUGALL
		ConsoleLog(LOG_DEBUG, "CORE: sc2kfix built with DEBUGALL. Strap in.\n");
#endif

		ConsoleLog(LOG_INFO, "CORE: SC2K session started at %lld.\n", time(NULL));

		if (bConsoleEnabled) {
			ConsoleLog(LOG_INFO, "CORE: Spawned console session.\n");
			printf("[INFO ] CORE: ");
			ConsoleCmdShowDebug(NULL, NULL);
		}

		// Install our top-level exception handler
		SetUnhandledExceptionFilter(CrashHandler);

		// If we're attached to SCURK, switch over to the SCURK fix code
		GetModuleBaseName(GetCurrentProcess(), NULL, szModuleBaseName, 200);
		if (!_stricmp(szModuleBaseName, "winscurk.exe")) {
			InjectSCURKFix();
			break;
		}

		// SMK..
		GetSMKFuncs();
		
		// Seed the libc RNG -- we'll need this later
		srand((unsigned int)time(NULL));

		// Determine what version of SC2K this is
		// HACK: there's probably a better way to do this
		dwSC2KAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSC2KAppModule)->e_lfanew + (UINT_PTR)hSC2KAppModule))->FileHeader.TimeDateStamp;
		switch (dwSC2KAppTimestamp) {
		case 0x302FEA8A:
			dwDetectedVersion = SC2KVERSION_1995;
			ConsoleLog(LOG_NOTICE, "CORE: 1995 CD Collection version detected. Most features and gameplay fixes will not be available.\n");
			ConsoleLog(LOG_NOTICE, "CORE: Please consider using the 1996 Special Edition for the fully restored SimCity 2000 experience.\n");
			break;

		case 0x313E706E:
			dwDetectedVersion = SC2KVERSION_1996;
			break;

		default:
			dwDetectedVersion = SC2KVERSION_UNKNOWN;
			char msg[300];
			sprintf_s(msg, 300, "Could not detect SC2K version (got timestamp %08Xd). Your game will probably crash.\r\n\r\n"
				"Please let us know in a GitHub issue what version of the game you're running so we can look into this.", dwSC2KAppTimestamp);
			MessageBox(GetActiveWindow(), msg, "sc2kfix warning", MB_OK | MB_ICONWARNING);
			ConsoleLog(LOG_WARNING, "CORE: SC2K version could not be detected (got timestamp 0x%08X). Game will probably crash.\n", dwSC2KAppTimestamp);
		}


		// Registry check
		int iInstallCheck;

		iInstallCheck = DoRegistryCheckAndInstall();
		if (iInstallCheck == 2)
			ConsoleLog(LOG_INFO, "CORE: Portable entries created by faux-installer.");
		else if (iInstallCheck == 1)
			ConsoleLog(LOG_INFO, "CORE: Registry entries migrated by faux-installer.");

		// Check for updates
		if (bSettingsCheckForUpdates) {
			CreateThread(NULL, 0, UpdaterThread, 0, 0, NULL);
			ConsoleLog(LOG_INFO, "UPD:  Update notifier thread started.\n");
		}

		// Create music thread
		if (bSettingsUseMultithreadedMusic) {
			CreateThread(NULL, 0, MusicThread, 0, 0, &dwMusicThreadID);
			ConsoleLog(LOG_INFO, "MUS:  Music thread started.\n");
		}

		// Palette animation fix
		LPVOID lpAnimationFix;
		PBYTE lpAnimationFixSrc;
		UINT uAnimationFixLength;
		switch (dwDetectedVersion) {
		case SC2KVERSION_1995:
			lpAnimationFix = (LPVOID)0x00456B23;
			lpAnimationFixSrc = bAnimationPatch1995;
			uAnimationFixLength = 30;
			break;

		case SC2KVERSION_1996:
		default:
			lpAnimationFix = (LPVOID)0x004571D3;
			lpAnimationFixSrc = bAnimationPatch1996;
			uAnimationFixLength = 30;
		}
		
		VirtualProtect(lpAnimationFix, uAnimationFixLength, PAGE_EXECUTE_READWRITE, &dwDummy);
		memcpy(lpAnimationFix, lpAnimationFixSrc, uAnimationFixLength);
		ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix.\n");

		// Dialog crash fix - hat tip to Aleksander Krimsky (@alekasm on GitHub)
		LPVOID lpDialogFix1;
		LPVOID lpDialogFix2;
		switch (dwDetectedVersion) {
		case SC2KVERSION_1995:
			lpDialogFix1 = (LPVOID)0x0049EE93;
			lpDialogFix2 = (LPVOID)0x0049EEF2;
			break;

		case SC2KVERSION_1996:
		default:
			lpDialogFix1 = (LPVOID)0x004A04FA;
			lpDialogFix2 = (LPVOID)0x004A0559;
		}

		VirtualProtect(lpDialogFix1, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(LPBYTE)lpDialogFix1 = 0x20;
		VirtualProtect(lpDialogFix2, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(LPBYTE)lpDialogFix2 = 0xEB;
		*(LPBYTE)((UINT_PTR)lpDialogFix2 + 1) = 0xEB;
		ConsoleLog(LOG_INFO, "CORE: Patched dialog crash fix.\n");

		// Remove palette warnings
		LPVOID lpWarningFix1;
		LPVOID lpWarningFix2;
		switch (dwDetectedVersion) {
		case SC2KVERSION_1995:
			lpWarningFix1 = (LPVOID)0x00408749;
			lpWarningFix2 = (LPVOID)0x0040878E;
			break;

		case SC2KVERSION_1996:
		default:
			lpWarningFix1 = (LPVOID)0x00408A79;
			lpWarningFix2 = (LPVOID)0x00408ABE;
		}
		VirtualProtect(lpWarningFix1, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
		VirtualProtect(lpWarningFix2, 18, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(LPBYTE)lpWarningFix1 = 0x90;
		*(LPBYTE)((UINT_PTR)lpWarningFix1 + 1) = 0x90;
		memset((LPVOID)lpWarningFix2, 0x90, 18);   // nop nop nop nop nop
		ConsoleLog(LOG_INFO, "CORE: Patched 8-bit colour warnings.\n");

		// Hooks we only want to inject on the 1996 Special Edition version
		// and the registry hooks that are for the 1995 CD Collection version.
		if (dwDetectedVersion == SC2KVERSION_1996)
			InstallMiscHooks();
		else if (dwDetectedVersion == SC2KVERSION_1995)
			InstallRegistryPathingHooks_SC2K1995();

		// Start the console thread.
		if (bConsoleEnabled) {
			ConsoleLog(LOG_INFO, "CORE: Starting console thread.\n");
			hConsoleThread = CreateThread(NULL, 0, ConsoleThread, 0, 0, &dwConsoleThreadID);
		}
		break;

	// Nothing to do for these two cases
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	// Clean up after ourselves and get ready to exit
	case DLL_PROCESS_DETACH:
		// Shut down the music thread
		PostThreadMessage(dwMusicThreadID, WM_QUIT, NULL, NULL);

		// Shut down the Kuroko thread
		if (bKurokoVMInitialized)
			PostThreadMessage(dwKurokoThreadID, WM_QUIT, NULL, NULL);

		// Send a closing message and close the log file
		ReleaseSMKFuncs();
		ConsoleLog(LOG_INFO, "CORE: Closing down at %lld. Goodnight!\n", time(NULL));
		fflush(fdLog);
		fclose(fdLog);
		break;
	}
	return TRUE;
}

// Called on any top-level unhandled exception
LONG WINAPI CrashHandler(LPEXCEPTION_POINTERS lpExceptions) {
	HANDLE hFaultingProcess, hFaultingThread;
	char szProcessName[64];
	char szModuleName[64];
	STACKFRAME stStackFrame = { 0 };
	CONTEXT stContext = { 0 };
	BOOL bHaveDebugSyms = FALSE;

	// She's dead, Jim.
	bGameDead = TRUE;

	hFaultingProcess = GetCurrentProcess();
	hFaultingThread = GetCurrentThread();
	GetModuleFileName(NULL, szProcessName, sizeof(szProcessName));
	GetModuleBaseName(GetCurrentProcess(), (HMODULE)SymGetModuleBase(hFaultingProcess, (DWORD)lpExceptions->ContextRecord->Eip), szModuleName, sizeof(szModuleName));

	// Attempt to load symbols if we can
	if (SymInitialize(hFaultingProcess, NULL, TRUE))
		bHaveDebugSyms = TRUE;

	// Dump the header
	ConsoleLog(LOG_EMERGENCY, "CORE:\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: !!! TOP-LEVEL EXCEPTION HANDLER CALLED !!!\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: \n");
	ConsoleLog(LOG_EMERGENCY, "CORE: Unhandled exception 0x%08X caught at %i.\n", lpExceptions->ExceptionRecord->ExceptionCode, time(NULL));
	ConsoleLog(LOG_EMERGENCY, "CORE: Faulting Process: %s\n", szProcessName);
	ConsoleLog(LOG_EMERGENCY, "CORE: Faulting Module:  %s\n", szModuleName);
	ConsoleLog(LOG_EMERGENCY, "CORE: Faulting Address: 0x%08X\n", (DWORD)lpExceptions->ContextRecord->Eip);
	if ((DWORD)lpExceptions->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		if ((DWORD)lpExceptions->ContextRecord->Eip == (DWORD)lpExceptions->ExceptionRecord->ExceptionAddress)
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Execution of address 0x%08X\n", (DWORD)lpExceptions->ContextRecord->Eip);
		else if ((DWORD)lpExceptions->ExceptionRecord->ExceptionInformation[0])
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Write of address 0x%08X\n", (DWORD)lpExceptions->ExceptionRecord->ExceptionAddress);
		else
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Read of address 0x%08X\n", (DWORD)lpExceptions->ExceptionRecord->ExceptionAddress);
	}

	ConsoleLog(LOG_EMERGENCY, "CORE: \n");
	ConsoleLog(LOG_EMERGENCY, "CORE: Stack Trace:\n");
	ConsoleLog(LOG_EMERGENCY, "CORE:  - EIP         ESP         EBP         SYM\n");

	// Set up the stack frame and contexts
	stStackFrame.AddrPC.Offset = lpExceptions->ContextRecord->Eip;
	stStackFrame.AddrPC.Mode = AddrModeFlat;
	stStackFrame.AddrStack.Offset = lpExceptions->ContextRecord->Esp;
	stStackFrame.AddrStack.Mode = AddrModeFlat;
	stStackFrame.AddrFrame.Offset = lpExceptions->ContextRecord->Ebp;
	stStackFrame.AddrFrame.Mode = AddrModeFlat;
	stContext.ContextFlags = CONTEXT_FULL;
	RtlCaptureContext(&stContext);

	// Unwind and dump the stack
	while (StackWalk(IMAGE_FILE_MACHINE_I386, hFaultingProcess, hFaultingThread, &stStackFrame, &stContext, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
		std::string strSymbolInfo;
		DWORD dwDisplacement = 0;
		char szCurrentModuleName[64];

		GetModuleBaseName(GetCurrentProcess(), (HMODULE)SymGetModuleBase(hFaultingProcess, stStackFrame.AddrPC.Offset), szCurrentModuleName, sizeof(szCurrentModuleName));
		strSymbolInfo = "<";
		strSymbolInfo += szCurrentModuleName;
		strSymbolInfo += ">:";

		char szBuf[sizeof(IMAGEHLP_SYMBOL) + 255];
		PIMAGEHLP_SYMBOL stSymbol = (PIMAGEHLP_SYMBOL)szBuf;
		stSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) + 255;
		stSymbol->MaxNameLength = 254;
		if (SymGetSymFromAddr(hFaultingProcess, stStackFrame.AddrPC.Offset, &dwDisplacement, stSymbol))
			strSymbolInfo += stSymbol->Name;
		else
			strSymbolInfo += "unknown";
		strSymbolInfo += "()";

		ConsoleLog(LOG_EMERGENCY, "CORE:  - 0x%08X  0x%08X  0x%08X  %s\n", stStackFrame.AddrPC.Offset, stStackFrame.AddrStack.Offset, stStackFrame.AddrFrame.Offset, strSymbolInfo.c_str());
	}

	ConsoleLog(LOG_EMERGENCY, "CORE:\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: End of stack trace. Time to die.\n");
	
	MessageBox(GameGetRootWindowHandle(), "sc2kfix has detected an unhandled top-level exception in SimCity 2000. If you have the console open, check the console for details. Fault information has been logged to the console and to sc2kfix.log.\n\nClicking the OK button will immediately terminate SimCity 2000. Any unsaved process will be lost.", "She's dead, Jim.", MB_OK | MB_ICONSTOP);
	return EXCEPTION_EXECUTE_HANDLER;
}

// Exports for WinMM hook
ALLEXPORTS_PASSTHROUGH(DEFPROC)
