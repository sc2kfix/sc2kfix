// sc2kfix hooks/hook_mciSendCommand.cpp: hook for mciSendCommandA
// (c) 2025 github.com/araxestroy - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>

#pragma intrinsic(_ReturnAddress)

#define MCI_DEBUG_CALLS 1
#define MCI_DEBUG_DUMPS 2
#define MCI_DEBUG_SONGS 4
#define MCI_DEBUG_THREAD 8

#define MCI_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MCI_DEBUG
#define MCI_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mci_debug = MCI_DEBUG;

std::vector<int> vectorRandomSongIDs = { 10001, 10004, 10008, 10012, 10018, 10003, 10007, 10011, 10013 };
int iCurrentSong = 0;
DWORD dwMusicThreadID;
MCIDEVICEID mciDevice = NULL;
BOOL bUseMultithreadedMusic = TRUE;

void MusicShufflePlaylist(int iLastSongPlayed) {
    if (bSettingsShuffleMusic) {
        do {
            std::shuffle(vectorRandomSongIDs.begin(), vectorRandomSongIDs.end(), mtMersenneTwister);
        } while (vectorRandomSongIDs[0] == iLastSongPlayed);

        if (mci_debug & MCI_DEBUG_SONGS)
            ConsoleLog(LOG_DEBUG, "MCI: Shuffled song list (next song will be %i).\n", vectorRandomSongIDs[iCurrentSong]);
    }
}

DWORD WINAPI MusicMCINotifyCallback(WPARAM wFlags, LPARAM lDevID) {
    if (wFlags & MCI_NOTIFY_SUCCESSFUL) {
        PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
        if (mci_debug & MCI_DEBUG_THREAD)
            ConsoleLog(LOG_DEBUG, "MUS: MusicMCINotifyCallback posted WM_MUSIC_STOP.\n");
    }
    return 0;
}

DWORD WINAPI MusicThread(LPVOID lpParameter) {
    MSG msg;
    MCIERROR dwMCIError = NULL;
    ConsoleLog(LOG_INFO, "MUS: Music thread started.\n");

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_MUSIC_STOP) {
            if (!mciDevice)
                goto next;

            dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);

            if (mci_debug & MCI_DEBUG_THREAD)
                ConsoleLog(LOG_DEBUG, "MUS: Sent MCI_CLOSE to mciDevice 0x%08X.\n", mciDevice);

            if (dwMCIError) {
                char szErrorBuf[MAXERRORLENGTH];
                mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
                if (dwMCIError == 0x101)
                    ConsoleLog(LOG_DEBUG, "MUS: MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
                else
                    ConsoleLog(LOG_ERROR, "MUS: MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
                goto next;
            }
            mciDevice = NULL;
        } else if (msg.message == WM_MUSIC_PLAY) {
            if (bOptionsMusicEnabled && !mciDevice) {
                if (msg.wParam >= 10000 && msg.wParam <= 10018) {
                    std::string strSongPath = (char*)0x4CDB88;      // szSoundsPath
                    strSongPath += std::to_string(msg.wParam);
                    strSongPath += ".mid";

                    MCI_OPEN_PARMS mciOpenParms = { NULL, NULL, "sequencer", strSongPath.c_str(), NULL };
                    dwMCIError = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms);
                    if (dwMCIError) {
                        char szErrorBuf[MAXERRORLENGTH];
                        mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
                        ConsoleLog(LOG_ERROR, "MUS: MCI_OPEN failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
                        goto next;
                    }

                    if (mci_debug & MCI_DEBUG_THREAD)
                        ConsoleLog(LOG_DEBUG, "MUS: Received mciDevice 0x%08X from MCI_OPEN.\n", mciDevice);

                    mciDevice = mciOpenParms.wDeviceID;
                    DWORD* CWndMainWindow = (DWORD*)*(DWORD*)0x4C702C;
                    HWND hWndMainWindow = (HWND)CWndMainWindow[7];
                    MCI_PLAY_PARMS mciPlayParms = { (DWORD_PTR)hWndMainWindow, NULL, NULL };
                    dwMCIError = mciSendCommand(mciDevice, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciPlayParms);
                    // SC2K sometimes tries to run over its own sequencer device. We ignore the
                    // error that causes (0x151) just like the game itself does.
                    if (dwMCIError && dwMCIError != 0x151) {
                        char szErrorBuf[MAXERRORLENGTH];
                        mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
                        ConsoleLog(LOG_ERROR, "MUS: MCI_PLAY failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
                        goto next;
                    }
                }
            } else if (bOptionsMusicEnabled) {
                if (mci_debug & MCI_DEBUG_THREAD)
                    ConsoleLog(LOG_DEBUG, "MUS: WM_MUSIC_PLAY message received but MCI is still active; discarding message.\n");
                goto next;
            }
        } else if (msg.message == WM_APP+3) {
            ConsoleLog(LOG_DEBUG, "MUS: Hello from the music thread!\n");
        } else if (msg.message == WM_QUIT)
            break;
        
next:
        DispatchMessage(&msg);
    }

    if (mciDevice)
        mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
    ConsoleLog(LOG_INFO, "MUS: Shutting down music thread.\n");

    return EXIT_SUCCESS;
}

extern "C" int __stdcall Hook_MusicPlay(int iSongID) {
    UINT uThis;
    __asm {
        mov [uThis], ecx
    }

    // Certain songs should interrupt others
    switch (iSongID) {
    case 10002:
    case 10005:
    case 10010:
    case 10012:
        PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
        if (mci_debug & MCI_DEBUG_THREAD)
            ConsoleLog(LOG_DEBUG, "MUS: Hook_MusicPlay posted WM_MUSIC_STOP.\n");
        break;
    }

    // Post the play message to the music thread
    PostThreadMessage(dwMusicThreadID, WM_MUSIC_PLAY, iSongID, NULL);
    if (mci_debug & MCI_DEBUG_THREAD)
        ConsoleLog(LOG_DEBUG, "MUS: Hook_MusicPlay posted WM_MUSIC_PLAY for iSongID = %u.\n", iSongID);

    // Restore "this" and leave
    __asm {
        mov ecx, [uThis]
        mov eax, 1
    }
}

extern "C" int __stdcall Hook_MusicStop(void) {
    UINT uThis;
    __asm {
        mov [uThis], ecx
    }

    // Post the stop message to the music thread
    PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
    if (mci_debug & MCI_DEBUG_THREAD)
        ConsoleLog(LOG_DEBUG, "MUS: Hook_MusicStop posted WM_MUSIC_STOP.\n");

    // Restore "this" and leave
    __asm {
        mov ecx, [uThis]
        xor eax, eax
    }
}

// Replaces the original MusicPlayNextRefocusSong
extern "C" int __stdcall Hook_MusicPlayNextRefocusSong(void) {
    UINT uThis;
    int retval, iSongToPlay;
    
    // This is actually a __thiscall we're overriding, so save "this"
    __asm {
        mov [uThis], ecx
    }

    iSongToPlay = vectorRandomSongIDs[iCurrentSong++];
    if (mci_debug & MCI_DEBUG_SONGS)
        ConsoleLog(LOG_DEBUG, "MCI: Playing song %i (next iCurrentSong will be %i).\n", iSongToPlay, (iCurrentSong > 8 ? 0 : iCurrentSong));

    __asm {
        mov ecx, [uThis]
        mov edx, [iSongToPlay]
        push edx
        mov edx, 0x402414
        call edx
        mov [retval], eax
    }

    // Loop and/or shuffle.
    if (iCurrentSong > 8) {
        iCurrentSong = 0;

        // Shuffle the songs, making sure we don't get the same one twice in a row
        MusicShufflePlaylist(iSongToPlay);
    }

    __asm {
        mov eax, [retval]
    }
}

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
        return HexPls(uMsg, 8);
    }
}

extern "C" BOOL __stdcall Hook_mciSendCommandA(void* pReturnAddress, MCIERROR* retval, MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    if (mci_debug & MCI_DEBUG_CALLS)
        ConsoleLog(LOG_DEBUG, "MCI: 0x%08p -> mciSendCommand(0x%08X, %s, 0x%08X, 0x%08X)\n", pReturnAddress, IDDevice, MCIMessageIDToString(uMsg), fdwCommand, dwParam);
    switch (uMsg) {
    case MCI_OPEN: {
        if (mci_debug & (MCI_DEBUG_CALLS | MCI_DEBUG_DUMPS)) {
            MCI_OPEN_PARMS* pMCIOpenParms = (MCI_OPEN_PARMS*)dwParam;
			ConsoleLog(LOG_NONE,
                "MCI_OPEN_PARMS {\n"
				"    dwCallback       = 0x%08X,\n"
				"    wDeviceID        = 0x%08X,\n"
				"    lpstrDeviceType  = %s,\n"
				"    lpstrElementName = %s,\n"
				"    lpstrAlias       = %s\n"
				"}\n\n", pMCIOpenParms->dwCallback, pMCIOpenParms->wDeviceID,
				((UINT)(pMCIOpenParms->lpstrDeviceType) > 4096 ? pMCIOpenParms->lpstrDeviceType : HexPls((UINT)(pMCIOpenParms->lpstrDeviceType), 8)),
				pMCIOpenParms->lpstrElementName,
				((UINT)(pMCIOpenParms->lpstrAlias) == 0 ? pMCIOpenParms->lpstrAlias : "NULL"));
        }
        break;
    }

    case MCI_STATUS: {
        if (mci_debug & (MCI_DEBUG_CALLS | MCI_DEBUG_DUMPS)) {
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
        if (mci_debug & (MCI_DEBUG_CALLS | MCI_DEBUG_DUMPS)) {
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
        if (mci_debug & MCI_DEBUG_CALLS)
            ConsoleLog(LOG_WARNING, "MCA: mciSendCommand sent with unexpected uMsg %s.\n", MCIMessageIDToString(uMsg));
    }

    *retval = 0;
    return TRUE;
}

extern "C" BOOL __stdcall HookAfter_mciSendCommandA(void* pReturnAddress, MCIERROR* retval, MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    return TRUE;
}

extern "C" MCIERROR __stdcall _mciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
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
