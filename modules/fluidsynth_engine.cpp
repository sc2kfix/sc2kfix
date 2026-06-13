// sc2kfix modules/fluidsynth_engine.cpp: fluidsynth engine
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Calls for FluidSynth have now been moved here.

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "music.h"
#include "../resource.h"

#define MAX_START 35

HMODULE hmodFluidSynth = NULL;
DWORD dwFSMIDIThreadID = 0;
bool bFluidSynthPlaying = false;

static HANDLE hCurrentFSSongThread = 0;
static BOOL bUseFluidSynth = FALSE;
static fluid_settings_t* pFluidSynthSettings = NULL;
static fluid_synth_t* pFluidSynthSynth = NULL;
static fluid_player_t* pFluidSynthPlayer = NULL;
static fluid_audio_driver_t* pFluidSynthDriver = NULL;
static bool bFSSongThreadActive = false;

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

static int MusicFluidSynthMidiEventHandler(void* data, fluid_midi_event_t* event) {
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

static void MusicFluidSynthLoggerError(int level, const char* message, void* data) {
	// Ignore the error we get if we're loading the default Windows GM "soundfont"
	if (message && !strcmp(message, "Not a SoundFont file") && !_stricmp(GetFileBaseName(jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString().c_str()), "gm.dls"))
		return;

	ConsoleLog(LOG_ERROR, "MUS:  FluidSynth: %s\n", message);
}

static void MusicFluidSynthLoggerWarning(int level, const char* message, void* data) {
	if (message && (!strncmp(message, "SDL", 3) || !strncmp(message, "Ignoring unknown top-level DLS chunk", 36)))
		return;
	ConsoleLog(LOG_WARNING, "MUS:  FluidSynth: %s\n", message);
}

bool MusicLoadFluidSynth(void) {
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
		hmodFluidSynth = NULL;
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

static void FluidSynthStopSong(fluid_audio_driver_t** pAudDriver, fluid_synth_t** pSynth, fluid_player_t** pPlayer) {
	int i;
	
	if (!pAudDriver && !pSynth && !pPlayer)
		return;

	// Stop and delete the existing player-driver combo if it exists
	if (*pPlayer) {
		FS_fluid_player_stop(*pPlayer);

		if (bFSSongThreadActive || hCurrentFSSongThread) {
			if (hCurrentFSSongThread) {
				if (bFSSongThreadActive) {
					for (i=MAX_START; i>0; i--)
					{
						if (!bFSSongThreadActive) break;
						Sleep(100);
					}
				}

				DWORD dwThreadID = GetThreadId(hCurrentFSSongThread);
				if (dwThreadID) {
					if (!TerminateThread(hCurrentFSSongThread, EXIT_SUCCESS))
						ConsoleLog(LOG_DEBUG, "FluidSynthStopSong(): Hmmm... 0x%06X\n", GetLastError());
					else {
						hCurrentFSSongThread = 0;
						bFSSongThreadActive = false;
					}
				}
			}
		}

		for (i = 0; i < 16; i++)
			FS_fluid_synth_all_sounds_off(*pSynth, i);
		FS_delete_fluid_audio_driver(*pAudDriver);
		FS_delete_fluid_player(*pPlayer);

		*pAudDriver = 0;
		*pPlayer = 0;
	}

	bFluidSynthPlaying = false;
}

static DWORD WINAPI FluidSynthSongThread(LPVOID lpParameter) {
	fluid_player_t *pPlayer = (fluid_player_t *)lpParameter;
	if (bFSSongThreadActive) {
		if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
			ConsoleLog(LOG_DEBUG, "MUS: FluidSynth song 'join' monitoring thread already active.\n");
		return EXIT_SUCCESS;
	}
	bFSSongThreadActive = true;
	
	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS: Starting new FluidSynth song 'join' monitoring thread.\n");

	// Wait until the FluidSynth player thread exits
	FS_fluid_player_join(pPlayer);

	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS: Exiting FluidSynth song 'join' monitoring thread.\n");

	bFSSongThreadActive = false;
	bFluidSynthPlaying = false;

	// In the old watchdog thread there was a sleep call at
	// the end that was present to avoid any reverb-effect
	// overlap. It is being kept here in a commented-out state
	// just in case it is indeed needed:
	// Sleep(250);
	return EXIT_SUCCESS;
}

static bool FluidSynthPlaySong(fluid_audio_driver_t** pAudDriver, fluid_synth_t** pSynth, fluid_settings_t** pSettings, fluid_player_t** pPlayer, const char *szSongPath, bool bOverride) {
	if (!pPlayer)
		return false;

	if (!bOverride && bFluidSynthPlaying)
		return false;

	FluidSynthStopSong(pAudDriver, pSynth, pPlayer);
	
	if (bFSSongThreadActive) {
		// Display this all the time; if it "does" crop up
		// then it "should" only be if the thread fails
		// to terminate, in which case another notice should
		// also be displayed - if not then we cross that bridge.
		if (hCurrentFSSongThread)
			ConsoleLog(LOG_DEBUG, "MUS: ??? - FluidSynth song 'join' monitoring thread is still active: 0x%06X\n", hCurrentFSSongThread);
		else
			bFSSongThreadActive = false;
	}

	// Spin up a new player-driver combo
	*pPlayer = FS_new_fluid_player(*pSynth);
	if (FS_fluid_is_soundfont(jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString().c_str()))
		FS_fluid_synth_sfload(*pSynth, jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString().c_str(), 1);
	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Loaded soundfont \"%s\" into new pFluidSynthPlayer.\n", jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDFONT].ToString().c_str());

	FS_fluid_player_set_playback_callback(*pPlayer, MusicFluidSynthMidiEventHandler, *pSynth);
	FS_fluid_player_add(*pPlayer, szSongPath);
	*pAudDriver = FS_new_fluid_audio_driver(pFluidSynthSettings, *pSynth);
	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Loaded MIDI file \"%s\" into pFluidSynthPlayer.\n", szSongPath);

	// Disable chorus and set reverb and gain to the default level used by Roland synthesizers
	// Also update the music volume here
	FS_fluid_settings_setint(*pSettings, "synth.chorus.active", 0);
	FS_fluid_settings_setnum(*pSettings, "synth.reverb.level", 0.3);
	FS_fluid_settings_setnum(*pSettings, "synth.gain", 0.5 * jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICVOLUME].ToFloat() * jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MASTERVOLUME].ToFloat());

	// Play track
	FS_fluid_player_play(*pPlayer);
	bFluidSynthPlaying = true;
	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Started playback on pFluidSynthPlayer; joining player thread.\n");

	hCurrentFSSongThread = CreateThread(NULL, 0, FluidSynthSongThread, *pPlayer, 0, NULL);

	if (!hCurrentFSSongThread) {
		FluidSynthStopSong(pAudDriver, pSynth, pPlayer);
		return false;
	}

	SetThreadPriority(hCurrentFSSongThread, THREAD_PRIORITY_TIME_CRITICAL);
	return true;
}

DWORD WINAPI FSMIDIThread(LPVOID lpParameter) {
	MSG msg;

	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_DEBUG, "MUS:  Starting new FluidSynth MIDI thread.\n");

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_FS_PLAY) {
			int iPlayingSongID = (int)msg.wParam;
			if (iPlayingSongID >= 10000 && iPlayingSongID <= 10018) {
				if (bFluidSynthPlaying)
					goto next;
				const char* szSongPath = GetGameMusicSoundPath(iPlayingSongID, FALSE);
				if (szSongPath)
					FluidSynthPlaySong(&pFluidSynthDriver, &pFluidSynthSynth, &pFluidSynthSettings, &pFluidSynthPlayer, szSongPath, true);
			}
		}
		else if (msg.message == WM_FS_STOP)
			FluidSynthStopSong(&pFluidSynthDriver, &pFluidSynthSynth, &pFluidSynthPlayer);
		else if (msg.message == WM_QUIT)
			break;
	next:
		DispatchMessage(&msg);
	}

	FluidSynthStopSong(&pFluidSynthDriver, &pFluidSynthSynth, &pFluidSynthPlayer);
	if (mus_debug & MUS_DEBUG_THREAD || mus_debug & MUS_DEBUG_FLUIDSYNTH)
		ConsoleLog(LOG_INFO, "MUS:  Shutting down FluidSynth MIDI thread.\n");
	return EXIT_SUCCESS;
}
