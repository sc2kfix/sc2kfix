// sc2kfix include/music.h: new music engine and associated defines
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

#define WM_MUSIC_STOP WM_APP+1
#define WM_MUSIC_PLAY WM_APP+2

void InstallMusicEngineHooks(void);
DWORD WINAPI MusicThread(LPVOID lpParameter);
DWORD WINAPI MusicMCINotifyCallback(WPARAM wFlags, LPARAM lDevID);
void MusicShufflePlaylist(int iLastSongPlayed);

extern BOOL bUseMultithreadedMusic;
extern DWORD dwMusicThreadID;
