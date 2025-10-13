// sc2kfix hooks/hook_sndPlaySound.cpp: hook for sndPlaySoundA
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SND_DEBUG_PLAYS 1
#define SND_DEBUG_REPLACEMENTS 2

#define SND_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SND_DEBUG
#define SND_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT snd_debug = SND_DEBUG;

std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
std::map<int, sound_replacement_t> mapReplacementSounds;

static DWORD dwDummy;

extern "C" int __stdcall Hook_LoadSoundBuffer(int iSoundID, void* lpBuffer) {
    if (snd_debug & SND_DEBUG_PLAYS)
        ConsoleLog(LOG_DEBUG, "SND:  Loading %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);

    if (mapSoundBuffers.find((DWORD)lpBuffer) != mapSoundBuffers.end()) {
        mapSoundBuffers[(DWORD)lpBuffer].iSoundID = iSoundID;
        mapSoundBuffers[(DWORD)lpBuffer].iReloadCount++;
    } else {
        mapSoundBuffers[(DWORD)lpBuffer] = { iSoundID, 1 };
    }

    if (mapReplacementSounds.find(iSoundID) != mapReplacementSounds.end() && bSettingsUseSoundReplacements) {
        memcpy_s(lpBuffer, mapReplacementSounds[iSoundID].nBufSize, mapReplacementSounds[iSoundID].bBuffer, mapReplacementSounds[iSoundID].nBufSize);
        if (snd_debug & SND_DEBUG_PLAYS)
            ConsoleLog(LOG_DEBUG, "SND:  Detour! Copied replacement %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);
        return 1;
    }

    return GameMain_LoadSoundBuffer(iSoundID, lpBuffer);
}

extern "C" BOOL __stdcall Hook_sndPlaySoundA(void* pReturnAddress, BOOL* retval, LPCTSTR lpszSound, UINT fuSound) {
    if (snd_debug & SND_DEBUG_PLAYS) {
        if (fuSound & SND_MEMORY)
            ConsoleLog(LOG_DEBUG, "SND:  0x%08p -> sndPlaySound(<0x%08X>, 0x%08X)\n", pReturnAddress, lpszSound, fuSound);
        else if (!lpszSound && !fuSound)
            ConsoleLog(LOG_DEBUG, "SND:  0x%08p -> sndPlaySound(0, 0)\n", pReturnAddress);
        else
            ConsoleLog(LOG_DEBUG, "SND:  0x%08p -> sndPlaySound(%s, 0x%08X)\n", pReturnAddress, (lpszSound ? lpszSound : "NULL"), fuSound);
    }
        

    return TRUE;
}

extern "C" BOOL __stdcall HookAfter_sndPlaySoundA(void* pReturnAddress, BOOL* retval, LPCTSTR lpszSound, UINT fuSound) {
    return TRUE;
}

extern "C" BOOL __stdcall _sndPlaySoundA(LPCTSTR lpszSound, UINT fuSound) {
    BOOL retval = TRUE;
    if (!Hook_sndPlaySoundA(_ReturnAddress(), &retval, lpszSound, fuSound))
        return retval;

    __asm {
        push fuSound
        push lpszSound
        call fpWinMMHookList[132 * 4]
        mov [retval], eax
    }

    HookAfter_sndPlaySoundA(_ReturnAddress(), &retval, lpszSound, fuSound);
    return retval;
}

void LoadReplacementSounds(void) {
    HRSRC hResFind;
    HGLOBAL hWaveResource;

    // Increase sound buffer sizes to 256K each
    VirtualProtect((LPVOID)0x480C2B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
    *(DWORD*)0x480C2B = 262144;
    VirtualProtect((LPVOID)0x480C4B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
    *(DWORD*)0x480C4B = 262144;
    VirtualProtect((LPVOID)0x480C5B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
    *(DWORD*)0x480C5B = 262144;
    VirtualProtect((LPVOID)0x480C6B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
    *(DWORD*)0x480C6B = 262144;
    VirtualProtect((LPVOID)0x480C7B, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
    *(DWORD*)0x480C7B = 262144;

    // Hook sound buffer loading
    VirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
    NEWJMP((LPVOID)0x401F9B, Hook_LoadSoundBuffer);

    // Load the replacement sound resources
    hResFind = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_WAVE_500), "WAVE");
    if (hResFind) {
        hWaveResource = LoadResource(hSC2KFixModule, hResFind);
        if (hWaveResource) {
            sound_replacement_t entry;
            entry.nBufSize = SizeofResource(hSC2KFixModule, hResFind);
            entry.bBuffer = (BYTE*)malloc(entry.nBufSize);

            if (entry.bBuffer) {
                mapReplacementSounds[500] = entry;
                void* ptr = LockResource(hWaveResource);
                if (ptr) {
                    memcpy_s(entry.bBuffer, entry.nBufSize, ptr, entry.nBufSize);
                    FreeResource(hWaveResource);
                    if (snd_debug & SND_DEBUG_REPLACEMENTS)
                        ConsoleLog(LOG_DEBUG, "SND:  Loaded replacement for 500.wav.\n");
                }
            } else
                if(snd_debug & SND_DEBUG_REPLACEMENTS)
                    ConsoleLog(LOG_DEBUG, "SND:  Couldn't allocate replacement sound buffer.\n");
        }
    } else
        if (snd_debug & SND_DEBUG_REPLACEMENTS)
            ConsoleLog(LOG_DEBUG, "SND:  Couldn't find resource for sound 500.\n");

    hResFind = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_WAVE_514), "WAVE");
    if (hResFind) {
        hWaveResource = LoadResource(hSC2KFixModule, hResFind);
        if (hWaveResource) {
            sound_replacement_t entry;
            entry.nBufSize = SizeofResource(hSC2KFixModule, hResFind);
            entry.bBuffer = (BYTE*)malloc(entry.nBufSize);

            if (entry.bBuffer) {
                mapReplacementSounds[514] = entry;
                void* ptr = LockResource(hWaveResource);
                if (ptr) {
                    memcpy_s(entry.bBuffer, entry.nBufSize, ptr, entry.nBufSize);
                    FreeResource(hWaveResource);
                    if (snd_debug & SND_DEBUG_REPLACEMENTS)
                        ConsoleLog(LOG_DEBUG, "SND:  Loaded replacement for 514.wav.\n");
                }
            }
            else
                if (snd_debug & SND_DEBUG_REPLACEMENTS)
                    ConsoleLog(LOG_DEBUG, "SND:  Couldn't allocate replacement sound buffer.\n");
        }
    }
    else
        if (snd_debug & SND_DEBUG_REPLACEMENTS)
            ConsoleLog(LOG_DEBUG, "SND:  Couldn't find resource for sound 514.\n");

    hResFind = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_WAVE_529), "WAVE");
    if (hResFind) {
        hWaveResource = LoadResource(hSC2KFixModule, hResFind);
        if (hWaveResource) {
            sound_replacement_t entry;
            entry.nBufSize = SizeofResource(hSC2KFixModule, hResFind);
            entry.bBuffer = (BYTE*)malloc(entry.nBufSize);

            if (entry.bBuffer) {
                mapReplacementSounds[529] = entry;
                void* ptr = LockResource(hWaveResource);
                if (ptr) {
                    memcpy_s(entry.bBuffer, entry.nBufSize, ptr, entry.nBufSize);
                    FreeResource(hWaveResource);
                    if (snd_debug & SND_DEBUG_REPLACEMENTS)
                        ConsoleLog(LOG_DEBUG, "SND:  Loaded replacement for 529.wav.\n");
                }
            }
            else
                if (snd_debug & SND_DEBUG_REPLACEMENTS)
                    ConsoleLog(LOG_DEBUG, "SND:  Couldn't allocate replacement sound buffer.\n");
        }
    }
    else
        if (snd_debug & SND_DEBUG_REPLACEMENTS)
            ConsoleLog(LOG_DEBUG, "SND:  Couldn't find resource for sound 529.\n");
}
