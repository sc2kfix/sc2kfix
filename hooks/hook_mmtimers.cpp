// sc2kfix hooks/hook_mmtimers.cpp: hook for winmm timer interfaces
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

#pragma intrinsic(_ReturnAddress)

#define TIMER_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TIMER_DEBUG
#define TIMER_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT timer_debug = TIMER_DEBUG;

extern "C" BOOL __stdcall Hook_timeSetEvent(void* pReturnAddress, MMRESULT* retval, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent) {
    if (timer_debug && (DWORD)pReturnAddress < 0x563000)
        ConsoleLog(LOG_DEBUG, "TIME: 0x%08p -> timeSetEvent(%u, %u, 0x%08X, 0x%08X, 0x%04X)\n", pReturnAddress, uDelay, uResolution, lpTimeProc, dwUser, fuEvent);

    return TRUE;
}

extern "C" BOOL __stdcall HookAfter_timeSetEvent(void* pReturnAddress, MMRESULT* retval, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent) {
    if (timer_debug && (DWORD)pReturnAddress < 0x563000)
        ConsoleLog(LOG_DEBUG, "TIME: timeSetEvent() returned 0x%08X\n", *retval);

    return TRUE;
}

extern "C" MMRESULT __stdcall _timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent) {
    MMRESULT retval = NULL;
    if (!Hook_timeSetEvent(_ReturnAddress(), &retval, uDelay, uResolution, lpTimeProc, dwUser, fuEvent))
        return retval;

    __asm {
        push fuEvent
        push dwUser
        push lpTimeProc
        push uResolution
        push uDelay
        call fpWinMMHookList[140 * 4]
        mov [retval], eax
    }

    HookAfter_timeSetEvent(_ReturnAddress(), &retval, uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
    return retval;
}
