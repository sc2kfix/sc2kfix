// sc2kfix modules/music.cpp: new music engine
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>

UINT mus_debug = MUS_DEBUG;

static DWORD dwDummy;

static int iPlayingSongID = 0;
static MCIDEVICEID mciDevice = -1;
static bool bMusicForceIntroSongOnce = true;

std::vector<int> vectorRandomSongIDs = { 10001, 10004, 10008, 10012, 10018, 10003, 10007, 10011, 10013, 10017 };
int iCurrentSong = 0;
DWORD dwMusicThreadID = 0;

const char *GetGameSoundPath() {
	return szSoundPath;
}

static void SetCurrentActiveSongID(int iSongID) {
	iPlayingSongID = iSongID;
}

int GetCurrentActiveSongID() {
	return iPlayingSongID;
}

void SetMCIDevID(__int16 wMCIDevID) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSound *pSound = pSCApp->SCASNDLayer;

	if (pSCApp && pSound) {
		pSound->wSNDMCIDevID = wMCIDevID;
	}
}

void SetSongPlaying(bool bPlaying) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSound *pSound = pSCApp->SCASNDLayer;

	if (pSCApp && pSound) {
		pSound->dwSNDMusPlaying = (bPlaying) ? 1 : 0;
	}
}

bool IsSongPlaying() {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSound *pSound = pSCApp->SCASNDLayer;

	if (pSCApp && pSound)
		return (pSound->dwSNDMusPlaying) ? true : false;
	return false;
}

static void MusicShufflePlaylist(int iLastSongPlayed) {
	if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SHUFFLEMUSIC].ToBool()) {
		do {
			std::shuffle(vectorRandomSongIDs.begin(), vectorRandomSongIDs.end(), mtMersenneTwister);
		} while (vectorRandomSongIDs[0] == iLastSongPlayed);

		if (mus_debug & MUS_DEBUG_SONGS)
			ConsoleLog(LOG_DEBUG, "MUS:  Shuffled song list (next song will be %i).\n", vectorRandomSongIDs[iCurrentSong]);
	}
}

const char* MusicEngineIntToString(UINT iMusicEngine) {
	switch (iMusicEngine) {
	case MUSIC_ENGINE_NONE:
		return "none";
	case MUSIC_ENGINE_SEQUENCER:
		return "sequencer";
	case MUSIC_ENGINE_FLUIDSYNTH:
		return "fluidsynth";
	case MUSIC_ENGINE_MP3:
		return "mp3";
	default:
		return "sequencer";
	}
}

UINT MusicEngineStringToInt(const char* szMusicEngine) {
	if (!strcmp(szMusicEngine, "none"))
		return MUSIC_ENGINE_NONE;
	if (!strcmp(szMusicEngine, "sequencer"))
		return MUSIC_ENGINE_SEQUENCER;
	if (!strcmp(szMusicEngine, "fluidsynth"))
		return MUSIC_ENGINE_FLUIDSYNTH;
	if (!strcmp(szMusicEngine, "mp3"))
		return MUSIC_ENGINE_MP3;
	return MUSIC_ENGINE_SEQUENCER;
}

static void WINAPI MusicMCINotifyCallback(WPARAM wFlags, LPARAM lDevID) {
	if (wFlags == MCI_NOTIFY_SUCCESSFUL || wFlags == MCI_NOTIFY_FAILURE) {
		if (dwMusicThreadID)
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
		if (mus_debug & MUS_DEBUG_THREAD)
			ConsoleLog(LOG_DEBUG, "MUS:  MusicMCINotifyCallback posted WM_MUSIC_STOP. (%u, %u)\n", wFlags, lDevID);
	}
}

static int GetAliasIndexFromSongID(int iSongID) {
	if (iSongID < 10000 || iSongID > 10018)
		return -1;

	return iSongID - 10000;
}

static BOOL ValidateFilename(const char *pName, const char *pExt) {
	unsigned nNameLen, nExtLen;

	if (!pName)
		return FALSE;

	if (!pExt)
		return FALSE;

	// No going up a level, or just referencing the current level.
	if (strstr(pName, "..") || _stricmp(pName, "..") == 0 || _stricmp(pName, ".") == 0)
		return FALSE;

	// No path level or drive letter separation characters.
	if (strchr(pName, '\\') || strchr(pName, '/') || strchr(pName, ':'))
		return FALSE;

	// Further validation
	if (!IsFileNameValid(pName))
		return FALSE;

	// No
	nExtLen = strlen(pExt);
	if (nExtLen <= 1)
		return FALSE;

	// The first extension character must be '.'
	if (pExt[0] != '.')
		return FALSE;

	// Name length must be greater than the extension length.
	nNameLen = strlen(pName);
	if (nNameLen > nExtLen) {
		nNameLen -= nExtLen;
		if (_strnicmp(pName + nNameLen, pExt, nExtLen) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

const char *GetGameMusicSoundPath(BOOL bDoMP3) {
	const char *pExt;
	static std::string strSongPath;
	BOOL bUseAliasedSong;
	int iAliasIdx;
	int iSongID;

	iSongID = GetCurrentActiveSongID();
	// TODO: clean up alias index stuff
	pExt = (bDoMP3) ? ".mp3" : ".mid";
	strSongPath = szSoundPath;
	bUseAliasedSong = FALSE;
	iAliasIdx = GetAliasIndexFromSongID(iSongID);
	if (iAliasIdx >= 0 && iAliasIdx <= MUSIC_TRACKS) {
		char szTrackName[32];
		sprintf_s(szTrackName, sizeof(szTrackName), "100%02d", iAliasIdx);
		const char *pName = (bDoMP3) ? jsonSettingsCore[C_SC2KFIX][S_FIX_MUSMP3][szTrackName].ToString().c_str() : jsonSettingsCore[C_SC2KFIX][S_FIX_MUSMID][szTrackName].ToString().c_str();
		if (ValidateFilename(pName, pExt)) {
			strSongPath += pName;
			if (FileExists(strSongPath.c_str()))
				bUseAliasedSong = TRUE;
		}
	}

	if (!bUseAliasedSong) {
		// Set back to szSoundPath before appending.
		strSongPath = szSoundPath;
		strSongPath += std::to_string(iSongID);
		strSongPath += pExt;
	}

	return strSongPath.c_str();
}

DWORD WINAPI MusicThread(LPVOID lpParameter) {
	CSimcityAppPrimary *pSCApp;
	MSG msg;
	MCIERROR dwMCIError = NULL;

	if (mus_debug & MUS_DEBUG_THREAD)
		ConsoleLog(LOG_DEBUG, "MUS:  Starting music engine! Initial music driver set to \"%s\".\n", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString().c_str());
	
	while (GetMessage(&msg, NULL, 0, 0)) {
		pSCApp = &pCSimcityAppThis;
		if (msg.message == WM_MUSIC_STOP) {
			// Log a debug message at best if the music engine is set to none.
			// In this case even with the engine set to MUSIC_ENGINE_NONE it
			// will still continue, however once mciDevice is NULL it'll then
			// getout.
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "none")
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Music driver set to None; ignoring WM_MUSIC_STOP message.\n");

			// Stop the FluidSynth thread if it's active
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "fluidsynth") {
				if (hmodFluidSynth) {
					PostThreadMessageA(dwFSMIDIThreadID, WM_FS_STOP, 0, 0);
					if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
						ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped FluidSynth player due to WM_MUSIC_STOP message.\n");
					goto next;
				}
			}

			// If we're playing an MP3, stop it and unload it from memory
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "mp3") {
				PostThreadMessageA(dwSDLSongThreadID, WM_SDL_STOP, 0, 0);
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped MP3 playback due to WM_MUSIC_STOP message.\n");
				goto next;
			}

			// Failing the above, run what we need to through the MCI interface
			if (mciDevice == -1)
				goto next;

			dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
			SetCurrentActiveSongID(0);

			if (mus_debug & MUS_DEBUG_THREAD)
				ConsoleLog(LOG_DEBUG, "MUS:  Sent MCI_CLOSE to mciDevice 0x%08X.\n", mciDevice);

			if (dwMCIError) {
				char szErrorBuf[MAXERRORLENGTH];
				mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
				if (dwMCIError == MCIERR_INVALID_DEVICE_ID && mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
				else
					ConsoleLog(LOG_ERROR, "MUS:  MCI_CLOSE failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
				goto next;
			}
			mciDevice = -1;
			SetMCIDevID(mciDevice);
			SetSongPlaying(false);
		}
		else if (msg.message == WM_MUSIC_PLAY) {
			// Log a debug message at best if the music engine is set to none
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "none") {
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Music driver set to None; ignoring WM_MUSIC_PLAY message.\n");
				goto next;
			}

			// If we're using FluidSynth, set up a watchdog thread that runs the FluidSynth engine
			// and waits for it to exit
			if (pSCApp->dwSCAGameMusic) {
				if (msg.wParam >= 10000 && msg.wParam <= 10018) {
					SetCurrentActiveSongID(msg.wParam);
					if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "fluidsynth") {
						if (hmodFluidSynth) {
							const char* szSongPath = GetGameMusicSoundPath(FALSE);
							if (szSongPath)
								PostThreadMessageA(dwFSMIDIThreadID, WM_FS_PLAY, msg.wParam, 0);
							goto next;
						}
						else
							ConsoleLog(LOG_ERROR, "MUS: FluidSynth not loaded; failing back to MIDI sequencer.\n");
					}

					// Attempt MP3 playback via SDL3 if we've got it selected
					if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() == "mp3") {
						const char* szSongPath = GetGameMusicSoundPath(TRUE);
						if (szSongPath) {
							PostThreadMessageA(dwSDLSongThreadID, WM_SDL_PLAY, msg.wParam, 0);
							goto next;
						}
						else
							ConsoleLog(LOG_ERROR, "MUS:  Could not find music file %s; failing back to MIDI sequencer.\n", szSongPath);
					}

					// Failing all of the above, use MCI to handle MIDI playback
					const char* szSongPath = GetGameMusicSoundPath(FALSE);
					if (!szSongPath || IsSongPlaying())
						goto next;

					if (mciDevice != -1) {
						mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
						mciDevice = -1;
						SetMCIDevID(mciDevice);
					}

					if (mciDevice == -1) {
						SetSongPlaying(true);
						MCI_OPEN_PARMS mciOpenParms = { NULL, NULL, "sequencer", szSongPath, NULL };
						dwMCIError = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms);
						if (dwMCIError) {
							char szErrorBuf[MAXERRORLENGTH];
							mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
							ConsoleLog(LOG_ERROR, "MUS:  MCI_OPEN failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
							SetCurrentActiveSongID(0);
							SetSongPlaying(false);
							goto next;
						}

						if (mus_debug & MUS_DEBUG_THREAD)
							ConsoleLog(LOG_DEBUG, "MUS:  Received mciDevice 0x%08X from MCI_OPEN.\n", mciDevice);

						mciDevice = mciOpenParms.wDeviceID;
						MCI_PLAY_PARMS mciPlayParms = { (DWORD_PTR)GameGetRootWindowHandle(), NULL, NULL };
						dwMCIError = mciSendCommand(mciDevice, MCI_PLAY, MCI_NOTIFY, (DWORD_PTR)&mciPlayParms);
						// SC2K sometimes tries to run over its own sequencer device. We ignore the
						// error that causes (0x151, MCIERR_SEQ_PORT_INUSE) just like the game itself does.
						if (dwMCIError && dwMCIError != MCIERR_SEQ_PORT_INUSE) {
							char szErrorBuf[MAXERRORLENGTH];
							mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
							ConsoleLog(LOG_ERROR, "MUS:  MCI_PLAY failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
							SetCurrentActiveSongID(0);
							SetSongPlaying(false);
							goto next;
						}

						SetMCIDevID(mciDevice);
						SetSongPlaying(true);
					}
					else {
						if (mus_debug & MUS_DEBUG_THREAD)
							ConsoleLog(LOG_DEBUG, "MUS:  WM_MUSIC_PLAY message received but MCI is still active; discarding message.\n");
					}
				}
			}
		}
		else if (msg.message == WM_MUSIC_RESET) {
			// Attempt to hard stop all music engines, since our music engine has probably changed
			if (hmodFluidSynth) {
				PostThreadMessageA(dwFSMIDIThreadID, WM_FS_STOP, 0, 0);
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped FluidSynth player due to WM_MUSIC_RESET message.\n");
			}

			if (pStreamCurrentSong) {
				PostThreadMessageA(dwSDLSongThreadID, WM_SDL_STOP, 0, 0);
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped MP3 player due to WM_MUSIC_RESET message.\n");
			}

			if (mciDevice != -1) {
				dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped mciDevice 0x%08X due to WM_MUSIC_RESET message.\n", mciDevice);
			}

			mciDevice = -1;
			SetMCIDevID(mciDevice);
			SetSongPlaying(false);

			// Only restart if the engine is not set to MUSIC_ENGINE_NONE
			if (jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICDRIVER].ToString() != "none") {
				// Restart the active song if there is one
				int iSongID = GetCurrentActiveSongID();
				if (iSongID)
					Game_SimcityApp_MusicPlay(pSCApp, iSongID);
			}
		}
		else if (msg.message == WM_QUIT)
			break;

	next:
		DispatchMessage(&msg);
	}

	if (mciDevice != -1) {
		mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
		mciDevice = -1;
	}
	SetMCIDevID(mciDevice);
	SetSongPlaying(false);
	ConsoleLog(LOG_INFO, "MUS:  Shutting down music thread.\n");

	return EXIT_SUCCESS;
}

extern "C" void __stdcall Hook_MainFrame_OnMCINotify(WPARAM wFlags, LPARAM lDevID) {
	CMainFrame *pThis;
	__asm mov [pThis], ecx

	MusicMCINotifyCallback(wFlags, lDevID);
}

extern "C" void __stdcall Hook_SimcityApp_MusicPlay(int iSongID) {
	CSimcityAppPrimary *pThis;
	__asm mov [pThis], ecx

	if (pThis->dwSCAGameMusic) {
		// Always do this.
		if (dwMusicThreadID)
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
		if (mus_debug & MUS_DEBUG_THREAD)
			ConsoleLog(LOG_DEBUG, "MUS:  Hook_SimcityApp_MusicPlay posted WM_MUSIC_STOP.\n");

		// Post the play message to the music thread
		if (dwMusicThreadID)
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_PLAY, iSongID, NULL);
		if (mus_debug & MUS_DEBUG_THREAD)
			ConsoleLog(LOG_DEBUG, "MUS:  Hook_SimcityApp_MusicPlay posted WM_MUSIC_PLAY for iSongID = %u.\n", iSongID);
	}
}

extern "C" void __stdcall Hook_Sound_MusicStop(void) {
	CSound *pThis;
	__asm mov [pThis], ecx

	// Post the stop message to the music thread
	if (dwMusicThreadID)
		PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
	if (mus_debug & MUS_DEBUG_THREAD)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_Sound_MusicStop posted WM_MUSIC_STOP.\n");
}

// Replaces the original MusicPlayNextRefocusSong
extern "C" void __stdcall Hook_SimcityApp_MusicPlayNextRefocusSong(void) {
	CSimcityAppPrimary *pThis;
	int iSongToPlay;

	// This is actually a __thiscall we're overriding, so save "this"
	__asm mov [pThis], ecx

	iSongToPlay = vectorRandomSongIDs[iCurrentSong++];
	if (mus_debug & MUS_DEBUG_SONGS)
		ConsoleLog(LOG_DEBUG, "MUS:  Playing song %i (next iCurrentSong will be %i).\n", iSongToPlay, (iCurrentSong > 9 ? 0 : iCurrentSong));

	Game_SimcityApp_MusicPlay(pThis, iSongToPlay);

	// Loop and/or shuffle if needed or if the intro music is done being played.
	if (iCurrentSong > 9 || bMusicForceIntroSongOnce) {
		iCurrentSong = 0;

		if (bMusicForceIntroSongOnce)
			bMusicForceIntroSongOnce = false;

		// Shuffle the songs, making sure we don't get the same one twice in a row
		MusicShufflePlaylist(iSongToPlay);
	}
}

extern "C" void __stdcall Hook_SimcityApp_MusicPlayNext(BOOL bNext) {
	CSimcityAppPrimary *pThis;

	__asm mov [pThis], ecx

	int nSpeed;
	int iRandMusic;
	int iSongID;

	if (!pThis->dwSCAGameMusic)
		return;
	nSpeed = pThis->wSCAGameSpeedLOW;
	if (nSpeed == GAME_SPEED_PAUSED)
		nSpeed = GAME_SPEED_TURTLE;
	if (!Game_Sound_IsMusicPlaying(pThis->SCASNDLayer)) {
		if (bNext)
			Game_SimcityApp_MusicPlayNextRefocusSong(pThis);
		else if ((!(rand() % (8 * (3 * nSpeed - 3)))) || jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_ALWAYSPLAYMUSIC].ToBool()) {
			iRandMusic = rand();
			iSongID = 10000 + (iRandMusic % 19);
			Game_SimcityApp_MusicPlay(pThis, iSongID);
		}
	}
}

void InstallMusicEngineHooks(void) {
	// Restore additional music
	SafeVirtualProtect((LPVOID)0x401A9B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401A9B, Hook_SimcityApp_MusicPlayNextRefocusSong);

	// Hook for CSimcityApp::MusicPlayNext
	SafeVirtualProtect((LPVOID)0x402AEF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402AEF, Hook_SimcityApp_MusicPlayNext);

	// Replace music functions with ones to post messages to the music thread
	SafeVirtualProtect((LPVOID)0x40145B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40145B, Hook_MainFrame_OnMCINotify);
	SafeVirtualProtect((LPVOID)0x402414, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402414, Hook_SimcityApp_MusicPlay);
	SafeVirtualProtect((LPVOID)0x402BE4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402BE4, Hook_Sound_MusicStop);
}
