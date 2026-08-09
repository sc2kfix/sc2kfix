// sc2kfix modules/sc2x.cpp: JSON-based extensible save game file format
//                           and hooks for fixing save/load bugs.
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

//#define SC2X_USE_VANILLA_LOAD_REPLACEMENT

#define SC2X_DEBUG_LOAD 1
#define SC2X_DEBUG_SAVE 2
#define SC2X_DEBUG_VANILLA_LOAD 4
#define SC2X_DEBUG_VANILLA_SAVE 8
#define SC2X_DEBUG_JSON_LOAD 16
#define SC2X_DEBUG_JSON_SAVE 32

#define SC2X_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SC2X_DEBUG
#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#define BAILOUT(s, ...) do { \
	ConsoleLog(LOG_ERROR, "SAVE: " s, __VA_ARGS__); \
	return 0; \
} while (0)

UINT sc2x_debug = SC2X_DEBUG;

static DWORD *pMiscInfo = NULL;

#define MISCINF_ALLOC_SIZE   0x12C0 // Pending demystification
#define FULLMAP_ALLOC_SIZE   (GAME_MAP_SIZE * GAME_MAP_SIZE)
#define ALTM_ALLOC_SIZE      (FULLMAP_ALLOC_SIZE * 2)
#define MINIMAP64_ALLOC_SIZE (MINI_MAP_64 * MINI_MAP_64)
#define MINIMAP32_ALLOC_SIZE (MINI_MAP_32 * MINI_MAP_32)
#define LABEL_ALLOC_SIZE     (MAX_LABEL_COUNT * sizeof(map_XLAB_t))
#define MICROSIM_ALLOC_SIZE  (MAX_MICROSIM_COUNT * sizeof(microsim_t))
#define THING_ALLOC_SIZE     (MAX_THING_COUNT * sizeof(map_XTHG_t))
#define GRAPH_ALLOC_SIZE     (MAX_GRAPHS * MAX_GRAPH_ENTRIES * sizeof(DWORD))

void LoadInterleavedBudgetVanilla(budget_t* pTarget, DWORD* pSource) {
	pTarget->iCurrentCosts = ntohl(pSource[0]);
	pTarget->iFundingPercent = ntohl(pSource[1]);
	pTarget->iYearToDateCost = ntohl(pSource[2]);
	pTarget->iEstimatedCost = 0;

	for (int i = 0; i < 12; i++) {
		pTarget->iCountMonth[i] = ntohl(pSource[3 + i * 2]);
		pTarget->iFundMonth[i] = ntohl(pSource[4 + i * 2]);
	}
}

#ifdef SC2X_USE_VANILLA_LOAD_REPLACEMENT
// WIP replacement for CSimcityApp::OpenCity for vanilla save game files.
// This is incredibly ugly and should probably be rewritten at some point.
BOOL SC2XLoadVanillaGame(CSimcityAppPrimary* pThis, const char* szFileName) {
	if (!szFileName)
		return FALSE;

	std::ifstream infile(szFileName, std::ios::binary | std::ios::ate);
	size_t sc2size = infile.tellg();
	BYTE* sc2file = (BYTE*)malloc(sc2size);
	if (!sc2file)
		BAILOUT("Couldn't malloc %d bytes for sc2file.\n", sc2size);

	infile.seekg(0, std::ios::beg);
	infile.read((char*)sc2file, sc2size);
	infile.close();

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: Read %d bytes of \"%s\" into sc2file buffer.\n", sc2size, szFileName);

	if (*(DWORD*)&sc2file[0] != IFF_HEAD('F', 'O', 'R', 'M') || *(DWORD*)&sc2file[8] != IFF_HEAD('S', 'C', 'D', 'H'))
		BAILOUT("Save file is not a valid vanilla SC2 file.\n");

	int iChunkStart = 12;
	int iChunkSize = 0;
	int iConvertedChunks = 0;

	do {
		iChunkStart += iChunkSize;
		iChunkSize = ntohl(*(DWORD*)&sc2file[iChunkStart + 4]);
		if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
			ConsoleLog(LOG_DEBUG, "LOAD: dwChunkType = '%c%c%c%c', iChunkStart = 0x%08X, iChunkSize = %d\n", sc2file[iChunkStart], sc2file[iChunkStart + 1], sc2file[iChunkStart + 2], sc2file[iChunkStart + 3], iChunkStart, iChunkSize);

		for (int i = 0; i < iChunkSize; ) {
			if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('C', 'N', 'A', 'M')) {
				std::string strCityName((char*)&sc2file[iChunkStart + 9]);
				GameMain_String_OperatorSet(&pszCityName, (char *)strCityName.c_str());
				i += iChunkSize;
				iConvertedChunks++;
			}
			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('M', 'I', 'S', 'C')) {
				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkMISC = (BYTE*)malloc(4800);
				if (!pChunkMISC)
					BAILOUT("Couldn't malloc 4800 bytes for MISC.");

				MaxisDecompress(pChunkMISC, 4800, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));

				//sc2json["MISC"]["dwAlways290"] = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityMode = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wViewRotation = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityStartYear = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityDays = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityFunds = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityBonds = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityDifficulty = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityProgression = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityValue = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityLandValue = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityCrime = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityTrafficCount = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityPollution = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityFame = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityAdvertising = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityGarbage = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityWorkforcePercent = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityWorkforceLE = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityWorkforceEQ = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwNationalPopulation = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwNationalValue = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wNationalFedRate = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wNationalEconomyTrend = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherHeat = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherWind = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherRain = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherTrend = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wSetTriggerDisasterType = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityOldResPopulation = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityRewardsUnlocked = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				// TODO: figure out what this actually is
				for (int i = 0; i < 20; i++) {
					pRawPopRatioTable[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					pEQRatioTable[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					pLERatioTable[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
				}
				i += 4 * 60;

				for (int i = 0; i < 11; i++) {
					pIndividualIndDemands[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					pIndividualIndTaxRate[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					pIndividualIndRatio[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
				}
				i += 4 * 33;

				for (int i = 0; i < 256; i++)
					wTileCount[i] = (BYTE)(ntohl(*(DWORD*)&pChunkMISC[i * 4]));
				i += 4 * 256;

				for (int i = 0; i < 8; i++)
					pZonePops[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 8;

				for (int i = 0; i < 50; i++)
					wArrBondData[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 50;

				// TODO: Encode as arrays of useful JSON
				for (int i = 0; i < 4; i++) {
					wNeighborNameIdx[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					if (wNeighborNameIdx[i])
						Game_LoadNamedEntryFromRsrcOffset((char*)stNeighborCities + i * 32, 1000, wNeighborNameIdx[i]);
					else
						strcpy_s((char*)stNeighborCities + i * 32, 32, "Ocean");
					dwNeighborPopulation[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					dwNeighborValue[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
					dwNeighborFame[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 12]);
				}

				for (int i = 0; i < 8; i++)
					wCityDemand[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 8;

				for (int i = 0; i < 17; i++)
					wCityInventionYears[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 17;

				LoadInterleavedBudgetVanilla(pBudgetArr, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+1, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+2, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+3, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+4, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+5, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+6, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+7, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+8, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+9, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+10, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+11, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+12, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+13, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+14, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				LoadInterleavedBudgetVanilla(pBudgetArr+15, (DWORD*)&pChunkMISC[i]);
				i += 4 * 27;

				bYearEndFlag = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wWaterLevel = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bCityHasOcean = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bCityHasRiver = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bMilitaryBaseType = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				// 2026-07-28 - AF: The types for pPaperArr and pNewsArr
				// were adjusted; the loops have since been adjusted and
				// the function does call once more, however it has NOT
				// yet been tested.

				// TODO: Encode as arrays of useful JSON
				//sc2json["MISC"]["pPaperArr"] = EncodeDWORDArray((DWORD*)&pChunkMISC[i], 30, TRUE);
				for (int i = 0; i < 6; i++) {
					pPaperArr[i].bName = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					pPaperArr[i].bStyle = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					pPaperArr[i].bTag = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
					pPaperArr[i].bSurvey = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 12]);
					pPaperArr[i].bWeather = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 16]);
				}
				i += 4 * 30;

				// TODO: Encode as arrays of useful JSON
				//pNewsArr = EncodeDWORDArray((DWORD*)&pChunkMISC[i], 54, TRUE);
				for (int i = 0; i < 9; i++) {
					*(WORD*)&pNewsArr[i].wType = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					*(WORD*)&pNewsArr[i].wPower = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					pNewsArr[i].bValue = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
					pNewsArr[i].bItem = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 12]);
					pNewsArr[i].bName = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 16]);
					pNewsArr[i].bScore = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 20]);
				}
				i += 4 * 54;

				dwCityOrdinances = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityUnemployment = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				// This table is staying as a Base64Encode because you SHOULD NOT mess with it
				//sc2json["MISC"]["wMilitaryTiles"] = Base64Encode(&pChunkMISC[i], 4 * 16);
				for (int i = 0; i < 16; i++)
					wMilitaryTiles[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 16;

				wSubwayXUNDCount = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				pThis->wSCAGameSpeedLOW = ntohl(*(DWORD*)&pChunkMISC[i]);		// XXX - CHECK IF THIS NEEDS TO BE THISCASTED
				pThis->wSCAGameSpeedHIGH = pThis->wSCAGameSpeedLOW;
				i += 4;

				bOptionsAutoBudget = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bOptionsAutoGoto = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				pThis->dwSCAGameSound = ntohl(*(DWORD*)&pChunkMISC[i]);	// XXX - needs a good name
				i += 4;

				pThis->dwSCAGameMusic = ntohl(*(DWORD*)&pChunkMISC[i]);
				if (!pThis->dwSCAGameMusic)
					Game_Sound_MusicStop(pThis->SCASNDLayer);
				i += 4;

				bNoDisasters = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bNewspaperSubscription = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bNewspaperExtra = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wNewspaperChoice = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				__int16 wViewCoords = ntohl(*(DWORD*)&pChunkMISC[i]);
				if (wViewCoords == -1) {
					wViewInitialCoordX = 64;
					wViewInitialCoordY = 128;		// I don't know. This is just what the game does.
				}
				else {
					wViewInitialCoordX = wViewCoords & 0x7F;
					wViewInitialCoordY = wViewCoords >> 8;
				}
				i += 4;

				wViewInitialZoom = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityCenterX = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCityCenterY = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwArcologyPopulation = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wConnectTiles = ntohl(*(DWORD*)&pChunkMISC[i]);	// Unused, but we'll load it anyways.
				i += 4;

				wStadiumSportsTeams = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityPopulation = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wIndustrialMixBonus = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wIndustrialMixPollutionBonus = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wOldArrests = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wPrisonBonus = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wDisasterObject = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCurrentDisasterType = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwDisasterActive = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wSewerBonus = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				free(pChunkMISC);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('A', 'L', 'T', 'M')) {
				//memcpy(dwMapALTM, &sc2file[iChunkStart + 8], 16384);
				for (int i = 0; i < 128; i++)
					memcpy(dwMapALTM[i], &sc2file[iChunkStart + 8 + i * 256], 256);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'E', 'R') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'B', 'L', 'D') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'Z', 'O', 'N') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'U', 'N', 'D') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'X', 'T') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'B', 'I', 'T')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(16384);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 16384 bytes for %s.", strIFFHead.c_str());

				MaxisDecompress(pChunkData, 16384, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'E', 'R'))
					memcpy(dwMapXTER, pChunkData, 16384);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'B', 'L', 'D'))
					memcpy(dwMapXBLD, pChunkData, 16384);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'Z', 'O', 'N'))
					memcpy(dwMapXZON, pChunkData, 16384);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'U', 'N', 'D'))
					memcpy(dwMapXUND, pChunkData, 16384);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'X', 'T'))
					memcpy(dwMapXTXT, pChunkData, 16384);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'B', 'I', 'T'))
					memcpy(dwMapXBIT, pChunkData, 16384);
				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'L', 'A', 'B')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(6400);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 6400 bytes for XLAB.");

				MaxisDecompress(pChunkData, 6400, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				memcpy(dwMapXLAB, pChunkData, 6400);
				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'R', 'F') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'L', 'T') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'V', 'A', 'L') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'C', 'R', 'M')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(4096);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 4096 bytes for %s.", strIFFHead.c_str());

				MaxisDecompress(pChunkData, 4096, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'R', 'F'))
					memcpy(dwMapXTRF, pChunkData, 4096);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'L', 'T'))
					memcpy(dwMapXPLT, pChunkData, 4096);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'V', 'A', 'L'))
					memcpy(dwMapXVAL, pChunkData, 4096);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'C', 'R', 'M'))
					memcpy(dwMapXCRM, pChunkData, 4096);
				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'G', 'R', 'P')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(3328);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 3328 bytes for XGRP.");

				MaxisDecompress(pChunkData, 3328, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				memcpy(dwMapXGRP, pChunkData, 3328);
				Game_FlipDWORDArrayEndianness(dwMapXGRP, 3328);
				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'M', 'I', 'C')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(1200);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 1200 bytes for XMIC.");

				MaxisDecompress(pChunkData, 1200, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				memcpy(pMicrosimArr, pChunkData, 1200);

				__asm {
					push 1200
					push pMicrosimArr
					mov eax, 0x401FB4
					call eax
				}

				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'L', 'C') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'F', 'I', 'R') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'O', 'P') ||
				*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'R', 'O', 'G')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(1024);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 1024 bytes for %s.", strIFFHead.c_str());

				MaxisDecompress(pChunkData, 1024, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'L', 'C'))
					memcpy(dwMapXPLC, pChunkData, 1024);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'F', 'I', 'R'))
					memcpy(dwMapXFIR, pChunkData, 1024);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'P', 'O', 'P'))
					memcpy(dwMapXPOP, pChunkData, 1024);
				else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'R', 'O', 'G'))
					memcpy(dwMapXROG, pChunkData, 1024);
				free(pChunkData);
				iConvertedChunks++;
			}

			else if (*(DWORD*)&sc2file[iChunkStart] == IFF_HEAD('X', 'T', 'H', 'G')) {
				std::string strIFFHead((const char*)&sc2file[iChunkStart], 4);

				// Allocate and decompressed a fixed length chunk
				BYTE* pChunkData = (BYTE*)malloc(480);
				if (!pChunkData)
					BAILOUT("Couldn't malloc 480 bytes for XTHG.");

				MaxisDecompress(pChunkData, 480, &sc2file[iChunkStart + 8], ntohl(*(DWORD*)&sc2file[iChunkStart + 4]));
				memcpy(dwMapXTHG, pChunkData, 480);
				free(pChunkData);
				iConvertedChunks++;
			}

			else {
				char szChunkName[5] = { 0 };
				memcpy(szChunkName, &sc2file[iChunkStart], 4);
				ConsoleLog(LOG_WARNING, "LOAD: Skipping unknown chunk %s\n", szChunkName);
				//sc2json["sc2x"]["conversion"]["skipped_chunks"].append(szChunkName);
			}

		next:
			i += iChunkSize;
		}

		iChunkStart += 8;
	} while (iChunkStart + iChunkSize < sc2size);

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: Load complete. Fixing up afterwards using native game calls.\n");

	// Post-load function calls.
	__asm {
		// Recalculate neighbour statistics, etc.
		mov eax, 0x402743
		call eax
	}

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: Stats recalculated.\n");

	__asm {
		// Recalculate tile data
		mov eax, 0x402EF5
		call eax
	}

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: Tile data recalculated.\n");

	__asm {
		// Fix up XGRP
		mov eax, 0x401FFA
		call eax
	}

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: XGRP fixed up.\n");

	__asm {
		// Fix up XLAB
		mov eax, 0x402D15
		call eax
	}

	if (sc2x_debug & SC2X_DEBUG_VANILLA_LOAD)
		ConsoleLog(LOG_DEBUG, "LOAD: XLAB fixed up.\n");
	return 1;
}
#endif

static bool IsMatchingChunk(const char *pChunk, const char *pTargChunk) {
	if (!pChunk || strlen(pChunk) < 1)
		return false;

	if (!pTargChunk || strlen(pTargChunk) < 1)
		return false;

	return (memcmp(pTargChunk, pChunk, 4) == 0) ? true : false;
}

static __int16 L_SimcityApp_AllocateMiscInfo(CSimcityAppPrimary *pSCApp) {
	int ret;

	ret = 0;
	if (pMiscInfo) {
		free(pMiscInfo);
		pMiscInfo = 0;
	}
	pMiscInfo = (DWORD *)malloc(MISCINF_ALLOC_SIZE);
	if (pMiscInfo) {
		memset(pMiscInfo, 0, MISCINF_ALLOC_SIZE);
		ret = 1;
	}
	return ret;
}

static int L_IsClassicCityFileValid(const char *lpFileName) {
	int ret, nFileLen;
	FILE *f;

	ret = 0;
	f = old_fopen(lpFileName, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		nFileLen = ftell(f);
		fseek(f, 0, SEEK_SET);
		// The referenced filesize here
		// is for 1.0 Classic cities.
		// Those that go outside of this
		// value appear to be for later
		// versions of Classic.
		// 27264 has been observed for 1.1
		// cities for instance.
		if (nFileLen == 27248)
			ret = 1;
		fclose(f);
	}
	return ret;
}

static int L_OpenCityHeader(FILE *pFile, const char *lpFileName, int *pLength, __int16 nClassicPreCheck) {
	int nActualLength = 0;
	bool bSupportFixUp;
	char szChunk[4], szResStr[255 + 1];

	memset(szResStr, 0, sizeof(szResStr));

	bSupportFixUp = PathMatchSpecA(lpFileName, "*.sc2") ? true : false;
	if (bSupportFixUp) {
		fseek(pFile, 0, SEEK_END);
		nActualLength = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
		nActualLength -= 8;
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: city file nActualLength is %d bytes.\n", nActualLength);
	}
	if (!fread(szChunk, 1, sizeof(szChunk), pFile)) {
		L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
		GameMain_AfxMessageBoxStr(szResStr, 0, 0);
		return 0;
	}
	if (IsMatchingChunk(szChunk, "FORM")) {
		if (!fread(pLength, 1, sizeof(*pLength), pFile)) {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
			GameMain_AfxMessageBoxStr(szResStr, 0, 0);
			return 0;
		}
		*pLength = _byteswap_ulong(*pLength);
		if (*pLength == 0) {
			// If we don't have a fixup size, inform the user that their game is about to crash
			if (!nActualLength) {
				MessageBoxA(GetActiveWindow(),
					"sc2kfix has detected a corrupted city file but was unable to recover enough information to "
					"attempt to fix it. Your game is likely to crash after closing this dialog box. Please file"
					"a save corruption report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues) and attach the sc2kfix.log file.\n\n"

					"Developer info:\n"
					"Game header corrupted (FORM header chunk size 0)\n"
					"Failed to load nActualLength.", "sc2kfix error", MB_OK | MB_ICONERROR);

				// A crash "should" no longer occur since we're now hitting
				// the exception call and returning zero rather than with
				// the original detour going into the next read call.
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
				GameMain_AfxMessageBoxStr(szResStr, 0, 0);
				return 0;
			}

			// Let's only do the fixup if bSupportFixUp is true, otherwise "safely" abort.
			if (bSupportFixUp) {
				// Log that we're attempting a fixup
				ConsoleLog(LOG_NOTICE, "SC2X: Detected possible corrupted file \"%s\".\n", lpFileName);
				ConsoleLog(LOG_NOTICE, "SC2X: Attempting to fix up corrupted save header, new size = %d.\n", nActualLength);

				// Inform the user about what's going on
				MessageBoxA(GetActiveWindow(),
					"sc2kfix has detected a corrupted city file and will try to restore it. If your city loads "
					"successfully, you should save it to a new save game file as soon as possible, restart "
					"SimCity 2000, and load the new save.\n\n"

					"If the game crashes after closing this dialog box or after reloading the new save file, "
					"please file a report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues) and attach the sc2kfix.log file.\n\n"

					"Developer info:\n"
					"Game header corrupted (FORM header chunk size 0).", "sc2kfix warning", MB_OK | MB_ICONWARNING);

				*pLength = nActualLength;
			}
			else {
				// Inform the user about what's going on
				MessageBoxA(GetActiveWindow(),
					"sc2kfix has detected a corrupted file. Unfortunately the file in question doesn't support "
					"the fixup process.\n\n"

					"If the game crashes after closing this dialog box, "
					"please file a report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues) and attach the sc2kfix.log file.\n\n"

					"Developer info:\n"
					"Game header corrupted (FORM header chunk size 0)\n"
					"Unsupported file type.", "sc2kfix error", MB_OK | MB_ICONERROR);

				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
				GameMain_AfxMessageBoxStr(szResStr, 0, 0);
				return 0;
			}
		}
		if (!fread(szChunk, 1, sizeof(szChunk), pFile)) {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
			GameMain_AfxMessageBoxStr(szResStr, 0, 0);
			return 0;
		}
		if (IsMatchingChunk(szChunk, "SCDH"))
			return 1;
		else {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 54, szResStr, sizeof(szResStr) - 1);
			GameMain_AfxMessageBoxStr(szResStr, 0, 0);
		}
	}
	else {
		if (nClassicPreCheck) {
			if (!L_IsClassicCityFileValid(lpFileName)) {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 53, szResStr, sizeof(szResStr) - 1);
				GameMain_AfxMessageBoxStr(szResStr, 0, 0);
			}
		}
	}
	return 0;
}

static int L_OpenCityUncompressed(FILE *pFile, unsigned int nSize, void *pDat) {
	return fread(pDat, nSize, 1, pFile) > 0;
}

static int L_SimcityApp_OpenCityCompressed(CSimcityAppPrimary *pSCApp, FILE *pFile, int nSize, void *pDat, int nDatSize) {
	int ret;
	char *pDst, *pTmp;
	int nTp, nDp;
	char szResStr[255 + 1];

	ret = 0;
	pDst = (char *)pDat;
	pTmp = (char *)malloc(nSize);
	if (pTmp) {
		memset(pTmp, 0, nSize);
		if (fread(pTmp, nSize, 1, pFile) > 0) {
			nTp = nDp = 0;
#if 0
			int ix = 0;
			char dat;
			while (nSize > nTp && nDatSize > nDp) {
				dat = pTmp[nTp++];
				if (dat >= 0) {
					for (ix = dat; ix > 0; --ix)
						pDst[nDp++] = pTmp[nTp++];
				}
				else {
					ix = dat & 0x7F;
					dat = pTmp[nTp++];
					while (ix >= 0) {
						pDst[nDp++] = dat;
						--ix;
					}
				}
			}
#else
			nDp = MaxisDecompress((BYTE *)pDst, nDatSize, (BYTE *)pTmp, nSize, &nTp);
#endif
			if (nTp == nSize)
				ret = 1;
			else {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 49, szResStr, sizeof(szResStr) - 1);
				GameMain_AfxMessageBoxStr(szResStr, 0, 0);
			}
		}
		else {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
			GameMain_AfxMessageBoxStr(szResStr, 0, 0);
		}
		free(pTmp);
	}
	return ret;
}

static int L_SimcityApp_OpenCityInfo(CSimcityAppPrimary *pSCApp, FILE *pFile, int nSize) {
	int ret;
	__int16 nArrOffset, nArrNextOffset, nPosMain, nPosSub;
	char szResStr[255 + 1], szBuf[31 + 1], szErrStr[1024 + 1];
	__int16 wGrantedRewards;
	bool bDisableRewardButton;

	ret = 0;
	// This was previously referenced though seemingly not used; preserving.
	//strcpy_s(szErrStr, "Unable To Load, Old File Version = \r\n");
	if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pMiscInfo, MISCINF_ALLOC_SIZE)) {
		L_byteswap_buffer(pMiscInfo, MISCINF_ALLOC_SIZE);
		nArrOffset = 0;
		if (pMiscInfo[nArrOffset] == 290) {
			nArrNextOffset = nArrOffset + 1;
			wCityMode = (__int16)pMiscInfo[nArrNextOffset++];
			wViewRotation = (__int16)pMiscInfo[nArrNextOffset++];
			wCityStartYear = (__int16)pMiscInfo[nArrNextOffset++];
			dwCityDays = pMiscInfo[nArrNextOffset++];
			dwCityFunds = pMiscInfo[nArrNextOffset++];
			dwCityBonds = pMiscInfo[nArrNextOffset++];
			wCityDifficulty = (__int16)pMiscInfo[nArrNextOffset++];
			wCityProgression = (__int16)pMiscInfo[nArrNextOffset++];
			dwCityValue = pMiscInfo[nArrNextOffset++];
			dwCityLandValue = pMiscInfo[nArrNextOffset++];
			dwCityCrime = pMiscInfo[nArrNextOffset++];
			dwCityTrafficCount = pMiscInfo[nArrNextOffset++];
			dwCityPollution = pMiscInfo[nArrNextOffset++];
			dwCityFame = pMiscInfo[nArrNextOffset++];
			dwCityAdvertising = pMiscInfo[nArrNextOffset++];
			dwCityGarbage = pMiscInfo[nArrNextOffset++];
			dwCityWorkforcePercent = pMiscInfo[nArrNextOffset++];
			dwCityWorkforceLE = pMiscInfo[nArrNextOffset++];
			dwCityWorkforceEQ = pMiscInfo[nArrNextOffset++];
			dwNationalPopulation = pMiscInfo[nArrNextOffset++];
			dwNationalValue = pMiscInfo[nArrNextOffset++];
			wNationalFedRate = (__int16)pMiscInfo[nArrNextOffset++];
			wNationalEconomyTrend = (__int16)pMiscInfo[nArrNextOffset++];
			bWeatherHeat = (BYTE)pMiscInfo[nArrNextOffset++];
			bWeatherWind = (BYTE)pMiscInfo[nArrNextOffset++];
			bWeatherRain = (BYTE)pMiscInfo[nArrNextOffset++];
			bWeatherTrend = (BYTE)pMiscInfo[nArrNextOffset++];
			wSetTriggerDisasterType = (__int16)pMiscInfo[nArrNextOffset++];
			dwCityOldResPopulation = pMiscInfo[nArrNextOffset++];
			wGrantedRewards = (__int16)pMiscInfo[nArrNextOffset++];
			bDisableRewardButton = false;
			if (!dwGrantedItems[CITYTOOL_GROUP_REWARDS] || wGrantedRewards) {
				if (dwGrantedItems[CITYTOOL_GROUP_REWARDS] || !wGrantedRewards)
					bDisableRewardButton = true;
			}
			if (bDisableRewardButton)
				Game_MainFrame_DisableCityToolBarButton((CMainFrame *)pSCApp->m_pMainWnd, CITYTOOL_BUTTON_REWARDS);
			dwGrantedItems[CITYTOOL_GROUP_REWARDS] = wGrantedRewards;
			nArrOffset = nArrNextOffset;
			for (nPosMain = 0; nPosMain < 20; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				pRawPopRatioTable[nPosMain] = pMiscInfo[nArrOffset];
				pEQRatioTable[nPosMain] = pMiscInfo[nArrNextOffset++];
				pLERatioTable[nPosMain] = pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
			}
			for (nPosMain = 0; nPosMain < 11; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				pIndividualIndDemands[nPosMain] = (__int16)pMiscInfo[nArrOffset];
				pIndividualIndTaxRate[nPosMain] = (__int16)pMiscInfo[nArrNextOffset++];
				pIndividualIndRatio[nPosMain] = pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
			}
			for (nPosMain = 0; nPosMain < 256; ++nPosMain)
				wTileCount[nPosMain] = (__int16)pMiscInfo[nArrOffset++];
			for (nPosMain = 0; nPosMain < 8; ++nPosMain)
				pZonePops[nPosMain] = pMiscInfo[nArrOffset++];
			for (nPosMain = 0; nPosMain < 50; ++nPosMain)
				wArrBondData[nPosMain] = (__int16)pMiscInfo[nArrOffset++];
			for (nPosMain = 0; nPosMain < 4; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				wNeighborNameIdx[nPosMain] = (__int16)pMiscInfo[nArrOffset];
				__int16 nIdx = wNeighborNameIdx[nPosMain];
				if (nIdx)
					Game_LoadNamedEntryFromRsrcOffset(&szNeighborCities[MAX_NEIGH_BUF_SIZE * nPosMain], 1000, nIdx);
				else
					strcpy_s(&szNeighborCities[MAX_NEIGH_BUF_SIZE * nPosMain], MAX_NEIGH_BUF_SIZE, "Ocean");
				dwNeighborPopulation[nPosMain] = pMiscInfo[nArrNextOffset++];
				dwNeighborValue[nPosMain] = pMiscInfo[nArrNextOffset++];
				dwNeighborFame[nPosMain] = pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
			}
			for (nPosMain = 0; nPosMain < 8; ++nPosMain)
				wCityDemand[nPosMain] = (__int16)pMiscInfo[nArrOffset++];
			for (nPosMain = 0; nPosMain < 17; ++nPosMain)
				wCityInventionYears[nPosMain] = (__int16)pMiscInfo[nArrOffset++];
			for (nPosMain = 0; nPosMain < 16; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				pBudgetArr[nPosMain].iCurrentCosts = pMiscInfo[nArrOffset];
				pBudgetArr[nPosMain].iFundingPercent = pMiscInfo[nArrNextOffset++];
				pBudgetArr[nPosMain].iYearToDateCost = pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
				for (nPosSub = 0; nPosSub < 12; ++nPosSub) {
					nArrNextOffset = nArrOffset + 1;
					pBudgetArr[nPosMain].iCountMonth[nPosSub] = pMiscInfo[nArrOffset];
					pBudgetArr[nPosMain].iFundMonth[nPosSub] = pMiscInfo[nArrNextOffset];
					nArrOffset = nArrNextOffset + 1;
				}
			}
			nArrNextOffset = nArrOffset + 1;
			bYearEndFlag = pMiscInfo[nArrOffset];
			wWaterLevel = (__int16)pMiscInfo[nArrNextOffset++];
			bCityHasOcean = pMiscInfo[nArrNextOffset++];
			bCityHasRiver = pMiscInfo[nArrNextOffset++];
			bMilitaryBaseType = (BYTE)pMiscInfo[nArrNextOffset];
			nArrOffset = nArrNextOffset + 1;
			for (nPosMain = 0; nPosMain < 6; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				pPaperArr[nPosMain].bName = (BYTE)pMiscInfo[nArrOffset];
				pPaperArr[nPosMain].bStyle = (BYTE)pMiscInfo[nArrNextOffset++];
				pPaperArr[nPosMain].bTag = (BYTE)pMiscInfo[nArrNextOffset++];
				pPaperArr[nPosMain].bSurvey = (BYTE)pMiscInfo[nArrNextOffset++];
				pPaperArr[nPosMain].bWeather = (BYTE)pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
			}
			for (nPosMain = 0; nPosMain < 9; ++nPosMain) {
				nArrNextOffset = nArrOffset + 1;
				pNewsArr[nPosMain].wType = (__int16)pMiscInfo[nArrOffset];
				pNewsArr[nPosMain].wPower = (__int16)pMiscInfo[nArrNextOffset++];
				pNewsArr[nPosMain].bValue = (BYTE)pMiscInfo[nArrNextOffset++];
				pNewsArr[nPosMain].bItem = (BYTE)pMiscInfo[nArrNextOffset++];
				pNewsArr[nPosMain].bName = (BYTE)pMiscInfo[nArrNextOffset++];
				pNewsArr[nPosMain].bScore = (BYTE)pMiscInfo[nArrNextOffset];
				nArrOffset = nArrNextOffset + 1;
			}
			nArrNextOffset = nArrOffset + 1;
			dwCityOrdinances = pMiscInfo[nArrOffset];
			dwCityUnemployment = pMiscInfo[nArrNextOffset];
			nArrOffset = nArrNextOffset + 1;
			for (nPosMain = 0; nPosMain < 16; ++nPosMain)
				wMilitaryTiles[nPosMain] = (WORD)pMiscInfo[nArrOffset++];
			nArrNextOffset = nArrOffset + 1;
			wSubwayXUNDCount = (WORD)pMiscInfo[nArrOffset];
			pSCApp->wSCAGameSpeedLOW = (__int16)pMiscInfo[nArrNextOffset++];
			pSCApp->wSCAGameSpeedHIGH = pSCApp->wSCAGameSpeedLOW;
			bOptionsAutoBudget = pMiscInfo[nArrNextOffset++];
			bOptionsAutoGoto = pMiscInfo[nArrNextOffset++];
			pSCApp->dwSCAGameSound = pMiscInfo[nArrNextOffset++];
			pSCApp->dwSCAGameMusic = pMiscInfo[nArrNextOffset++];
			bNoDisasters = pMiscInfo[nArrNextOffset++];
			bNewspaperSubscription = pMiscInfo[nArrNextOffset++];
			bNewspaperExtra = pMiscInfo[nArrNextOffset++];
			wNewspaperChoice = (__int16)pMiscInfo[nArrNextOffset++];
			int nTilePos = pMiscInfo[nArrNextOffset++];
			if (nTilePos == -1) {
				wViewInitialCoordX = 64;
				wViewInitialCoordY = 128;
			}
			else {
				wViewInitialCoordX = nTilePos & 0x7F;
				wViewInitialCoordY = nTilePos >> 8;
			}
			wViewInitialZoom = (__int16)pMiscInfo[nArrNextOffset++];
			wCityCenterX = (__int16)pMiscInfo[nArrNextOffset++];
			wCityCenterY = (__int16)pMiscInfo[nArrNextOffset++];
			dwArcologyPopulation = pMiscInfo[nArrNextOffset++];
			wConnectTiles = (__int16)pMiscInfo[nArrNextOffset++];
			wStadiumSportsTeams = (__int16)pMiscInfo[nArrNextOffset++];
			dwCityPopulation = pMiscInfo[nArrNextOffset++];
			wIndustrialMixBonus = (WORD)pMiscInfo[nArrNextOffset++];
			wIndustrialMixPollutionBonus = (WORD)pMiscInfo[nArrNextOffset++];
			wOldArrests = (WORD)pMiscInfo[nArrNextOffset++];
			wPrisonBonus = (WORD)pMiscInfo[nArrNextOffset++];
			wDisasterObject = (__int16)pMiscInfo[nArrNextOffset++];
			wCurrentDisasterType = (__int16)pMiscInfo[nArrNextOffset++];
			dwDisasterActive = pMiscInfo[nArrNextOffset++];
			wSewerBonus = (WORD)pMiscInfo[nArrNextOffset];
			ret = 1;
		}
		else {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 55, szResStr, sizeof(szResStr) - 1);
			_itoa_s(pMiscInfo[nArrOffset], szBuf, 10);
			sprintf_s(szErrStr, "%s%s", szResStr, szBuf);
			GameMain_AfxMessageBoxStr(szErrStr, 0, 0);
		}
	}
	free(pMiscInfo);
	pMiscInfo = 0;
	return ret;
}

static int L_OpenCityUnknownChunkRead(FILE *pFile, char *pChunk, int nSize) {
	int ret;
	char *pTemp;

	if (pChunk)
		ConsoleLog(LOG_DEBUG, "Unknown Chunk: '%c%c%c%c' (Size: %d)\n", pChunk[0], pChunk[1], pChunk[2], pChunk[3], nSize);

	ret = 0;
	pTemp = (char *)malloc(nSize);
	if (pTemp) {
		if (fread(pTemp, nSize, 1, pFile) > 0)
			ret = 1;
		free(pTemp);
	}
	return ret;
}

static void L_MakeCityNameFromFileName(const char *lpFileName) {
	int nLen;
	char szTemp[MAX_PATH + 1], szCityName[CITY_NAME_LEN + 1];

	memset(szCityName, 0, sizeof(szCityName));
	strcpy_s(szTemp, lpFileName);
	PathStripPathA(szTemp);
	PathRemoveExtensionA(szTemp);
	strncpy_s(szCityName, szTemp, sizeof(szCityName) - 1);
	nLen = strlen(szCityName);
	if (nLen > CITY_NAME_LEN)
		nLen = CITY_NAME_LEN;
	szCityName[nLen] = 0;
	GameMain_String_OperatorSet(&pszCityName, szCityName);
}

static void L_InitializeCityData() {
	__int16 iX, iY, iXHalf, iYHalf, iXQuarter, iYQuarter;
	__int16 *pTempMapResCom, *pTempMapInd;
	__int16 nBaseResComValue, nBaseIndValue;
	BYTE iTileID, iTerrainTileID;
	DWORD dwCurrBonds;
	WORD wCurrBond;

	wCommerceConnect = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			if (XTXTGetTextOverlayID(iX, iY) == NGHBR_CONNECTION_TEXT_ENTRY) {
				iTileID = GetTileID(iX, iY);
				if (GET_TILE_RANGE(iTileID, TILE_ROAD_LR, TILE_ROAD_LTBR) ||
					GET_TILE_RANGE(iTileID, TILE_TUNNEL_T, TILE_TUNNEL_L) ||
					GET_TILE_RANGE(iTileID, TILE_CROSSOVER_HIGHWAYLR_ROADTB, TILE_CROSSOVER_HIGHWAYTB_ROADLR) ||
					GET_TILE_RANGE(iTileID, TILE_ONRAMP_TL, TILE_ONRAMP_BR))
					++wCommerceConnect;
			}
		}
	}

	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loaded %d $1000 neighbor connections (Commerce Connect).\n", wCommerceConnect);

	// The 'wIndustryConnect' block was previously missing
	// and fixed via a separate detour, however it has now been
	// formalized in this re-constructed call.
	wIndustryConnect = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			if (XTXTGetTextOverlayID(iX, iY) == NGHBR_CONNECTION_TEXT_ENTRY) {
				iTileID = GetTileID(iX, iY);
				if (GET_TILE_RANGE(iTileID, TILE_RAIL_LR, TILE_RAIL_HHLR) ||
					GET_TILE_RANGE(iTileID, TILE_CROSSOVER_ROADLR_RAILTB, TILE_CROSSOVER_HIGHWAYTB_POWERLR) ||
					GET_TILE_RANGE(iTileID, TILE_HIGHWAY_HTB, TILE_HIGHWAY_LTBR))
					++wIndustryConnect;
			}
		}
	}

	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loaded %d $1500 neighbor connections (Industry Connect).\n", wIndustryConnect);

	dwBusPassengers = 0;
	dwRailPassengers = 0;
	dwSubwayPassengers = 0;

	Game_SimulationUpdatePowerConsumption();
	Game_SimulationUpdateWaterConsumption();

	wCityDevelopedTiles = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		iXHalf = iX / 2;
		iXQuarter = iXHalf / 2;
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			iYHalf = iY / 2;
			iYQuarter = iYHalf / 2;
			pTempMapResCom = GetTMap(iXQuarter, iYQuarter);
			pTempMapInd = GetTMap(iXQuarter + MINI_MAP_32, iYQuarter);
			nBaseResComValue = *pTempMapResCom;
			nBaseIndValue = *pTempMapInd;
			iTileID = GetTileID(iX, iY);
			if (iTileID) {
				if (iTileID == TILE_SERVICES_BIGPARK)
					nBaseResComValue += 40;
				else if (iTileID < TILE_TREES1 || iTileID > TILE_SMALLPARK) {
					if (iTileID <= TILE_RADIOACTIVITY)
						nBaseResComValue -= 20;
				}
				else
					nBaseResComValue += 20;
			}
			else if (XBITReturnIsWater(iX, iY)) {
				nBaseResComValue += 12;
				nBaseIndValue += 12;
			}
			else
				nBaseResComValue += 4;
			if (iTileID >= TILE_ROAD_LR || XZONReturnZone(iX, iY) != ZONE_NONE) {
				// Maximum bound here changed from GAME_SIZE_MAP to
				// MINI_MAP_64 due to it using the half-coordinate vars.
				// In this context even if the coordinate values may not
				// have a bearing on actual placement on the full-size map
				// it seems that the intent is a temporary corresponding
				// value that can be used in-conjunction with the temp map(s)
				// and any referenced mini-maps.
				if (iXHalf < MINI_MAP_64 && iYHalf < MINI_MAP_64)
					XBITSetBits(iXHalf, iYHalf, XBIT_MARK);
				++wCityDevelopedTiles;
			}
			if (XBITReturnIsWatered(iX, iY)) {
				nBaseResComValue += 4;
				nBaseIndValue += 4;
			}
			iTerrainTileID = GetTerrainTileID(iX, iY);
			if (iTerrainTileID) {
				if (iTerrainTileID < SUBMERGED_00)
					nBaseResComValue += 12;
			}
			*pTempMapResCom = nBaseResComValue;
			*pTempMapInd = nBaseIndValue;
		}
	}

	wDisasterWindy = 0;
	wDisasterFloodArea = 0;

	wCurrBond = 0;
	dwCurrBonds = dwCityBonds;
	dwInterestRateSum = 0;
	if (dwCityBonds > 0) {
		do {
			dwInterestRateSum += wArrBondData[wCurrBond];
			++wCurrBond;
			--dwCurrBonds;
		} while (dwCurrBonds);
	}
}

extern "C" void __stdcall Hook_InitializeCityData() {
	L_InitializeCityData();
}

#define COPYBLOCKTO(D, S, P, SZ, MLT) memcpy(D[P], &S[P * (SZ * MLT)], SZ * MLT)

#define CHUNK_BAD_HEAD 4
#define CHUNK_BAD_SIZE 3
#define CHUNK_BAD_BODY 2
#define CHUNK_BAD_PROC 1
#define CHUNK_OKAY     0

static int L_SimcityApp_OpenCity(CSimcityAppPrimary *pSCApp, FILE* pFile, char* lpFileName) {
	int ret;
	int nExpectedLength;
	int nCurrentReadLength;
	int iBadRead;
	bool bReadComplete, bGotName, bGotLabel;
	int nSize;
	char *pTemp;
	int nPos;
	char szChunk[4], szResStr[255 + 1], szErrStr[1024 +1 ], szTempCityName[255 + 1], szCityName[255 + 1];

	ret = 0;
	nExpectedLength = 0;
	if (L_OpenCityHeader(pFile, lpFileName, &nExpectedLength, 1)) {
		GameMain_String_Empty(&pszCityName);
		nCurrentReadLength = 4;
		bReadComplete = false;
		bGotName = false;
		bGotLabel = false;
		pTemp = NULL;
		while (!bReadComplete) {
			iBadRead = CHUNK_BAD_HEAD;
			if (fread(szChunk, 1, sizeof(szChunk), pFile) == sizeof(szChunk)) {
				iBadRead = CHUNK_BAD_SIZE;
				if (fread(&nSize, 1, sizeof(nSize), pFile) == sizeof(nSize)) {
					iBadRead = CHUNK_BAD_BODY;
					nSize = _byteswap_ulong(nSize);
					if (IsMatchingChunk(szChunk, "CNAM")) {
						memset(szTempCityName, 0, sizeof(szTempCityName));
						if (nSize > 0) {
							if (L_OpenCityUncompressed(pFile, nSize, szTempCityName)) {
								iBadRead = CHUNK_OKAY;
								bGotName = true;
							}
						}
						else {
							iBadRead = CHUNK_OKAY;
							GameMain_String_Empty(&pszCityName);
						}
					}
					else if (IsMatchingChunk(szChunk, "MISC")) {
						if (L_SimcityApp_OpenCityInfo(pSCApp, pFile, nSize))
							iBadRead = CHUNK_OKAY;
					}
					else if (IsMatchingChunk(szChunk, "ALTM")) {
						pTemp = (char *)malloc(ALTM_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, ALTM_ALLOC_SIZE);
							if (L_OpenCityUncompressed(pFile, nSize, pTemp)) {
								L_byteswap_ushorts((WORD *)pTemp, nSize);
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapALTM, pTemp, nPos, sizeof(map_ALTM_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XTER")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXTER, pTemp, nPos, sizeof(map_XTER_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XBLD")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXBLD, pTemp, nPos, sizeof(map_XBLD_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XZON")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXZON, pTemp, nPos, sizeof(map_XZON_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XUND")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXUND, pTemp, nPos, sizeof(map_XUND_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XTXT")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXTXT, pTemp, nPos, sizeof(map_XTXT_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XLAB")) {
						pTemp = (char *)malloc(LABEL_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MAX_LABEL_COUNT * sizeof(map_XLAB_t));
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, LABEL_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MAX_LABEL_COUNT; ++nPos)
									COPYBLOCKTO(&dwMapXLAB[0], pTemp, nPos, sizeof(map_XLAB_t), 1);
								iBadRead = CHUNK_OKAY;
								bGotLabel = true;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XMIC")) {
						pTemp = (char *)malloc(MICROSIM_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MICROSIM_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MICROSIM_ALLOC_SIZE)) {
								L_byteswap_micro((WORD *)pTemp, MICROSIM_ALLOC_SIZE);
								for (nPos = 0; nPos < MAX_MICROSIM_COUNT; ++nPos)
									COPYBLOCKTO(&pMicrosimArr, pTemp, nPos, sizeof(microsim_t), 1);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XTHG")) {
						pTemp = (char *)malloc(THING_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, THING_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, THING_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MAX_THING_COUNT; ++nPos)
									COPYBLOCKTO(&dwMapXTHG[0], pTemp, nPos, sizeof(map_XTHG_t), 1);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XBIT")) {
						pTemp = (char *)malloc(FULLMAP_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, FULLMAP_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, FULLMAP_ALLOC_SIZE)) {
								for (nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
									COPYBLOCKTO(dwMapXBIT, pTemp, nPos, sizeof(map_XBIT_t), GAME_MAP_SIZE);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XTRF")) {
						pTemp = (char *)malloc(MINIMAP64_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP64_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP64_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_64; ++nPos)
									COPYBLOCKTO(dwMapXTRF, pTemp, nPos, sizeof(map_mini64_t), MINI_MAP_64);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XPLT")) {
						pTemp = (char *)malloc(MINIMAP64_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP64_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP64_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_64; ++nPos)
									COPYBLOCKTO(dwMapXPLT, pTemp, nPos, sizeof(map_mini64_t), MINI_MAP_64);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XVAL")) {
						pTemp = (char *)malloc(MINIMAP64_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP64_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP64_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_64; ++nPos)
									COPYBLOCKTO(dwMapXVAL, pTemp, nPos, sizeof(map_mini64_t), MINI_MAP_64);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XCRM")) {
						pTemp = (char *)malloc(MINIMAP64_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP64_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP64_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_64; ++nPos)
									COPYBLOCKTO(dwMapXCRM, pTemp, nPos, sizeof(map_mini64_t), MINI_MAP_64);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XPLC")) {
						pTemp = (char *)malloc(MINIMAP32_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP32_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP32_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_32; ++nPos)
									COPYBLOCKTO(dwMapXPLC, pTemp, nPos, sizeof(map_mini32_t), MINI_MAP_32);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XFIR")) {
						pTemp = (char *)malloc(MINIMAP32_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP32_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP32_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_32; ++nPos)
									COPYBLOCKTO(dwMapXFIR, pTemp, nPos, sizeof(map_mini32_t), MINI_MAP_32);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XPOP")) {
						pTemp = (char *)malloc(MINIMAP32_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP32_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP32_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_32; ++nPos)
									COPYBLOCKTO(dwMapXPOP, pTemp, nPos, sizeof(map_mini32_t), MINI_MAP_32);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XROG")) {
						pTemp = (char *)malloc(MINIMAP32_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, MINIMAP32_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, MINIMAP32_ALLOC_SIZE)) {
								for (nPos = 0; nPos < MINI_MAP_32; ++nPos)
									COPYBLOCKTO(dwMapXROG, pTemp, nPos, sizeof(map_mini32_t), MINI_MAP_32);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (IsMatchingChunk(szChunk, "XGRP")) {
						pTemp = (char *)malloc(GRAPH_ALLOC_SIZE);
						if (pTemp) {
							iBadRead = CHUNK_BAD_PROC;
							memset(pTemp, 0, GRAPH_ALLOC_SIZE);
							if (L_SimcityApp_OpenCityCompressed(pSCApp, pFile, nSize, pTemp, GRAPH_ALLOC_SIZE)) {
								L_byteswap_buffer((DWORD *)pTemp, GRAPH_ALLOC_SIZE);
								for (nPos = 0; nPos < MAX_GRAPHS; ++nPos)
									COPYBLOCKTO(dwMapXGRP, pTemp, nPos, sizeof(DWORD), MAX_GRAPH_ENTRIES);
								iBadRead = CHUNK_OKAY;
							}
						}
					}
					else if (L_OpenCityUnknownChunkRead(pFile, szChunk, nSize))
						iBadRead = CHUNK_OKAY;
				}
			}
			if (iBadRead <= CHUNK_BAD_PROC) {
				// To reach CHUNK_BAD_PROC <= pTemp isn't NULL, but best to check.
				if (pTemp) {
					free(pTemp);
					pTemp = NULL;
				}
			}
			if (iBadRead > CHUNK_OKAY) {
				if (iBadRead > CHUNK_BAD_BODY) {
					L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 48, szResStr, sizeof(szResStr) - 1);
					sprintf_s(szErrStr, "%s\n%s", szResStr, lpFileName);
					GameMain_AfxMessageBoxStr(szErrStr, 0, 0);
				}
				// Set explicitly just in case.
				bReadComplete = false;
				break;
			}

			nCurrentReadLength += nSize + 8;
			if (nCurrentReadLength >= nExpectedLength)
				bReadComplete = true;
		}
		if (bReadComplete) {
			memset(szCityName, 0, sizeof(szCityName));
			if (bGotName && szTempCityName[0]) {
				if (L_PascalStringToCharString(szTempCityName, szCityName))
					GameMain_String_OperatorSet(&pszCityName, szCityName);
				else
					bGotName = false;
			}
			if (!bGotName)
				L_MakeCityNameFromFileName(lpFileName);
			L_InitializeCityData();
			Game_GetOccupiedTileCount();
			Game_GraphKludge();
			if (bGotLabel)
				GameMain_ResetLabelStringState();
			else
				Game_ClearLabels();
			ret = 1;
		}
	}
	return ret;
}

// Function prototype: HOOKCB void L_SimcityApp_DoLoad_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will have a
//   chance to pre-load any information and optionally manipulate the save file before it's parsed
//   by sc2kfix and loaded into the SimCity 2000 engine.
std::vector<hook_function_t> stHooks_L_SimcityApp_DoLoad_Before;

// Function prototype: HOOKCB void L_SimcityApp_DoLoad_After(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will be fed a
//   pointer to a JSON object wherein they can load their data and version information or a NULL
//   or similar object to inform them that they have no known state to load.
std::vector<hook_function_t> stHooks_L_SimcityApp_DoLoad_After;

static int L_SimcityApp_DoLoad(CSimcityAppPrimary *pSCApp, char *lpFileName) {
	int ret;
	FILE *f;
	char szResStr[255 + 1], szErrStr[1024 + 1];
	CSimcityView *pSCView;

	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loading saved game \"%s\".\n", lpFileName);

	for (const auto& hook : stHooks_L_SimcityApp_DoLoad_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, char*) = (void(*)(CSimcityAppPrimary*, char*))hook.pFunction;
			fnHook(pSCApp, lpFileName);
		}
	}

	ret = 0;
	f = old_fopen(lpFileName, "rb");
	if (f) {
		nRetState = L_SimcityApp_AllocateMiscInfo(pSCApp);
		GameMain_String_OperatorSet(&strCityFilename, lpFileName);
		GameMain_CmdTarget_BeginWaitCursor(pSCApp);
		if (PathMatchSpecA(lpFileName, "*.sc2") || PathMatchSpecA(lpFileName, "*.scn")) {
#ifdef SC2X_USE_VANILLA_LOAD_REPLACEMENT
			if (PathMatchSpecA(lpFileName, "*.sc2")) {
				ret = SC2XLoadVanillaGame(pSCApp, lpFileName);
			}
			else
#endif
			{
				if (sc2x_debug & SC2X_DEBUG_LOAD)
					ConsoleLog(LOG_DEBUG, "SC2X: Passing control to SC2K for load.\n");
				ret = L_SimcityApp_OpenCity(pSCApp, f, lpFileName);
			}
		}
		fclose(f);
		if (!ret) {
			if (L_IsClassicCityFileValid(lpFileName)) {
				memset(szResStr, 0, sizeof(szResStr));
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 46, szResStr, sizeof(szResStr) - 1);
				GameMain_CmdTarget_EndWaitCursor(pSCApp);
				if (GameMain_AfxMessageBoxStr(szResStr, MB_YESNO, 0) == IDYES) {
					GameMain_CmdTarget_BeginWaitCursor(pSCApp);
					ret = Game_SimcityApp_ConvertClassicCity(pSCApp, lpFileName);
				}
				else
					GameMain_CmdTarget_BeginWaitCursor(pSCApp);
			}
		}
		GameMain_CmdTarget_EndWaitCursor(pSCApp);
		Game_SimcityApp_AdjustNewspaperMenu(pSCApp);
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		Game_SimcityView_ResetAttributesAndCoordinates(pSCView, wViewInitialCoordX, wViewInitialCoordY, wViewInitialZoom);
		Game_ToolMenuUpdate();
	}
	else {
		// Adjust so this error is displayed primarily during this specific failure case.
		// Once we've accounted for Scenario loading.
		L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 47, szResStr, sizeof(szResStr) - 1);
		sprintf_s(szErrStr, "%s\n%s", szResStr, lpFileName);
		GameMain_AfxMessageBoxStr(szErrStr, 0, 0);
	}

	for (const auto& hook : stHooks_L_SimcityApp_DoLoad_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, char*) = (void(*)(CSimcityAppPrimary*, char*))hook.pFunction;
			fnHook(pSCApp, lpFileName);
		}
	}

	return ret;
}

extern "C" int __stdcall Hook_SimcityApp_DoLoad(char *lpFileName) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	return L_SimcityApp_DoLoad(pThis, lpFileName);
}

extern "C" void __stdcall Hook_SimcityApp_LoadCity() {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	CMainFrame *pMainFrm;
	char szFileTypes[255 + 1], szCaption[255 + 1];
	CMFC3XString strFilePath;
	int nRet, nPathLen;
	char szFilePath[MAX_PATH + 1], szPath[MAX_PATH + 1], szDirPath[MAX_PATH + 1];
	extFileDlg_t m_extFileDlg;
	OPENFILENAMEA m_ofn;

	pMainFrm = (CMainFrame *)pThis->m_pMainWnd;
	if (Game_SimcityApp_CheckActiveGame(pThis) == IDCANCEL) {
		if (pThis->dwSCAOnInitToggleToolBar)
			Game_MainFrame_ToggleToolBars(pMainFrm, TRUE);
		else {
			pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
			pThis->dwSCASetNextStep = TRUE;
			pThis->dwSCAGameStarted = FALSE;
		}
	}
	else {
		pThis->dwSCABackgroundColourCyclingActive = TRUE;
		
		memset(szPath, 0, sizeof(szPath));
		memset(szDirPath, 0, sizeof(szDirPath));

		L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 4002, szFileTypes, sizeof(szFileTypes) - 1);
		L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 4003, szCaption, sizeof(szCaption) - 1);

		Game_SimcityApp_GetValueStringA(pThis, &strFilePath, aPaths, aCities);

		strcpy_s(szFilePath, sizeof(szFilePath) - 1, strFilePath.m_pchData);

		memset(&m_extFileDlg, 0, sizeof(m_extFileDlg));
		m_extFileDlg.nExtType = FEXT_TYPE_OPENCITYATTR;

		memset(&m_ofn, 0, sizeof(OPENFILENAMEA));
		m_ofn.lStructSize = sizeof(OPENFILENAMEA);
		m_ofn.hwndOwner = pMainFrm->m_hWnd;
		m_ofn.hInstance = hSC2KFixModule;
		m_ofn.lpstrFilter = ConvertFileTypeFilterString(szFileTypes);
		m_ofn.lpstrInitialDir = szFilePath;
		m_ofn.lpstrTitle = szCaption;
		m_ofn.nMaxFile = _countof(szPath);
		m_ofn.nMaxFileTitle = _countof(szPath);
		m_ofn.nFilterIndex = 1;
		m_ofn.lpstrDefExt = CITY_DEFAULT_EXTENSION;
		m_ofn.lpstrFile = szPath;
		m_ofn.lpstrFileTitle = szFilePath;
		m_ofn.Flags = OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
		m_ofn.lpfnHook = (LPOFNHOOKPROC)FileHookProc;
		m_ofn.lpTemplateName = MAKEINTRESOURCEA(IDD_FILEDLGEXT);
		m_ofn.lCustData = (LPARAM)&m_extFileDlg;
		m_ofn.FlagsEx = OFN_EX_NOPLACESBAR;
		nRet = GetOpenFileNameA(&m_ofn);
		nRetState = (!nRet) ? IDCANCEL : nRet;
		if (nRetState == IDCANCEL || strlen(m_ofn.lpstrFile) == 0) {
			if (pThis->dwSCAOnInitToggleToolBar) {
				Game_MainFrame_ToggleToolBars(pMainFrm, TRUE);
				Game_UpdateSectionsAndResetWindowMenu();
			}
			else {
				pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
				pThis->dwSCASetNextStep = TRUE;
			}
			pThis->dwSCABackgroundColourCyclingActive = FALSE;
		}
		else {
			pThis->dwSCABackgroundColourCyclingActive = FALSE;
			pThis->iSCAProgramStep = ONIDLE_STATE_LOADCITY_RETURN;
			pThis->dwSCASetNextStep = TRUE;
			Game_StartCleanGame();
			Game_PrepareGame();

			strcpy_s(szDirPath, m_ofn.lpstrFile);
			PathRemoveFileSpecA(szDirPath);
			nPathLen = strlen(szDirPath);
			if (szDirPath[nPathLen - 1] != '\\')
				strcat_s(szDirPath, "\\");
			if (Game_SimcityApp_DoLoad(pThis, m_ofn.lpstrFile)) {
				if (L_IsPathValid(szDirPath))
					jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES] = szDirPath;
				GameMain_Document_UpdateAllViews(pCSimcityDoc, 0, SCD_UPDATE_VIEW_UPDATE, 0);
				CSimcityView *pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pThis);
				Game_ShowViewControls();
				UpdateWindow(pSCView->m_hWnd);
				pThis->iSCAProgramStep = ONIDLE_STATE_INGAME;
				pThis->dwSCASetNextStep = TRUE;
				Game_SimcityApp_AdjustMenus(pThis, wCityMode);
				dwMapEditingMode = wCityMode == 0;
				Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
				if (pThis->dwSCAOnInitToggleToolBar)
					Game_MainFrame_ToggleToolBars(pMainFrm, TRUE);
				pThis->dwSCAMapModeVarCheck = FALSE;
				pThis->dwSCAGameStarted = TRUE;
			}
			else {
				GameMain_AfxMessageBoxID(412, 0, 0xFFFFFFFF);
				pThis->dwSCAOnInitToggleToolBar = 0;
				pThis->iSCAProgramStep = ONIDLE_STATE_PENDINGACTION;
				pThis->dwSCASetNextStep = TRUE;
				Game_SimcityApp_CloseWidgetWindows(pThis);
				Game_MainFrame_ToggleToolBars(pMainFrm, FALSE);
				pThis->dwSCAGameStarted = FALSE;
			}
			_chdir(pThis->dwSCACStringDriveCurrentWorkingDirectory.m_pchData);
			Game_UpdateSectionsAndResetWindowMenu();
		}
		GameMain_String_Dest(&strFilePath);
	}
}

static int L_SimcityApp_WriteCityHeader(CSimcityAppPrimary *pSCApp, FILE *pFile, int nDatSize) {
	char szChunk[4];
	DWORD nScrDatSize;

	if (fseek(pFile, 0, SEEK_SET))
		return 0;
	memcpy(szChunk, "FORM", 4);
	if (!fwrite(szChunk, sizeof(szChunk), 1, pFile))
		return 0;
	nScrDatSize = _byteswap_ulong(nDatSize);
	if (!fwrite(&nScrDatSize, sizeof(nScrDatSize), 1, pFile))
		return 0;
	memcpy(szChunk, "SCDH", 4);
	if (!fwrite(szChunk, sizeof(szChunk), 1, pFile))
		return 0;
	return 1;
}

static int L_SimcityApp_WriteCityName(CSimcityAppPrimary *pSCApp, FILE *pFile, char *pTargChunk, const char *pPCityName) {
	char szChunk[4];
	DWORD nFullLen;

	if (!pTargChunk || strlen(pTargChunk) < 1)
		return 0;
	memcpy(szChunk, pTargChunk, 4);
	if (!fwrite(szChunk, sizeof(szChunk), 1, pFile))
		return 0;
	nFullLen = _byteswap_ulong(CNAM_DAT_LEN);
	if (!fwrite(&nFullLen, sizeof(nFullLen), 1, pFile))
		return 0;
	if (!fwrite(pPCityName, CNAM_DAT_LEN, 1, pFile))
		return 0;
	nDataOffset += CNAM_DAT_LEN + 8;
	return 1;
}

static int L_SimcityApp_WriteCityUncompressed(CSimcityAppPrimary *pSCApp, FILE *pFile, char *pTargChunk, const void *pDat, int nDatSize) {
	int ret;
	WORD *pDst;
	char szChunk[4];
	DWORD nScrDatSize;

	ret = 0;
	if (!pTargChunk || strlen(pTargChunk) < 1)
		return 0;
	pDst = (WORD *)malloc(nDatSize);
	if (!pDst)
		return 0;
	memset(pDst, 0, nDatSize);
	memcpy(pDst, pDat, nDatSize);
	memcpy(szChunk, pTargChunk, 4);
	if (!fwrite(szChunk, sizeof(szChunk), 1, pFile))
		goto ABORTWRITE;
	nScrDatSize = _byteswap_ulong(nDatSize);
	if (!fwrite(&nScrDatSize, sizeof(nScrDatSize), 1, pFile))
		goto ABORTWRITE;
	L_byteswap_ushorts(pDst, nDatSize);
	if (!fwrite(pDst, nDatSize, 1, pFile))
		goto ABORTWRITE;
	nDataOffset += nDatSize + 8;
	ret = 1;
ABORTWRITE:
	free(pDst);
	return ret;
}

static int L_SimcityApp_WriteCityCompressed(CSimcityAppPrimary *pSCApp, FILE *pFile, char *pTargChunk, const void *pDat, int nDatSize) {
	int ret;
	char *pDst;
	char *pTmp;
	int nDp, nTp, ix;
	BYTE dat;
	char szChunk[4];
	DWORD nScrTp;

	ret = 0;
	if (!pTargChunk || strlen(pTargChunk) < 1)
		return 0;
	pDst = (char *)malloc(nDatSize);
	if (!pDst)
		return 0;
	memset(pDst, 0, nDatSize);
	memcpy(pDst, pDat, nDatSize);
	pTmp = (char *)malloc(3 * nDatSize / 2);
	if (!pTmp) {
		free(pDst);
		return 0;
	}
	memset(pTmp, 0, 3 * nDatSize / 2);
	if (IsMatchingChunk(pTargChunk, "MISC") || IsMatchingChunk(pTargChunk, "XGRP"))
		L_byteswap_buffer((DWORD *)pDst, nDatSize);
	else if (IsMatchingChunk(pTargChunk, "XMIC"))
		L_byteswap_micro((WORD *)pDst, nDatSize);
	nDp = nTp = 0;
	while (nDatSize - 1 > nDp) {
		dat = pDst[nDp];
		if (pDst[nDp + 1] == dat) {
			for (ix = 2; pDst[nDp + ix] == dat && ix < 128 && nDp + ix < nDatSize; ++ix)
				;
			pTmp[nTp] = (ix - 1) | 0x80;
			pTmp[++nTp] = dat;
			++nTp;
			nDp += ix;
		}
		else {
			ix = 1;
			pTmp[nTp + 1] = dat;
			while (pDst[nDp + ix] != dat && ix < 128 && nDp + ix < nDatSize) {
				dat = pDst[nDp + ix++];
				pTmp[nTp + ix] = dat;
			}
			pTmp[nTp] = ix - 1;
			nDp += ix - 1;
			nTp += ix;
		}
	}
	if (nDatSize - 1 == nDp) {
		pTmp[nTp] = 1;
		pTmp[++nTp] = pDst[nDp++];
		++nTp;
	}
	nDataOffset += nTp + 8;
	memcpy(szChunk, pTargChunk, 4);
	if (!fwrite(szChunk, sizeof(szChunk), 1, pFile))
		goto ABORTWRITE;
	nScrTp = _byteswap_ulong(nTp);
	if (!fwrite(&nScrTp, sizeof(nScrTp), 1, pFile))
		goto ABORTWRITE;
	if (!fwrite(pTmp, nTp, 1, pFile))
		goto ABORTWRITE;
	ret = 1;
ABORTWRITE:
	free(pTmp);
	free(pDst);
	return ret;
}

static int L_SimcityApp_WriteCityInfo(CSimcityAppPrimary *pSCApp, FILE *pFile) {
	CSimcityView *pSCView;
	__int16 nArrOffset, nArrNextOffset, nPosMain, nPosSub;

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	nArrOffset = 0;
	nArrNextOffset = nArrOffset + 1;
	pMiscInfo[nArrOffset] = 290;
	pMiscInfo[nArrNextOffset++] = wCityMode;
	pMiscInfo[nArrNextOffset++] = wViewRotation;
	pMiscInfo[nArrNextOffset++] = wCityStartYear;
	pMiscInfo[nArrNextOffset++] = dwCityDays;
	pMiscInfo[nArrNextOffset++] = dwCityFunds;
	pMiscInfo[nArrNextOffset++] = dwCityBonds;
	pMiscInfo[nArrNextOffset++] = wCityDifficulty;
	pMiscInfo[nArrNextOffset++] = wCityProgression;
	pMiscInfo[nArrNextOffset++] = dwCityValue;
	pMiscInfo[nArrNextOffset++] = dwCityLandValue;
	pMiscInfo[nArrNextOffset++] = dwCityCrime;
	pMiscInfo[nArrNextOffset++] = dwCityTrafficCount;
	pMiscInfo[nArrNextOffset++] = dwCityPollution;
	pMiscInfo[nArrNextOffset++] = dwCityFame;
	pMiscInfo[nArrNextOffset++] = dwCityAdvertising;
	pMiscInfo[nArrNextOffset++] = dwCityGarbage;
	pMiscInfo[nArrNextOffset++] = dwCityWorkforcePercent;
	pMiscInfo[nArrNextOffset++] = dwCityWorkforceLE;
	pMiscInfo[nArrNextOffset++] = dwCityWorkforceEQ;
	pMiscInfo[nArrNextOffset++] = dwNationalPopulation;
	pMiscInfo[nArrNextOffset++] = dwNationalValue;
	pMiscInfo[nArrNextOffset++] = wNationalFedRate;
	pMiscInfo[nArrNextOffset++] = wNationalEconomyTrend;
	pMiscInfo[nArrNextOffset++] = bWeatherHeat;
	pMiscInfo[nArrNextOffset++] = bWeatherWind;
	pMiscInfo[nArrNextOffset++] = bWeatherRain;
	pMiscInfo[nArrNextOffset++] = bWeatherTrend;
	pMiscInfo[nArrNextOffset++] = wSetTriggerDisasterType;
	pMiscInfo[nArrNextOffset++] = cwCityStats[1];
	pMiscInfo[nArrNextOffset] = dwGrantedItems[CITYTOOL_GROUP_REWARDS];
	nArrOffset = nArrNextOffset + 1;
	for (nPosMain = 0; nPosMain < 20; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = pRawPopRatioTable[nPosMain];
		pMiscInfo[nArrNextOffset++] = pEQRatioTable[nPosMain];
		pMiscInfo[nArrNextOffset] = pLERatioTable[nPosMain];
		nArrOffset = nArrNextOffset + 1;
	}
	for (nPosMain = 0; nPosMain < 11; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = pIndividualIndDemands[nPosMain];
		pMiscInfo[nArrNextOffset++] = pIndividualIndTaxRate[nPosMain];
		pMiscInfo[nArrNextOffset] = pIndividualIndRatio[nPosMain];
		nArrOffset = nArrNextOffset + 1;
	}
	for (nPosMain = 0; nPosMain < 256; ++nPosMain)
		pMiscInfo[nArrOffset++] = wTileCount[nPosMain];
	for (nPosMain = 0; nPosMain < 8; ++nPosMain)
		pMiscInfo[nArrOffset++] = pZonePops[nPosMain];
	for (nPosMain = 0; nPosMain < 50; ++nPosMain)
		pMiscInfo[nArrOffset++] = wArrBondData[nPosMain];
	for (nPosMain = 0; nPosMain < 4; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = wNeighborNameIdx[nPosMain];
		pMiscInfo[nArrNextOffset++] = dwNeighborPopulation[nPosMain];
		pMiscInfo[nArrNextOffset++] = dwNeighborValue[nPosMain];
		pMiscInfo[nArrNextOffset] = dwNeighborFame[nPosMain];
		nArrOffset = nArrNextOffset + 1;
	}
	for (nPosMain = 0; nPosMain < 8; ++nPosMain)
		pMiscInfo[nArrOffset++] = wCityDemand[nPosMain];
	for (nPosMain = 0; nPosMain < 17; ++nPosMain)
		pMiscInfo[nArrOffset++] = wCityInventionYears[nPosMain];
	for (nPosMain = 0; nPosMain < 16; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = pBudgetArr[nPosMain].iCurrentCosts;
		pMiscInfo[nArrNextOffset++] = pBudgetArr[nPosMain].iFundingPercent;
		pMiscInfo[nArrNextOffset] = pBudgetArr[nPosMain].iYearToDateCost;
		nArrOffset = nArrNextOffset + 1;
		for (nPosSub = 0; nPosSub < 12; ++nPosSub) {
			nArrNextOffset = nArrOffset + 1;
			pMiscInfo[nArrOffset] = pBudgetArr[nPosMain].iCountMonth[nPosSub];
			pMiscInfo[nArrNextOffset] = pBudgetArr[nPosMain].iFundMonth[nPosSub];
			nArrOffset = nArrNextOffset + 1;
		}
	}
	nArrNextOffset = nArrOffset + 1;
	pMiscInfo[nArrOffset] = bYearEndFlag;
	pMiscInfo[nArrNextOffset++] = wWaterLevel;
	pMiscInfo[nArrNextOffset++] = bCityHasOcean;
	pMiscInfo[nArrNextOffset++] = bCityHasRiver;
	pMiscInfo[nArrNextOffset] = bMilitaryBaseType;
	nArrOffset = nArrNextOffset + 1;
	for (nPosMain = 0; nPosMain < 6; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = pPaperArr[nPosMain].bName;
		pMiscInfo[nArrNextOffset++] = pPaperArr[nPosMain].bStyle;
		pMiscInfo[nArrNextOffset++] = pPaperArr[nPosMain].bTag;
		pMiscInfo[nArrNextOffset++] = pPaperArr[nPosMain].bSurvey;
		pMiscInfo[nArrNextOffset] = pPaperArr[nPosMain].bWeather;
		nArrOffset = nArrNextOffset + 1;
	}
	for (nPosMain = 0; nPosMain < 9; ++nPosMain) {
		nArrNextOffset = nArrOffset + 1;
		pMiscInfo[nArrOffset] = pNewsArr[nPosMain].wType;
		pMiscInfo[nArrNextOffset++] = pNewsArr[nPosMain].wPower;
		pMiscInfo[nArrNextOffset++] = pNewsArr[nPosMain].bValue;
		pMiscInfo[nArrNextOffset++] = pNewsArr[nPosMain].bItem;
		pMiscInfo[nArrNextOffset++] = pNewsArr[nPosMain].bName;
		pMiscInfo[nArrNextOffset] = pNewsArr[nPosMain].bScore;
		nArrOffset = nArrNextOffset + 1;
	}
	nArrNextOffset = nArrOffset + 1;
	pMiscInfo[nArrOffset] = dwCityOrdinances;
	pMiscInfo[nArrNextOffset] = dwCityUnemployment;
	nArrOffset = nArrNextOffset + 1;
	for (nPosMain = 0; nPosMain < 16; ++nPosMain)
		pMiscInfo[nArrOffset++] = wMilitaryTiles[nPosMain];
	nArrNextOffset = nArrOffset + 1;
	pMiscInfo[nArrOffset] = wSubwayXUNDCount;
	pMiscInfo[nArrNextOffset++] = pSCApp->wSCAGameSpeedLOW;
	pMiscInfo[nArrNextOffset++] = bOptionsAutoBudget;
	pMiscInfo[nArrNextOffset++] = bOptionsAutoGoto;
	pMiscInfo[nArrNextOffset++] = pSCApp->dwSCAGameSound;
	pMiscInfo[nArrNextOffset++] = pSCApp->dwSCAGameMusic;
	pMiscInfo[nArrNextOffset++] = bNoDisasters;
	pMiscInfo[nArrNextOffset++] = bNewspaperSubscription;
	pMiscInfo[nArrNextOffset++] = bNewspaperExtra;
	pMiscInfo[nArrNextOffset++] = wNewspaperChoice;
	// -- Originally the second argument for PointToTile was also iScreenPointX
	// (bug reference: save -> (re)load position wild discrepancy).
	// Note: The 'else' case here now uses a standard default value of 128 and ZOOM_LEVEL_SMALL.
	pMiscInfo[nArrNextOffset++] = (pSCView) ? Game_PointToTile(iScreenPointX, iScreenPointY) : 128;
	pMiscInfo[nArrNextOffset++] = (pSCView) ? pSCView->wSCVZoomLevel : ZOOM_LEVEL_SMALL;
	// ^--
	pMiscInfo[nArrNextOffset++] = wCityCenterX;
	pMiscInfo[nArrNextOffset++] = wCityCenterY;
	pMiscInfo[nArrNextOffset++] = dwArcologyPopulation;
	pMiscInfo[nArrNextOffset++] = wConnectTiles;
	pMiscInfo[nArrNextOffset++] = wStadiumSportsTeams;
	pMiscInfo[nArrNextOffset++] = dwCityPopulation;
	pMiscInfo[nArrNextOffset++] = wIndustrialMixBonus;
	pMiscInfo[nArrNextOffset++] = wIndustrialMixPollutionBonus;
	pMiscInfo[nArrNextOffset++] = wOldArrests;
	pMiscInfo[nArrNextOffset++] = wPrisonBonus;
	pMiscInfo[nArrNextOffset++] = wDisasterObject;
	pMiscInfo[nArrNextOffset++] = wCurrentDisasterType;
	pMiscInfo[nArrNextOffset++] = dwDisasterActive;
	pMiscInfo[nArrNextOffset] = wSewerBonus;
	for (nArrOffset = nArrNextOffset + 1; nArrOffset < 1200; ++nArrOffset)
		pMiscInfo[nArrOffset] = 0;
	return L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "MISC", pMiscInfo, MISCINF_ALLOC_SIZE);
}

static int L_SimcityApp_WriteCity(CSimcityAppPrimary *pSCApp, FILE *pFile) {
	char szTempStr[CNAM_DAT_LEN];

	memset(szTempStr, 0, sizeof(szTempStr));

	int ret = 0;
	Game_GetOccupiedTileCount();
	if (!L_SimcityApp_WriteCityHeader(pSCApp, pFile, 0))
		goto ABORTWRITE;
	nDataOffset = 4;
	L_CharStringToPascalString(pszCityName.m_pchData, szTempStr, CITY_NAME_LEN, true);
	if (!L_SimcityApp_WriteCityName(pSCApp, pFile, "CNAM", szTempStr))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityInfo(pSCApp, pFile))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityUncompressed(pSCApp, pFile, "ALTM", (const void *)dwMapALTM[0], ALTM_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XTER", (const void *)dwMapXTER[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XBLD", (const void *)dwMapXBLD[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XZON", (const void *)dwMapXZON[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XUND", (const void *)dwMapXUND[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XTXT", (const void *)dwMapXTXT[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	GameMain_SaveLabels();
	int nRetXLAB = L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XLAB", (const void *)dwMapXLAB[0], LABEL_ALLOC_SIZE);
	GameMain_ResetLabelStringState();
	if (!nRetXLAB)
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XMIC", (const void *)pMicrosimArr, MICROSIM_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XTHG", (const void *)dwMapXTHG[0], THING_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XBIT", (const void *)dwMapXBIT[0], FULLMAP_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XTRF", (const void *)dwMapXTRF[0], MINIMAP64_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XPLT", (const void *)dwMapXPLT[0], MINIMAP64_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XVAL", (const void *)dwMapXVAL[0], MINIMAP64_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XCRM", (const void *)dwMapXCRM[0], MINIMAP64_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XPLC", (const void *)dwMapXPLC[0], MINIMAP32_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XFIR", (const void *)dwMapXFIR[0], MINIMAP32_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XPOP", (const void *)dwMapXPOP[0], MINIMAP32_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XROG", (const void *)dwMapXROG[0], MINIMAP32_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, "XGRP", (const void *)dwMapXGRP[0], GRAPH_ALLOC_SIZE))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityHeader(pSCApp, pFile, nDataOffset))
		goto ABORTWRITE;
	Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
	ret = 1;
ABORTWRITE:
	return ret;
}

// Function prototype: HOOKCB void L_SimcityApp_DoSave_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will be fed a
//   pointer to a JSON object wherein they can save their data and version information.
std::vector<hook_function_t> stHooks_L_SimcityApp_DoSave_Before;

// Function prototype: HOOKCB void L_SimcityApp_DoSave_After(void)
// Cannot be ignored.
// SPECIAL NOTE: Functionally useless. Likely to end up either being removed before the modding
//   API is finalized or for its argument to be BOOL bSaveSuccessful.
std::vector<hook_function_t> stHooks_L_SimcityApp_DoSave_After;

// Note: The 'pNewCityName' and 'bChangeCityName' arguments are only relevant for "Save City As".
int L_SimcityApp_DoSave(CSimcityAppPrimary *pSCApp, const char *lpFileName, char *pNewCityName, bool bChangeCityName) {
	FILE *f;
	bool bCanChangeCityName;
	char szOldCityName[CNAM_DAT_LEN];
	DWORD dwWasZoomedIn;
	int ret;
	CSimcityView *pSCView;

	for (const auto& hook : stHooks_L_SimcityApp_DoSave_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, const char*, char*, bool) = (void(*)(CSimcityAppPrimary*, const char*, char*, bool))hook.pFunction;
			fnHook(pSCApp, lpFileName, pNewCityName, bChangeCityName);
		}
	}

	bCanChangeCityName = (bChangeCityName && pNewCityName && strlen(pNewCityName) > 0) ? true : false;

	memset(szOldCityName, 0, sizeof(szOldCityName));

	ret = -1;
	f = old_fopen(lpFileName, "wb+");
	if (f) {
		// Store the old city name.
		strcpy_s(szOldCityName, pszCityName.m_pchData);

		// If bChangeCityName true, then change the current
		// city name prior to saving.
		if (bCanChangeCityName) {
			GameMain_String_Empty(&pszCityName);
			GameMain_String_OperatorSet(&pszCityName, pNewCityName);
		}
		nRetState = L_SimcityApp_AllocateMiscInfo(pSCApp);
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		// This variable is needed so it can store the last state
		// of the dwSCVIsZoomed variable (assuming pSCView is valid)
		// so it can be restored later.
		dwWasZoomedIn = 0;
		if (pSCView) {
			dwWasZoomedIn = pSCView->dwSCVIsZoomed;
			if (dwWasZoomedIn)
				Game_SimcityView_ScaleOut(pSCView);
		}
		GameMain_CmdTarget_BeginWaitCursor(pSCApp);
		// It should be noted that the 'strFileName' parameter
		// doesn't appear to be used in the subsequent call (or
		// perhaps not in the release build - beyond the downstream
		// local-copy being destroyed).
		ret = L_SimcityApp_WriteCity(pSCApp, f);
		GameMain_CmdTarget_EndWaitCursor(pSCApp);
		if (pSCView) {
			if (dwWasZoomedIn)
				Game_SimcityView_ScaleIn(pSCView);
		}
		// Remote free due to the allocation
		// occurring in the native program (for now).
		free(pMiscInfo);
		pMiscInfo = 0;
		fclose(f);
		if (!ret) {
			remove(lpFileName);
			// Saving has failed, revert the city name back to the old
			// one.
			if (bCanChangeCityName) {
				GameMain_String_Empty(&pszCityName);
				GameMain_String_OperatorSet(&pszCityName, szOldCityName);
			}
		}
	}

	for (const auto& hook : stHooks_L_SimcityApp_DoSave_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, const char*, char*, bool) = (void(*)(CSimcityAppPrimary*, const char*, char*, bool))hook.pFunction;
			fnHook(pSCApp, lpFileName, pNewCityName, bChangeCityName);
		}
	}

	return ret;
}

extern "C" void __stdcall Hook_SimcityApp_SaveCity() {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	char szPath[MAX_PATH + 1], szErrStr[512 + 1];
	bool bCanDoDirectSave;
	UINT uType;

	memset(szPath, 0, sizeof(szPath));

	if (pThis->dwSCAGameStarted) {
		// If you're loading a scenario or any object that doesn't
		// match the intended city file extension, go to Save City As.
		// Let's avoid any downstream filename post-processing.
		bCanDoDirectSave = false;
		if (strCityFilename.m_nDataLength > 0) {
			strcpy_s(szPath, strCityFilename.m_pchData);
			if (PathMatchSpecA(szPath, CITY_DEFAULT_SAVE_MATCH))
				bCanDoDirectSave = true;
		}
		if (bCanDoDirectSave) {
			int nSaveRet = L_SimcityApp_DoSave(pThis, szPath, NULL, false);
			if (nSaveRet > 0) {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 51, szErrStr, sizeof(szErrStr) - 1);
				uType = 0;

				// When a general save is performed, under normal
				// circumstances you shouldn't need to worry about
				// updating the current globally stored filename,
				// however if you're saving a Scenario as a City
				// that's where trouble may occur, so now always
				// update the path and go from there.
				GameMain_String_Empty(&strUnusedString);
				GameMain_String_OperatorSet(&strCityFilename, szPath);
			}
			else {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((nSaveRet < 0) ? 47 : 60), szErrStr, sizeof(szErrStr) - 1);
				strcat_s(szErrStr, ": ");
				uType = MB_ICONERROR;
			}
			strcat_s(szErrStr, szPath);
			L_MessageBoxA(0, szErrStr, gamePrimaryKey, uType);
		}
		else {
			pThis->dwSCAOnQuitSuspendSim = 1;
			Game_SimcityApp_SaveCityAs(pThis);
		}
		pThis->dwSCAOnQuitSuspendSim = 0;
	}
}

extern "C" void __stdcall Hook_SimcityApp_SaveCityAs() {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	CMainFrame *pMainFrm;
	CMFC3XString strFilePath;
	int nLen, nRet, nPathLen;
	char szFilePath[MAX_PATH + 1], szPath[MAX_PATH + 1], szDirPath[MAX_PATH + 1], szErrStr[512 + 1];
	extFileDlg_t m_extFileDlg;
	bool bChangeCityName;
	OPENFILENAMEA m_ofn;
	UINT uType;

	pMainFrm = (CMainFrame *)pThis->m_pMainWnd;
	if (pThis->dwSCAGameStarted) {
		memset(szPath, 0, sizeof(szPath));
		memset(szDirPath, 0, sizeof(szDirPath));

		Game_SimcityApp_GetValueStringA(pThis, &strFilePath, aPaths, aSavegame);

		strcpy_s(szFilePath, sizeof(szFilePath) - 1, strFilePath.m_pchData);

		memset(&m_extFileDlg, 0, sizeof(m_extFileDlg));
		m_extFileDlg.nExtType = FEXT_TYPE_SAVECITYNAME;
		m_extFileDlg.bCityNameChanged = false;
		memcpy(m_extFileDlg.szCityName, pszCityName.m_pchData, CITY_NAME_LEN);
		m_extFileDlg.pSaveExt = CITY_DEFAULT_EXTENSION;
		nLen = strlen(m_extFileDlg.szCityName);
		m_extFileDlg.szCityName[nLen] = 0;

		if (strCityFilename.m_nDataLength > 0) {
			strcpy_s(szPath, strCityFilename.m_pchData);
			PathStripPathA(szPath);
			PathRemoveExtensionA(szPath);
		}
		else
			strcpy_s(szPath, m_extFileDlg.szCityName);
		strcat_s(szPath, CITY_DEFAULT_APPEND_EXTENSION);

		memset(&m_ofn, 0, sizeof(OPENFILENAMEA));
		m_ofn.lStructSize = sizeof(OPENFILENAMEA);
		m_ofn.hwndOwner = pThis->m_pMainWnd->m_hWnd;
		m_ofn.hInstance = hSC2KFixModule;
		m_ofn.lpstrFilter = ConvertFileTypeFilterString(CITY_DEFAULT_TYPE_STRING);
		m_ofn.lpstrInitialDir = szFilePath;
		m_ofn.nMaxFile = _countof(szPath);
		m_ofn.nMaxFileTitle = _countof(szPath);
		m_ofn.nFilterIndex = 1;
		m_ofn.lpstrFile = szPath;
		m_ofn.lpstrFileTitle = szFilePath;
		m_ofn.Flags = OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING;
		m_ofn.lpfnHook = (LPOFNHOOKPROC)FileHookProc;
		m_ofn.lpTemplateName = MAKEINTRESOURCEA(IDD_FILEDLGEXT);
		m_ofn.lCustData = (LPARAM)&m_extFileDlg;
		m_ofn.FlagsEx = OFN_EX_NOPLACESBAR;
		ToggleFloatingStatusDialog(FALSE);
		nRet = GetSaveFileNameA(&m_ofn);
		nRetState = (!nRet) ? IDCANCEL : nRet;
		if (nRetState != IDCANCEL) {
			bChangeCityName = false;
			if (m_extFileDlg.bCityNameChanged) {
				int nLen = strlen(m_extFileDlg.szCityName);
				if (nLen >= 1 && nLen <= CITY_NAME_LEN) {
					if (sc2x_debug & SC2X_DEBUG_VANILLA_SAVE)
						ConsoleLog(LOG_DEBUG, "New City Name: '%s' (%d)\n", m_extFileDlg.szCityName, nLen);
					bChangeCityName = true;
				}
			}
			strcpy_s(szDirPath, m_extFileDlg.szAdjustedFile);
			PathRemoveFileSpecA(szDirPath);
			nPathLen = strlen(szDirPath);
			if (szDirPath[nPathLen - 1] != '\\')
				strcat_s(szDirPath, "\\");
			int nSaveRet = L_SimcityApp_DoSave(pThis, m_extFileDlg.szAdjustedFile, m_extFileDlg.szCityName, bChangeCityName);
			if (nSaveRet > 0) {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 51, szErrStr, sizeof(szErrStr) - 1);
				uType = 0;

				GameMain_String_Empty(&strUnusedString);
				GameMain_String_OperatorSet(&strCityFilename, m_extFileDlg.szAdjustedFile);
				strcat_s(szErrStr, m_extFileDlg.szAdjustedFile);

				if (L_IsPathValid(szDirPath))
					jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES] = szDirPath;
			}
			else {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, ((nSaveRet < 0) ? 47 : 60), szErrStr, sizeof(szErrStr) - 1);
				uType = MB_ICONERROR;
			}
			L_MessageBoxA(0, szErrStr, gamePrimaryKey, uType);
		}
		pThis->dwSCAOnQuitSuspendSim = 0;
		ToggleFloatingStatusDialog(TRUE);

		GameMain_String_Dest(&strFilePath);
	}
}

extern "C" int __cdecl Hook_ByteSwap_LongLabel(char *pBuf) {
	return L_byteswap_longlabel(pBuf);
}

extern "C" void __cdecl Hook_ByteSwap_UShorts(WORD *pBuf, int nCount) {
	L_byteswap_ushorts(pBuf, nCount);
}

extern "C" void __cdecl Hook_ByteSwap_Buffer(DWORD *pBuf, int nCount) {
	L_byteswap_buffer(pBuf, nCount);
}

extern "C" void __cdecl Hook_ByteSwap_Micro(WORD *pBuf, int nCount) {
	L_byteswap_micro(pBuf, nCount);
}

void InstallSaveHooks_SC2K1996(void) {
	// Internal long label byteswap call
	SafeVirtualProtect((LPVOID)0x40223E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40223E, Hook_ByteSwap_LongLabel);

	// Internal Shorts byteswap call
	SafeVirtualProtect((LPVOID)0x402301, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402301, Hook_ByteSwap_UShorts);

	// Internal long buffer byteswap call
	SafeVirtualProtect((LPVOID)0x402FF9, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402FF9, Hook_ByteSwap_Buffer);

	// Internal short micro byteswap call
	SafeVirtualProtect((LPVOID)0x401FB4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401FB4, Hook_ByteSwap_Micro);

	// InitializeCityData:
	// - Demystification
	// - A formal fix for the $1500 neighbor connections on game load (IndustryConnect)
	SafeVirtualProtect((LPVOID)0x402743, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402743, Hook_InitializeCityData);

	// CSimcityApp::DoLoad
	SafeVirtualProtect((LPVOID)0x401721, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401721, Hook_SimcityApp_DoLoad);

	// CSimcityApp::LoadCity
	SafeVirtualProtect((LPVOID)0x401E1F, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E1F, Hook_SimcityApp_LoadCity);

	// CSimcityApp::SaveCity
	SafeVirtualProtect((LPVOID)0x4015A0, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015A0, Hook_SimcityApp_SaveCity);

	// CSimcityApp::SaveCityAs
	SafeVirtualProtect((LPVOID)0x401929, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401929, Hook_SimcityApp_SaveCityAs);
}
