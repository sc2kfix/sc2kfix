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

static char* szLoadFileName = NULL;
static int iCorruptedFixupSize = 0;

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

				wNationalTax = ntohl(*(DWORD*)&pChunkMISC[i]);
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
					wArrBondData[i] = ntohl(*(DWORD*)&pChunkMISC[i * 4]);
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
				if (!pThis->dwSCAGameMusic) {
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

				wConnectTiles = ntohl(*(DWORD*)&pChunkMISC[i]);	// Unused, but we'll load it anyways.
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

				wDisasterObject = ntohl(*(DWORD*)&pChunkMISC[i]);
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

// Function prototype: HOOKCB void Hook_SimcityApp_OpenCityData_Before(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will have a
//   chance to pre-load any information and optionally manipulate the save file before it's parsed
//   by sc2kfix and loaded into the SimCity 2000 engine.
std::vector<hook_function_t> stHooks_Hook_SimcityApp_OpenCityData_Before;

// Function prototype: HOOKCB void Hook_SimcityApp_OpenCityData_After(void)
// Cannot be ignored.
// SPECIAL NOTE: When the SC2X save format is implemented, this will be where mods will be fed a
//   pointer to a JSON object wherein they can load their data and version information or a NULL
//   or similar object to inform them that they have no known state to load.
std::vector<hook_function_t> stHooks_Hook_SimcityApp_OpenCityData_After;

extern "C" DWORD __stdcall Hook_SimcityApp_OpenCityData(CMFC3XFile* pFile, char* src) {
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

	for (const auto& hook : stHooks_Hook_SimcityApp_OpenCityData_Before) {
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
		ret = GameMain_SimcityApp_OpenCityData(pThis, pFile, src);
#endif
	} else if (std::regex_search(szLoadFileName, std::regex("\\.[Ss][Cc][Nn]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game is a vanilla SCN file. Passing control to SC2K.\n");

		ret = GameMain_SimcityApp_OpenCityData(pThis, pFile, src);
	} else if (std::regex_search(szLoadFileName, std::regex("\\.[Cc][Tt][Yy]$"))) {
		if (sc2x_debug & SC2X_DEBUG_LOAD)
			ConsoleLog(LOG_DEBUG, "SC2X: Saved game is a SimCity Classic file. Passing control to SC2K.\n");

		ret = GameMain_SimcityApp_OpenCityData(pThis, pFile, src);
	}

	for (const auto& hook : stHooks_Hook_SimcityApp_OpenCityData_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, CMFC3XFile*, char*) = (void(*)(CSimcityAppPrimary*, CMFC3XFile*, char*))hook.pFunction;
			fnHook(pThis, pFile, src);
		}
	}

	return ret;
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
		m_ofn.lpstrDefExt = "sc2";
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
			if (Game_SimcityApp_OpenCity(pThis, m_ofn.lpstrFile)) {
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

extern "C" int __stdcall Hook_SimcityApp_WriteCityHeader(CMFC3XFile *pFile, int nDatSize) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	DWORD scrChunk, nChunk, nScrDatSize;

	GameMain_File_Seek(pFile, 0, 0);
	scrChunk = L_byteswap_longlabel("FORM");
	nChunk = _byteswap_ulong(scrChunk);
	GameMain_File_Write(pFile, &nChunk, sizeof(nChunk));
	nScrDatSize = _byteswap_ulong(nDatSize);
	GameMain_File_Write(pFile, &nScrDatSize, sizeof(nScrDatSize));
	scrChunk = L_byteswap_longlabel("SCDH");
	nChunk = _byteswap_ulong(scrChunk);
	GameMain_File_Write(pFile, &nChunk, sizeof(nChunk));
	return 1;
}

// For the CNAM chunk this must not be changed, it must
// remain set to a value of 32.
#define CNAM_DAT_LEN 32

extern "C" int __stdcall Hook_SimcityApp_WriteCityName(CMFC3XFile *pFile, DWORD scrChunk, CMFC3XString pName) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	DWORD nChunk, nFullLen;
	DWORD nNameLen;

	nChunk = _byteswap_ulong(scrChunk);
	GameMain_File_Write(pFile, &nChunk, sizeof(nChunk));
	nFullLen = _byteswap_ulong(CNAM_DAT_LEN);
	GameMain_File_Write(pFile, &nFullLen, sizeof(nFullLen));
	nNameLen = CNAM_DAT_LEN - 1; // 31
	GameMain_File_Write(pFile, &nNameLen, 1);
	GameMain_File_Write(pFile, pName.m_pchData, nNameLen);
	nDataOffset += CNAM_DAT_LEN + 8;
	GameMain_String_Dest(&pName);
	return 1;
}

extern "C" int __stdcall Hook_SimcityApp_WriteCityUncompressed(CMFC3XFile *pFile, DWORD scrChunk, WORD *pDat, int nDatSize) {
	CSimcityAppPrimary* pThis;

	__asm mov [pThis], ecx

	DWORD nChunk, nScrDatSize;

	nChunk = _byteswap_ulong(scrChunk);
	GameMain_File_Write(pFile, &nChunk, sizeof(nChunk));
	nScrDatSize = _byteswap_ulong(nDatSize);
	GameMain_File_Write(pFile, &nScrDatSize, sizeof(nScrDatSize));
	L_byteswap_ushorts(pDat, nDatSize);
	GameMain_File_Write(pFile, pDat, nDatSize);
	L_byteswap_ushorts(pDat, nDatSize);
	nDataOffset += nDatSize + 8;
	return 1;
}

// The new local call concerning wonky filenames - the original remote hook
// is no longer necessary.
static bool L_CheckAndAppendCityExtension(char *lpFileName, char *pExt) {
	char szTempFile[MAX_PATH + 1], szTempExt[16 + 1], szTempPath[MAX_PATH + 1];
	int nLen;

	// NOTE: An interesting quirk in an MDI program with multiple
	// supported document types (file extensions) is that if you
	// happen to specify either 'sc2' or 'scn' then either of those
	// extensions will be used - before they're stipped in this function.
	// However if you specify any other extension than the above
	// then '.sc2' will be appended. Due to this the 'lpstrDefExt' attribute
	// has been disabled to avoid this behaviour, the intended extension
	// is then appended here.
	strcpy_s(szTempFile, lpFileName);
	strcpy_s(szTempExt, pExt);
	PathStripPathA(szTempFile);
	PathRemoveExtensionA(szTempFile);
	_strlwr_s(szTempExt);
	nLen = strlen(szTempFile);
	// nLen above 0.
	if (nLen > 0) {
		// Check for a valid last stored city path, otherwise use the default
		// derived from the game path.
		if (L_IsDirectoryPathValid(jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES].ToString().c_str()))
			strcpy_s(szTempPath, jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES].ToString().c_str());
		else
			sprintf_s(szTempPath, "%s\\Cities\\", szGamePath);

		// Empty the filename string and rebuild it.
		memset(lpFileName, 0, MAX_PATH + 1);
		sprintf_s(lpFileName, MAX_PATH, "%s%s.%s", szTempPath, szTempFile, szTempExt);
		return true;
	}
	return false;
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

int L_SimcityApp_DoSave(CSimcityAppPrimary *pSCApp, char* lpFileName) {
	CMFC3XFile cFile;
	int ret;
	CSimcityView *pSCView;
	CMFC3XString strFileName;

	// With this positioning the lpFileName will be prior to any adjustment.
	for (const auto& hook : stHooks_L_SimcityApp_DoSave_Before) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, char*) = (void(*)(CSimcityAppPrimary*, char*))hook.pFunction;
			fnHook(pSCApp, lpFileName);
		}
	}

	GameMain_File_Cons(&cFile);

	ret = 0;

	if (L_CheckAndAppendCityExtension(lpFileName, "SC2")) {
		GameMain_String_Cons(&strFileName);
		GameMain_String_OperatorSet(&strFileName, lpFileName);
		// For the flag names see CMFC3XFile -> OpenFlags
		if (GameMain_File_Open(&cFile, lpFileName, (0x8000 | 0x1000 | 0x0010 | 0x0001), &fileExcept)) {
			nRetState = Game_SimcityApp_AllocateMiscInfo(pSCApp);
			pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
			if (pSCView) {
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_ScaleOut(pSCView);
			}
			GameMain_CmdTarget_BeginWaitCursor(pSCApp);
			// It should be noted that the 'strFileName' parameter
			// doesn't appear to be used in the subsequent call (or
			// perhaps not in the release build - beyond the downstream
			// local-copy being destroyed).
			ret = Game_SimcityApp_WriteCity(pSCApp, &cFile, strFileName);
			GameMain_CmdTarget_EndWaitCursor(pSCApp);
			if (pSCView) {
				if (pSCView->dwSCVIsZoomed)
					Game_SimcityView_ScaleIn(pSCView);
			}
			// Remote free due to the allocation
			// occurring in the native program (for now).
			GameMain_Op_Delete(MiscInfo);
			MiscInfo = 0;
			GameMain_File_Close(&cFile);
			if (!ret)
				GameMain_File_Remove(lpFileName);
		}
		else
			Game_GetFileExceptionError(47, &fileExcept, &strFileName);
		GameMain_String_Dest(&strFileName);
	}

	GameMain_File_Dest(&cFile);

	// With this positioning the lpFIleName will be after any adjustment.
	for (const auto& hook : stHooks_L_SimcityApp_DoSave_After) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void (*fnHook)(CSimcityAppPrimary*, char*) = (void(*)(CSimcityAppPrimary*, char*))hook.pFunction;
			fnHook(pSCApp, lpFileName);
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
		// If you're loading a scenario or any object that's
		// NOT a standard sc2 city, go to SaveCityAs.
		bCanDoDirectSave = false;
		if (strCityFilename.m_nDataLength > 0) {
			strcpy_s(szPath, strCityFilename.m_pchData);
			if (PathMatchSpecA(szPath, "*.sc2"))
				bCanDoDirectSave = true;
		}
		if (bCanDoDirectSave) {
			if (L_SimcityApp_DoSave(pThis, szPath)) {
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
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 60, szErrStr, sizeof(szErrStr) - 1);
				uType = 0xFFFFFFFF;
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
	OPENFILENAMEA m_ofn;

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
		nLen = strlen(m_extFileDlg.szCityName);
		m_extFileDlg.szCityName[nLen] = 0;

		if (strCityFilename.m_nDataLength > 0) {
			strcpy_s(szPath, strCityFilename.m_pchData);
			PathStripPathA(szPath);
			PathRemoveExtensionA(szPath);
		}
		else
			strcpy_s(szPath, m_extFileDlg.szCityName);
		strcat_s(szPath, ".sc2");

		memset(&m_ofn, 0, sizeof(OPENFILENAMEA));
		m_ofn.lStructSize = sizeof(OPENFILENAMEA);
		m_ofn.hwndOwner = pThis->m_pMainWnd->m_hWnd;
		m_ofn.hInstance = hSC2KFixModule;
		m_ofn.lpstrFilter = ConvertFileTypeFilterString("SimCity 2000 City (*.sc2)|*.sc2||");
		m_ofn.lpstrInitialDir = szFilePath;
		m_ofn.nMaxFile = _countof(szPath);
		m_ofn.nMaxFileTitle = _countof(szPath);
		m_ofn.nFilterIndex = 1;
		// Deliberately commented out - see comment in L_CheckAndAppendCityExtension()
		//m_ofn.lpstrDefExt = "sc2";
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
			if (m_extFileDlg.bCityNameChanged) {
				int nLen = strlen(m_extFileDlg.szCityName);
				if (nLen >= 1 && nLen <= CITY_NAME_LEN) {
					if (sc2x_debug & SC2X_DEBUG_VANILLA_SAVE)
						ConsoleLog(LOG_DEBUG, "New City Name: '%s' (%d)\n", m_extFileDlg.szCityName, nLen);
					GameMain_String_Empty(&pszCityName);
					GameMain_String_OperatorSet(&pszCityName, m_extFileDlg.szCityName);
				}
			}
			strcpy_s(szDirPath, m_ofn.lpstrFile);
			PathRemoveFileSpecA(szDirPath);
			nPathLen = strlen(szDirPath);
			if (szDirPath[nPathLen - 1] != '\\')
				strcat_s(szDirPath, "\\");
			if (L_SimcityApp_DoSave(pThis, m_ofn.lpstrFile)) {
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 51, szErrStr, sizeof(szErrStr) - 1);
				GameMain_String_Empty(&strUnusedString);
				GameMain_String_OperatorSet(&strCityFilename, m_ofn.lpstrFile);
				strcat_s(szErrStr, m_ofn.lpstrFile);

				if (L_IsPathValid(szDirPath))
					jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_CITIES] = szDirPath;
			}
			else
				L_LoadStringA(game_AfxCoreState.m_hCurrentResourceHandle, 60, szErrStr, sizeof(szErrStr) - 1);
			L_MessageBoxA(0, szErrStr, gamePrimaryKey, 0);
		}
		pThis->dwSCAOnQuitSuspendSim = 0;
		ToggleFloatingStatusDialog(TRUE);

		GameMain_String_Dest(&strFilePath);
	}
}

// Assembly language hook to try to fix up corrupted save file headers.
void __declspec(naked) Hook_OpenCityHeader_FormChunkCheck(void) {
	// Replace the code we're clobbering to inject ourselves
	__asm {
		// Original call flow
		mov eax, 0x401429     // program-side equivalent of _byteswap_ulong()
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
			"Failed to load iCorruptedFixupSize in Hook_SimcityApp_OpenCityData", "sc2kfix error", MB_OK | MB_ICONERROR);

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
			--dwCurrBonds;
			dwInterestRateSum += wArrBondData[wCurrBond];
			++wCurrBond;
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

	// InitializeCityData:
	// - Demystification
	// - A formal fix for the $1500 neighbor connections on game load (IndustryConnect)
	SafeVirtualProtect((LPVOID)0x402743, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402743, Hook_InitializeCityData);

	// Load game hook
	SafeVirtualProtect((LPVOID)0x4025A4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4025A4, Hook_SimcityApp_OpenCityData);

	// CSimcityApp::LoadCity
	SafeVirtualProtect((LPVOID)0x401E1F, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E1F, Hook_SimcityApp_LoadCity);
	
	// Patch to stop CFile::CFile() from being called in exclusive mode when loading a game
	SafeVirtualProtect((LPVOID)0x430118, 5, PAGE_EXECUTE_READWRITE);
	*(DWORD*)0x430118 = 0x8040;

	// Patch to attempt to fix loading partially corrupted saves
	SafeVirtualProtect((LPVOID)0x431212, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x431212, Hook_OpenCityHeader_FormChunkCheck);

	// CSimcityApp::WriteCityName
	SafeVirtualProtect((LPVOID)0x4010CD, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4010CD, Hook_SimcityApp_WriteCityHeader);

	// CSimcityApp::WriteCityName
	SafeVirtualProtect((LPVOID)0x402400, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402400, Hook_SimcityApp_WriteCityName);

	// CSimcityApp::WriteCityUncompress
	SafeVirtualProtect((LPVOID)0x401C2B, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401C2B, Hook_SimcityApp_WriteCityUncompressed);

	// CSimcityApp::SaveCity
	SafeVirtualProtect((LPVOID)0x4015A0, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015A0, Hook_SimcityApp_SaveCity);

	// CSimcityApp::SaveCityAs
	SafeVirtualProtect((LPVOID)0x401929, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401929, Hook_SimcityApp_SaveCityAs);
}
