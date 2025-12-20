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
#include <VersionHelpers.h>

#include <sc2kfix.h>
#include <winmm_exports.h>
#include "resource.h"

#if !NOKUROKO
#include <kuroko/kuroko.h>
#include <kuroko/util.h>
#endif

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma intrinsic(_ReturnAddress)

// Global variables that we need to keep handy
BOOL bGameDead = FALSE;
HMODULE hRealWinMM = NULL;
HMODULE hSC2KAppModule = NULL;
HMODULE hSC2KFixModule = NULL;
HMENU hMainMenu = NULL;
HMENU hGameMenu = NULL;
HMENU hDebugMenu = NULL;
FARPROC fpWinMMHookList[180] = { NULL };
DWORD dwDetectedVersion = VERSION_PROG_UNKNOWN;
DWORD dwSC2KFixMode = SC2KFIX_MODE_UNKNOWN;
DWORD dwDetectedAppTimestamp = 0;
DWORD dwSC2KFixVersion = SC2KFIX_VERSION_MAJOR << 24 | SC2KFIX_VERSION_MINOR << 16 | SC2KFIX_VERSION_PATCH << 8;
const char* szSC2KFixVersion = SC2KFIX_VERSION;
const char* szSC2KFixReleaseTag = SC2KFIX_RELEASE_TAG;
const char* szSC2KFixBuildInfo = __DATE__ " " __TIME__;
FILE* fdLog = NULL;
#if !NOKUROKO
BOOL bKurokoVMInitialized = FALSE;
#endif
BOOL bUseAdvancedQuery = TRUE;
BOOL bSkipLoadingMods = FALSE;
BOOL bFixFileAssociations = FALSE;
int iForcedBits = 0;

std::random_device rdRandomDevice;
std::mt19937 mtMersenneTwister(rdRandomDevice());

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// Statics
static DWORD dwDummy;

static DWORD GetSC2KFixModeFromModule(char *szModuleName) {
	if (_stricmp(szModuleName, "simcity.exe") == 0)
		return SC2KFIX_MODE_SC2K;
	else if (_stricmp(szModuleName, "simdemo.exe") == 0)
		return SC2KFIX_MODE_SC2KDEMO;
	else if (_stricmp(szModuleName, "winscurk.exe") == 0)
		return SC2KFIX_MODE_SCURK;

	return SC2KFIX_MODE_UNKNOWN;
}

static const char *GetLogSuffixFromSC2KFixMode() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SC2K)
		return "";
	else if (dwSC2KFixMode == SC2KFIX_MODE_SC2KDEMO)
		return "-demo";
	else if (dwSC2KFixMode == SC2KFIX_MODE_SCURK)
		return "-scurk";
	
	return "-unknown";
}

static const char *GetProgramNameFromSC2KFixMode() {
	if (dwSC2KFixMode == SC2KFIX_MODE_SC2K)
		return "SC2K";
	else if (dwSC2KFixMode == SC2KFIX_MODE_SC2KDEMO)
		return "SC2K Interactive Demo";
	else if (dwSC2KFixMode == SC2KFIX_MODE_SCURK)
		return "SCURK";
	
	return "Unknown Game";
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	int argc = 0;
	LPWSTR* argv = NULL;
	BOOL bSkipLoadSettings = FALSE;
	INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES };

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		char szModuleBaseName[MAX_PATH + 1], szLogPath[MAX_PATH + 1];
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
		GetModuleBaseName(GetCurrentProcess(), NULL, szModuleBaseName, MAX_PATH);
		dwSC2KFixMode = GetSC2KFixModeFromModule(szModuleBaseName);
		if (dwSC2KFixMode == SC2KFIX_MODE_UNKNOWN)
			break;

		// Save the SimCity 2000 EXE's module handle
		if (!(hSC2KAppModule = GetModuleHandle(NULL))) {
			MessageBox(GetActiveWindow(), "Could not GetModuleHandle(NULL) (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
			return FALSE;
		}

		// Ensure that the common controls library is loaded
		InitCommonControlsEx(&icc);

		// Get our command line. WARNING: This uses WIDE STRINGS.
		int iSetBitMode;
		BOOL bSubArg;
		int iLastArgPos;
		int iArg;

		iSetBitMode = 0;
		bSubArg = FALSE;
		iLastArgPos = -1;
		iArg = 0;

		argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		if (argv) {
			for (int i = 0; i < argc; i++) {
				if (!bSubArg) {
					if (!lstrcmpiW(argv[i], L"-noadvquery"))
						bUseAdvancedQuery = FALSE;
					if (!lstrcmpiW(argv[i], L"-console"))
						bConsoleEnabled = TRUE;
					if (!lstrcmpiW(argv[i], L"-debugall")) {
						guzzardo_debug = DEBUG_FLAGS_EVERYTHING;
						mci_debug = DEBUG_FLAGS_EVERYTHING;
						military_debug = DEBUG_FLAGS_EVERYTHING;
						mischook_debug = DEBUG_FLAGS_EVERYTHING;
						modloader_debug = DEBUG_FLAGS_EVERYTHING;
						mov_debug = DEBUG_FLAGS_EVERYTHING;
						mus_debug = DEBUG_FLAGS_EVERYTHING;
						registry_debug = DEBUG_FLAGS_EVERYTHING;
						sc2x_debug = DEBUG_FLAGS_EVERYTHING;
						snd_debug = DEBUG_FLAGS_EVERYTHING;
						sprite_debug = DEBUG_FLAGS_EVERYTHING;
						timer_debug = DEBUG_FLAGS_EVERYTHING;
						updatenotifier_debug = DEBUG_FLAGS_EVERYTHING;
					}
					if (!lstrcmpiW(argv[i], L"-undebugall")) {
						guzzardo_debug = DEBUG_FLAGS_NONE;
						mci_debug = DEBUG_FLAGS_NONE;
						military_debug = DEBUG_FLAGS_NONE;
						mischook_debug = DEBUG_FLAGS_NONE;
						modloader_debug = DEBUG_FLAGS_NONE;
						mov_debug = DEBUG_FLAGS_NONE;
						mus_debug = DEBUG_FLAGS_NONE;
						registry_debug = DEBUG_FLAGS_NONE;
						sc2x_debug = DEBUG_FLAGS_NONE;
						snd_debug = DEBUG_FLAGS_NONE;
						sprite_debug = DEBUG_FLAGS_NONE;
						timer_debug = DEBUG_FLAGS_NONE;
						updatenotifier_debug = DEBUG_FLAGS_NONE;
					}
					if (!lstrcmpiW(argv[i], L"-resetfileassociations"))
						bFixFileAssociations = TRUE;
					if (!lstrcmpiW(argv[i], L"-defaults"))
						bSkipLoadSettings = TRUE;
					if (!lstrcmpiW(argv[i], L"-skipintro"))
						bSkipIntro = TRUE;
					if (!lstrcmpiW(argv[i], L"-skipmods"))
						bSkipLoadingMods = TRUE;
					if (!lstrcmpiW(argv[i], L"-bitmode"))
					{
						if (!iSetBitMode) {
							iForcedBits = 0;
							iSetBitMode = 2;
							bSubArg = TRUE;
							iLastArgPos = i;
						}
					}
				}
				else {
					if (i == iLastArgPos + 1) {
						if (wcslen(argv[i]) > 0) {
							if (iSetBitMode == 2) {
								iArg = _wtoi(argv[i]);
								if (iArg != 4 && iArg != 8) {
									iSetBitMode = 0;
								}
								else {
									iForcedBits = iArg;
									iSetBitMode = 1; // Finished setting, any duplicate parameters will be ignored.
								}
							}
						}
					}
					bSubArg = FALSE; // Unset sub-argument switch, this is limited to only one sub-arg at this time.
				}
			}
		}

		// Update the game path in memory
		SetGamePath();

		sprintf_s(szLogPath, sizeof(szLogPath) - 1, "%s\\sc2kfix%s.log", szGamePath, GetLogSuffixFromSC2KFixMode());

		// Open a log file. If it fails, we handle that safely elsewhere
		// AF - Relocated so it will record any messages that occur
		//      prior to the console itself being enabled.
		fdLog = old_fopen(szLogPath, "w");
		if (!fdLog)
			ConsoleLog(LOG_ERROR, "Failed to open file handle for '%s'. Logs won't be recorded.\n", szLogPath);

		// Allocate a console and immediately hide it. We will later send a ShowWindow to make it
		// visible if the console is to be made user-facing.
		{
			AllocConsole();
			ShowWindow(GetConsoleWindow(), SW_HIDE);
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
			SendMessage(GetConsoleWindow(), WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
			SendMessage(GetConsoleWindow(), WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		}

		// Print the version banner
		// Yes, I know, there's no CORE: prefix here. That's intentional. I promise.
		ConsoleLog(LOG_INFO, "sc2kfix version %s started - https://sc2kfix.net\n", szSC2KFixVersion);
#ifdef DEBUGALL
		ConsoleLog(LOG_DEBUG, "CORE: sc2kfix built with DEBUGALL. Strap in.\n");
#endif

		ConsoleLog(LOG_INFO, "CORE: %s session started at %lld.\n", GetProgramNameFromSC2KFixMode(), time(NULL));
		ConsoleLog(LOG_INFO, "CORE: Command line: %s\n", GetCommandLine());

		// Dump some OS version information for bug reports
		{
			BOOL (WINAPI *RtlGetVersion)(LPOSVERSIONINFOW*) = (BOOL (WINAPI *)(LPOSVERSIONINFOW*))GetProcAddress(LoadLibrary("ntdll.dll"), "RtlGetVersion");
			OSVERSIONINFOEXW stOSVersionInfoEx = { 0 };
			stOSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
			RtlGetVersion((LPOSVERSIONINFOW*)&stOSVersionInfoEx);
			BOOL bX64 = FALSE;
			IsWow64Process(GetCurrentProcess(), &bX64);

			ConsoleLog(LOG_INFO, "CORE: OS is Windows%s %d.%d Build %d (%s)\n",
				stOSVersionInfoEx.wProductType == VER_NT_WORKSTATION ? "" : " Server",
				stOSVersionInfoEx.dwMajorVersion, stOSVersionInfoEx.dwMinorVersion, stOSVersionInfoEx.dwBuildNumber,
				bX64 ? "x64" : "x86");
		}

		// Dump some console opening info
		if (bConsoleEnabled) {
			ConsoleLog(LOG_INFO, "CORE: Spawned console session.\n");
			printf("[INFO ] CORE: ");
			ConsoleCmdShowDebug(NULL, NULL);
		}

		// Load the FluidSynth library in case we need it
		MusicLoadFluidSynth();

		// Initialize settings
		InitializeSettings();

		// Load settings
		if (!bSkipLoadSettings)
			LoadSettings();
		else
			ConsoleLog(LOG_INFO, "CORE: -default passed, skipping LoadSettings()\n");

		// Force the console to be visible if bSettingsAlwaysConsole is set
		if (bSettingsAlwaysConsole)
			bConsoleEnabled = true;

		// Foce n-bit mode if requested
		if (iForcedBits > 0)
			ConsoleLog(LOG_INFO, "CORE: -bitmode passed, forcing %d-bit mode.\n", iForcedBits);

		// Force the console to be visible if DEBUGALL is defined
#ifdef DEBUGALL
		bConsoleEnabled = true;
#endif

		// Enable the console window if requested
		if (bConsoleEnabled)
			ShowWindow(GetConsoleWindow(), SW_NORMAL);

		// Install our top-level exception handler
		SetUnhandledExceptionFilter(CrashHandler);
		
		// Seed the libc RNG -- we'll need this later
		srand((unsigned int)time(NULL));

		// Determine what version of SC2K this is
		// HACK: there's probably a better way to do this
		dwDetectedAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSC2KAppModule)->e_lfanew + (UINT_PTR)hSC2KAppModule))->FileHeader.TimeDateStamp;
		switch (dwDetectedAppTimestamp) {
		case 0x313E706E:
			if (dwSC2KFixMode == SC2KFIX_MODE_SC2K) {
				dwDetectedVersion = VERSION_SC2K_1996;
				break;
			}

		case 0x302FEA8A:
			if (dwSC2KFixMode == SC2KFIX_MODE_SC2K) {
				dwDetectedVersion = VERSION_SC2K_1995;
				ConsoleLog(LOG_NOTICE, "CORE: 1995 CD Collection version detected. Most features and gameplay fixes will not be available.\n");
				ConsoleLog(LOG_NOTICE, "CORE: Please consider using the 1996 Special Edition for the fully restored SimCity 2000 experience.\n");
				break;
			}

		case 0x3103B687:
			if (dwSC2KFixMode == SC2KFIX_MODE_SC2KDEMO) {
				dwDetectedVersion = VERSION_SC2K_DEMO;
				ConsoleLog(LOG_NOTICE, "CORE: Interactive Demo version detected. Good luck!\n");
				break;
			}

		case 0xBC7B1F0E: // Yes, for some reason the timestamp is set to 2070.
			if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
				dwDetectedVersion = VERSION_SCURK_PRIMARY;
				ConsoleLog(LOG_NOTICE, "CORE: SCURK version primary (1995) detected.\n");
				break;
			}

		case 0x6C261F75:
			if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
				dwDetectedVersion = VERSION_SCURK_1996;
				ConsoleLog(LOG_NOTICE, "CORE: SCURK version Network Edition (1996) detected.\n");
				break;
			}

		default:
			dwDetectedVersion = VERSION_PROG_UNKNOWN;
			char msg[512 + 1];
			sprintf_s(msg, sizeof(msg) - 1, "Could not detect program version (got timestamp 0x%08X). Fixes and features will not be loaded.\r\n\r\n"
				"Please let us know in a GitHub issue what version of the game you're running so we can look into this.", dwDetectedAppTimestamp);
			MessageBox(GetActiveWindow(), msg, "sc2kfix warning", MB_OK | MB_ICONWARNING);
			ConsoleLog(LOG_WARNING, "CORE: Program version could not be detected (got timestamp 0x%08X). Fixes and features will not be loaded.\n", dwDetectedAppTimestamp);
		}

		// Let's break out instead if this case is hit.
		// Better to avoid anomalous behaviour.
		if (dwDetectedVersion == VERSION_PROG_UNKNOWN)
			break;

		// If we're attached to SCURK, switch over to the SCURK fix code
		if (dwSC2KFixMode == SC2KFIX_MODE_SCURK) {
			if (dwDetectedVersion == VERSION_SCURK_PRIMARY)
				InstallFixes_SCURKPrimary();
			else if (dwDetectedVersion == VERSION_SCURK_1996)
				InstallFixes_SCURK1996();
			break;
		}

		if (dwDetectedVersion == VERSION_SC2K_DEMO)
			gamePrimaryKey = "SimCity 2000 Win95 Demo";

		// Registry check
		int iInstallCheck;

		iInstallCheck = DoCheckAndInstall();
		if (iInstallCheck)
			ConsoleLog(LOG_INFO, "CORE: Portable entries created by faux-installer.\n");

		if (dwDetectedVersion == VERSION_SC2K_1996) {
			ConsoleLog(LOG_INFO, "CORE: Loading last stored load/save city and load tileset paths.\n");
			LoadStoredPaths();
		}

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
		BOOL bCanFixAnimation;

		bCanFixAnimation = TRUE;
		if (dwDetectedVersion == VERSION_SC2K_1996)
			InstallAnimationHooks_SC2K1996();
		else if (dwDetectedVersion == VERSION_SC2K_1995)
			InstallAnimationHooks_SC2K1995();
		else if (dwDetectedVersion == VERSION_SC2K_DEMO)
			InstallAnimationHooks_SC2KDemo();
		else
			bCanFixAnimation = FALSE;

		if (bCanFixAnimation)
			ConsoleLog(LOG_INFO, "CORE: Patched palette animation fix.\n");

		// Dialog crash fix - hat tip to Aleksander Krimsky (@alekasm on GitHub)
		BOOL bCanFixDialogCrash;
		LPVOID lpDialogFix1;
		LPVOID lpDialogFix2;

		lpDialogFix1 = NULL;
		lpDialogFix2 = NULL;
		switch (dwDetectedVersion) {
		case VERSION_SC2K_DEMO:
			bCanFixDialogCrash = TRUE;
			lpDialogFix1 = (LPVOID)0x48A2EB;
			lpDialogFix2 = (LPVOID)0x48A34A;
			break;

		case VERSION_SC2K_1995:
			bCanFixDialogCrash = TRUE;
			lpDialogFix1 = (LPVOID)0x49EE93;
			lpDialogFix2 = (LPVOID)0x49EEF2;
			break;

		case VERSION_SC2K_1996:
			bCanFixDialogCrash = TRUE;
			lpDialogFix1 = (LPVOID)0x4A04FA;
			lpDialogFix2 = (LPVOID)0x4A0559;
			break;

		default:
			bCanFixDialogCrash = FALSE;
			break;
		}

		if (bCanFixDialogCrash) {
			VirtualProtect(lpDialogFix1, 1, PAGE_EXECUTE_READWRITE, &dwDummy);
			*(LPBYTE)lpDialogFix1 = 0x20;
			VirtualProtect(lpDialogFix2, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
			*(LPBYTE)lpDialogFix2 = 0xEB;
			*(LPBYTE)((UINT_PTR)lpDialogFix2 + 1) = 0xEB;
			ConsoleLog(LOG_INFO, "CORE: Patched dialog crash fix.\n");
		}

		// Remove palette warnings
		BOOL bCanFixPaletteWarnings;
		LPVOID lpWarningFix1;
		LPVOID lpWarningFix2;

		lpWarningFix1 = NULL;
		lpWarningFix2 = NULL;
		switch (dwDetectedVersion) {
		case VERSION_SC2K_DEMO:
			bCanFixPaletteWarnings = TRUE;
			lpWarningFix1 = (LPVOID)0x478CFB;
			lpWarningFix2 = (LPVOID)0x478D40;
			break;

		case VERSION_SC2K_1995:
			bCanFixPaletteWarnings = TRUE;
			lpWarningFix1 = (LPVOID)0x408749;
			lpWarningFix2 = (LPVOID)0x40878E;
			break;

		case VERSION_SC2K_1996:
			bCanFixPaletteWarnings = TRUE;
			lpWarningFix1 = (LPVOID)0x408A79;
			lpWarningFix2 = (LPVOID)0x408ABE;
			break;

		default:
			bCanFixPaletteWarnings = FALSE;
			break;
		}

		if (bCanFixPaletteWarnings) {
			VirtualProtect(lpWarningFix1, 2, PAGE_EXECUTE_READWRITE, &dwDummy);
			VirtualProtect(lpWarningFix2, 18, PAGE_EXECUTE_READWRITE, &dwDummy);
			*(LPBYTE)lpWarningFix1 = 0x90;
			*(LPBYTE)((UINT_PTR)lpWarningFix1 + 1) = 0x90;
			memset((LPVOID)lpWarningFix2, 0x90, 18);   // nop nop nop nop nop
			ConsoleLog(LOG_INFO, "CORE: Patched 8-bit colour warnings.\n");
		}

		// Hooks we only want to inject on the 1996 Special Edition version
		// and the registry hooks that are for the 1995 CD Collection version.
		if (dwDetectedVersion == VERSION_SC2K_1996)
			InstallMiscHooks_SC2K1996();
		else if (dwDetectedVersion == VERSION_SC2K_1995)
			InstallMiscHooks_SC2K1995();
		else if (dwDetectedVersion == VERSION_SC2K_DEMO)
			InstallMiscHooks_SC2KDemo();

		// Start the console thread.
		if (bConsoleEnabled) {
			ConsoleLog(LOG_INFO, "CORE: Starting console thread.\n");
#if !NOKUROKO
			hConsoleThread = CreateThread(NULL, 0, ConsoleThread, 0, 0, &dwConsoleThreadID);
#else
			hConsoleThread = CreateThread(NULL, 0, ConsoleThread, 0, 0, NULL);
#endif
		}

		// Set up the modding infrastructure for the 1996 Special Edition version.
		if (dwDetectedVersion == VERSION_SC2K_1996) {
#if !NOKUROKO
			// Initialize the Kuroko VM
			CreateThread(NULL, 0, KurokoThread, 0, 0, &dwKurokoThreadID);
#endif

			// Load native code mods.
			if (!bSkipLoadingMods && !bSettingsDontLoadMods)
				LoadNativeCodeMods();
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

#if !NOKUROKO
		// Shut down the Kuroko thread
		if (bKurokoVMInitialized)
			PostThreadMessage(dwKurokoThreadID, WM_QUIT, NULL, NULL);
#endif

		// Only save the stored paths during a graceful exit. (SC2K1996 only for now)
		if (!bGameDead)
			if (dwDetectedVersion == VERSION_SC2K_1996)
				SaveStoredPaths();

		// Clear out the stored sprite IDs (no allocated data are contained).
		spriteIDs.clear();

		// Send a closing message and close the log file
		ConsoleLog(LOG_INFO, "CORE: Closing down at %lld. Goodnight!\n", time(NULL));
		if (fdLog) {
			fflush(fdLog);
			fclose(fdLog);
		}
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
	int iStackFrameLimit = 20;

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
		if ((DWORD)lpExceptions->ContextRecord->Eip == (DWORD)lpExceptions->ExceptionRecord->ExceptionInformation[1])
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Execution of address 0x%08X\n", (DWORD)lpExceptions->ContextRecord->Eip);
		else if ((DWORD)lpExceptions->ExceptionRecord->ExceptionInformation[0])
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Write of address 0x%08X\n", (DWORD)lpExceptions->ExceptionRecord->ExceptionInformation[1]);
		else
			ConsoleLog(LOG_EMERGENCY, "CORE: Exception Cause:  Read of address 0x%08X\n", (DWORD)lpExceptions->ExceptionRecord->ExceptionInformation[1]);
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
	while (StackWalk(IMAGE_FILE_MACHINE_I386, hFaultingProcess, hFaultingThread, &stStackFrame, lpExceptions->ContextRecord, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) {
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
		if (!--iStackFrameLimit) {
			ConsoleLog(LOG_EMERGENCY, "CORE: !!!WARNING!!! Reached stack frame unwind limit; stack corruption VERY LIKELY!\n");
			break;
		}
	}

	ConsoleLog(LOG_EMERGENCY, "CORE:\n");
	ConsoleLog(LOG_EMERGENCY, "CORE: End of stack trace. Time to die.\n");
	
	MessageBox(GetActiveWindow(),
		"sc2kfix has detected an unhandled top-level exception in SimCity 2000. If you have the "
		"console open, check the console for details. Fault information has been logged to the "
		"console and to sc2kfix.log.\n\n"

		"Please submit a crash report to the sc2kfix developers either via the sc2kfix GitHub "
		"issues page (https://github.com/sc2kfix/sc2kfix/issues -- preferred) or via the sc2kfix "
		"Discord server (https://sc2kfix.net/discord). In order for us to best assist with the "
		"crash, please make a copy of the sc2kfix.log file after closing this dialog and before "
		"you re-open SimCity 2000. Submit this copy of the log file along with your crash report, "
		"and we will do our best to investigate.\n\n"
		
		"Clicking the OK button will immediately terminate SimCity 2000. Any unsaved progress "
		"will be lost.", "sc2kfix fatal error", MB_OK | MB_ICONSTOP);
	return EXCEPTION_EXECUTE_HANDLER;
}

// Exports for WinMM hook
ALLEXPORTS_PASSTHROUGH(DEFPROC)
