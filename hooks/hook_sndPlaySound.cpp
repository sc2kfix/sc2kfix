// sc2kfix hook_mcisendcommand.cpp: hook for mciSendCommandA
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

#define SND_DEBUG 0

#ifdef DEBUGALL
#undef SND_DEBUG
#define SND_DEBUG 1
#endif

UINT snd_debug = SND_DEBUG;

std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;

extern "C" void __stdcall Hook_401F9B(int iSoundID, void* lpBuffer) {
    if (snd_debug)
        ConsoleLog(LOG_DEBUG, "SND: Loading %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);

    if (mapSoundBuffers.find((DWORD)lpBuffer) != mapSoundBuffers.end()) {
        mapSoundBuffers[(DWORD)lpBuffer].iSoundID = iSoundID;
        mapSoundBuffers[(DWORD)lpBuffer].iReloadCount++;
    } else {
        mapSoundBuffers[(DWORD)lpBuffer] = { iSoundID, 1 };
    }

    __asm {
        push lpBuffer
        push iSoundID
        mov edi, 0x480140
        call edi
    }
}

extern "C" BOOL __stdcall Hook_sndPlaySoundA(void* pReturnAddress, BOOL* retval, LPCTSTR lpszSound, UINT fuSound) {
    if (snd_debug) {
        if (fuSound & SND_MEMORY)
            ConsoleLog(LOG_DEBUG, "SND: 0x%08p -> sndPlaySound(<0x%08X>, 0x%08X)\n", pReturnAddress, lpszSound, fuSound);
        else if (!lpszSound && !fuSound)
            ConsoleLog(LOG_DEBUG, "SND: 0x%08p -> sndPlaySound(0, 0)\n", pReturnAddress);
        else
            ConsoleLog(LOG_DEBUG, "SND: 0x%08p -> sndPlaySound(%s, 0x%08X)\n", pReturnAddress, (lpszSound ? lpszSound : "NULL"), fuSound);
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
