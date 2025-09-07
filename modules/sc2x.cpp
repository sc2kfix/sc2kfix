// sc2kfix modules/sc2x.cpp: JSON-based extensible save game file format
//                           and hooks for fixing save/load bugs.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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

#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING

#ifdef DEBUGALL
#undef SC2X_DEBUG
#define SC2X_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

#if !NOKUROKO
#define BAILOUT(s, ...) do { \
	ConsoleLog(LOG_ERROR, "SC2X: " s, __VA_ARGS__); \
	return 0; \
} while (0)
#else
#define BAILOUT(s, ...) do { \
	ConsoleLog(LOG_ERROR, "SAVE: " s, __VA_ARGS__); \
	return 0; \
} while (0)
#endif

UINT sc2x_debug = SC2X_DEBUG;

static char* szLoadFileName = NULL;
static int iCorruptedFixupSize = 0;
static DWORD dwDummy;

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
BOOL SC2XLoadVanillaGame(DWORD* pThis, const char* szFileName) {
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
				CMFC3XString* (__thiscall * H_CStringOperatorSet)(CMFC3XString*, const char*) = (CMFC3XString * (__thiscall*)(CMFC3XString*, const char*))0x4A2E6A;
				std::string strCityName((char*)&sc2file[iChunkStart + 9]);
				H_CStringOperatorSet(&pszCityName, strCityName.c_str());
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

				dwCityTrafficUnknown = ntohl(*(DWORD*)&pChunkMISC[i]);
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

				wNationalTax = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wNationalEconomyTrend = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherHeat = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherWind = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bWeatherHumidity = ntohl(*(DWORD*)&pChunkMISC[i]);
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
					*(DWORD*)(0x4C94B4 + i * 4) = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					*(DWORD*)(0x4C94BC + i * 4) = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					*(DWORD*)(0x4CAA70 + i * 4) = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
				}
				i += 4 * 60;

				for (int i = 0; i < 11; i++) {
					wArrIndustrialDemands[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					wArrIndustrialTaxRates[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					dwArrIndustrialPopulations[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
				}
				i += 4 * 33;

				for (int i = 0; i < 256; i++)
					dwTileCount[i] = (BYTE)(ntohl(*(DWORD*)&pChunkMISC[i * 4]));
				i += 4 * 256;

				for (int i = 0; i < 8; i++)
					pZonePops[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 8;

				for (int i = 0; i < 50; i++)
					wBondArr[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 50;

				// TODO: Encode as arrays of useful JSON
				for (int i = 0; i < 4; i++) {
					wNeighborNameIdx[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					if (wNeighborNameIdx[i]) {
						// TODO: name this function
						void (__cdecl * H_40146A)(char*, int, int) = (void (__cdecl*)(char*, int, int))0x40146A;
						H_40146A((char*)stNeighborCities + i * 32, 1000, wNeighborNameIdx[i]);
					} else
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

				// TODO: Encode as arrays of useful JSON
				//sc2json["MISC"]["dwArrNewspaperTable1"] = EncodeDWORDArray((DWORD*)&pChunkMISC[i], 30, TRUE);
				for (int i = 0; i < 30; i++)
					bArrNewspaperTable1[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 30;

				// TODO: Encode as arrays of useful JSON
				//dwArrNewspaperTable2 = EncodeDWORDArray((DWORD*)&pChunkMISC[i], 54, TRUE);
				for (int i = 0; i < 9; i++) {
					*(WORD*)&bArrNewspaperTable2[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
					*(WORD*)&bArrNewspaperTable2[i + 1] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 4]);
					bArrNewspaperTable2[i + 2] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 8]);
					bArrNewspaperTable2[i + 3] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 12]);
					bArrNewspaperTable2[i + 4] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 16]);
					bArrNewspaperTable2[i + 5] = ntohl(*(DWORD*)&pChunkMISC[i * 4 + 20]);
				}
				i += 4 * 54;

				dwCityOrdinances = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				dwCityUnemployment = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				// This table is staying as a Base64Encode because you SHOULD NOT mess with it
				//sc2json["MISC"]["dwMilitaryTiles"] = Base64Encode(&pChunkMISC[i], 4 * 16);
				for (int i = 0; i < 16; i++)
					dwMilitaryTiles[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
				i += 4 * 16;

				wSubwayXUNDCount = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wSimulationSpeed = ntohl(*(DWORD*)&pChunkMISC[i]);		// XXX - CHECK IF THIS NEEDS TO BE THISCASTED
				i += 4;

				bOptionsAutoBudget = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bOptionsAutoGoto = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				pThis[121] = ntohl(*(DWORD*)&pChunkMISC[i]);	// XXX - needs a good name
				i += 4;

				bOptionsMusicEnabled = ntohl(*(DWORD*)&pChunkMISC[i]);
				pThis[120] = bOptionsMusicEnabled;				// XXX - is this the same?
				if (!bOptionsMusicEnabled) {
					// Stop music
					__asm {
						push ecx
						mov ecx, [0x4C7158]
						mov eax, 0x402BE4
						call eax
						pop ecx
					}
				}
				i += 4;

				bNoDisasters = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bNewspaperSubscription = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				bNewspaperExtra = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wNewspaperChoice = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				WORD wViewCoords = ntohl(*(DWORD*)&pChunkMISC[i]);
				if (wViewCoords == 0xFFFF) {
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

				*(WORD*)(0x4CADDC) = ntohl(*(DWORD*)&pChunkMISC[i]);	// Unused, but we'll load it anyways.
				i += 4;

				wSportsTeams = ntohl(*(DWORD*)&pChunkMISC[i]);
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

				wMonsterXTHGIndex = ntohl(*(DWORD*)&pChunkMISC[i]);
				i += 4;

				wCurrentDisasterID = ntohl(*(DWORD*)&pChunkMISC[i]);
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

// Function prototype: HOOKCB void Hook_LoadGame_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will have a
//   chance to pre-load any information and optionally manipulate the save file before it's parsed
//   by sc2kfix and loaded into the SimCity 2000 engine.
std::vector<hook_function_t> stHooks_Hook_LoadGame_Before;

// Function prototype: HOOKCB void Hook_LoadGame_After(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will be fed a
//   pointer to a JSON object wherein they can load their data and version information or a NULL
//   or similar object to inform them that they have no known state to load.
std::vector<hook_function_t> stHooks_Hook_LoadGame_After;

extern "C" DWORD __stdcall Hook_LoadGame(CMFC3XFile* pFile, char* src) {
	DWORD(__thiscall * H_SimcityAppDoLoadGame)(CSimcityAppPrimary*, CMFC3XFile*, char*) = (DWORD(__thiscall*)(CSimcityAppPrimary*, CMFC3XFile*, char*))0x4302E0;
	CSimcityAppPrimary* pThis;
	DWORD ret;

	__asm mov [pThis], ecx

	szLoadFileName = src;
	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loading saved game \"%s\".\n", szLoadFileName);

	std::ifstream infile(szLoadFileName, std::ios::binary | std::ios::ate);
	if (infile.is_open()) {
		iCorruptedFixupSize = infile.tellg();
		iCorruptedFixupSize -= 8;
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game iCorruptedFixupSize is %d bytes.\n", iCorruptedFixupSize);
		infile.close();
	}
	else {
		ConsoleLog(LOG_WARNING, "SC2X: Couldn't open saved game \"%s\" to determine iCorruptedFixupSize.\n", szLoadFileName);
		ConsoleLog(LOG_WARNING, "SC2X: If this save is corrupted, sc2kfix will not be able to attempt to fix it.\n");
		iCorruptedFixupSize = 0;
	}

	for (const auto& hook : stHooks_Hook_LoadGame_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, CMFC3XFile*, char*) = (void(*)(CSimcityAppPrimary*, CMFC3XFile*, char*))hook.pFunction;
			fnHook(pThis, pFile, src);
		}
	}

	// Make sure it's an .sc2 file and attempt to load if so.
	if (std::regex_search(szLoadFileName, std::regex("\\.[Ss][Cc]2$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game is a vanilla SC2 file.\n");

#ifdef SC2X_USE_VANILLA_LOAD_REPLACEMENT
		ret = SC2XLoadVanillaGame(pThis, szLoadFileName);
#else
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Passing control to SC2K for load.\n");
		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
#endif
	} else if (std::regex_search(szLoadFileName, std::regex("\\.[Ss][Cc][Nn]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game is a vanilla SCN file. Passing control to SC2K.\n");

		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
	} else if (std::regex_search(szLoadFileName, std::regex("\\.[Cc][Tt][Yy]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game is a SimCity Classic file. Passing control to SC2K.\n");

		ret = H_SimcityAppDoLoadGame(pThis, pFile, src);
	}

	for (const auto& hook : stHooks_Hook_LoadGame_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, CMFC3XFile*, char*) = (void(*)(CSimcityAppPrimary*, CMFC3XFile*, char*))hook.pFunction;
			fnHook(pThis, pFile, src);
		}
	}

	return ret;
}

// Function prototype: HOOKCB void Hook_SaveGame_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will be fed a
//   pointer to a JSON object wherein they can save their data and version information.
std::vector<hook_function_t> stHooks_Hook_SaveGame_Before;

// Function prototype: HOOKCB void Hook_SaveGame_After(void)
// Cannot be ignored.
// SPECIAL NOTE: Functionally useless. Likely to end up either being removed before the modding
//   API is finalized or for its argument to be BOOL bSaveSuccessful.
std::vector<hook_function_t> stHooks_Hook_SaveGame_After;

extern "C" DWORD __stdcall Hook_SaveGame(CMFC3XString* lpFileName) {
	DWORD(__thiscall * H_SimcityAppDoSaveGame)(CSimcityAppPrimary*, CMFC3XString*) = (DWORD(__thiscall*)(CSimcityAppPrimary*, CMFC3XString*))0x432180;
	CSimcityAppPrimary* pThis;
	DWORD ret;

	__asm mov [pThis], ecx

	for (const auto& hook : stHooks_Hook_SaveGame_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, CMFC3XString*) = (void(*)(CSimcityAppPrimary*, CMFC3XString*))hook.pFunction;
			fnHook(pThis, lpFileName);
		}
	}

	ret = H_SimcityAppDoSaveGame(pThis, lpFileName);

	for (const auto& hook : stHooks_Hook_SaveGame_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, CMFC3XString*) = (void(*)(CSimcityAppPrimary*, CMFC3XString*))hook.pFunction;
			fnHook(pThis, lpFileName);
		}
	}

	return ret;
}

// Assembly language hook to try to fix up corrupted save file headers.
void __declspec(naked) Hook_431212(void) {
	// Replace the code we're clobbering to inject ourselves
	__asm {
		// Original call flow
		mov eax, 0x401429
		call eax
		add esp, 4

		// Check if eax is 0; skip otherwise
		push ebp
		mov ebp, esp
		cmp eax, 0
		jne skip
	}

	// If we don't have a fixup size, inform the user that their game is about to crash
	if (!iCorruptedFixupSize) {
		MessageBox(GetActiveWindow(),
			"sc2kfix has detected a corrupted save file but was unable to recover enough information to "
			"attempt to fix it. Your game is likely to crash after closing this dialog box. Please file"
			"a save corruption report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues).\n\n"

			"Developer info:\n"
			"Save header corrupted (FORM header chunk size 0)\n"
			"Failed to load iCorruptedFixupSize in Hook_LoadGame", "sc2kfix error", MB_OK | MB_ICONERROR);

		__asm jmp skip
	}

	// Log that we're attempting a fixup
	ConsoleLog(LOG_NOTICE, "SC2X: Detected possible corrupted save \"%s\".\n", szLoadFileName);
	ConsoleLog(LOG_NOTICE, "SC2X: Attempting to fix up corrupted save header, new size = %d.\n", iCorruptedFixupSize);

	// Inform the user about what's going on
	MessageBox(GetActiveWindow(),
		"sc2kfix has detected a corrupted save file and will try to restore it. If your city loads "
		"successfully, you should save it to a new save game file as soon as possible, restart "
		"SimCity 2000, and load the new save.\n\n"

		"If the game crashes after closing this dialog box or after reloading the new save file, "
		"please file a report on the sc2kfix GitHub issues page (https://github.com/sc2kfix/sc2kfix/issues).\n\n"

		"Developer info:\n"
		"Save header corrupted (FORM header chunk size 0)", "sc2kfix warning", MB_OK | MB_ICONWARNING);

	// Inject the right (or, close enough) size back into the original code path
	__asm {
		mov eax, [iCorruptedFixupSize]
	skip:
		pop ebp
		push 0x43121A		// jump back to original control flow
		retn
	}
}

// Fix rail and highway border connections not loading properly
extern "C" void __stdcall Hook_LoadNeighborConnections1500(void) {
	short* wCityNeighborConnections1500 = (short*)0x4CA3F0;
	*wCityNeighborConnections1500 = 0;
	*(DWORD*)0x4C85A0 = 0;

	for (int x = 0; x < GAME_MAP_SIZE; x++) {
		for (int y = 0; y < GAME_MAP_SIZE; y++) {
			if (XTXTGetTextOverlayID(x, y) == 0xFA) {
				BYTE iTileID = GetTileID(x, y);
				if (iTileID >= TILE_RAIL_LR && iTileID < TILE_TUNNEL_T
					|| iTileID >= TILE_CROSSOVER_ROADLR_RAILTB && iTileID < TILE_SUSPENSION_BRIDGE_START_B
					|| iTileID >= TILE_HIGHWAY_HTB && iTileID < TILE_REINFORCED_BRIDGE_PYLON)
					++*wCityNeighborConnections1500;
			}
		}
	}

	if (sc2x_debug & SC2X_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SC2X: Loaded %d $1500 neighbor connections.\n", *wCityNeighborConnections1500);
}

void InstallSaveHooks_SC2K1996(void) {
	// Fix city name being overwritten by filename on save
	BYTE bFilenamePatch[6] = { 0xB9, 0xA0, 0xA1, 0x4C, 0x00, 0x51 };
	VirtualProtect((LPVOID)0x42FE62, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE62, bFilenamePatch, 6);
	VirtualProtect((LPVOID)0x42FE99, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	memcpy((LPVOID)0x42FE99, bFilenamePatch, 6);

	// Adjust the Save File dialog type criterion
	VirtualProtect((LPVOID)0x4E7344, 32, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4E7344, 0, 32);
	memcpy_s((LPVOID)0x4E7344, 32, "SimCity Files (*.sc2)|*.sc2||", 30);

	// Fix save filenames going wonky 
	VirtualProtect((LPVOID)0x4321B9, 8, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x4321B9, 0x90, 8);

	// Fix $1500 neighbor connections on game load
	VirtualProtect((LPVOID)0x434BEA, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWCALL((LPVOID)0x434BEA, Hook_LoadNeighborConnections1500);
	*(BYTE*)0x434BEF = 0x90;

	// Load game hook
	VirtualProtect((LPVOID)0x4025A4, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4025A4, Hook_LoadGame);
	
	// Patch to stop CFile::CFile() from being called in exclusive mode when loading a game
	VirtualProtect((LPVOID)0x430118, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(DWORD*)0x430118 = 0x8040;

	// Patch to attempt to fix loading partially corrupted saves
	VirtualProtect((LPVOID)0x431212, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x431212, Hook_431212);

	// Save game hook
	VirtualProtect((LPVOID)0x401870, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401870, Hook_SaveGame);
}
