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

SNDFILE* (*SF_open)(const char* path, int mode, SF_INFO* sfinfo);
SNDFILE* (*SF_open_virtual)(SF_VIRTUAL_IO* sfvirtual, int mode, SF_INFO* sfinfo, void* user_data);
sf_count_t(*SF_seek)(SNDFILE* sndfile, sf_count_t frames, int whence);
int (*SF_close)(SNDFILE* sndfile);
sf_count_t(*SF_readf_short)(SNDFILE* sndfile, short* ptr, sf_count_t frames);

bool (*SDL_Init)(SDL_InitFlags flags);
void (*SDL_DestroyAudioStream)(SDL_AudioStream* stream);
SDL_AudioStream* (*SDL_OpenAudioDeviceStream)(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata);
bool (*SDL_PutAudioStreamData)(SDL_AudioStream* stream, const void* buf, int len);
bool (*SDL_ResumeAudioStreamDevice)(SDL_AudioStream* stream);
int (*SDL_GetAudioStreamAvailable)(SDL_AudioStream* stream);
const char* (*SDL_GetError)(void);
bool (*SDL_MixAudio)(uint8_t* dst, const uint8_t* src, SDL_AudioFormat format, uint32_t len, float volume);
bool (*SDL_SetAudioStreamGain)(SDL_AudioStream* stream, float gain);

HMODULE hmodSndFile = NULL;
HMODULE hmodSDL3 = NULL;

int iCurrentSongID = 0;
int iCurrentSoundID = 0;
SDL_AudioStream* pStreamCurrentSong = NULL;
SDL_AudioStream* pStreamCurrentSound = NULL;
HANDLE hCurrentSongThread = NULL;
HANDLE hCurrentSoundThread = NULL;

DWORD WINAPI SoundEngineOneShotThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	while (SDL_GetAudioStreamAvailable(pStreamCurrentSound) > 0)
		Sleep(10);

	pStreamCurrentSound = NULL;
	return EXIT_SUCCESS;
}
DWORD WINAPI SoundEngineSongThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	while (SDL_GetAudioStreamAvailable(pStreamCurrentSong) > 0)
		Sleep(10);

	pStreamCurrentSong = NULL;
	return EXIT_SUCCESS;
}

DWORD WINAPI SoundEngineLoopThread(LPVOID lpParameter) {
	audio_entity_t* stAudioData = (audio_entity_t*)lpParameter;

	for (;;) {
		if (!pStreamCurrentSound)
			return EXIT_SUCCESS;
		if (SDL_GetAudioStreamAvailable(pStreamCurrentSound) < stAudioData->uBufferSize)
			SDL_PutAudioStreamData(pStreamCurrentSound, stAudioData->pBuffer, stAudioData->uBufferSize);
		Sleep(10);
	}

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
	SDL_DestroyAudioStream = (decltype(SDL_DestroyAudioStream))GetProcAddress(hmodSDL3, "SDL_DestroyAudioStream");
	SDL_OpenAudioDeviceStream = (decltype(SDL_OpenAudioDeviceStream))GetProcAddress(hmodSDL3, "SDL_OpenAudioDeviceStream");
	SDL_PutAudioStreamData = (decltype(SDL_PutAudioStreamData))GetProcAddress(hmodSDL3, "SDL_PutAudioStreamData");
	SDL_ResumeAudioStreamDevice = (decltype(SDL_ResumeAudioStreamDevice))GetProcAddress(hmodSDL3, "SDL_ResumeAudioStreamDevice");
	SDL_GetAudioStreamAvailable = (decltype(SDL_GetAudioStreamAvailable))GetProcAddress(hmodSDL3, "SDL_GetAudioStreamAvailable");
	SDL_GetError = (decltype(SDL_GetError))GetProcAddress(hmodSDL3, "SDL_GetError");
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

	return true;
}

void SoundEngineStopStream(SDL_AudioStream** pStream) {
	if (!pStream)
		return;
	if (*pStream)
		SDL_DestroyAudioStream(*pStream);
	if (hCurrentSoundThread)
		TerminateThread(hCurrentSoundThread, EXIT_SUCCESS);

	*pStream = NULL;
}

bool SoundEnginePlayStream(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume, bool bOverride, bool bLoop) {
	if (!pStream || !stAudioData)
		return false;

	if (!bOverride && *pStream)
		return false;
	else if (*pStream)
		SoundEngineStopStream(pStream);

	SDL_AudioSpec spec = {
		SDL_AUDIO_S16,
		stAudioData->iChannels,
		stAudioData->iSampleRate
	};

	*pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!*pStream)
		ConsoleLog(LOG_ERROR, "SND:  SDL_OpenAudioDeviceStream failed: %s.\n", SDL_GetError());

	SDL_SetAudioStreamGain(*pStream, fVolume);
	SDL_PutAudioStreamData(*pStream, stAudioData->pBuffer, stAudioData->uBufferSize);
	SDL_ResumeAudioStreamDevice(*pStream);

	if (bLoop)
		hCurrentSoundThread = CreateThread(NULL, 0, SoundEngineLoopThread, stAudioData, 0, NULL);
	else
		hCurrentSoundThread = CreateThread(NULL, 0, SoundEngineOneShotThread, stAudioData, 0, NULL);

	if (!hCurrentSoundThread) {
		SoundEngineStopStream(pStream);
		return false;
	}

	SetThreadPriority(hCurrentSoundThread, THREAD_PRIORITY_TIME_CRITICAL);
	return true;
}

bool SoundEnginePlaySong(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume, bool bOverride) {
	if (!pStream || !stAudioData)
		return false;

	if (!bOverride && *pStream)
		return false;
	else if (*pStream)
		SoundEngineStopStream(pStream);

	SDL_AudioSpec spec = {
		SDL_AUDIO_S16,
		stAudioData->iChannels,
		stAudioData->iSampleRate
	};

	*pStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!*pStream)
		ConsoleLog(LOG_ERROR, "SND:  SDL_OpenAudioDeviceStream failed: %s.\n", SDL_GetError());

	SDL_SetAudioStreamGain(*pStream, fVolume);
	SDL_PutAudioStreamData(*pStream, stAudioData->pBuffer, stAudioData->uBufferSize);
	SDL_ResumeAudioStreamDevice(*pStream);

	hCurrentSongThread = CreateThread(NULL, 0, SoundEngineOneShotThread, stAudioData, 0, NULL);

	if (!hCurrentSongThread) {
		SoundEngineStopStream(pStream);
		return false;
	}

	SetThreadPriority(hCurrentSongThread, THREAD_PRIORITY_TIME_CRITICAL);
	return true;
}