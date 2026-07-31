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
// WIP replacement for CSimcityApp::DoLoadGame for vanilla save game files.
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

				dwCityResidentialPopulation = ntohl(*(DWORD*)&pChunkMISC[i]);
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

static int L_IsClassicCityFileValid(const char *lpFileName) {
	int ret, nFileLen;
	FILE *f;

	ret = 0;
	f = old_fopen(lpFileName, "rb");
	if (!f)
		return 0;
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
	if (nFileLen != 27248)
		goto NOTVALID;
	ret = 1;
NOTVALID:
	fclose(f);
	return ret;
}

static int L_OpenCityHeader(CMFC3XFile *pFile, const char *lpFileName, int *pLength, __int16 nClassicPreCheck) {
	int nActualLength = 0;
	bool bSupportFixUp;
	DWORD dwChunk, scrChunk;
	char szResStr[255 + 1];

	memset(szResStr, 0, sizeof(szResStr));

	bSupportFixUp = PathMatchSpecA(lpFileName, "*.sc2") ? true : false;
	if (bSupportFixUp) {
		nActualLength = GameMain_File_GetLength(pFile);
		nActualLength -= 8;
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: city file nActualLength is %d bytes.\n", nActualLength);
	}
	if (!GameMain_File_Read(pFile, &dwChunk, sizeof(dwChunk))) {
		Game_GetFileExceptionError(48, &fileExcept, 0);
		return 0;
	}
	dwChunk = _byteswap_ulong(dwChunk);
	scrChunk = L_byteswap_longlabel("FORM");
	if (scrChunk == dwChunk) {
		if (!GameMain_File_Read(pFile, pLength, sizeof(*pLength))) {
			Game_GetFileExceptionError(48, &fileExcept, 0);
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
				Game_GetFileExceptionError(48, &fileExcept, 0);
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

				Game_GetFileExceptionError(48, &fileExcept, 0);
				return 0;
			}
		}
		if (!GameMain_File_Read(pFile, &dwChunk, sizeof(dwChunk))) {
			Game_GetFileExceptionError(48, &fileExcept, 0);
			return 0;
		}
		dwChunk = _byteswap_ulong(dwChunk);
		scrChunk = L_byteswap_longlabel("SCDH");
		if (scrChunk == dwChunk)
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

extern "C" int __stdcall Hook_OpenCityHeader(CMFC3XFile *pFile, const char *lpFileName, int *pLength, __int16 nClassicPreCheck) {
	return L_OpenCityHeader(pFile, lpFileName, pLength, nClassicPreCheck);
}

static int L_OpenCityUncompressed(CMFC3XFile *pFile, unsigned int nSize, void *pDat) {
	return GameMain_File_Read(pFile, pDat, nSize);
}

extern "C" int __stdcall Hook_OpenCityUncompressed(CMFC3XFile *pFile, unsigned int nSize, void *pDat) {
	return L_OpenCityUncompressed(pFile, nSize, pDat);
}

static int L_SimcityApp_OpenCityCompressed(CSimcityAppPrimary *pSCApp, CMFC3XFile *pFile, int nSize, void *pDat, int nDatSize) {
	int ret;
	char *pDst, *pTmp;
	int nTp, nDp, ix;
	char dat;
	char szResStr[255 + 1];

	ret = 0;
	pDst = (char *)pDat;
	pTmp = (char *)malloc(nSize);
	if (!pTmp)
		return 0;
	if (GameMain_File_Read(pFile, pTmp, nSize)) {
		nTp = nDp = 0;
		ix = 0;
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
		if (nTp != nSize) {
			L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 49, szResStr, sizeof(szResStr) - 1);
			GameMain_AfxMessageBoxStr(szResStr, 0, 0);
			goto ABORTREAD;
		}
	}
	else {
		Game_GetFileExceptionError(48, &fileExcept, 0);
		goto ABORTREAD;
	}
	ret = 1;
ABORTREAD:
	free(pTmp);
	return ret;
}

extern"C" int __stdcall Hook_SimcityApp_OpenCityCompressed(CMFC3XFile *pFile, int nSize, void *pDat, int nDatSize) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	return L_SimcityApp_OpenCityCompressed(pThis, pFile, nSize, pDat, nDatSize);
}

static int L_SimcityApp_OpenCity(CSimcityAppPrimary *pSCApp, CMFC3XFile* pFile, char* lpFileName) {
	return Game_SimcityApp_OpenCity(pSCApp, pFile, lpFileName);
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
	CMFC3XFile cFile;
	char szResStr[255 + 1];
	CSimcityView *pSCView;

	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loading saved game \"%s\".\n", lpFileName);

	for (const auto& hook : stHooks_L_SimcityApp_DoLoad_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, char*) = (void(*)(CSimcityAppPrimary*, char*))hook.pFunction;
			fnHook(pSCApp, lpFileName);
		}
	}

	GameMain_File_Cons(&cFile);

	ret = 0;
	if (GameMain_File_Open(&cFile, lpFileName, (0x8000 | 0x0040), &fileExcept)) {
		nRetState = Game_SimcityApp_AllocateMiscInfo(pSCApp);
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
				ret = L_SimcityApp_OpenCity(pSCApp, &cFile, lpFileName);
			}
		}
		GameMain_File_Close(&cFile);
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
	else
		Game_FailRadioException(47, &fileExcept, lpFileName);

	GameMain_File_Dest(&cFile);

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
	DWORD scrChunk, nChunk, nScrDatSize;

	if (fseek(pFile, 0, SEEK_SET))
		return 0;
	scrChunk = L_byteswap_longlabel("FORM");
	nChunk = _byteswap_ulong(scrChunk);
	if (!fwrite(&nChunk, sizeof(nChunk), 1, pFile))
		return 0;
	nScrDatSize = _byteswap_ulong(nDatSize);
	if (!fwrite(&nScrDatSize, sizeof(nScrDatSize), 1, pFile))
		return 0;
	scrChunk = L_byteswap_longlabel("SCDH");
	nChunk = _byteswap_ulong(scrChunk);
	if (!fwrite(&nChunk, sizeof(nChunk), 1, pFile))
		return 0;
	return 1;
}

static int L_SimcityApp_WriteCityName(CSimcityAppPrimary *pSCApp, FILE *pFile, DWORD scrChunk, const char *pPCityName) {
	DWORD nChunk, nFullLen;

	nChunk = _byteswap_ulong(scrChunk);
	if (!fwrite(&nChunk, sizeof(nChunk), 1, pFile))
		return 0;
	nFullLen = _byteswap_ulong(CNAM_DAT_LEN);
	if (!fwrite(&nFullLen, sizeof(nFullLen), 1, pFile))
		return 0;
	if (!fwrite(pPCityName, CNAM_DAT_LEN, 1, pFile))
		return 0;
	nDataOffset += CNAM_DAT_LEN + 8;
	return 1;
}

static int L_SimcityApp_WriteCityUncompressed(CSimcityAppPrimary *pSCApp, FILE *pFile, DWORD scrChunk, const void *pDat, int nDatSize) {
	int ret;
	WORD *pDst;
	DWORD nChunk, nScrDatSize;

	ret = 0;
	pDst = (WORD *)malloc(nDatSize);
	if (!pDst)
		return 0;
	memset(pDst, 0, nDatSize);
	memcpy(pDst, pDat, nDatSize);
	nChunk = _byteswap_ulong(scrChunk);
	if (!fwrite(&nChunk, sizeof(nChunk), 1, pFile))
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

static int L_SimcityApp_WriteCityCompressed(CSimcityAppPrimary *pSCApp, FILE *pFile, DWORD scrChunk, const void *pDat, int nDatSize) {
	int ret;
	char *pDst;
	char *pTmp;
	DWORD cmpChunkMISC, cmpChunkXGRP, cmpChunkXMIC;
	int nDp, nTp, ix;
	BYTE dat;
	DWORD nChunk, nScrTp;

	ret = 0;
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
	cmpChunkMISC = L_byteswap_longlabel("MISC");
	cmpChunkXGRP = L_byteswap_longlabel("XGRP");
	cmpChunkXMIC = L_byteswap_longlabel("XMIC");
	if (cmpChunkMISC == scrChunk || cmpChunkXGRP == scrChunk)
		L_byteswap_buffer((DWORD *)pDst, nDatSize);
	else if (cmpChunkXMIC == scrChunk)
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
	nChunk = _byteswap_ulong(scrChunk);
	if (!fwrite(&nChunk, sizeof(nChunk), 1, pFile))
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
	DWORD nScrChunk;

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
	nScrChunk = L_byteswap_longlabel("MISC");
	return L_SimcityApp_WriteCityCompressed(pSCApp, pFile, nScrChunk, pMiscInfo, 0x12C0);
}

static int L_SimcityApp_WriteCity(CSimcityAppPrimary *pSCApp, FILE *pFile) {
	char szTempStr[CNAM_DAT_LEN];
	DWORD scrChunk;

	memset(szTempStr, 0, sizeof(szTempStr));

	int ret = 0;
	Game_GetOccupiedTileCount();
	if (!L_SimcityApp_WriteCityHeader(pSCApp, pFile, 0))
		goto ABORTWRITE;
	nDataOffset = 4;
	L_CharStringToPascalString(pszCityName.m_pchData, szTempStr, CNAM_DAT_LEN - 1, true);
	scrChunk = L_byteswap_longlabel("CNAM");
	if (!L_SimcityApp_WriteCityName(pSCApp, pFile, scrChunk, szTempStr))
		goto ABORTWRITE;
	if (!L_SimcityApp_WriteCityInfo(pSCApp, pFile))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("ALTM");
	if (!L_SimcityApp_WriteCityUncompressed(pSCApp, pFile, scrChunk, (const void *)dwMapALTM[0], 0x8000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XTER");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXTER[0], 0x4000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XBLD");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXBLD[0], 0x4000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XZON");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXZON[0], 0x4000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XUND");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXUND[0], 0x4000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XTXT");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXTXT[0], 0x4000))
		goto ABORTWRITE;
	GameMain_SaveLabels();
	scrChunk = L_byteswap_longlabel("XLAB");
	int nRetXLAB = L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXLAB[0], 0x1900);
	GameMain_ResetLabelStringState();
	if (!nRetXLAB)
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XMIC");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)pMicrosimArr, 0x4B0))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XTHG");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXTHG[0], 0x1E0))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XBIT");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXBIT[0], 0x4000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XTRF");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXTRF[0], 0x1000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XPLT");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXPLT[0], 0x1000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XVAL");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXVAL[0], 0x1000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XCRM");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXCRM[0], 0x1000))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XPLC");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXPLC[0], 0x400))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XFIR");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXFIR[0], 0x400))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XPOP");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXPOP[0], 0x400))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XROG");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXROG[0], 0x400))
		goto ABORTWRITE;
	scrChunk = L_byteswap_longlabel("XGRP");
	if (!L_SimcityApp_WriteCityCompressed(pSCApp, pFile, scrChunk, (const void *)dwMapXGRP[0], 0xD00))
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
	char szOldCityName[CNAM_DAT_LEN - 1];
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
		nRetState = Game_SimcityApp_AllocateMiscInfo(pSCApp);
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
		GameMain_Op_Delete(pMiscInfo);
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

extern "C" void __stdcall Hook_InitializeCityData() {
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

	// OpenCityHeader
	// This now integrates the fix-up case concerning corrupted
	// headers.
	SafeVirtualProtect((LPVOID)0x4020E0, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4020E0, Hook_OpenCityHeader);

	// OpenCityUncompressed
	SafeVirtualProtect((LPVOID)0x4019CE, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4019CE, Hook_OpenCityUncompressed);

	// CSimcityApp::OpenCityCompressed
	SafeVirtualProtect((LPVOID)0x40245A, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40245A, Hook_SimcityApp_OpenCityCompressed);

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
