// sc2kfix hook_mcisendcommand.cpp: hook for mciSendCommandA
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>

#include "sc2kfix.h"

#pragma intrinsic(_ReturnAddress)

#define MCI_DEBUG 0

#ifdef DEBUGALL
#undef MCI_DEBUG
#define MCI_DEBUG 1
#endif

UINT mci_debug = MCI_DEBUG;

static const char* MCIMessageIDToString(UINT uMsg) {
    switch (uMsg) {
    case MCI_OPEN:
        return "MCI_OPEN";
    case MCI_CLOSE:
        return "MCI_CLOSE";
    case MCI_PLAY:
        return "MCI_PLAY";
    case MCI_SEEK:
        return "MCI_SEEK";
    case MCI_STOP:
        return "MCI_STOP";
    case MCI_PAUSE:
        return "MCI_PAUSE";
    case MCI_INFO:
        return "MCI_INFO";
    case MCI_STATUS:
        return "MCI_STATUS";
    case MCI_RESUME:
        return "MCI_RESUME";
    default:
        return HexPls(uMsg);
    }
}

extern "C" BOOL _stdcall Hook_mciSendCommandA(void* pReturnAddress, MCIERROR* retval, MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    if (mci_debug)
        ConsoleLog(LOG_DEBUG, "MCI: 0x%08p -> mciSendCommand(0x%08X, %s, 0x%08X, 0x%08X)\n", pReturnAddress, IDDevice, MCIMessageIDToString(uMsg), fdwCommand, dwParam);
    switch (uMsg) {
    case MCI_OPEN: {
        if (mci_debug) {
            MCI_OPEN_PARMS* pMCIOpenParms = (MCI_OPEN_PARMS*)dwParam;
			ConsoleLog(LOG_NONE,
                "MCI_OPEN_PARMS {\n"
				"    dwCallback       = 0x%08X,\n"
				"    wDeviceID        = 0x%08X,\n"
				"    lpstrDeviceType  = %s,\n"
				"    lpstrElementName = %s,\n"
				"    lpstrAlias       = %s\n"
				"}\n\n", pMCIOpenParms->dwCallback, pMCIOpenParms->wDeviceID,
				((UINT)(pMCIOpenParms->lpstrDeviceType) > 4096 ? pMCIOpenParms->lpstrDeviceType : HexPls((UINT)(pMCIOpenParms->lpstrDeviceType))),
				pMCIOpenParms->lpstrElementName,
				((UINT)(pMCIOpenParms->lpstrAlias) == 0 ? pMCIOpenParms->lpstrAlias : "NULL"));
        }
        break;
    }

    case MCI_STATUS: {
        if (mci_debug) {
            MCI_STATUS_PARMS* pMCIStatusParms = (MCI_STATUS_PARMS*)dwParam;
			ConsoleLog(LOG_NONE,
                "MCI_STATUS_PARMS {\n"
				"    dwCallback = 0x%08X,\n"
				"    dwReturn   = 0x%08X,\n"
				"    dwItem     = 0x%08X,\n"
				"    dwTrack    = 0x%08X\n"
				"}\n\n", pMCIStatusParms->dwCallback, pMCIStatusParms->dwReturn, pMCIStatusParms->dwItem, pMCIStatusParms->dwTrack);
        }
        break;
    }

    case MCI_PLAY: {
        if (mci_debug) {
            MCI_PLAY_PARMS* pMCIPlayParms = (MCI_PLAY_PARMS*)dwParam;
			ConsoleLog(LOG_NONE,
                "MCI_PLAY_PARMS {\n"
				"    dwCallback = 0x%08X,\n"
				"    dwFrom     = 0x%08X,\n"
				"    dwTo       = 0x%08X\n"
				"}\n\n", pMCIPlayParms->dwCallback, pMCIPlayParms->dwFrom, pMCIPlayParms->dwTo);
        }
        break;
    }

    case MCI_CLOSE:
        break;

    default:
        if (mci_debug)
            ConsoleLog(LOG_WARNING, "MCA: mciSendCommand sent with unexpected uMsg %s.\n", MCIMessageIDToString(uMsg));
    }

    *retval = 0;
    return TRUE;
}

extern "C" BOOL _stdcall HookAfter_mciSendCommandA(void* pReturnAddress, MCIERROR* retval, MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    return TRUE;
}

extern "C" MCIERROR _stdcall _mciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    MCIERROR retval = 0;
    if (!Hook_mciSendCommandA(_ReturnAddress(), &retval, IDDevice, uMsg, fdwCommand, dwParam))
        return retval;

    __asm {
        push dwParam
        push fdwCommand
        push uMsg
        push IDDevice
        call fpWinMMHookList[41 * 4]
        mov [retval], eax
    }
    
    HookAfter_mciSendCommandA(_ReturnAddress(), &retval, IDDevice, uMsg, fdwCommand, dwParam);
    return retval;
}
