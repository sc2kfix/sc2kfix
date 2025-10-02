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
int iPlayingSongID = 0;
DWORD dwMusicThreadID;
MCIDEVICEID mciDevice = NULL;
BOOL bMultithreadedMusicEnabled = FALSE;
BOOL bUseFluidSynth = FALSE;
extern char szSettingsFluidSynthSoundfont[MAX_PATH + 1];

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
int (*FS_fluid_settings_setint)(fluid_settings_t* settings, const char* name, int val);
int (*FS_fluid_settings_setnum)(fluid_settings_t* settings, const char* name, double val);

const char *GetGameSoundPath() {
	return szSoundPath;
}

int GetCurrentActiveSongID() {
	return iPlayingSongID;
}

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
	if (message && !strncmp(message, "SDL", 3))
		return;
	ConsoleLog(LOG_WARNING, "MUS:  FluidSynth: %s\n", message);
}

BOOL MusicLoadFluidSynth(void) {
	if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  FluidSynth enabled, probing for valid FluidSynth library.\n");

	// Disable FluidSynth support if the library doesn't exist.
	if (!FileExists("libfluidsynth-3.dll")) {
		if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
			ConsoleLog(LOG_DEBUG, "MUS:  FluidSynth library doesn't exist; disabling support.\n");
		hmodFluidSynth = FALSE;
		return FALSE;
	}

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
	if ((FS_fluid_settings_setint = (decltype(FS_fluid_settings_setint))GetProcAddress(hmodFluidSynth, "fluid_settings_setint")) == NULL)
		bUseFluidSynth = FALSE;
	if ((FS_fluid_settings_setnum = (decltype(FS_fluid_settings_setnum))GetProcAddress(hmodFluidSynth, "fluid_settings_setnum")) == NULL)
		bUseFluidSynth = FALSE;

	// If any of our loads failed, release the FluidSynth library, throw an error, and fall 
	// back to using MCI.
	if (!bUseFluidSynth) {
		ConsoleLog(LOG_ERROR, "MUS:  One or more FluidSynth functions could not be loaded. Disabling FluidSynth.\n");
		FreeLibrary(hmodFluidSynth);
		return FALSE;
	}

	if (mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Loaded FluidSynth function pointers.\n");

	// Set FluidSynth's log functions
	FS_fluid_set_log_function(FLUID_PANIC, MusicFluidSynthLoggerError, NULL);
	FS_fluid_set_log_function(FLUID_ERR, MusicFluidSynthLoggerError, NULL);
	FS_fluid_set_log_function(FLUID_WARN, MusicFluidSynthLoggerWarning, NULL);
	FS_fluid_set_log_function(FLUID_INFO, NULL, NULL);
	FS_fluid_set_log_function(FLUID_DBG, NULL, NULL);

	// Create initial FluidSynth data
	pFluidSynthSettings = FS_new_fluid_settings();
	pFluidSynthSynth = FS_new_fluid_synth(pFluidSynthSettings);

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
	if (FS_fluid_is_soundfont(szSettingsFluidSynthSoundfont))
		FS_fluid_synth_sfload(pFluidSynthSynth, szSettingsFluidSynthSoundfont, 1);
	FS_fluid_player_set_playback_callback(pFluidSynthPlayer, MusicFluidSynthMidiEventHandler, pFluidSynthSynth);
	FS_fluid_player_add(pFluidSynthPlayer, szSongPath);
	pFluidSynthDriver = FS_new_fluid_audio_driver(pFluidSynthSettings, pFluidSynthSynth);

	// Disable chorus and set reverb and gain to the default level used by Roland synthesizers
	FS_fluid_settings_setint(pFluidSynthSettings, "synth.chorus.active", 0);
	FS_fluid_settings_setnum(pFluidSynthSettings, "synth.reverb.level", 0.3);
	FS_fluid_settings_setnum(pFluidSynthSettings, "synth.gain", 0.5);

	// Play track
	FS_fluid_player_play(pFluidSynthPlayer);
	bFluidSynthPlaying = TRUE;
	
	// Wait until the FluidSynth player thread exits
	FS_fluid_player_join(pFluidSynthPlayer);

	// Mark our setup as dead
	bFluidSynthPlaying = FALSE;
	iPlayingSongID = 0;

	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Exiting FluidSynth watchdog thread.\n");

	// Sleep for 250ms to ensure we don't overlap reverb with a new song
	Sleep(250);

	// Avoid memory leaks -- free the string we were passed
	free(lpParameter);
	return 0;
}

const char* SettingsSaveMusicEngine(UINT iMusicEngine);

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

static const char *GetGameSoundPath(int iSongID, BOOL bDoMP3) {
	const char *pExt;
	static std::string strSongPath;
	BOOL bUseAliasedSong;
	int iAliasIdx;

	pExt = (bDoMP3) ? ".mp3" : ".mid";
	strSongPath = szSoundPath;
	bUseAliasedSong = FALSE;
	iAliasIdx = GetAliasIndexFromSongID(iSongID);
	if (iAliasIdx >= 0 && iAliasIdx <= MUSIC_TRACKS) {
		const char *pName = (bDoMP3) ? szSettingsMP3TrackPath[iAliasIdx] : szSettingsMIDITrackPath[iAliasIdx];
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
		ConsoleLog(LOG_DEBUG, "MUS:  Starting music engine! Initial engine set to \"%s\".\n", SettingsSaveMusicEngine(iSettingsMusicEngineOutput));
	
	while (GetMessage(&msg, NULL, 0, 0)) {
		pSCApp = &pCSimcityAppThis;
		if (msg.message == WM_MUSIC_STOP) {
			// Log a debug message at best if the music engine is set to none.
			// In this case even with the engine set to MUSIC_ENGINE_NONE it
			// will still continue, however once mciDevice is NULL it'll then
			// getout.
			if (iSettingsMusicEngineOutput == MUSIC_ENGINE_NONE)
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Music engine set to None; ignoring WM_MUSIC_STOP message.\n");

			// Stop the FluidSynth thread if it's active
			if (iSettingsMusicEngineOutput == MUSIC_ENGINE_FLUIDSYNTH && hmodFluidSynth) {
				FS_fluid_player_stop(pFluidSynthPlayer);
				bFluidSynthPlaying = FALSE;
				if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped FluidSynth player due to WM_MUSIC_STOP message.\n");
				goto next;
			}

			// Failing the above, run what we need to through the MCI interface
			if (!mciDevice)
				goto next;

			dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
			iPlayingSongID = 0;

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
			// Log a debug message at best if the music engine is set to none
			if (iSettingsMusicEngineOutput == MUSIC_ENGINE_NONE) {
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Music engine set to None; ignoring WM_MUSIC_PLAY message.\n");
				goto next;
			}

			// If we're using FluidSynth, set up a watchdog thread that runs the FluidSynth engine
			// and waits for it to exit
			if (pSCApp->dwSCAGameMusic) {
				if (iSettingsMusicEngineOutput == MUSIC_ENGINE_FLUIDSYNTH && hmodFluidSynth) {
					if (msg.wParam >= 10000 && msg.wParam <= 10018) {
						iPlayingSongID = msg.wParam;
						const char* szSongPath = GetGameSoundPath(iPlayingSongID, FALSE);

						if (szSongPath)
							CreateThread(NULL, 0, FluidSynthWatchdogThread, (LPVOID)szSongPath, 0, NULL);
						goto next;
					}
				}

				// Failing all of the above, use MCI to handle MIDI and MP3 playback
				if (!mciDevice) {
					if (msg.wParam >= 10000 && msg.wParam <= 10018) {
						iPlayingSongID = msg.wParam;
						const char* szSongPath = "";
						BOOL bUseMP3 = FALSE;
						if (iSettingsMusicEngineOutput == MUSIC_ENGINE_MP3) {
							szSongPath = GetGameSoundPath(iPlayingSongID, TRUE);
							if (FileExists(szSongPath))
								bUseMP3 = TRUE;
							else {
								ConsoleLog(LOG_ERROR, "MUS:  Could not find music file %s; failing back to MIDI sequencer.\n", szSongPath);
								bUseMP3 = FALSE;
							}
						}

						if (!bUseMP3) {
							szSongPath = GetGameSoundPath(iPlayingSongID, FALSE);
						}
						
						if (!szSongPath)
							goto next;

						MCI_OPEN_PARMS mciOpenParms = { NULL, NULL, (bUseMP3 ? "mpegvideo" : "sequencer"), szSongPath, NULL };
						dwMCIError = mciSendCommand(NULL, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, (DWORD_PTR)&mciOpenParms);
						if (dwMCIError) {
							char szErrorBuf[MAXERRORLENGTH];
							mciGetErrorString(dwMCIError, szErrorBuf, MAXERRORLENGTH);
							ConsoleLog(LOG_ERROR, "MUS:  MCI_OPEN failed, 0x%08X (%s)\n", dwMCIError, szErrorBuf);
							iPlayingSongID = 0;
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
							iPlayingSongID = 0;
							goto next;
						}
					}
				}
				else {
					if (mus_debug & MUS_DEBUG_THREAD && msg.lParam != 1)
						ConsoleLog(LOG_DEBUG, "MUS:  WM_MUSIC_PLAY message received but MCI is still active; discarding message.\n");
					goto next;
				}
			}
		}
		else if (msg.message == WM_MUSIC_RESET) {
			// Attempt to hard stop all music engines, since our music engine has probably changed
			if (bFluidSynthPlaying && hmodFluidSynth) {
				FS_fluid_player_stop(pFluidSynthPlayer);
				bFluidSynthPlaying = FALSE;
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped FluidSynth player due to WM_MUSIC_RESET message.\n");
			}
			if (mciDevice) {
				dwMCIError = mciSendCommand(mciDevice, MCI_CLOSE, MCI_WAIT, NULL);
				if (mus_debug & MUS_DEBUG_THREAD)
					ConsoleLog(LOG_DEBUG, "MUS:  Thread stopped mciDevice 0x%08X due to WM_MUSIC_RESET message.\n", mciDevice);
				mciDevice = NULL;
			}

			// Only restart if the engine is not set to MUSIC_ENGINE_NONE
			if (iSettingsMusicEngineOutput != MUSIC_ENGINE_NONE) {
				// Restart the active song if there is one
				if (iPlayingSongID)
					DoMusicPlay(iPlayingSongID, FALSE);
			}

			goto next;
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
				ConsoleLog(LOG_DEBUG, "MUS:  Hook_SimcityApp_MusicPlay posted WM_MUSIC_STOP.\n");
			break;
		}
	}

	// Post the play message to the music thread
	PostThreadMessage(dwMusicThreadID, WM_MUSIC_PLAY, iSongID, (bInterrupt) ? NULL : 1);
	if (mus_debug & MUS_DEBUG_THREAD && bInterrupt)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_SimcityApp_MusicPlay posted WM_MUSIC_PLAY for iSongID = %u.\n", iSongID);
}

extern "C" int __stdcall Hook_SimcityApp_MusicPlay(int iSongID) {
	CSimcityAppPrimary *pThis;
	__asm mov [pThis], ecx

	if (pThis->dwSCAGameMusic)
		DoMusicPlay(iSongID, TRUE);

	// Restore "this" and leave
	__asm {
		mov ecx, [pThis]
		mov eax, 1
	}
}

extern "C" int __stdcall Hook_Sound_MusicStop(void) {
	CSound *pThis;
	__asm mov [pThis], ecx

	// Post the stop message to the music thread
	PostThreadMessage(dwMusicThreadID, WM_MUSIC_STOP, NULL, NULL);
	if (mus_debug & MUS_DEBUG_THREAD)
		ConsoleLog(LOG_DEBUG, "MUS:  Hook_Sound_MusicStop posted WM_MUSIC_STOP.\n");

	// Restore "this" and leave
	__asm {
		mov ecx, [pThis]
		xor eax, eax
	}
}

// Replaces the original MusicPlayNextRefocusSong
extern "C" int __stdcall Hook_SimcityApp_MusicPlayNextRefocusSong(void) {
	CSimcityAppPrimary *pThis;
	int retval, iSongToPlay;

	// This is actually a __thiscall we're overriding, so save "this"
	__asm mov [pThis], ecx

	// Fix for the wrong song being played after the intro video
	if (_ReturnAddress() == (void*)0x4061EE || _ReturnAddress() == (void*)0x425444) {
		if (mus_debug & MUS_DEBUG_SONGS)
			ConsoleLog(LOG_DEBUG, "MUS:  Forcing song 10001 for call returning to 0x%08X.\n", (DWORD)_ReturnAddress());
		return Game_SimcityApp_MusicPlay(pThis, 10001);
	}

	iSongToPlay = vectorRandomSongIDs[iCurrentSong++];
	if (mus_debug & MUS_DEBUG_SONGS)
		ConsoleLog(LOG_DEBUG, "MUS:  Playing song %i (next iCurrentSong will be %i).\n", iSongToPlay, (iCurrentSong > 8 ? 0 : iCurrentSong));

	retval = Game_SimcityApp_MusicPlay(pThis, iSongToPlay);

	// Loop and/or shuffle.
	if (iCurrentSong > 8) {
		iCurrentSong = 0;

		// Shuffle the songs, making sure we don't get the same one twice in a row
		MusicShufflePlaylist(iSongToPlay);
	}

	__asm mov eax, [retval]
}

static void L_MusicPlay(CSimcityAppPrimary *pThis, int iSongID) {
	if (bMultithreadedMusicEnabled)
		DoMusicPlay(iSongID, FALSE);
	else
		Game_SimcityApp_MusicPlay(pThis, iSongID);
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
	if (!Game_Sound_GetMCIResult(pThis->SCASNDLayer)) {
		if (bNext)
			Game_SimcityApp_MusicPlayNextRefocusSong(pThis);
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
	NEWJMP((LPVOID)0x401A9B, Hook_SimcityApp_MusicPlayNextRefocusSong);

	// Hook for CSimcityApp::MusicPlayNext
	VirtualProtect((LPVOID)0x402AEF, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402AEF, Hook_SimcityApp_MusicPlayNext);

	// Shuffle music if the shuffle setting is enabled
	MusicShufflePlaylist(0);

	// Replace music functions with ones to post messages to the music thread
	if (bSettingsUseMultithreadedMusic) {
		VirtualProtect((LPVOID)0x402414, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402414, Hook_SimcityApp_MusicPlay);
		VirtualProtect((LPVOID)0x402BE4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
		NEWJMP((LPVOID)0x402BE4, Hook_Sound_MusicStop);
		VirtualProtect((LPVOID)0x4D2BFC, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(DWORD*)0x4D2BFC = (DWORD)MusicMCINotifyCallback;

		// XXX - effectively always TRUE because the opt-in setting is now always TRUE as of
		// r10-dev 2025-08-31. maybe this needs to go away?
		bMultithreadedMusicEnabled = TRUE;
	}
}
