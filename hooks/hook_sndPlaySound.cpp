// sc2kfix hooks/hook_sndPlaySound.cpp: hook for sndPlaySoundA
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SND_DEBUG_PLAYS 1
#define SND_DEBUG_REPLACEMENTS 2
#define SND_DEBUG_INTERNALS 4

#define SND_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SND_DEBUG
#define SND_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT snd_debug = SND_DEBUG;

std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
std::map<int, sound_replacement_t> mapReplacementSounds;
std::map<int, audio_entity_t> mapSoundCache;

bool bSoundKickstart = false;

static bool LoadSoundFromFile(int iSoundID, std::string strPath);

static int GetSoundPosBySoundID(int iSoundID) {
	return iSoundID - SOUND_START;
}

int GetSoundPlayTicksBySoundID_SC2K1996(int iSoundID) {
	for (int i = 0; i < SOUND_ENTRIES; i++) {
		if (GetSoundPosBySoundID(iSoundID) == i) {
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "GetSoundPlayTicksBySoundID_SC2K1996(%d): i(%d) nSoundPlayTicks[i](%d)\n", iSoundID, i, nSoundPlayTicks[i]);
			return nSoundPlayTicks[i];
		}
	}
	return 0;
}

int GetTickDurationBySoundID_SC2K1996(int iSoundID, int nDuration) {
	// For clarity it interpreted the needed array position based off
	// of the following originally for the address:
	// *((DWORD *)&rgbLoColor[8].wPos + iSoundID)
	int nSoundPlayTicksEntry = GetSoundPlayTicksBySoundID_SC2K1996(iSoundID);
	if (snd_debug & SND_DEBUG_INTERNALS)
		ConsoleLog(LOG_DEBUG, "GetTickDurationBySoundID_SC2K1996(%d, %d): nSoundPlayTicksEntry(%d)\n", iSoundID, nDuration, nSoundPlayTicksEntry);
	return nDuration * nSoundPlayTicksEntry;
}

extern "C" void __stdcall Hook_Sound_PlayPrioritySound() {
	CSound *pThis;

	__asm mov [pThis], ecx

	pThis->bSNDPlaySound = FALSE;
	if (pThis->bSNDWasPlaying) {
		if (pThis->iSNDCurrSoundID == pThis->iSNDActionThingSoundID) {
			if (nCurrentActionThingSoundID > 0) {
				pThis->iSNDActionThingSoundID = -1;
				Game_Sound_PlayActionThingSound(pThis, nCurrentActionThingSoundID, -1);
				nCurrentActionThingSoundID = -1;
				return;
			}
			nActionThingSoundPlayTicks = 0;
			pThis->iSNDCurrSoundID = -1;
		}
		else if (nActionThingSoundPlayTicksCurrent) {
			int nRemainingDuration = nActionThingSoundPlayTicksCurrent / GetSoundPlayTicksBySoundID_SC2K1996(pThis->iSNDActionThingSoundID);
			nActionThingSoundPlayTicksCurrent = 0;
			Game_Sound_PlayActionThingSound(pThis, pThis->iSNDActionThingSoundID, nRemainingDuration);
		}
		else {
			if (nCurrentActionThingSoundID > 0) {
				pThis->iSNDActionThingSoundID = -1;
				Game_Sound_PlayActionThingSound(pThis, nCurrentActionThingSoundID, -1);
				nCurrentActionThingSoundID = -1;
				return;
			}
			Game_Sound_PlayActionThingSound(pThis, pThis->iSNDActionThingSoundID, -1);
		}
	}
	else {
		nActionThingSoundPlayTicks = 0;
		pThis->iSNDCurrSoundID = -1;
	}
}

extern "C" void __stdcall Hook_Sound_InitSoundLayer(HWND m_hWnd) {
	CSound *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	CMFC3XString strSndPath;

	pSCApp = &pCSimcityAppThis;

	pThis->hMainWnd = m_hWnd;
	GameMain_String_Empty(&pThis->dwSNDMusicString);
	pThis->dwSNDUnknownOne = 0;
	pThis->dwSNDMusPlaying = 0;
	pThis->dwSNDUnknownTwo = -1;
	pThis->bSNDPlaySound = FALSE;
	pThis->bSNDWasPlaying = FALSE;
	pThis->iSNDCurrSoundID = -1;
	pThis->iSNDActionThingSoundID = -1;
	pThis->iSNDToolSoundID = -1;
	pThis->iSNDGeneralSoundID = -1;
	pThis->wSNDMCIDevID = -1;
	Game_SimcityApp_GetValueStringA(pSCApp, &strSndPath, aPaths, aMusic);
	strcpy_s(szSoundPath, MAX_PATH, strSndPath.m_pchData);
	if (!szSoundPath[0])
		strcpy_s(szSoundPath, MAX_PATH, aSounds);
	for (int i = 0; i < SOUND_ENTRIES; i++) {
		if (snd_debug & SND_DEBUG_INTERNALS)
			ConsoleLog(LOG_DEBUG, "B: CSound::InitSoundLayer(0x%06X): i(%d), nSoundPlayTicks[i](%d -> %d), &nSoundPlayTicks[i](0x%06X)\n", m_hWnd, i, nSoundPlayTicks[i], (nSoundPlayTicks[i] / 200 + 1), &nSoundPlayTicks[i]);
		nSoundPlayTicks[i] = nSoundPlayTicks[i] / 200 + 1;
		if (snd_debug & SND_DEBUG_INTERNALS)
			ConsoleLog(LOG_DEBUG, "A: CSound::InitSoundLayer(0x%06X): i(%d), nSoundPlayTicks[i](%d), &nSoundPlayTicks[i](0x%06X)\n", m_hWnd, i, nSoundPlayTicks[i], &nSoundPlayTicks[i]);
	}
	_heapmin();
	pThis->dwSNDBufferClick = malloc(0x40000);
	Game_Sound_LoadClickSound(pThis);
	if (dwSoundBufferClear) {
		pThis->dwSNDBufferTool = 0;
		pThis->dwSNDBufferActionThing = 0;
		pThis->dwSNDBufferExplosion = 0;
		pThis->dwSNDBufferGeneral = 0;
	}
	else {
		pThis->dwSNDBufferTool = malloc(0x40000);
		pThis->dwSNDBufferActionThing = malloc(0x40000);
		pThis->dwSNDBufferExplosion = malloc(0x40000);
		pThis->dwSNDBufferGeneral = malloc(0x40000);
		Game_Sound_LoadExplosionSound(pThis);
		Game_Sound_LoadActionThingSound(pThis, SOUND_BULLDOZER);
	}
	GameMain_String_Dest(&strSndPath);
}

void L_PlaySound_SC2K1996(int nAttrib, int nDuration) {
	if (nAttrib) {
		if (snd_debug & SND_DEBUG_PLAYS)
			ConsoleLog(LOG_DEBUG, "SND:  L_PlaySound_SC2K1996(%d, %d) - Play: (%d, %d)\n", nAttrib, nDuration, LOWORD(nAttrib), HIWORD(nAttrib));
		PostThreadMessageA(dwSDLSoundThreadID, WM_SDL_PLAY, (WPARAM)nDuration, (LPARAM)nAttrib);
	}
	else {
		if (snd_debug & SND_DEBUG_PLAYS)
			ConsoleLog(LOG_DEBUG, "SND:  L_PlaySound_SC2K1996(0, 0) - Stop\n");
		PostThreadMessageA(dwSDLSoundThreadID, WM_SDL_STOP, 0, 0);
	}
}

extern "C" void __stdcall Hook_Sound_StopSoundBySoundID(int iSoundID) {
	CSound *pThis;

	__asm mov [pThis], ecx

	if (pThis->iSNDCurrSoundID == iSoundID)
		Game_Sound_StopSound(pThis);
}

extern "C" void __stdcall Hook_Sound_StopSound() {
	CSound *pThis;

	__asm mov [pThis], ecx

	L_PlaySound_SC2K1996(0, 0);
	pThis->bSNDWasPlaying = FALSE;
	Game_Sound_PlayPrioritySound(pThis);
}

extern "C" int __stdcall Hook_LoadSoundIntoBuffer(int iSoundID, void* lpBuffer) {
	DWORD nNumBytesToRead = 0;
	BOOL bSuccess = FALSE;

	if (snd_debug & SND_DEBUG_PLAYS)
		ConsoleLog(LOG_DEBUG, "SND:  Loading %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);

	if (mapSoundBuffers.find((DWORD)lpBuffer) != mapSoundBuffers.end()) {
		mapSoundBuffers[(DWORD)lpBuffer].iSoundID = iSoundID;
		mapSoundBuffers[(DWORD)lpBuffer].iReloadCount++;
	}
	else {
		mapSoundBuffers[(DWORD)lpBuffer] = { iSoundID, 1 };
	}

	bSuccess = FALSE;
	if (mapReplacementSounds.find(iSoundID) != mapReplacementSounds.end() && jsonSettingsCore[C_SC2KFIX][S_FIX_AUDIO][I_FIX_AUD_USESNDREPLACE].ToBool()) {
		memcpy_s(lpBuffer, mapReplacementSounds[iSoundID].nBufSize, mapReplacementSounds[iSoundID].bBuffer, mapReplacementSounds[iSoundID].nBufSize);
		nNumBytesToRead = mapReplacementSounds[iSoundID].nBufSize;
		if (snd_debug & SND_DEBUG_PLAYS)
			ConsoleLog(LOG_DEBUG, "SND:  Detour! Copied replacement %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);
		bSuccess = TRUE;
	}
	else {
		char szSoundFileName[24 + 1], szCurrentSoundPath[MAX_PATH + 1];
		if (lpBuffer) {
			strcpy_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundPath);
			sprintf_s(szSoundFileName, sizeof(szSoundFileName) - 1, aDWav, iSoundID);
			strcat_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundFileName);
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "LoadSoundIntoBuffer(%d, 0x%06X): [%s] [%s]\n", iSoundID, lpBuffer, szCurrentSoundPath, szSoundFileName);
			FILE *f = old_fopen(szCurrentSoundPath, "rb");
			if (f) {
				fseek(f, 0, SEEK_END);
				nNumBytesToRead = ftell(f);
				fseek(f, 0, SEEK_SET);
				fread(lpBuffer, nNumBytesToRead, 1, f);
				fclose(f);
				bSuccess = TRUE;
			}
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "LoadSoundIntoBuffer(%d, 0x%06X): [%s] [%s] - bSuccess(%d)\n", iSoundID, lpBuffer, szCurrentSoundPath, szSoundFileName, bSuccess);
			// This should only be encountered if the original file failed to load
			// and was never cached.
			if (!mapSoundCache[iSoundID].pBuffer)
				bSuccess = LoadSoundFromFile(iSoundID, string_format("%s\\SOUNDS\\%d.wav", szGamePath, iSoundID)) ? TRUE : FALSE;
		}
	}

	return bSuccess;
}

extern "C" void __stdcall Hook_Sound_LoadClickSound() {
	CSound *pThis;

	__asm mov [pThis], ecx

	Game_LoadSoundIntoBuffer(SOUND_CLICK, pThis->dwSNDBufferClick);
}

extern "C" void __stdcall Hook_Sound_LoadExplosionSound() {
	CSound *pThis;

	__asm mov [pThis], ecx

	Game_LoadSoundIntoBuffer(SOUND_EXPLODE, pThis->dwSNDBufferExplosion);
}

extern "C" void __stdcall Hook_Sound_DestroySoundLayer() {
	CSound *pThis;

	__asm mov [pThis], ecx

	Game_Sound_MusicStop(pThis);
	Game_Sound_StopSound(pThis);
	if (pThis->dwSNDBufferTool) {
		free(pThis->dwSNDBufferTool);
		pThis->dwSNDBufferTool = 0;
	}
	if (pThis->dwSNDBufferActionThing) {
		free(pThis->dwSNDBufferActionThing);
		pThis->dwSNDBufferActionThing = 0;
	}
	if (pThis->dwSNDBufferExplosion) {
		free(pThis->dwSNDBufferExplosion);
		pThis->dwSNDBufferExplosion = 0;
	}
	if (pThis->dwSNDBufferGeneral) {
		free(pThis->dwSNDBufferGeneral);
		pThis->dwSNDBufferGeneral = 0;
	}
	_heapmin();
}

static int __stdcall L_Sound_LoadActionThingSound_SC2K1996(CSound *pSound, int iSoundID) {
	if (pSound->iSNDActionThingSoundID != iSoundID) {
		if (Game_LoadSoundIntoBuffer(iSoundID, pSound->dwSNDBufferActionThing))
			pSound->iSNDActionThingSoundID = iSoundID;
		else
			pSound->iSNDActionThingSoundID = SOUND_BULLDOZER;
	}
	return pSound->iSNDActionThingSoundID;
}

extern "C" void __stdcall Hook_Sound_PlayActionThingSound(int iSoundID, int nDuration) {
	CSound *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	int nAttrib;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp->dwSCAGameSound) {
		if (bMainFrameInactive || IsIconic(pSCApp->m_pMainWnd->m_hWnd)) {
			if (iSoundID != SOUND_SIREN)
				return;
		}
		if (iSoundID != pThis->iSNDActionThingSoundID || !pThis->bSNDWasPlaying || !pThis->bSNDPlaySound) {
			if (pThis->iSNDActionThingSoundID == SOUND_SIREN && pThis->bSNDWasPlaying && pThis->bSNDPlaySound)
				nCurrentActionThingSoundID = iSoundID;
			else {
				if (iSoundID != pThis->iSNDActionThingSoundID)
					iSoundID = L_Sound_LoadActionThingSound_SC2K1996(pThis, iSoundID);
				pThis->bSNDWasPlaying = TRUE;
				ULOWORD(nAttrib) = iSoundID;
				UHIWORD(nAttrib) = (pThis->iSNDActionThingSoundID == iSoundID && pThis->dwSNDBufferActionThing != 0) ? SND_ORIG_ACTSND_EXIST : SND_ORIG_ACTSND_NONBUF;
				L_PlaySound_SC2K1996(nAttrib, nDuration);
			}
		}
	}
}

extern "C" int __stdcall Hook_Sound_GetToolSound() {
	CSound *pThis;

	__asm mov [pThis], ecx

	__int16 iSelectedCitySubtool;
	int iSND = pThis->iSNDToolSoundID;

	if (wCityMode == GAME_MODE_CITY || wCityMode == GAME_MODE_DISASTER) {
		iSelectedCitySubtool = wSelectedSubtool[wCurrentCityToolGroup];
		switch (wCurrentCityToolGroup) {
			case CITYTOOL_GROUP_NATURE:
				if (iSelectedCitySubtool == NATURE_WATER)
					iSND = SOUND_FLOOD;
				else
					iSND = SOUND_PLOP;
				break;
			case CITYTOOL_GROUP_DISPATCH:
				if (iSelectedCitySubtool == DISPATCH_FIRE)
					iSND = SOUND_FIRETRUCK;
				else if (iSelectedCitySubtool == DISPATCH_MILITARY)
					iSND = SOUND_MILITARY;
				else
					iSND = SOUND_POLICE;
				break;
			case CITYTOOL_GROUP_POWER:
				if (iSelectedCitySubtool > POWER_WIRES)
					iSND = SOUND_BUILD;
				else
					iSND = SOUND_ZAP;
				break;
			case CITYTOOL_GROUP_WATER:
			case CITYTOOL_GROUP_PORTS:
				iSND = SOUND_BUILD;
				break;
			case CITYTOOL_GROUP_REWARDS:
				iSND = SOUND_CHEERS;
				break;
			case CITYTOOL_GROUP_ROADS:
				if (iSelectedCitySubtool == ROADS_BUSSTATION)
					iSND = SOUND_HORNS;
				else
					iSND = SOUND_BUILD;
				break;
			case CITYTOOL_GROUP_RAIL:
				// Originally the 'RAILS_DEPOT' case was set by an uninitialized int
				// so it's now been set to use the train whistle sound (as expected
				// in the Mac version).
				if (iSelectedCitySubtool == RAILS_DEPOT)
					iSND = SOUND_TRAIN;
				else
					iSND = SOUND_BUILD;
				break;
			case CITYTOOL_GROUP_RESIDENTIAL:
			case CITYTOOL_GROUP_COMMERCIAL:
			case CITYTOOL_GROUP_INDUSTRIAL:
				iSND = SOUND_PLOP;
				break;
			case CITYTOOL_GROUP_EDUCATION:
				iSND = SOUND_SCHOOL;
				break;
			case CITYTOOL_GROUP_SERVICES:
				if (iSelectedCitySubtool == SERVICES_FIRESTATION)
					iSND = SOUND_FIRETRUCK;
				else if (iSelectedCitySubtool == SERVICES_PRISON)
					iSND = SOUND_PRISON;
				else
					iSND = SOUND_POLICE;
				break;
			case CITYTOOL_GROUP_PARKS:
				if (iSelectedCitySubtool == PARKS_ZOO)
					iSND = SOUND_MONSTER;
				else
					iSND = SOUND_CHEERS;
				break;
		}
	}
	else if (wCurrentMapToolGroup >= MAPTOOL_GROUP_TREES && wCurrentMapToolGroup <= MAPTOOL_GROUP_FOREST)
		iSND = SOUND_PLOP;
	else
		iSND = SOUND_FLOOD;

	return iSND;
}

extern "C" void __stdcall Hook_Sound_LoadActionThingSound(int iSoundID) {
	CSound *pThis;

	__asm mov [pThis], ecx

	L_Sound_LoadActionThingSound_SC2K1996(pThis, iSoundID);
}

extern "C" void __stdcall Hook_Sound_StopActionThingSound(int iSoundID) {
	CSound *pThis;

	__asm mov [pThis], ecx

	if (pThis->iSNDActionThingSoundID == iSoundID) {
		pThis->bSNDWasPlaying = FALSE;
		if (pThis->iSNDCurrSoundID == iSoundID) {
			L_PlaySound_SC2K1996(0, 0);
			pThis->bSNDPlaySound = FALSE;
			pThis->iSNDCurrSoundID = -1;
		}
	}
}

extern "C" void __stdcall Hook_Sound_PlaySound(int iSoundID) {
	CSound *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	int nAttrib;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp->dwSCAGameSound) {
		if (bMainFrameInactive || !pSCApp->m_pMainWnd || IsIconic(pSCApp->m_pMainWnd->m_hWnd)) {
			if (iSoundID != SOUND_SIREN)
				return;
		}
		if (iSoundID >= SOUND_START && 
			iSoundID < SOUND_SILENT || bSoundKickstart) {
			if (bSoundKickstart) {
				bSoundKickstart = false;
				if (iSoundID < SOUND_START || iSoundID > SOUND_SILENT)
					return;
			}
			int nSoundPlayTicksEntry = GetSoundPlayTicksBySoundID_SC2K1996(iSoundID);
			if (!pThis->bSNDPlaySound || pThis->iSNDCurrSoundID != iSoundID || nSoundPlayTicksEntry - nActionThingSoundPlayTicks >= 3) {
				ULOWORD(nAttrib) = iSoundID;
				UHIWORD(nAttrib) = SND_ORIG_PLAYSND;
				if (pThis->iSNDActionThingSoundID == pThis->iSNDCurrSoundID && pThis->bSNDWasPlaying)
					nActionThingSoundPlayTicksCurrent = nActionThingSoundPlayTicks;
				else
					nActionThingSoundPlayTicksCurrent = 0;

				L_PlaySound_SC2K1996(nAttrib, 0);
			}
		}
	}
}

static bool LoadSoundFromFile(int iSoundID, std::string strPath) {
	SF_INFO stSoundFileInfo;
	audio_entity_t stAudioEntity;
	SNDFILE* sndfile = SF_open(strPath.c_str(), SFM_READ, &stSoundFileInfo);

	memset(&mapSoundCache[iSoundID], 0, sizeof(mapSoundCache[iSoundID]));

	if (!sndfile) {
		ConsoleLog(LOG_ERROR, "SND:  Couldn't load sound file \"%s\" (sndfile).\n", strPath.c_str());
		return false;
	}

	// Build the audio entity data
	stAudioEntity.iFrames = (int)stSoundFileInfo.frames;
	stAudioEntity.iSampleRate = stSoundFileInfo.samplerate;
	stAudioEntity.iChannels = stSoundFileInfo.channels;
	stAudioEntity.iFormat = stSoundFileInfo.format;
	stAudioEntity.bSeekable = stSoundFileInfo.seekable ? true : false;
	stAudioEntity.uBufferSize = stAudioEntity.iChannels * sizeof(short) * stAudioEntity.iFrames;
	stAudioEntity.pBuffer = (short*)malloc(stAudioEntity.uBufferSize);
	if (!stAudioEntity.pBuffer) {
		ConsoleLog(LOG_ERROR, "SND:  Couldn't load sound file \"%s\" (malloc).\n", strPath.c_str());
		return false;
	}

	// Read the audio into the buffer as 16-bit PCM
	SF_readf_short(sndfile, stAudioEntity.pBuffer, stAudioEntity.iFrames);

	// Cache it and return
	mapSoundCache[iSoundID] = stAudioEntity;
	SF_close(sndfile);
	return true;
}

static BOOL LoadSoundFromResource(int iSoundID, int iResourceID) {
    HRSRC hResFind;
    HGLOBAL hWaveResource;

    hResFind = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(iResourceID), "WAVE");
    if (hResFind) {
        hWaveResource = LoadResource(hSC2KFixModule, hResFind);
        if (hWaveResource) {
            sound_replacement_t entry;
            entry.nBufSize = SizeofResource(hSC2KFixModule, hResFind);
            entry.bBuffer = (BYTE*)malloc(entry.nBufSize);

            if (entry.bBuffer) {
                mapReplacementSounds[iSoundID] = entry;
                void* ptr = LockResource(hWaveResource);
                if (ptr) {
                    memcpy_s(entry.bBuffer, entry.nBufSize, ptr, entry.nBufSize);
                    FreeResource(hWaveResource);

					// There's probably a better way to do this.
					char szTempFileName[L_tmpnam];
					tmpnam_s(szTempFileName);
					FILE* fileTempWav;
					fopen_s(&fileTempWav, szTempFileName, "wb");
					if (fileTempWav) {
						fwrite(entry.bBuffer, 1, entry.nBufSize, fileTempWav);
						fclose(fileTempWav);
						LoadSoundFromFile(iSoundID, szTempFileName);
						_unlink(szTempFileName);
					}

                    if (snd_debug & SND_DEBUG_REPLACEMENTS)
                        ConsoleLog(LOG_DEBUG, "SND:  Loaded replacement for %d.wav.\n", iSoundID);
                        return TRUE;
                }
            }
            else
                ConsoleLog(LOG_ERROR, "SND:  Couldn't allocate replacement sound buffer for sound %d.\n", iSoundID);
        }
    }
    else
        ConsoleLog(LOG_ERROR, "SND:  Couldn't find resource for sound %d.\n", iSoundID);

    return FALSE;
}

void L_PlayToolSound_SC2K1996(CSimcityAppPrimary *pSCApp, int iSoundID) {
	int iSND;

	if (!iSoundID)
		Game_SimcityApp_GetToolSound(pSCApp);
	iSND = (!iSoundID) ? pSCApp->SCASNDLayer->iSNDToolSoundID : iSoundID;
	Game_SimcityApp_SoundPlaySound(pSCApp, iSND);
}

void InstallSoundEngineHooks_SC2K1996(void) {
	// Hook CSound::PlayPrioritySound
	SafeVirtualProtect((LPVOID)0x401163, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401163, Hook_Sound_PlayPrioritySound);

	// Hook CSound::InitSoundLayer
	SafeVirtualProtect((LPVOID)0x4015FF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015FF, Hook_Sound_InitSoundLayer);

	// Hook CSound::StopSoundBySoundID
	SafeVirtualProtect((LPVOID)0x4019F1, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4019F1, Hook_Sound_StopSoundBySoundID);

	// Hook CSound::StopSound
	SafeVirtualProtect((LPVOID)0x401BB3, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401BB3, Hook_Sound_StopSound);

	// Hook sound buffer loading
	SafeVirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401F9B, Hook_LoadSoundIntoBuffer);

	// Hook CSound::LoadClickSound
	SafeVirtualProtect((LPVOID)0x40212B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40212B, Hook_Sound_LoadClickSound);

	// Hook CSound::LoadExplosionSound
	SafeVirtualProtect((LPVOID)0x40257C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40257C, Hook_Sound_LoadExplosionSound);

	// Hook CSound::DestroySoundLayer
	SafeVirtualProtect((LPVOID)0x4026CB, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4026CB, Hook_Sound_DestroySoundLayer);

	// Hook CSound::PlayActionSound
	SafeVirtualProtect((LPVOID)0x4026DF, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4026DF, Hook_Sound_PlayActionThingSound);

	// Hook CSound::GetToolSound
	SafeVirtualProtect((LPVOID)0x40291E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40291E, Hook_Sound_GetToolSound);

	// Hook CSound::LoadThingSound
	SafeVirtualProtect((LPVOID)0x402B8A, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402B8A, Hook_Sound_LoadActionThingSound);

	// Hook CSound::StopActionSound
	SafeVirtualProtect((LPVOID)0x402F31, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402F31, Hook_Sound_StopActionThingSound);

	// Hook CSound::PlaySound
	SafeVirtualProtect((LPVOID)0x403026, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403026, Hook_Sound_PlaySound);

	// Cache all sounds
	LARGE_INTEGER uTickStart, uTickEnd, uTicksPerSecond;
	QueryPerformanceFrequency(&uTicksPerSecond);
	QueryPerformanceCounter(&uTickStart);
	for (int i = SOUND_START; i <= SOUND_SILENT; i++)
		LoadSoundFromFile(i, string_format("%s\\SOUNDS\\%d.wav", szGamePath, i));
	QueryPerformanceCounter(&uTickEnd);
	if (snd_debug & SND_DEBUG_INTERNALS)
		ConsoleLog(LOG_DEBUG, "SND:  Pre-caching sounds from disk took %llu microseconds.\n", ((uTickEnd.QuadPart - uTickStart.QuadPart) * 1000000 / uTicksPerSecond.QuadPart));

	// Load the replacement sound resources
	LoadSoundFromResource(SOUND_BUILD, IDR_WAVE_500);
	LoadSoundFromResource(SOUND_PLOP, IDR_WAVE_503);
	LoadSoundFromResource(SOUND_BULLDOZER, IDR_WAVE_508);
	LoadSoundFromResource(SOUND_ZAP, IDR_WAVE_514);
	LoadSoundFromResource(SOUND_RETICULATINGSPLINES, IDR_WAVE_529);
}
