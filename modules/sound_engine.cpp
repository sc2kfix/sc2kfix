// sc2kfix modules/sound_engine.cpp: new sound engine
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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
#include "../resource.h"
#include "../thirdparty/sndfile.h"

#define SDL_DEBUG_GENERAL 1
#define SDL_DEBUG_THREADS 2
#define SDL_DEBUG_SOUND 4
#define SDL_DEBUG_SONG 8

#define SDL_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SDL_DEBUG
#define SDL_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT sdl_debug = SDL_DEBUG;

extern std::map<int, audio_entity_t> mapSoundCache;

SNDFILE* (*SF_open)(const char* path, int mode, SF_INFO* sfinfo);
SNDFILE* (*SF_open_virtual)(SF_VIRTUAL_IO* sfvirtual, int mode, SF_INFO* sfinfo, void* user_data);
sf_count_t(*SF_seek)(SNDFILE* sndfile, sf_count_t frames, int whence);
int (*SF_close)(SNDFILE* sndfile);
sf_count_t(*SF_readf_short)(SNDFILE* sndfile, short* ptr, sf_count_t frames);

bool (*SDL_Init)(SDL_InitFlags flags);
void (*SDL_DestroyAudioStream)(SDL_AudioStream* stream);
bool (*SDL_ClearAudioStream)(SDL_AudioStream* stream);
SDL_AudioStream* (*SDL_OpenAudioDeviceStream)(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata);
bool (*SDL_PutAudioStreamData)(SDL_AudioStream* stream, const void* buf, int len);
bool (*SDL_ResumeAudioStreamDevice)(SDL_AudioStream* stream);
int (*SDL_GetAudioStreamAvailable)(SDL_AudioStream* stream);
bool (*SDL_SetAudioStreamFormat)(SDL_AudioStream* stream, SDL_AudioSpec* src_spec, SDL_AudioSpec* dst_spec);
const char* (*SDL_GetError)(void);
bool (*SDL_MixAudio)(uint8_t* dst, const uint8_t* src, SDL_AudioFormat format, uint32_t len, float volume);
bool (*SDL_SetAudioStreamGain)(SDL_AudioStream* stream, float gain);

HMODULE hmodSndFile = NULL;
HMODULE hmodSDL3 = NULL;

DWORD dwSDLSoundThreadID = 0;
DWORD dwSDLSongThreadID = 0;

int iCurrentSongID = 0;
int iCurrentSoundID = 0;
SDL_AudioStream* pStreamCurrentSong = NULL;
SDL_AudioStream* pStreamCurrentSound = NULL;
HANDLE hCurrentSongThread = NULL;
HANDLE hCurrentSoundThread = NULL;
bool bSongPlaying = false;
bool bSoundPlaying = false;

bool bSongStop = false;
bool bSoundStop = false;

bool bSongThreadActive = false;
bool bSoundThreadActive[2] = { false, false };

#define MAX_START 35

static void StopCurrentSound(SDL_AudioStream** pStream) {
	if (*pStream) {
		if (bSoundThreadActive[0] || bSoundThreadActive[1]) {
			if (hCurrentSoundThread) {
				int i = 0;

				if (bSoundThreadActive[0]) {
					bSoundStop = true;
					for (i=MAX_START; i>0; i--)
					{
						if (!bSoundThreadActive[0]) break;
						Sleep(100);
					}
				}
				else if (bSoundThreadActive[1]) {
					bSoundStop = true;
					for (i=MAX_START; i>0; i--)
					{
						if (!bSoundThreadActive[1]) break;
						Sleep(100);
					}
				}

				DWORD dwThreadID = GetThreadId(hCurrentSoundThread);
				if (dwThreadID) {
					if (!TerminateThread(hCurrentSoundThread, EXIT_SUCCESS))
						ConsoleLog(LOG_DEBUG, "StopCurrentSound(): Hmmm... 0x%06X\n", GetLastError());
					else
						hCurrentSoundThread = 0;
				}
			}
			SoundEngineStopStream(pStream);
		}
	}
}

DWORD WINAPI SDLSoundThread(LPVOID lpParameter) {
	MSG msg;
	int nDuration;
	gameSoundAttrib_t soundAttrib;
	float fVolume;
	bool bLoop;
	CSimcityAppPrimary *pSCApp;
	CSound *pSound;

	if (sdl_debug & SDL_DEBUG_THREADS)
		ConsoleLog(LOG_DEBUG, "SND:  Starting SDL Sound Thread!\n");

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_SDL_PLAY) {
			nDuration = (int)msg.wParam;
			soundAttrib.iSoundID = LOWORD(msg.lParam);
			soundAttrib.nOrig = HIWORD(msg.lParam);
			if (sdl_debug & SDL_DEBUG_SOUND)
				ConsoleLog(LOG_DEBUG, "SDLSoundThread: WM_SDL_PLAY: (%d) (%d) %d\n", soundAttrib.iSoundID, nDuration, soundAttrib.nOrig);
			
			fVolume = jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_SOUNDVOLUME].ToFloat() * jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MASTERVOLUME].ToFloat();

			pSCApp = &pCSimcityAppThis;
			if (pSCApp) {
				pSound = pSCApp->SCASNDLayer;
				if (soundAttrib.nOrig == SND_ORIG_ACTSND_EXIST) {
					if (SoundEnginePlayStream(&pStreamCurrentSound, &mapSoundCache[soundAttrib.iSoundID], fVolume, true, true)) {
						pSound->iSNDActionThingSoundID = soundAttrib.iSoundID;
						pSound->iSNDCurrSoundID = soundAttrib.iSoundID;
						pSound->bSNDPlaySound = TRUE;
						pSound->bSNDWasPlaying = TRUE;
						nActionThingSoundPlayTicks = (nDuration <= 0) ? 0 : GetTickDurationBySoundID_SC2K1996(soundAttrib.iSoundID, nDuration);
						if (sdl_debug & SDL_DEBUG_SOUND)
							ConsoleLog(LOG_DEBUG, "SDLSoundThread: Sound(%d), Duration(%d), nActionThingSoundPlayTicks(%d)\n", soundAttrib.iSoundID, nDuration, nActionThingSoundPlayTicks);
					}
					else
						pSound->bSNDWasPlaying = FALSE;
				}
				else if (soundAttrib.nOrig == SND_ORIG_ACTSND_NONBUF) {
					pSound->bSNDPlaySound = SoundEnginePlayStream(&pStreamCurrentSound, &mapSoundCache[soundAttrib.iSoundID], fVolume, true, true);
					nActionThingSoundPlayTicks = (nDuration <= 0) ? 0 : GetTickDurationBySoundID_SC2K1996(soundAttrib.iSoundID, nDuration);
					pSound->iSNDCurrSoundID = soundAttrib.iSoundID;
					pSound->bSNDWasPlaying = pSound->bSNDPlaySound;
				}
				else if (soundAttrib.nOrig == SND_ORIG_PLAYSND) {
					bLoop = (pSound->iSNDActionThingSoundID == soundAttrib.iSoundID && pSound->dwSNDBufferActionThing ? true : false);
					pSound->bSNDPlaySound = SoundEnginePlayStream(&pStreamCurrentSound, &mapSoundCache[soundAttrib.iSoundID], fVolume, true, bLoop);
					if (pSound->bSNDPlaySound) {
						pSound->iSNDCurrSoundID = soundAttrib.iSoundID;
						if (pSound->bSNDWasPlaying && pSound->iSNDActionThingSoundID == soundAttrib.iSoundID)
							nActionThingSoundPlayTicks = 0;
						else
							nActionThingSoundPlayTicks = GetSoundPlayTicksBySoundID_SC2K1996(soundAttrib.iSoundID);
					}
					else
						Game_Sound_PlayPrioritySound(pSound);
				}
			}
		}
		else if (msg.message == WM_SDL_STOP) {
			StopCurrentSound(&pStreamCurrentSound);
		}
		else if (msg.message == WM_QUIT)
			break;

		DispatchMessage(&msg);
	}

	StopCurrentSound(&pStreamCurrentSound);
	if (sdl_debug & SDL_DEBUG_THREADS)
		ConsoleLog(LOG_INFO, "SND:  Shutting down SDL Sound thread.\n");
	return EXIT_SUCCESS;
}

static void StopCurrentSong(SDL_AudioStream** pStream) {
	if (*pStream) {
		if (bSongThreadActive) {
			if (hCurrentSongThread) {
				int i = 0;

				if (bSongThreadActive) {
					bSongStop = true;
					for (i=MAX_START; i>0; i--)
					{
						if (!bSongThreadActive) break;
						Sleep(100);
					}
				}

				DWORD dwThreadID = GetThreadId(hCurrentSongThread);
				if (dwThreadID) {
					if (!TerminateThread(hCurrentSongThread, EXIT_SUCCESS))
						ConsoleLog(LOG_DEBUG, "StopCurrentSong(): Hmmm... 0x%06X\n", GetLastError());
					else
						hCurrentSongThread = 0;
				}
			}
			SoundEngineStopSong(pStream);
		}
	}
}

DWORD WINAPI SDLSongThread(LPVOID lpParameter) {
	MSG msg;
	SF_INFO stInfoMP3File = { 0 };
	audio_entity_t stAudioEntityMP3 = { 0 };

	if (sdl_debug & SDL_DEBUG_THREADS)
		ConsoleLog(LOG_DEBUG, "SND:  Starting SDL Song thread!\n");

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_SDL_PLAY) {
			int iPlayingSongID = (int)msg.wParam;
			if (iPlayingSongID >= 10000 && iPlayingSongID <= 10018) {
				if (bSongPlaying)
					goto next;
				const char* szSongPath = GetGameMusicSoundPath(iPlayingSongID, TRUE);
				if (szSongPath) {
					//uint64_t uTickStart = GetTickCount64();
					LARGE_INTEGER uTickStart, uTickEnd, uTicksPerSecond;
					QueryPerformanceFrequency(&uTicksPerSecond);
					QueryPerformanceCounter(&uTickStart);
					SNDFILE* sndfile = SF_open(szSongPath, SFM_READ, &stInfoMP3File);

					if (!sndfile) {
						ConsoleLog(LOG_ERROR, "MUS:  Couldn't load MP3 file \"%s\" (sndfile).\n", szSongPath);
						goto next;
					}

					// Build the audio entity data
					stAudioEntityMP3.iFrames = stInfoMP3File.frames;
					stAudioEntityMP3.iSampleRate = stInfoMP3File.samplerate;
					stAudioEntityMP3.iChannels = stInfoMP3File.channels;
					stAudioEntityMP3.iFormat = stInfoMP3File.format;
					stAudioEntityMP3.bSeekable = stInfoMP3File.seekable;
					stAudioEntityMP3.uBufferSize = stAudioEntityMP3.iChannels * sizeof(short) * stAudioEntityMP3.iFrames;
					stAudioEntityMP3.pBuffer = (short*)malloc(stAudioEntityMP3.uBufferSize);
					if (!stAudioEntityMP3.pBuffer) {
						ConsoleLog(LOG_ERROR, "MUS:  Couldn't load MP3 file \"%s\" (malloc).\n", szSongPath);
						goto next;
					}

					// Read the audio into the buffer as 16-bit PCM
					SF_readf_short(sndfile, stAudioEntityMP3.pBuffer, stAudioEntityMP3.iFrames);
					SF_close(sndfile);
					QueryPerformanceCounter(&uTickEnd);
					if (sdl_debug & SDL_DEBUG_SONG)
						ConsoleLog(LOG_DEBUG, "MUS:  Loading MP3 file \"%s\" took %llu microseconds.\n", szSongPath, ((uTickEnd.QuadPart - uTickStart.QuadPart) * 1000000 / uTicksPerSecond.QuadPart));

					// Start playing the song and finish processing this message
					if (sdl_debug & SDL_DEBUG_SONG)
						ConsoleLog(LOG_DEBUG, "MUS:  Playing MP3 file \"%s\" via SDL3.\n", szSongPath);
					SoundEnginePlaySong(&pStreamCurrentSong, &stAudioEntityMP3,
						jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MUSICVOLUME].ToFloat() * jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_MASTERVOLUME].ToFloat(),
						true);
				}
			}
		}
		else if (msg.message == WM_SDL_STOP) {
			StopCurrentSong(&pStreamCurrentSong);
			free(stAudioEntityMP3.pBuffer);
			memset(&stAudioEntityMP3, 0, sizeof(audio_entity_t));
		}
		else if (msg.message == WM_QUIT)
			break;
next:
		DispatchMessage(&msg);
	}

	StopCurrentSong(&pStreamCurrentSong);
	free(stAudioEntityMP3.pBuffer);
	memset(&stAudioEntityMP3, 0, sizeof(audio_entity_t));
	if (sdl_debug & SDL_DEBUG_THREADS)
		ConsoleLog(LOG_INFO, "SND:  Shutting down SDL Song thread.\n");
	return EXIT_SUCCESS;
}

DWORD WINAPI SoundEngineOneShotThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	if (bSoundThreadActive[0])
		return EXIT_SUCCESS;
	bSoundThreadActive[0] = true;
	bSoundStop = false;
	while (SDL_GetAudioStreamAvailable(pStreamCurrentSound) > 0) {
		if (bSoundStop)
			break;
		Sleep(10);
	}

	bSoundThreadActive[0] = false;
	bSoundPlaying = false;
	return EXIT_SUCCESS;
}
DWORD WINAPI SoundEngineSongThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	if (bSongThreadActive)
		return EXIT_SUCCESS;
	bSongThreadActive = true;
	bSongStop = false;
	while (SDL_GetAudioStreamAvailable(pStreamCurrentSong) > 0) {
		if (bSongStop)
			break;
		Sleep(10);
	}

	bSongThreadActive = false;
	bSongPlaying = false;
	return EXIT_SUCCESS;
}

DWORD WINAPI SoundEngineLoopThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	if (bSoundThreadActive[1])
		return EXIT_SUCCESS;
	bSoundThreadActive[1] = true;
	bSoundStop = false;
	for (;;) {
		if (bSoundStop)
			break;
		if (!bSoundPlaying || !pStreamCurrentSound)
			return EXIT_SUCCESS;
		if (SDL_GetAudioStreamAvailable(pStreamCurrentSound) < stAudioData->uBufferSize)
			SDL_PutAudioStreamData(pStreamCurrentSound, stAudioData->pBuffer, stAudioData->uBufferSize);
		Sleep(10);
	}

	bSoundThreadActive[1] = false;
	return EXIT_SUCCESS;
}

bool LoadSoundEngineLibraries(void) {
	hmodSndFile = LoadLibrary("sndfile.dll");
	if (!hmodSndFile) {
		ConsoleLog(LOG_ERROR, "SND:  Couldn't load sndfile.dll.\n");
		return false;
	}

	SF_open = (decltype(SF_open))GetProcAddress(hmodSndFile, "sf_open");
	SF_open_virtual = (decltype(SF_open_virtual))GetProcAddress(hmodSndFile, "sf_open_virtual");
	SF_seek = (decltype(SF_seek))GetProcAddress(hmodSndFile, "sf_seek");
	SF_close = (decltype(SF_close))GetProcAddress(hmodSndFile, "sf_close");
	SF_readf_short = (decltype(SF_readf_short))GetProcAddress(hmodSndFile, "sf_readf_short");

	hmodSDL3 = LoadLibrary("SDL3.dll");
	if (!hmodSDL3) {
		ConsoleLog(LOG_ERROR, "SND:  Couldn't load SDL3.dll.\n");
		return false;
	}

	SDL_Init = (decltype(SDL_Init))GetProcAddress(hmodSDL3, "SDL_Init");
	SDL_ClearAudioStream = (decltype(SDL_ClearAudioStream))GetProcAddress(hmodSDL3, "SDL_ClearAudioStream");
	SDL_DestroyAudioStream = (decltype(SDL_DestroyAudioStream))GetProcAddress(hmodSDL3, "SDL_DestroyAudioStream");
	SDL_OpenAudioDeviceStream = (decltype(SDL_OpenAudioDeviceStream))GetProcAddress(hmodSDL3, "SDL_OpenAudioDeviceStream");
	SDL_PutAudioStreamData = (decltype(SDL_PutAudioStreamData))GetProcAddress(hmodSDL3, "SDL_PutAudioStreamData");
	SDL_ResumeAudioStreamDevice = (decltype(SDL_ResumeAudioStreamDevice))GetProcAddress(hmodSDL3, "SDL_ResumeAudioStreamDevice");
	SDL_GetAudioStreamAvailable = (decltype(SDL_GetAudioStreamAvailable))GetProcAddress(hmodSDL3, "SDL_GetAudioStreamAvailable");
	SDL_GetError = (decltype(SDL_GetError))GetProcAddress(hmodSDL3, "SDL_GetError");
	SDL_SetAudioStreamFormat = (decltype(SDL_SetAudioStreamFormat))GetProcAddress(hmodSDL3, "SDL_SetAudioStreamFormat");
	SDL_MixAudio = (decltype(SDL_MixAudio))GetProcAddress(hmodSDL3, "SDL_MixAudio");
	SDL_SetAudioStreamGain = (decltype(SDL_SetAudioStreamGain))GetProcAddress(hmodSDL3, "SDL_SetAudioStreamGain");
	return true;
}

bool SoundEngineInitialize(void) {
	// Initialize SDL3 for sound only
	if (!SDL_Init(0x10)) {
		ConsoleLog(LOG_ERROR, "SND:  SDL_Init failed: %s.\n", SDL_GetError());
		return false;
	}

	if (sdl_debug & SDL_DEBUG_GENERAL)
		ConsoleLog(LOG_DEBUG, "SND: SDL Sound Engine Initialized.\n");

	// Create the two audio streams with default settings
	SDL_AudioSpec spec = {
		SDL_AUDIO_S16,
		2,
		44100
	};
	pStreamCurrentSong = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	pStreamCurrentSound = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);

	return true;
}

void SoundEngineDestroy(void) {
	if (pStreamCurrentSong)
		SDL_DestroyAudioStream(pStreamCurrentSong);
	if (pStreamCurrentSound)
		SDL_DestroyAudioStream(pStreamCurrentSound);

	if (sdl_debug & SDL_DEBUG_GENERAL)
		ConsoleLog(LOG_DEBUG, "SND: SDL Sound Engine Destroyed.\n");
}

void SoundEngineStopStream(SDL_AudioStream** pStream) {
	if (!pStream)
		return;
	if (*pStream)
		SDL_ClearAudioStream(*pStream);

	bSoundPlaying = false;
}

bool SoundEnginePlayStream(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume, bool bOverride, bool bLoop) {
	if (!pStream || !stAudioData)
		return false;

	if (!bOverride && bSoundPlaying)
		return false;
	
	StopCurrentSound(pStream);

	SDL_AudioSpec spec = {
		SDL_AUDIO_S16,
		stAudioData->iChannels,
		stAudioData->iSampleRate
	};

	/**pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!*pStream)
		ConsoleLog(LOG_ERROR, "SND:  SDL_OpenAudioDeviceStream failed: %s.\n", SDL_GetError());*/

	SDL_SetAudioStreamFormat(*pStream, &spec, NULL);
	SDL_SetAudioStreamGain(*pStream, fVolume);
	SDL_PutAudioStreamData(*pStream, stAudioData->pBuffer, stAudioData->uBufferSize);
	SDL_ResumeAudioStreamDevice(*pStream);
	bSoundPlaying = true;

	if (bLoop)
		hCurrentSoundThread = CreateThread(NULL, 0, SoundEngineLoopThread, stAudioData, 0, NULL);
	else
		hCurrentSoundThread = CreateThread(NULL, 0, SoundEngineOneShotThread, stAudioData, 0, NULL);

	if (!hCurrentSoundThread) {
		StopCurrentSound(pStream);
		return false;
	}

	SetThreadPriority(hCurrentSoundThread, THREAD_PRIORITY_TIME_CRITICAL);
	return true;
}

void SoundEngineStopSong(SDL_AudioStream** pStream) {
	if (!pStream)
		return;
	if (*pStream)
		SDL_ClearAudioStream(*pStream);

	bSongPlaying = false;
}

bool SoundEnginePlaySong(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume, bool bOverride) {
	if (!pStream || !stAudioData)
		return false;

	if (!bOverride && bSongPlaying)
		return false;
	
	StopCurrentSong(pStream);

	SDL_AudioSpec spec = {
		SDL_AUDIO_S16,
		stAudioData->iChannels,
		stAudioData->iSampleRate
	};

	/**pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!*pStream)
		ConsoleLog(LOG_ERROR, "SND:  SDL_OpenAudioDeviceStream failed: %s.\n", SDL_GetError());*/

	SDL_SetAudioStreamGain(*pStream, fVolume);
	SDL_PutAudioStreamData(*pStream, stAudioData->pBuffer, stAudioData->uBufferSize);
	SDL_ResumeAudioStreamDevice(*pStream);
	bSongPlaying = true;

	hCurrentSongThread = CreateThread(NULL, 0, SoundEngineSongThread, stAudioData, 0, NULL);

	if (!hCurrentSongThread) {
		StopCurrentSong(pStream);
		return false;
	}

	SetThreadPriority(hCurrentSongThread, THREAD_PRIORITY_TIME_CRITICAL);
	return true;
}
