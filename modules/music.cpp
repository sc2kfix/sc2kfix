// sc2kfix modules/music.cpp: new music engine
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>

#define MUS_DEBUG_SONGS 1
#define MUS_DEBUG_THREAD 2
#define MUS_DEBUG_SEQUENCER 4

#define MUS_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MUS_DEBUG
#define MUS_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mus_debug = MUS_DEBUG;

static DWORD dwDummy;

std::vector<int> vectorRandomSongIDs = { 10001, 10004, 10008, 10012, 10018, 10003, 10007, 10011, 10013 };
int iCurrentSong = 0;
DWORD dwMusicThreadID;
MCIDEVICEID mciDevice = NULL;

void MusicShufflePlaylist(int iLastSongPlayed) {
	if (bSettingsShuffleMusic) {
		do {
			std::shuffle(vectorRandomSongIDs.begin(), vectorRandomSongIDs.end(), mtMersenneTwister);
		} while (vectorRandomSongIDs[0] == iLastSongPlayed);

		if (mus_debug & MUS_DEBUG_SONGS)
			ConsoleLog(LOG_DEBUG, "MCI:  Shuffled song list (next song will be %i).\n", vectorRandomSongIDs[iCurrentSong]);
	}
}

DWORD WINAPI MusicMCINotifyCallback(WPARAM wFlags, LPARAM lDevID) {
	if (wFlags & MCI_NOTIFY_SUCCESSFUL) {
		PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
		if (mus_debug & MUS_DEBUG_THREAD)
			ConsoleLog(LOG_DEBUG, "MUS:  MusicMCINotifyCallback posted WM_MUSIC_STOP.\n");
	}
	return 0;
}

DWORD WINAPI MusicThread(LPVOID lpParameter) {
	MSG msg;
	MCIERROR dwMCIError = NULL;

	MCIDEVICEID mciDeviceList[19] = { 0 };
	// test to see how many of these things we can load at once
	/*ConsoleLog(LOG_INFO, "MUS:  Starting MCI load test.\n");
	DWORD dwStartTicks = GetTickCount();
	for (int i = 0; i < 19; i++) {
		std::string strSongPath = "sounds\\";      // szSoundsPath
		strSongPath += std::to_string(i + 10000);
		strSongPath += ".mp3";

		MCI_OPEN_PARMS mciOpenParms = { NULL, NULL, "mpegvideo", strSongPath.c_str(), NULL};
		dwMCIError = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms);
		if (dwMCIError) {
			char szErrorBuf[MAXERRORLENGTH];
			mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
			ConsoleLog(LOG_ERROR, "MUS:  Test MCI_OPEN of %i.mp3 failed, 0x%08X (%s)\n", i + 10000, dwMCIError, szErrorBuf);
			continue;
		}
		mciDeviceList[i] = mciOpenParms.wDeviceID;
		ConsoleLog(LOG_INFO, "MUS:  Test %i.mp3 loaded into device ID %i.\n", i + 10000, mciOpenParms.wDeviceID);
	}
	ConsoleLog(LOG_INFO, "MUS:  MCI load test took %i milliseconds.\n", GetTickCount() - dwStartTicks);*/

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_MUSIC_STOP) {
			if (!mciDevice)
				goto next;

			dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);

			if (mus_debug & MUS_DEBUG_THREAD)
				ConsoleLog(LOG_DEBUG, "MUS:  Sent MCI_CLOSE to mciDevice 0x%08X.\n", mciDevice);

			if (dwMCIError) {
				char szErrorBuf[MAXERRORLENGTH];
				mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
				if (dwMCIError == 0x101 && mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
				else
					ConsoleLog(LOG_ERROR, "MUS:  MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
				goto next;
			}
			mciDevice = NULL;
		}
		else if (msg.message == WM_MUSIC_PLAY) {
			if (bOptionsMusicEnabled && !mciDevice) {
				if (msg.wParam >= 10000 && msg.wParam <= 10018) {
					std::string strSongPath = (char*)0x4CDB88;      // szSoundsPath
					strSongPath += std::to_string(msg.wParam);
					if (bSettingsUseMP3Music)
						strSongPath += ".mp3";
					else
						strSongPath += ".mid";

					MCI_OPEN_PARMS mciOpenParms = { NULL, NULL, (bSettingsUseMP3Music ? "mpegvideo" : "sequencer"), strSongPath.c_str(), NULL };
					dwMCIError = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms);
					if (dwMCIError) {
						char szErrorBuf[MAXERRORLENGTH];
						mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
						ConsoleLog(LOG_ERROR, "MUS:  MCI_OPEN failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
						goto next;
					}

					if (mus_debug & MUS_DEBUG_THREAD)
						ConsoleLog(LOG_DEBUG, "MUS:  Received mciDevice 0x%08X from MCI_OPEN.\n", mciDevice);

					mciDevice = mciOpenParms.wDeviceID;
					MCI_PLAY_PARMS mciPlayParms = { (DWORD_PTR)GameGetRootWindowHandle(), NULL, NULL};
					dwMCIError = mciSendCommand(mciDevice, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciPlayParms);
					// SC2K sometimes tries to run over its own sequencer device. We ignore the
					// error that causes (0x151) just like the game itself does.
					if (dwMCIError && dwMCIError != 0x151) {
						char szErrorBuf[MAXERRORLENGTH];
						mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
						ConsoleLog(LOG_ERROR, "MUS:  MCI_PLAY failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
						goto next;
					}
				}
			}
			else if (bOptionsMusicEnabled) {
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  WM_MUSIC_PLAY message received but MCI is still active; discarding message.\n");
				goto next;
			}
		}
		else if (msg.message == WM_APP + 3) {
			ConsoleLog(LOG_DEBUG, "MUS:  Hello from the music thread!\n");
		}
		else if (msg.message == WM_QUIT)
			break;

	next:
		DispatchMessage(&msg);
	}

	if (mciDevice)
		mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
	ConsoleLog(LOG_INFO, "MUS:  Shutting down music thread.\n");

	return EXIT_SUCCESS;
}

extern "C" int __stdcall Hook_MusicPlay(int iSongID) {
	DWORD *pThis;
	__asm mov [pThis], ecx

	// Certain songs should interrupt others
	switch (iSongID) {
	case 10002:
	case 10005:
	case 10010:
	case 10012:
		PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
		if (mus_debug & MUS_DEBUG_THREAD)
			ConsoleLog(LOG_DEBUG, "MUS:  Hook_MusicPlay posted WM_MUSIC_STOP.\n");
		break;
	}

	// Post the play message to the music thread
	PostThreadMessage(dwMusicThreadID, WM_MUSIC_PLAY, iSongID, NULL);
	if (mus_debug & MUS_DEBUG_THREAD)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_MusicPlay posted WM_MUSIC_PLAY for iSongID = %u.\n", iSongID);

	// Restore "this" and leave
	__asm {
		mov ecx, [pThis]
		mov eax, 1
	}
}

extern "C" int __stdcall Hook_MusicStop(void) {
	DWORD *pThis;
	__asm mov [pThis], ecx

	// Post the stop message to the music thread
	PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
	if (mus_debug & MUS_DEBUG_THREAD)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_MusicStop posted WM_MUSIC_STOP.\n");

	// Restore "this" and leave
	__asm {
		mov ecx, [pThis]
		xor eax, eax
	}
}

// Replaces the original MusicPlayNextRefocusSong
extern "C" int __stdcall Hook_MusicPlayNextRefocusSong(void) {
	DWORD *pThis;
	int retval, iSongToPlay;

	// This is actually a __thiscall we're overriding, so save "this"
	__asm mov [pThis], ecx

	// Fix for the wrong song being played after the intro video
	if (_ReturnAddress() == (void*)0x4061EE || _ReturnAddress() == (void*)0x425444) {
		if (mus_debug & MUS_DEBUG_SONGS)
			ConsoleLog(LOG_DEBUG, "MUS:  Forcing song 10001 for call returning to 0x%08X.\n", (DWORD)_ReturnAddress());
		return Game_MusicPlay(pThis, 10001);
	}

	iSongToPlay = vectorRandomSongIDs[iCurrentSong++];
	if (mus_debug & MUS_DEBUG_SONGS)
		ConsoleLog(LOG_DEBUG, "MUS:  Playing song %i (next iCurrentSong will be %i).\n", iSongToPlay, (iCurrentSong > 8 ? 0 : iCurrentSong));

	retval = Game_MusicPlay(pThis, iSongToPlay);

	// Loop and/or shuffle.
	if (iCurrentSong > 8) {
		iCurrentSong = 0;

		// Shuffle the songs, making sure we don't get the same one twice in a row
		MusicShufflePlaylist(iSongToPlay);
	}

	__asm mov eax, [retval]
}

void InstallMusicEngineHooks(void) {
	// Restore additional music
	VirtualProtect((LPVOID)0x401A9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401A9B, Hook_MusicPlayNextRefocusSong);

	// Shuffle music if the shuffle setting is enabled
	MusicShufflePlaylist(0);

	// Replace music functions with ones to post messages to the music thread
	if (bSettingsUseMultithreadedMusic) {
		VirtualProtect((LPVOID)0x402414, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402414, Hook_MusicPlay);
		VirtualProtect((LPVOID)0x402BE4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402BE4, Hook_MusicStop);
		VirtualProtect((LPVOID)0x4D2BFC, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(DWORD*)0x4D2BFC = (DWORD)MusicMCINotifyCallback;
	}
}