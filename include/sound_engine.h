// sc2kfix include/sound_engine.h: new sound engine
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license
// attached to some LGPL code from libsndfile. see LICENSES for details.

#pragma once
#include <sc2kfix.h>
#include "../thirdparty/sndfile.h"

#define WM_SDL_STOP	WM_APP+1
#define WM_SDL_PLAY	WM_APP+2

#define SND_ORIG_ACTSND_EXIST  0
#define SND_ORIG_ACTSND_NONBUF 1
#define SND_ORIG_PLAYSND       2

// covers what we need in sc2kfix
typedef enum SDL_AudioFormat {
	SDL_AUDIO_UNKNOWN = 0x0000u,
	SDL_AUDIO_U8 = 0x0008u,
	SDL_AUDIO_S8 = 0x8008u,
	SDL_AUDIO_S16LE = 0x8010u,
	SDL_AUDIO_S16BE = 0x9010u,
	SDL_AUDIO_S32LE = 0x8020u,
	SDL_AUDIO_S32BE = 0x9020u,
	SDL_AUDIO_F32LE = 0x8120u,
	SDL_AUDIO_F32BE = 0x9120u,
	SDL_AUDIO_S16 = SDL_AUDIO_S16LE,
	SDL_AUDIO_S32 = SDL_AUDIO_S32LE,
	SDL_AUDIO_F32 = SDL_AUDIO_F32LE
} SDL_AudioFormat;

typedef struct SDL_AudioSpec {
	SDL_AudioFormat format;     /**< Audio data format */
	int channels;               /**< Number of channels: 1 mono, 2 stereo, etc */
	int freq;                   /**< sample rate: sample frames per second */
} SDL_AudioSpec;

typedef struct {
	short* pBuffer;
	size_t uBufferSize;
	int iFrames;
	int iSampleRate;
	int iChannels;
	int iFormat;
	bool bSeekable;
} audio_entity_t;

typedef struct {
	uint8_t* pBuffer;
	sf_count_t iLength;
	sf_count_t iOffset;
} SF_viodata;

typedef struct {
	__int16 iSoundID;
	__int16 nOrig;
} gameSoundAttrib_t;

typedef uint32_t SDL_InitFlags;
typedef void SDL_AudioStream;
typedef uint32_t SDL_AudioDeviceID;
typedef void* SDL_AudioStreamCallback;

#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK ((SDL_AudioDeviceID) 0xFFFFFFFFu)

extern HMODULE hmodSndFile;
extern HMODULE hmodSDL3;

extern SDL_AudioStream* pStreamCurrentSong;
extern SDL_AudioStream* pStreamCurrentSound;

extern DWORD dwSDLSoundThreadID;
extern DWORD dwSDLSongThreadID;

extern SNDFILE* (*SF_open)(const char* path, int mode, SF_INFO* sfinfo);
extern SNDFILE* (*SF_open_virtual)(SF_VIRTUAL_IO* sfvirtual, int mode, SF_INFO* sfinfo, void* user_data);
extern sf_count_t(*SF_seek)(SNDFILE* sndfile, sf_count_t frames, int whence);
extern int (*SF_close)(SNDFILE* sndfile);
extern sf_count_t(*SF_readf_short)(SNDFILE* sndfile, short* ptr, sf_count_t frames);

extern bool (*SDL_Init)(SDL_InitFlags flags);
extern bool (*SDL_ClearAudioStream)(SDL_AudioStream* stream);
extern void (*SDL_DestroyAudioStream)(SDL_AudioStream* stream);
extern SDL_AudioStream* (*SDL_OpenAudioDeviceStream)(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec, SDL_AudioStreamCallback callback, void* userdata);
extern bool (*SDL_PutAudioStreamData)(SDL_AudioStream* stream, const void* buf, int len);
extern bool (*SDL_ResumeAudioStreamDevice)(SDL_AudioStream* stream);
extern int (*SDL_GetAudioStreamAvailable)(SDL_AudioStream* stream);
extern bool (*SDL_SetAudioStreamFormat)(SDL_AudioStream* stream, SDL_AudioSpec* src_spec, SDL_AudioSpec* dst_spec);
extern const char* (*SDL_GetError)(void);
extern bool (*SDL_MixAudio)(uint8_t* dst, const uint8_t* src, SDL_AudioFormat format, uint32_t len, float volume);
extern bool (*SDL_SetAudioStreamGain)(SDL_AudioStream* stream, float gain);

DWORD WINAPI SDLSoundThread(LPVOID lpParameter);
DWORD WINAPI SDLSongThread(LPVOID lpParameter);

bool LoadSoundEngineLibraries(void);
bool SoundEngineInitialize(void);
void SoundEngineDestroy(void);
void SoundEngineStopStream(SDL_AudioStream** pStream);
bool SoundEnginePlayStream(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume, bool bOverride, bool bLoop);
void SoundEngineStopSong(SDL_AudioStream** pStream);
bool SoundEnginePlaySong(SDL_AudioStream** pStream, audio_entity_t* stAudioData, float fVolume);
