// sc2kfix include/music.h: new music engine and associated defines
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#define MUS_DEBUG_SONGS 1
#define MUS_DEBUG_THREAD 2
#define MUS_DEBUG_SEQUENCER 4
#define MUS_DEBUG_FLUIDSYNTH 8

#define MUS_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MUS_DEBUG
#define MUS_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define WM_MUSIC_STOP		WM_APP+1
#define WM_MUSIC_PLAY		WM_APP+2
#define WM_MUSIC_RESET		WM_APP+3
#define WM_MUSIC_CONFIRM	WM_APP+4

#define WM_FS_STOP	WM_APP+1
#define WM_FS_PLAY	WM_APP+2

enum {
	MUSIC_ENGINE_NONE,
	MUSIC_ENGINE_SEQUENCER,
	MUSIC_ENGINE_FLUIDSYNTH,
	MUSIC_ENGINE_MP3
};

void InstallMusicEngineHooks(void);
void SetMCIDevID(__int16 wMCIDevID);
void SetSongPlaying(bool bPlaying);
bool IsSongPlaying();
const char *GetGameMusicSoundPath(BOOL bDoMP3);
DWORD WINAPI MusicThread(LPVOID lpParameter);

extern DWORD dwMusicThreadID;
extern DWORD dwFSMIDIThreadID;

DWORD WINAPI FSMIDIThread(LPVOID lpParameter);
