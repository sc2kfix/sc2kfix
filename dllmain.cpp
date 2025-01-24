// sc2kfix dllmain.cpp: all the magic happens here
// (c) 2025 github.com/araxestroy - released under the MIT license

#define GETPROC(i, name) fpHookList[i] = GetProcAddress(hRealWinMM, #name);
#define DEFPROC(i, name) extern "C" __declspec(naked) void __stdcall _##name() { __asm { jmp fpHookList[i*4] }};

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#include "sc2kfix.h"
#include "resource.h"
#include "winmm_exports.h"

// From registry_install.cpp
extern char szMayorName[64];
extern char szCompanyName[64];

// Global variables that we need to keep handy
HMODULE hRealWinMM = NULL;
HMODULE hSC2KAppModule = NULL;
HMODULE hSC2KFixModule = NULL;
FARPROC fpHookList[180] = { NULL };
DWORD dwDetectedVersion = VERSION_UNKNOWN;
DWORD dwSC2KAppTimestamp = 0;

// This code replaces the original stack cleanup and return after the engine
// cycles the animation palette.
// 
// 6881000000      push dword 0x81              ; flags = RDW_INVALIDATE | RDW_ALLCHILDREN
// 6A00            push 0                       ; hrgnUpdate = NULL
// 6A00            push 0                       ; lprcUpdate = NULL
// 8B0D2C704C00    mov ecx, [pCwndMainWindow]
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
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        // Save our own module handle
        hSC2KFixModule = hModule;

        // Find the actual WinMM library
        char buf[200];
        GetSystemDirectory(buf, 200);
        strcat_s(buf, "\\winmm.dll");

        // Load the actual WinMM library
        if (!(hRealWinMM = LoadLibrary(buf))) {
            MessageBox(GetActiveWindow(), "Could not load winmm.dll (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
            return false;
        }

        // Retrieve the list of functions we need to pass through to WinMM
        ALLEXPORTS(GETPROC);

#ifdef DEBUG
        MessageBox(GetActiveWindow(), "DEBUG: Hooked winmm.dll!", "sc2kfix", MB_OK | MB_ICONINFORMATION);
#endif

        // Make sure this isn't SCURK
        char szModuleBaseName[200];
        GetModuleBaseName(GetCurrentProcess(), NULL, szModuleBaseName, 200);
        if (!_stricmp(szModuleBaseName, "winscurk.exe"))
            return true;

        // Determine what version of SC2K this is
        // HACK: there's probably a better way to do this
        if (!(hSC2KAppModule = GetModuleHandle(NULL))) {
            MessageBox(GetActiveWindow(), "Could not GetModuleHandle(NULL) (???)", "sc2kfix error", MB_OK | MB_ICONERROR);
            return false;
        }
        dwSC2KAppTimestamp = ((PIMAGE_NT_HEADERS)(((PIMAGE_DOS_HEADER)hSC2KAppModule)->e_lfanew + (UINT_PTR)hSC2KAppModule))->FileHeader.TimeDateStamp;
        switch (dwSC2KAppTimestamp) {
        case 0x302FEA8A:
            dwDetectedVersion = VERSION_1995;
            break;

        case 0x313E706E:
            dwDetectedVersion = VERSION_1996;
            break;

        default:
            dwDetectedVersion = VERSION_UNKNOWN;
            char msg[300];
            sprintf_s(msg, 300, "Could not detect SC2K version (got timestamp %08Xd). Your game will probably crash.\r\n\r\n"
                "Please let us know in a GitHub issue what version of the game you're running so we can look into this.", dwSC2KAppTimestamp);
            MessageBox(GetActiveWindow(), msg, "sc2kfix warning", MB_OK | MB_ICONWARNING);
        }

        // Registry check
        DoRegistryCheckAndInstall();

        // Palette animation fix
        LPVOID lpAnimationFix;
        PBYTE lpAnimationFixSrc;
        UINT uAnimationFixLength;
        switch (dwDetectedVersion) {
        case VERSION_1995:
            lpAnimationFix = (LPVOID)0x00456B23;
            lpAnimationFixSrc = bAnimationPatch1995;
            uAnimationFixLength = 30;
            break;

        case VERSION_1996:
        default:
            lpAnimationFix = (LPVOID)0x004571D3;
            lpAnimationFixSrc = bAnimationPatch1996;
            uAnimationFixLength = 30;
        }
        DWORD dummy;
        VirtualProtect(lpAnimationFix, uAnimationFixLength, PAGE_EXECUTE_READWRITE, &dummy);
        memcpy(lpAnimationFix, lpAnimationFixSrc, uAnimationFixLength);

        // Dialog crash fix - hat tip to Aleksander Krimsky (@alekasm on GitHub)
        LPVOID lpDialogFix1;
        LPVOID lpDialogFix2;
        switch (dwDetectedVersion) {
        case VERSION_1995:
            lpDialogFix1 = (LPVOID)0x0049EE93;
            lpDialogFix2 = (LPVOID)0x0049EEF2;
            break;

        case VERSION_1996:
        default:
            lpDialogFix1 = (LPVOID)0x004A04FA;
            lpDialogFix2 = (LPVOID)0x004A0559;
        }

        VirtualProtect(lpDialogFix1, 1, PAGE_EXECUTE_READWRITE, &dummy);
        *(LPBYTE)lpDialogFix1 = 0x20;
        VirtualProtect(lpDialogFix2, 2, PAGE_EXECUTE_READWRITE, &dummy);
        *(LPBYTE)lpDialogFix2 = 0xEB;
        *(LPBYTE)((UINT_PTR)lpDialogFix2 + 1) = 0xEB;

        // Remove palette warnings
        LPVOID lpWarningFix1;
        LPVOID lpWarningFix2;
        switch (dwDetectedVersion) {
        case VERSION_1995:
            lpWarningFix1 = (LPVOID)0x00408749;
            lpWarningFix2 = (LPVOID)0x0040878E;
            break;

        case VERSION_1996:
        default:
            lpWarningFix1 = (LPVOID)0x00408A79;
            lpWarningFix2 = (LPVOID)0x00408ABE;
        }
        VirtualProtect(lpWarningFix1, 2, PAGE_EXECUTE_READWRITE, &dummy);
        VirtualProtect(lpWarningFix2, 18, PAGE_EXECUTE_READWRITE, &dummy);
        *(LPBYTE)lpWarningFix1 = 0x90;
        *(LPBYTE)((UINT_PTR)lpWarningFix1 + 1) = 0x90;
        memset((LPVOID)lpWarningFix2, 0x90, 18);   // nop nop nop nop nop

#ifdef DEBUG
        MessageBox(GetActiveWindow(), "DEBUG: Patched SC2K!", "sc2kfix", MB_OK | MB_ICONINFORMATION);
#endif

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return true;
}

// Exports for WinMM hook
ALLEXPORTS(DEFPROC)
