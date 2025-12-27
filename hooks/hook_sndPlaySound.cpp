// sc2kfix hooks/hook_sndPlaySound.cpp: hook for sndPlaySoundA
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

#define SND_DEBUG SND_DEBUG_REPLACEMENTS

#ifdef DEBUGALL
#undef SND_DEBUG
#define SND_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT snd_debug = SND_DEBUG;

std::map<DWORD, soundbufferinfo_t> mapSoundBuffers;
std::map<int, sound_replacement_t> mapReplacementSounds;

static DWORD dwDummy;

static int GetSoundPosBySoundID(int iSoundID) {
	return iSoundID - SOUND_START;
}

static int GetSoundPlayTicksBySoundID_SC2K1996(int iSoundID) {
	for (int i = 0; i < SOUND_ENTRIES; i++) {
		if (GetSoundPosBySoundID(iSoundID) == i) {
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "GetSoundPlayTicksBySoundID_SC2K1996(%d): i(%d) nSoundPlayTicks[i](%d)\n", iSoundID, i, nSoundPlayTicks[i]);
			return nSoundPlayTicks[i];
		}
	}
	return 0;
}

static int GetTickDurationBySoundID_SC2K1996(int iSoundID, int nDuration) {
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
	pThis->dwSNDMCIError = 0;
	pThis->dwSNDUnknownTwo = -1;
	pThis->bSNDPlaySound = FALSE;
	pThis->bSNDWasPlaying = FALSE;
	pThis->iSNDCurrSoundID = -1;
	pThis->iSNDActionThingSoundID = -1;
	pThis->iSNDToolSoundID = -1;
	pThis->iSNDGeneralSoundID = -1;
	pThis->wSNDMCIDevID = -1;
	Game_SimcityApp_GetValueStringA(pSCApp, &strSndPath, aPaths, aMusic);
	strcpy(szSoundPath, strSndPath.m_pchData);
	if (!szSoundPath[0])
		strcpy(szSoundPath, aSounds);
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

BOOL L_PlaySound_SC2K1996(LPCTSTR pszSound, UINT fuSound) {
	if (snd_debug & SND_DEBUG_PLAYS) {
		if (fuSound & SND_MEMORY)
			ConsoleLog(LOG_DEBUG, "SND:  L_PlaySound_SC2K1996(<0x%06X>, 0x%06X)\n", pszSound, fuSound);
		else if (!pszSound && !fuSound)
			ConsoleLog(LOG_DEBUG, "SND:  L_PlaySound_SC2K1996(0, 0)\n");
		else
			ConsoleLog(LOG_DEBUG, "SND:  L_PlaySound_SC2K1996(%s, 0x%06X)\n", (pszSound ? pszSound : "NULL"), fuSound);
	}
	return sndPlaySoundA(pszSound, fuSound);
	//return PlaySoundA(pszSound, NULL, fuSound);
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
	if (mapReplacementSounds.find(iSoundID) != mapReplacementSounds.end() && bSettingsUseSoundReplacements) {
		memcpy_s(lpBuffer, mapReplacementSounds[iSoundID].nBufSize, mapReplacementSounds[iSoundID].bBuffer, mapReplacementSounds[iSoundID].nBufSize);
		nNumBytesToRead = mapReplacementSounds[iSoundID].nBufSize;
		if (snd_debug & SND_DEBUG_PLAYS)
			ConsoleLog(LOG_DEBUG, "SND:  Detour! Copied replacement %d.wav into buffer <0x%08X>.\n", iSoundID, lpBuffer);
		bSuccess = TRUE;
	}
	else {
		char szSoundFileName[24 + 1], szCurrentSoundPath[MAX_PATH + 1];
		CMFC3XFile cFile;
		GameMain_File_Cons(&cFile);
		if (lpBuffer) {
			strcpy_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundPath);
			sprintf_s(szSoundFileName, sizeof(szSoundFileName) - 1, aDWav, iSoundID);
			strcat_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundFileName);
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "LoadSoundIntoBuffer(%d, 0x%06X): [%s] [%s]\n", iSoundID, lpBuffer, szCurrentSoundPath, szSoundFileName);
			if (GameMain_File_Open(&cFile, szCurrentSoundPath, 0, 0)) {
				nNumBytesToRead = GameMain_File_GetLength(&cFile);
				GameMain_File_Read(&cFile, lpBuffer, nNumBytesToRead);
				GameMain_File_Close(&cFile);
				bSuccess = TRUE;
			}
			if (snd_debug & SND_DEBUG_INTERNALS)
				ConsoleLog(LOG_DEBUG, "LoadSoundIntoBuffer(%d, 0x%06X): [%s] [%s] - bSuccess(%d)\n", iSoundID, lpBuffer, szCurrentSoundPath, szSoundFileName, bSuccess);
		}
		GameMain_File_Dest(&cFile);
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

extern "C" void __stdcall Hook_Sound_PlayActionThingSound(int iSoundID, int nDuration) {
	CSound *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;

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
					Game_Sound_LoadActionThingSound(pThis, iSoundID);
				pThis->bSNDWasPlaying = TRUE;
				if (pThis->iSNDActionThingSoundID == iSoundID && pThis->dwSNDBufferActionThing != 0) {
					if (L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferActionThing, SND_ASYNC | SND_MEMORY | SND_LOOP)) {
						pThis->iSNDActionThingSoundID = iSoundID;
						pThis->iSNDCurrSoundID = iSoundID;
						pThis->bSNDPlaySound = TRUE;
						pThis->bSNDWasPlaying = TRUE;
						nActionThingSoundPlayTicks = (nDuration <= 0) ? 0 : GetTickDurationBySoundID_SC2K1996(iSoundID, nDuration);
						if (snd_debug & SND_DEBUG_INTERNALS)
							ConsoleLog(LOG_DEBUG, "CSound::PlayActionThingSound(%d, %d): nActionThingSoundPlayTicks(%d)\n", iSoundID, nDuration, nActionThingSoundPlayTicks);
					}
					else
						pThis->bSNDWasPlaying = FALSE;
				}
				else {
					if (snd_debug & SND_DEBUG_INTERNALS)
						ConsoleLog(LOG_DEBUG, "CSound::PlayActionThingSound(%d, %d): ActionSoundBufferEmpty.\n");

					char szSoundFileName[24 + 1], szCurrentSoundPath[MAX_PATH + 1];

					strcpy_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundPath);
					sprintf_s(szSoundFileName, sizeof(szSoundFileName) - 1, aDWav, iSoundID);
					strcat_s(szCurrentSoundPath, sizeof(szCurrentSoundPath) - 1, szSoundFileName);
					pThis->bSNDPlaySound = L_PlaySound_SC2K1996(szCurrentSoundPath, SND_ASYNC | SND_NODEFAULT | SND_LOOP);
					nActionThingSoundPlayTicks = (nDuration <= 0) ? 0 : GetTickDurationBySoundID_SC2K1996(iSoundID, nDuration);
					pThis->iSNDCurrSoundID = iSoundID;
					pThis->bSNDWasPlaying = pThis->bSNDPlaySound;
				}
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

	if (pThis->iSNDActionThingSoundID != iSoundID) {
		if (Game_LoadSoundIntoBuffer(iSoundID, pThis->dwSNDBufferActionThing))
			pThis->iSNDActionThingSoundID = iSoundID;
	}
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

	pSCApp = &pCSimcityAppThis;
	if (pSCApp->dwSCAGameSound) {
		if (bMainFrameInactive || !pSCApp->m_pMainWnd || IsIconic(pSCApp->m_pMainWnd->m_hWnd)) {
			if (iSoundID != SOUND_SIREN)
				return;
		}
		if (iSoundID >= SOUND_START && 
			iSoundID < SOUND_SILENT) {
			int nSoundPlayTicksEntry = GetSoundPlayTicksBySoundID_SC2K1996(iSoundID);
			if (!pThis->bSNDPlaySound || pThis->iSNDCurrSoundID != iSoundID || nSoundPlayTicksEntry - nActionThingSoundPlayTicks >= 3) {
				if (pThis->iSNDActionThingSoundID == pThis->iSNDCurrSoundID && pThis->bSNDWasPlaying)
					nActionThingSoundPlayTicksCurrent = nActionThingSoundPlayTicks;
				else
					nActionThingSoundPlayTicksCurrent = 0;

				BOOL bRet = -1;
				if (iSoundID == SOUND_CLICK && pThis->dwSNDBufferClick != 0)
					bRet = L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferClick, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
				else if (iSoundID == SOUND_EXPLODE && pThis->dwSNDBufferExplosion != 0)
					bRet = L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferExplosion, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
				else if (pThis->iSNDToolSoundID == iSoundID && pThis->dwSNDBufferTool != 0)
					bRet = L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferTool, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
				else {
					if (pThis->iSNDActionThingSoundID == iSoundID) {
						if (pThis->dwSNDBufferActionThing) {
							bRet = L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferActionThing, SND_ASYNC | SND_NODEFAULT | SND_MEMORY | SND_LOOP);
							pThis->bSNDWasPlaying = bRet;
						}
					}
					if (bRet < 0) {
						if (pThis->iSNDGeneralSoundID != iSoundID) {
							Game_LoadSoundIntoBuffer(iSoundID, pThis->dwSNDBufferGeneral);
							pThis->iSNDGeneralSoundID = iSoundID;
						}
						bRet = L_PlaySound_SC2K1996((LPCSTR)pThis->dwSNDBufferGeneral, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
					}
				}
				pThis->bSNDPlaySound = bRet;
				if (pThis->bSNDPlaySound) {
					pThis->iSNDCurrSoundID = iSoundID;
					if (pThis->bSNDWasPlaying && pThis->iSNDActionThingSoundID == iSoundID)
						nActionThingSoundPlayTicks = 0;
					else
						nActionThingSoundPlayTicks = nSoundPlayTicksEntry;
				}
				else
					Game_Sound_PlayPrioritySound(pThis);
			}
		}
	}
}

static BOOL LoadReplacementSoundFromResource(int iResourceID, int iSoundID) {
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
	VirtualProtect((LPVOID)0x401163, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401163, Hook_Sound_PlayPrioritySound);

	// Hook CSound::InitSoundLayer
	VirtualProtect((LPVOID)0x4015FF, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4015FF, Hook_Sound_InitSoundLayer);

	// Hook CSound::StopSoundBySoundID
	VirtualProtect((LPVOID)0x4019F1, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4019F1, Hook_Sound_StopSoundBySoundID);

	// Hook CSound::StopSound
	VirtualProtect((LPVOID)0x401BB3, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401BB3, Hook_Sound_StopSound);

	// Hook sound buffer loading
	VirtualProtect((LPVOID)0x401F9B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401F9B, Hook_LoadSoundIntoBuffer);

	// Hook CSound::LoadClickSound
	VirtualProtect((LPVOID)0x40212B, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40212B, Hook_Sound_LoadClickSound);

	// Hook CSound::LoadExplosionSound
	VirtualProtect((LPVOID)0x40257C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40257C, Hook_Sound_LoadExplosionSound);

	// Hook CSound::DestroySoundLayer
	VirtualProtect((LPVOID)0x4026CB, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026CB, Hook_Sound_DestroySoundLayer);

	// Hook CSound::PlayActionSound
	VirtualProtect((LPVOID)0x4026DF, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026DF, Hook_Sound_PlayActionThingSound);

	// Hook CSound::GetToolSound
	VirtualProtect((LPVOID)0x40291E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40291E, Hook_Sound_GetToolSound);

	// Hook CSound::LoadThingSound
	VirtualProtect((LPVOID)0x402B8A, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402B8A, Hook_Sound_LoadActionThingSound);

	// Hook CSound::StopActionSound
	VirtualProtect((LPVOID)0x402F31, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402F31, Hook_Sound_StopActionThingSound);

	// Hook CSound::PlaySound
	VirtualProtect((LPVOID)0x403026, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403026, Hook_Sound_PlaySound);

	// Load the replacement sound resources
	LoadReplacementSoundFromResource(IDR_WAVE_500, SOUND_BUILD);
	LoadReplacementSoundFromResource(IDR_WAVE_503, SOUND_PLOP);
	LoadReplacementSoundFromResource(IDR_WAVE_508, SOUND_BULLDOZER);
	LoadReplacementSoundFromResource(IDR_WAVE_514, SOUND_ZAP);
	LoadReplacementSoundFromResource(IDR_WAVE_529, SOUND_RETICULATINGSPLINES);
}
