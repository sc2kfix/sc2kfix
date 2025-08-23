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
#include <fluidsynth.h>

#define MUS_DEBUG_SONGS 1
#define MUS_DEBUG_THREAD 2
#define MUS_DEBUG_SEQUENCER 4
#define MUS_DEBUG_FLUIDSYNTH 8

#define MUS_DEBUG DEBUG_FLAGS_EVERYTHING

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
BOOL bMultithreadedMusicEnabled = FALSE;
BOOL bMusicFluidSynthEnabled = FALSE;
BOOL bUseFluidSynth = FALSE;
char szMusicFluidSynthSoundfontPath[MAX_PATH + 1] = { 0 };

HMODULE hmodFluidSynth = NULL;
fluid_settings_t* pFluidSynthSettings = NULL;
fluid_synth_t* pFluidSynthSynth = NULL;
fluid_player_t* pFluidSynthPlayer = NULL;
fluid_audio_driver_t* pFluidSynthDriver = NULL;
BOOL bFluidSynthPlaying = FALSE;

// FluidSynth imports -- note that because we don't actually link against libfluidsynth-3.lib we
// can't use the functions in the FluidSynth headers.

fluid_settings_t* (*FS_new_fluid_settings)(void) = NULL;
fluid_synth_t* (*FS_new_fluid_synth)(fluid_settings_t* settings) = NULL;
fluid_player_t* (*FS_new_fluid_player)(fluid_synth_t* synth) = NULL;
fluid_audio_driver_t* (*FS_new_fluid_audio_driver)(fluid_settings_t* settings, fluid_synth_t* synth) = NULL;
fluid_midi_router_t* (*FS_new_fluid_midi_router)(fluid_settings_t* settings, handle_midi_event_func_t handler, void* event_handler_data) = NULL;
int (*FS_fluid_synth_handle_midi_event)(void* data, fluid_midi_event_t* event) = NULL;
int (*FS_fluid_is_soundfont)(const char* filename) = NULL;
int (*FS_fluid_synth_sfload)(fluid_synth_t* synth, const char* filename, int reset_presets) = NULL;
int (*FS_fluid_player_add)(fluid_player_t* player, const char* midifile) = NULL;
int (*FS_fluid_player_play)(fluid_player_t* player) = NULL;
int (*FS_fluid_player_stop)(fluid_player_t* player) = NULL;
int (*FS_fluid_player_join)(fluid_player_t* player) = NULL;
int (*FS_fluid_player_get_status)(fluid_player_t* player) = NULL;
fluid_midi_router_rule_t* (*FS_new_fluid_midi_router_rule)(void) = NULL;
int (*FS_fluid_midi_router_clear_rules)(fluid_midi_router_t* router) = NULL;
void (*FS_fluid_midi_router_rule_set_chan)(fluid_midi_router_rule_t* rule, int min, int max, float mul, int add) = NULL;
void (*FS_fluid_midi_router_rule_set_param1)(fluid_midi_router_rule_t* rule, int min, int max, float mul, int add) = NULL;
void (*FS_fluid_midi_router_rule_set_param2)(fluid_midi_router_rule_t* rule, int min, int max, float mul, int add) = NULL;
int (*FS_fluid_midi_router_add_rule)(fluid_midi_router_t* router, fluid_midi_router_rule_t* rule, int type) = NULL;
void (*FS_delete_fluid_midi_router)(fluid_midi_router_t* router) = NULL;
void (*FS_delete_fluid_audio_driver)(fluid_audio_driver_t* driver) = NULL;
void (*FS_delete_fluid_player)(fluid_player_t* player) = NULL;
void (*FS_delete_fluid_synth)(fluid_synth_t* synth) = NULL;
void (*FS_delete_fluid_settings)(fluid_settings_t* settings) = NULL;
int (*FS_fluid_midi_event_get_type)(fluid_midi_event_t* event) = NULL;
int (*FS_fluid_midi_event_get_channel)(fluid_midi_event_t* event) = NULL;
int (*FS_fluid_player_set_playback_callback)(fluid_player_t* player, handle_midi_event_func_t handler, void* handler_data) = NULL;
int (*FS_fluid_synth_all_sounds_off)(fluid_synth_t* synth, int chan) = NULL;
fluid_log_function_t (*FS_fluid_set_log_function)(int level, fluid_log_function_t fun, void* data);

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

int MusicFluidSynthMidiEventHandler(void* data, fluid_midi_event_t* event) {
	fluid_synth_t* synth = (fluid_synth_t*)data;
	int type = FS_fluid_midi_event_get_type(event);
	int channel = FS_fluid_midi_event_get_channel(event);

	// Ignore all notes on channel 15; pass other notes. This works around the fact that the MIDI
	// files have the drum track duplicated on channel 15, which FluidSynth defaults to being a
	// full volume Acoustic Grand Piano track (plink plonk plink plonk i'm a drum machine!)
	if (type == 0x90 && channel == 15)
		return FLUID_OK;
	return FS_fluid_synth_handle_midi_event(data, event);
}

void MusicFluidSynthLoggerError(int level, const char* message, void* data) {
	ConsoleLog(LOG_ERROR, "MUS:  FluidSynth: %s\n", message);
}

void MusicFluidSynthLoggerWarning(int level, const char* message, void* data) {
	ConsoleLog(LOG_WARNING, "MUS:  FluidSynth: %s\n", message);
}

void MusicFluidSynthLoggerNull(int level, const char* message, void* data) {
	return;
}

BOOL MusicLoadFluidSynth(void) {
	if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  FluidSynth enabled, probing for valid FluidSynth library.\n");

	// Attempt to load the FluidSynth library.
	hmodFluidSynth = LoadLibrary("libfluidsynth-3.dll");
	if (!hmodFluidSynth) {
		ConsoleLog(LOG_ERROR, "MUS:  FluidSynth could not be loaded (error 0x%08X). Disabling FluidSynth.\n", GetLastError());
		return FALSE;
	}

	if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  FluidSynth loaded at address 0x%08X.\n", hmodFluidSynth);

	// Signal our intent to use FluidSynth and start locating function addresses.
	bUseFluidSynth = TRUE;
	if ((FS_new_fluid_settings = (decltype(FS_new_fluid_settings))GetProcAddress(hmodFluidSynth, "new_fluid_settings")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_new_fluid_synth = (decltype(FS_new_fluid_synth))GetProcAddress(hmodFluidSynth, "new_fluid_synth")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_new_fluid_player = (decltype(FS_new_fluid_player))GetProcAddress(hmodFluidSynth, "new_fluid_player")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_new_fluid_audio_driver = (decltype(FS_new_fluid_audio_driver))GetProcAddress(hmodFluidSynth, "new_fluid_audio_driver")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_new_fluid_midi_router = (decltype(FS_new_fluid_midi_router))GetProcAddress(hmodFluidSynth, "new_fluid_midi_router")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_synth_handle_midi_event = (decltype(FS_fluid_synth_handle_midi_event))GetProcAddress(hmodFluidSynth, "fluid_synth_handle_midi_event")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_is_soundfont = (decltype(FS_fluid_is_soundfont))GetProcAddress(hmodFluidSynth, "fluid_is_soundfont")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_synth_sfload = (decltype(FS_fluid_synth_sfload))GetProcAddress(hmodFluidSynth, "fluid_synth_sfload")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_add = (decltype(FS_fluid_player_add))GetProcAddress(hmodFluidSynth, "fluid_player_add")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_play = (decltype(FS_fluid_player_play))GetProcAddress(hmodFluidSynth, "fluid_player_play")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_stop = (decltype(FS_fluid_player_stop))GetProcAddress(hmodFluidSynth, "fluid_player_stop")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_join = (decltype(FS_fluid_player_join))GetProcAddress(hmodFluidSynth, "fluid_player_join")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_get_status = (decltype(FS_fluid_player_get_status))GetProcAddress(hmodFluidSynth, "fluid_player_get_status")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_new_fluid_midi_router_rule = (decltype(FS_new_fluid_midi_router_rule))GetProcAddress(hmodFluidSynth, "new_fluid_midi_router_rule")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_router_clear_rules = (decltype(FS_fluid_midi_router_clear_rules))GetProcAddress(hmodFluidSynth, "fluid_midi_router_clear_rules")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_router_rule_set_chan = (decltype(FS_fluid_midi_router_rule_set_chan))GetProcAddress(hmodFluidSynth, "fluid_midi_router_rule_set_chan")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_router_rule_set_param1 = (decltype(FS_fluid_midi_router_rule_set_param1))GetProcAddress(hmodFluidSynth, "fluid_midi_router_rule_set_param1")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_router_rule_set_param2 = (decltype(FS_fluid_midi_router_rule_set_param2))GetProcAddress(hmodFluidSynth, "fluid_midi_router_rule_set_param2")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_router_add_rule = (decltype(FS_fluid_midi_router_add_rule))GetProcAddress(hmodFluidSynth, "fluid_midi_router_add_rule")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_delete_fluid_midi_router = (decltype(FS_delete_fluid_midi_router))GetProcAddress(hmodFluidSynth, "delete_fluid_midi_router")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_delete_fluid_audio_driver = (decltype(FS_delete_fluid_audio_driver))GetProcAddress(hmodFluidSynth, "delete_fluid_audio_driver")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_delete_fluid_player = (decltype(FS_delete_fluid_player))GetProcAddress(hmodFluidSynth, "delete_fluid_player")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_delete_fluid_synth = (decltype(FS_delete_fluid_synth))GetProcAddress(hmodFluidSynth, "delete_fluid_synth")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_delete_fluid_settings = (decltype(FS_delete_fluid_settings))GetProcAddress(hmodFluidSynth, "delete_fluid_settings")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_event_get_type = (decltype(FS_fluid_midi_event_get_type))GetProcAddress(hmodFluidSynth, "fluid_midi_event_get_type")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_midi_event_get_channel = (decltype(FS_fluid_midi_event_get_channel))GetProcAddress(hmodFluidSynth, "fluid_midi_event_get_channel")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_player_set_playback_callback = (decltype(FS_fluid_player_set_playback_callback))GetProcAddress(hmodFluidSynth, "fluid_player_set_playback_callback")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_synth_all_sounds_off = (decltype(FS_fluid_synth_all_sounds_off))GetProcAddress(hmodFluidSynth, "fluid_synth_all_sounds_off")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_set_log_function = (decltype(FS_fluid_set_log_function))GetProcAddress(hmodFluidSynth, "fluid_set_log_function")) == NULL)
		bUseFluidSynth = FALSE;

	// If any of our loads failed, release the FluidSynth library, throw an error, and fall 
	// back to using MCI.
	if (!bUseFluidSynth) {
		ConsoleLog(LOG_ERROR, "MUS:  One or more FluidSynth functions could not be loaded. Disabling FluidSynth.\n");
		FreeLibrary(hmodFluidSynth);
		return FALSE;
	}

	if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Loaded all 27 FluidSynth function pointers.\n");

	// Create initial FluidSynth data
	pFluidSynthSettings = FS_new_fluid_settings();
	pFluidSynthSynth = FS_new_fluid_synth(pFluidSynthSettings);
	FS_fluid_set_log_function(FLUID_PANIC, MusicFluidSynthLoggerError, NULL);
	FS_fluid_set_log_function(FLUID_ERR, MusicFluidSynthLoggerError, NULL);
	FS_fluid_set_log_function(FLUID_WARN, MusicFluidSynthLoggerWarning, NULL);
	FS_fluid_set_log_function(FLUID_INFO, MusicFluidSynthLoggerNull, NULL);

	return TRUE;
}

DWORD WINAPI FluidSynthWatchdogThread(LPVOID lpParameter) {
	const char* szSongPath = (const char*)lpParameter;

	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Starting new FluidSynth watchdog thread.\n");
	
	// Stop and delete the existing player-driver combo if it exists
	if (pFluidSynthPlayer) {
		FS_fluid_player_stop(pFluidSynthPlayer);
		for (int i = 0; i < 16; i++)
			FS_fluid_synth_all_sounds_off(pFluidSynthSynth, i);
		FS_delete_fluid_audio_driver(pFluidSynthDriver);
		FS_delete_fluid_player(pFluidSynthPlayer);
	}

	// Spin up a new player-driver combo
	pFluidSynthPlayer = FS_new_fluid_player(pFluidSynthSynth);
	if (FS_fluid_is_soundfont(szMusicFluidSynthSoundfontPath))
		FS_fluid_synth_sfload(pFluidSynthSynth, szMusicFluidSynthSoundfontPath, 1);
	FS_fluid_player_set_playback_callback(pFluidSynthPlayer, MusicFluidSynthMidiEventHandler, pFluidSynthSynth);
	FS_fluid_player_add(pFluidSynthPlayer, szSongPath);
	pFluidSynthDriver = FS_new_fluid_audio_driver(pFluidSynthSettings, pFluidSynthSynth);
	FS_fluid_player_play(pFluidSynthPlayer);
	bFluidSynthPlaying = TRUE;
	
	// Wait until the FluidSynth player thread exits
	FS_fluid_player_join(pFluidSynthPlayer);

	// Mark our setup as dead
	bFluidSynthPlaying = FALSE;

	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Exiting FluidSynth watchdog thread.\n");

	// Sleep for 250ms to ensure we don't overlap reverb with a new song
	Sleep(250);

	// Avoid memory leaks -- free the string we were passed
	free(lpParameter);
	return 0;
}

DWORD WINAPI MusicThread(LPVOID lpParameter) {
	MSG msg;
	MCIERROR dwMCIError = NULL;

	// Load the FluidSynth library if we need it
	if (bMusicFluidSynthEnabled)
		MusicLoadFluidSynth();
	
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_MUSIC_STOP) {
			if (!bSettingsUseMP3Music && bUseFluidSynth) {
				FS_fluid_player_stop(pFluidSynthPlayer);
				bFluidSynthPlaying = FALSE;
				if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped FluidSynth player due to WM_MUSIC_STOP message.\n");
				goto next;
			}

			if (!mciDevice)
				goto next;

			dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);

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
			mciDevice = NULL;
		}
		else if (msg.message == WM_MUSIC_PLAY) {
			if (bOptionsMusicEnabled && !bSettingsUseMP3Music && bUseFluidSynth) {
				if (msg.wParam >= 10000 && msg.wParam <= 10018) {
					std::string strSongPath = (char*)0x4CDB88;      // szSoundsPath
					strSongPath += std::to_string(msg.wParam);
					strSongPath += ".mid";
					char* szSongPath = _strdup(strSongPath.c_str());

					CreateThread(NULL, 0, FluidSynthWatchdogThread, szSongPath, 0, NULL);
					goto next;
				}
			}

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
					// error that causes (0x151, MCIERR_SEQ_PORT_INUSE) just like the game itself does.
					if (dwMCIError && dwMCIError != MCIERR_SEQ_PORT_INUSE) {
						char szErrorBuf[MAXERRORLENGTH];
						mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
						ConsoleLog(LOG_ERROR, "MUS:  MCI_PLAY failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
						goto next;
					}
				}
			}
			else if (bOptionsMusicEnabled) {
				if (mus_debug & MUS_DEBUG_THREAD && msg.lParam != 1)
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

void DoMusicPlay(int iSongID, BOOL bInterrupt) {
	if (bInterrupt) {
		// Certain songs should interrupt others
		switch (iSongID) {
		case 10002:
		case 10005:
		case 10010:
		case 10012:
		case 10016:
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
			if (mus_debug & MUS_DEBUG_THREAD)
				ConsoleLog(LOG_DEBUG, "MUS:  Hook_MusicPlay posted WM_MUSIC_STOP.\n");
			break;
		}
	}

	// Post the play message to the music thread
	PostThreadMessage(dwMusicThreadID, WM_MUSIC_PLAY, iSongID, (bInterrupt) ? NULL : 1);
	if (mus_debug & MUS_DEBUG_THREAD && bInterrupt)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_MusicPlay posted WM_MUSIC_PLAY for iSongID = %u.\n", iSongID);
}

extern "C" int __stdcall Hook_MusicPlay(int iSongID) {
	DWORD *pThis;
	__asm mov [pThis], ecx

	if (bOptionsMusicEnabled)
		DoMusicPlay(iSongID, TRUE);

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

static void L_MusicPlay(void *pThis, int iSongID) {
	if (bMultithreadedMusicEnabled)
		DoMusicPlay(iSongID, FALSE);
	else
		Game_MusicPlay(pThis, iSongID);
}

extern "C" void __stdcall Hook_SimcityAppMusicPlayNext(BOOL bNext) {
	DWORD *pThis;

	__asm mov [pThis], ecx

	DWORD(__thiscall *H_SoundGetMCIResult)(void *) = (DWORD(__thiscall *)(void *))0x40148D;
	int(__thiscall *H_SimcityAppMusicPlayNextRefocusSong)(void *) = (int(__thiscall *)(void *))0x401A9B;

	int nSpeed;
	int iRandMusic;
	int iSongID;

	if (!bOptionsMusicEnabled)
		return;
	nSpeed = ((__int16 *)pThis)[388];
	if (nSpeed == GAME_SPEED_PAUSED)
		nSpeed = GAME_SPEED_TURTLE;
	if (!H_SoundGetMCIResult((void *)pThis[82])) {
		if (bNext)
			H_SimcityAppMusicPlayNextRefocusSong(pThis);
		else if ((!(rand() % (8 * (3 * nSpeed - 3)))) || bSettingsAlwaysPlayMusic) {
			iRandMusic = rand();
			iSongID = 10000 + (iRandMusic % 19);
			L_MusicPlay(pThis, iSongID);
		}
	}
}

void InstallMusicEngineHooks(void) {
	// Restore additional music
	VirtualProtect((LPVOID)0x401A9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401A9B, Hook_MusicPlayNextRefocusSong);

	// Hook for CSimcityApp::MusicPlayNext
	VirtualProtect((LPVOID)0x402AEF, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402AEF, Hook_SimcityAppMusicPlayNext);

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

		bMultithreadedMusicEnabled = TRUE;
	}
}
