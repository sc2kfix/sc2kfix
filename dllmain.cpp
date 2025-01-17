// sc2kfix dllmain.cpp: all the magic happens here
// (c) 2025 github.com/araxestroy - released under the MIT license

#define DEBUG

#define GETPROC(i, name) fpHookList[i] = GetProcAddress(hRealWinMM, #name);
#define DEFPROC(i, name) extern "C" __declspec(naked) void __stdcall _##name() { __asm { jmp fpHookList[i*4] }};

#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "winmm_exports.h"

HMODULE hRealWinMM = NULL;
FARPROC fpHookList[180] = { NULL };

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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
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
        MessageBox(GetActiveWindow(), "Hooked winmm.dll!", "sc2kfix", MB_OK | MB_ICONINFORMATION);
#endif

        // Palette animation fix
        DWORD dummy;
        VirtualProtect((LPVOID)0x004571D3, 30, PAGE_EXECUTE_READWRITE, &dummy);
        memcpy((LPVOID)0x004571D3, bAnimationPatch1996, 30);

        // Dialog crash fix - hat tip to Aleksander Krimsky (@alekasm on GitHub)
        VirtualProtect((LPVOID)0x004A04FA, 1, PAGE_EXECUTE_READWRITE, &dummy);
        *(LPBYTE)0x004A04FA = 0x20;
        VirtualProtect((LPVOID)0x004A0559, 2, PAGE_EXECUTE_READWRITE, &dummy);
        *(LPBYTE)0x004A0559 = 0xEB;
        *(LPBYTE)0x004A055A = 0xEB;

#ifdef DEBUG
        MessageBox(GetActiveWindow(), "Patched SC2K!", "sc2kfix", MB_OK | MB_ICONINFORMATION);
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
