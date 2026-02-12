// sc2kfix modules/military.cpp: hooks and restored simulations for military bases
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

#pragma intrinsic(_ReturnAddress)

#define MILITARY_DEBUG_PLACEMENT_OTHER 1
#define MILITARY_DEBUG_PLACEMENT_NAVAL 2
#define MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE 4

#define MILITARY_DEBUG DEBUG_FLAGS_NONE

#define MILITARY_RETRY_SILO_REPOSIT 32
#define MILITARY_RETRY_SILO_SPOTFIND 20
#define MILITARY_RETRY_ATTEMPT_MAX 10

#define MAX_SILOS 6
#define SILO_STRIP_LEN 3
#define SILO_PLOT_SIZE (SILO_STRIP_LEN * SILO_STRIP_LEN)

#ifdef DEBUGALL
#undef MILITARY_DEBUG
#define MILITARY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT military_debug = MILITARY_DEBUG;

static DWORD dwDummy;

static coords_w_t cornerCoords[VIEWROTATION_COUNT] = {
	{ MAP_EDGE_MAX, MAP_EDGE_MIN },
	{ MAP_EDGE_MIN, MAP_EDGE_MIN },
	{ MAP_EDGE_MIN, MAP_EDGE_MAX },
	{ MAP_EDGE_MAX, MAP_EDGE_MAX }
};

static coords_w_t directionalSteps[VIEWROTATION_COUNT] = {
	{ -1,  0  },
	{  0,  1  },
	{  1,  0  },
	{  0, -1  }
};

static __int16 advanceX[VIEWROTATION_COUNT] = {
	-1, 0, 1, 0
};

static __int16 advanceY[VIEWROTATION_COUNT] = {
	0, 1, 0, -1
};

static void SetNewCoords(coords_w_t *pCoords, __int16 x, __int16 y) {
	pCoords->x = x;
	pCoords->y = y;
}

static void SwapAxis(coords_w_t *pCoords) {
	coords_w_t tempPos;

	tempPos.x = pCoords->x;
	tempPos.y = pCoords->y;

	pCoords->x = tempPos.y;
	pCoords->y = tempPos.x;
}

static int CheckOverlappingSiloPosition(__int16 x1, __int16 y1, __int16 x2, __int16 y2) {
	__int16 iCurrXOne, iCurrYOne, iCurrXTwo, iCurrYTwo;

	for (iCurrXOne = 0; iCurrXOne < 3; iCurrXOne++) {
		__int16 xPos1 = x1 + iCurrXOne;
		if (xPos1 > MAP_EDGE_MAX)
			return 1;
		for (iCurrYOne = 0; iCurrYOne < 3; iCurrYOne++) {
			__int16 yPos1 = y1 + iCurrYOne;
			if (yPos1 > MAP_EDGE_MAX)
				return 1;
			for (iCurrXTwo = 0; iCurrXTwo < 3; iCurrXTwo++) {
				__int16 xPos2 = x2 + iCurrXTwo;
				if (xPos2 > MAP_EDGE_MAX)
					return 1;
				for (iCurrYTwo = 0; iCurrYTwo < 3; iCurrYTwo++) {
					__int16 yPos2 = y2 + iCurrYTwo;
					if (yPos2 > MAP_EDGE_MAX)
						return 1;
					if (xPos1 == xPos2 || (xPos1 >= xPos2 - 1 && xPos1 <= xPos2 + 4)) {
						if (yPos1 == yPos2 || (yPos1 >= yPos2 - 1 && yPos1 <= yPos2 + 4)) {
							//ConsoleLog(LOG_DEBUG, "CheckOverlappingSiloPosition (X-CHECK) (%d, %d, %d, %d) (%d, %d, %d, %d) xPos1(%d), yPos1(%d), xPos2(%d), yPos2(%d)\n", x1, y1, x2, y2, iCurrXOne, iCurrYOne, iCurrXTwo, iCurrYTwo, xPos1, yPos1, xPos2, yPos2);
							return 1;
						}
					}
					if (yPos1 == yPos2 || (yPos1 >= yPos2 - 1 && yPos1 <= yPos2 + 4)) {
						if (xPos1 == xPos2 || (xPos1 >= xPos2 - 1 && xPos1 <= xPos2 + 4)) {
							//ConsoleLog(LOG_DEBUG, "CheckOverlappingSiloPosition (Y-CHECK) (%d, %d, %d, %d) (%d, %d, %d, %d) xPos1(%d), yPos1(%d), xPos2(%d), yPos2(%d)\n", x1, y1, x2, y2, iCurrXOne, iCurrYOne, iCurrXTwo, iCurrYTwo, xPos1, yPos1, xPos2, yPos2);
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

static int CheckForOverlappingSiloPositions(coords_w_t *wSiloPos, int iPos, __int16 x1, __int16 y1) {
	int i;
	__int16 x2, y2;

	for (i = 0; i < iPos; i++) {
		x2 = wSiloPos[i].x;
		y2 = wSiloPos[i].y;
		if (CheckOverlappingSiloPosition(x1, y1, x2, y2))
			return 1;
		//ConsoleLog(LOG_DEBUG, "CheckForOverlappingSiloPositions(): (%d/%d) X/Y1(%d,%d), X/Y2(%d,%d)\n", i+1, iPos, x1, y1, x2, y2);
	}
	//ConsoleLog(LOG_DEBUG, "CheckForOverlappingSiloPositions(NO OVERLAP): X/Y1(%d,%d)\n", x1, y1);
	return 0;
}

static int MilitaryBaseMissileSilos(int iValidAltitudeTiles, int iValidTiles, int *nSiloCnt, coords_w_t *wSiloPos, BOOL bForce) {
	coords_w_t randPos;
	__int16 iCurrXPos, iCurrYPos;
	int iSiloIdx;
	
	if (iValidAltitudeTiles < 40 || bForce) {
		int iSiloAttempt = 0;
		int iValidPositions = *nSiloCnt;
		if (iValidTiles < 40 || bForce) {
RETRYFROMBEGINNING:
			int iRetrySiloAltitudePos = 0;
			int iRetrySiloOverlapPos = 0;
			int iRetrySiloValidPos = 0;
RETRYFROMCURRENT:
			int iIterations = 24;
			if (iSiloAttempt == 0)
				iValidPositions = *nSiloCnt;
			do {
				BOOL bMaxIteration = iIterations-- == 0;
				if (bMaxIteration)
					break;
				randPos.x = Game_RandomWordLCGMod(GAME_MAP_SIZE-4);
				randPos.y = Game_RandomWordLCGMod(GAME_MAP_SIZE-4);

				// Randomize rotation here to cause an axis-swap.
				srand((randPos.x * randPos.y) + 1);
				if ((rand() & MAP_EDGE_MAX) != 0)
					SwapAxis(&randPos);

				WORD iBaseLevel = ALTMReturnLandAltitude(randPos.x, randPos.y);
				if (!bForce) {
					if (iBaseLevel <= 5) {
						if (iRetrySiloAltitudePos < MILITARY_RETRY_SILO_REPOSIT) {
							iRetrySiloAltitudePos++;
							//ConsoleLog(LOG_DEBUG, "(VT: %d) Bad Altitude (%u) case found, trying again (%d).\n", iValidPositions, iBaseLevel, iRetrySiloAltitudePos);
							goto RETRYFROMCURRENT;
						}
						else
							return -1;
					}
				}

				//ConsoleLog(LOG_DEBUG, "(%u) (%d, %d) (%d) - (%d) !(%d, %d, %d)\n", wViewRotation, randPos.y, randPos.x, iBaseLevel, iValidPositions,
				//	iRetrySiloAltitudePos, iRetrySiloOverlapPos, iRetrySiloValidPos);

				__int16 iPlotArea = 0;
				for (iCurrXPos = randPos.x; iCurrXPos < randPos.x + SILO_STRIP_LEN; ++iCurrXPos) {
					for (iCurrYPos = randPos.y; iCurrYPos < randPos.y + SILO_STRIP_LEN; ++iCurrYPos) {
						if (GetTileID(iCurrXPos, iCurrYPos) < TILE_SMALLPARK &&
							!GetTerrainTileID(iCurrXPos, iCurrYPos) &&
							(iCurrXPos >= GAME_MAP_SIZE || iCurrYPos >= GAME_MAP_SIZE || !XBITReturnIsWater(iCurrXPos, iCurrYPos)) &&
							ALTMReturnLandAltitude(iCurrXPos, iCurrYPos) == iBaseLevel &&
							XZONReturnZone(iCurrXPos, iCurrYPos) != ZONE_MILITARY &&
							!GetUndergroundTileID(iCurrXPos, iCurrYPos)) {
							if (IsValidSiloPosCheck(iCurrXPos, iCurrYPos)) {
								if (!CheckForOverlappingSiloPositions(wSiloPos, iValidPositions, iCurrXPos, iCurrYPos)) {
									if (iPlotArea >= SILO_PLOT_SIZE)
										break;
									++iPlotArea;
								}
								else {
									if (iRetrySiloOverlapPos < MILITARY_RETRY_SILO_REPOSIT) {
										iRetrySiloOverlapPos++;
										//ConsoleLog(LOG_DEBUG, "(VT: %d) Overlapping case (%d, %d) found, trying again (%d).\n", iValidPositions, iCurrXPos, iCurrXPos, iRetrySiloOverlapPos);
										goto RETRYFROMCURRENT;
									}
								}
							}
							else {
								if (iRetrySiloValidPos < MILITARY_RETRY_SILO_REPOSIT) {
									iRetrySiloValidPos++;
									//ConsoleLog(LOG_DEBUG, "(VT: %d) Invalid case (%d, %d) found, trying again (%d).\n", iValidPositions, iCurrXPos, iCurrXPos, iRetrySiloValidPos);
									goto RETRYFROMCURRENT;
								}
							}
						}
					}
				}
				if (iPlotArea == SILO_PLOT_SIZE) {
					iSiloIdx = iValidPositions++;
					SetNewCoords(&wSiloPos[iSiloIdx], randPos.x, randPos.y);
					//ConsoleLog(LOG_DEBUG, "DBG: iPlotArea == SILO_PLOT_SIZE: (%d) (%u, %d)\n", iSiloIdx, wSiloPos[iSiloIdx].x, wSiloPos[iSiloIdx].y);
					*nSiloCnt = iSiloIdx + 1;
				}
			} while (iValidPositions < MAX_SILOS);
		}
		if (iValidPositions == MAX_SILOS) {
			randPos.x = -1;
			randPos.y = -1;
			for (iSiloIdx = 0; iSiloIdx < MAX_SILOS; iSiloIdx++) {
				randPos = wSiloPos[iSiloIdx];
				for (iCurrXPos = randPos.x; iCurrXPos < randPos.x + SILO_STRIP_LEN; ++iCurrXPos) {
					for (iCurrYPos = randPos.y; iCurrYPos < randPos.y + SILO_STRIP_LEN; ++iCurrYPos) {
						BYTE iTileID = GetTileID(iCurrXPos, iCurrYPos);
						--dwTileCount[iTileID];
						if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE)
							XZONSetNewZone(iCurrXPos, iCurrYPos, ZONE_MILITARY);
						++dwMilitaryTiles[MILITARYTILE_OTHER];
					}
				}
			}
			if (randPos.x != -1 && randPos.y != -1) {
				bMilitaryBaseType = MILITARY_BASE_MISSILE_SILOS;
				Game_CenterOnTileCoords(randPos.x, randPos.y);
				return GameMain_AfxMessageBoxID(244, 0, -1);
			}
		}
		else {
			if (iSiloAttempt < MILITARY_RETRY_SILO_SPOTFIND) {
				iSiloAttempt++;
				goto RETRYFROMBEGINNING;
			}
		}
	}
	// When 'bForce' is set to FALSE, this will lead to the normal
	// Air Force or Army Base cases.
	return (bForce) ? 0 : -1;
}

static void MilitaryBasePlotCheck(__int16 *iVAltitudeTiles, __int16 *iVTiles, coords_w_t *pRandPos) {
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	coords_w_t randPos;

	int iIterations = 24;
	iValidAltitudeTiles = 0;
	iValidTiles = 0;
	do {
		BOOL bMaxIteration = iIterations-- == 0;
		if (bMaxIteration)
			break;
		randPos.x = Game_RandomWordLCGMod(119);
		if (randPos.x < MAP_EDGE_MIN)
			randPos.x = MAP_EDGE_MIN;
		else if (randPos.x > MAP_EDGE_MAX)
			randPos.x = MAP_EDGE_MAX - 8;
		randPos.y = Game_RandomWordLCGMod(119);
		if (randPos.y < MAP_EDGE_MIN)
			randPos.y = MAP_EDGE_MIN;
		else if (randPos.y > MAP_EDGE_MAX)
			randPos.y = MAP_EDGE_MAX - 8;
		iValidAltitudeTiles = 0;
		iValidTiles = 0;
		WORD iBaseLevel = ALTMReturnLandAltitude(randPos.x, randPos.y);
		for (__int16 iCurrXPos = randPos.x; iCurrXPos < randPos.x + 8; ++iCurrXPos) {
			for (__int16 iCurrYPos = randPos.y; iCurrYPos < randPos.y + 8; ++iCurrYPos) {
				if (GetTileID(iCurrXPos, iCurrYPos) < TILE_SMALLPARK &&
					!GetTerrainTileID(iCurrXPos, iCurrYPos) &&
					(iCurrXPos >= GAME_MAP_SIZE || iCurrYPos >= GAME_MAP_SIZE || !XBITReturnIsWater(iCurrXPos, iCurrYPos)) &&
					XZONReturnZone(iCurrXPos, iCurrYPos) == ZONE_NONE) {
					++iValidTiles;
					if (ALTMReturnLandAltitude(iCurrXPos, iCurrYPos) == iBaseLevel)
						++iValidAltitudeTiles;
				}
			}
		}
	} while (iValidTiles < 40);

	*iVAltitudeTiles = iValidAltitudeTiles;
	*iVTiles = iValidTiles;
	*pRandPos = randPos;
}

static void MilitaryBasePlotPlacement(coords_w_t *pRandPos) {
	for (__int16 iCurrXPos = pRandPos->x; iCurrXPos < pRandPos->x + 8; ++iCurrXPos) {
		for (__int16 iCurrYPos = pRandPos->y; iCurrYPos < pRandPos->y + 8; ++iCurrYPos) {
			BYTE iTileID = GetTileID(iCurrXPos, iCurrYPos);
			if (iTileID < TILE_SMALLPARK &&
				!GetTerrainTileID(iCurrXPos, iCurrYPos) &&
				(iCurrXPos >= GAME_MAP_SIZE || iCurrYPos >= GAME_MAP_SIZE || !XBITReturnIsWater(iCurrXPos, iCurrYPos)) &&
				XZONReturnZone(iCurrXPos, iCurrYPos) == ZONE_NONE &&
				!GetUndergroundTileID(iCurrXPos, iCurrYPos)) {
				--dwTileCount[iTileID];
				if (iCurrXPos < GAME_MAP_SIZE && iCurrYPos < GAME_MAP_SIZE)
					XZONSetNewZone(iCurrXPos, iCurrYPos, ZONE_MILITARY);
				++dwMilitaryTiles[MILITARYTILE_OTHER];
			}
		}
	}
}

static int MilitaryBaseDecline(void) {
	int iRes = GameMain_AfxMessageBoxID(411, 0, -1);
	bMilitaryBaseType = MILITARY_BASE_DECLINED;
	return iRes;
}

static int MilitaryBaseAirForce(int iValidTiles, int iValidAltitudeTiles, coords_w_t *pRandPos) {
	if (iValidAltitudeTiles < 40)
		return 0;

	if (iValidTiles == iValidAltitudeTiles) {
		bMilitaryBaseType = MILITARY_BASE_AIR_FORCE;
		GameMain_AfxMessageBoxID(242, 0, -1);

		MilitaryBasePlotPlacement(pRandPos);

		return Game_CenterOnTileCoords(pRandPos->x + 4, pRandPos->y + 4);
	}

	return -1;
}

// This function has been replicated from the equivalent that was found
// in the DOS version of the game.
static void FormArmyBaseStrip(__int16 x1, __int16 y1, __int16 x2, __int16 y2) {
	WORD wOldTraceAction;
	__int16 nCnt;
	__int16 iX;
	__int16 iY;
	__int16 iNewX;
	__int16 iNewY;
	BYTE iTileID;

	wOldTraceAction = traceAction;
	iX = x2;
	iY = y2;
	traceAction = TRACE_ACTION_ARMYROADS;
	nCnt = 0;
	if (Game_BeginTrace(x1, y1, x2, y2)) {
		iNewX = iX = x1;
		iNewY = iY = y1;
		while (Game_StepTrace(&iNewX, &iNewY)) {
			Game_TraceEdit(iX, iY);
			iTileID = GetTileID(iX, iY);
			if (iTileID < TILE_SMALLPARK && iTileID != TILE_RADIOACTIVITY) {
				Game_PlaceRoadAtCoordinates(iX, iY);
				if (GetTileID(iX, iY) >= TILE_ROAD_LR && GetTileID(iX, iY) <= TILE_ROAD_LTBR) {
					XZONSetNewZone(iX, iY, ZONE_MILITARY);
					nCnt++;
				}
			}
			iX = iNewX;
			iY = iNewY;
		}
		Game_TraceEdit(iX, iY);
		iTileID = GetTileID(iX, iY);
		if (iTileID < TILE_SMALLPARK && iTileID != TILE_RADIOACTIVITY) {
			Game_PlaceRoadAtCoordinates(iX, iY);
			if (GetTileID(iX, iY) >= TILE_ROAD_LR && GetTileID(iX, iY) <= TILE_ROAD_LTBR) {
				XZONSetNewZone(iX, iY, ZONE_MILITARY);
				nCnt++;
			}
		}
	}
	if (nCnt > 0) {
		iTileID = GetTileID(x1, y1);
		if (iTileID == TILE_ROAD_LR || iTileID == TILE_ROAD_TB) {
			// TERRAIN_00 check added here to avoid the runwaycross
			// being placed into a dip (likely replacing a slope or granite block).
			if (!GetTerrainTileID(x1, y1) && XZONReturnZone(x1, y1) == ZONE_MILITARY) {
				XZONSetCornerMask(x1, y1, CORNER_ALL);
				Game_PlaceTileWithMilitaryCheck(x1, y1, TILE_INFRASTRUCTURE_RUNWAYCROSS);
			}
		}
		if (nCnt > 1) {
			iTileID = GetTileID(iX, iY);
			if (iTileID == TILE_ROAD_LR || iTileID <= TILE_ROAD_TB) {
				// TERRAIN_00 check added here to avoid the runwaycross
				// being placed into a dip (likely replacing a slope or granite block).
				if (!GetTerrainTileID(iX, iY) && XZONReturnZone(iX, iY) == ZONE_MILITARY) {
					XZONSetCornerMask(iX, iY, CORNER_ALL);
					Game_PlaceTileWithMilitaryCheck(iX, iY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
				}
			}
		}
	}
	traceAction = wOldTraceAction;
	Game_EndTrace();
}

static void DoArmyBaseStrips(__int16 iStartX, __int16 iStartY, __int16 iEndX, __int16 iEndY) {
	FormArmyBaseStrip(iStartX, iStartY, iEndX, iEndY);
	if (!GetTileID(iEndX, iEndY))
		FormArmyBaseStrip(iEndX, iEndY, iStartX, iStartY);
}

static int MilitaryBaseArmyBase(int iValidTiles, int iValidAltitudeTiles, coords_w_t *pRandPos) {
	if (iValidTiles < 40)
		return 0;

	if (iValidTiles != iValidAltitudeTiles) {
		bMilitaryBaseType = MILITARY_BASE_ARMY;
		GameMain_AfxMessageBoxID(241, 0, -1);

		MilitaryBasePlotPlacement(pRandPos);

		// Explanation:
		// First it lays down the depth-way roads and runwaycross.
		// Second it lays down the length-way roads and runwaycross.
		// It checks to make sure at X + N and Y + N that the tile is
		// empty (just in case a spawn failed), and then it lays the 
		// road-strip and runwaycross from the opposite direction.
		DoArmyBaseStrips(pRandPos->x + 2, pRandPos->y, pRandPos->x + 2, pRandPos->y + 7);
		DoArmyBaseStrips(pRandPos->x + 5, pRandPos->y, pRandPos->x + 5, pRandPos->y + 7);
		
		DoArmyBaseStrips(pRandPos->x, pRandPos->y + 2, pRandPos->x + 7, pRandPos->y + 2);
		DoArmyBaseStrips(pRandPos->x, pRandPos->y + 5, pRandPos->x + 7, pRandPos->y + 5);

		return Game_CenterOnTileCoords(pRandPos->x + 4, pRandPos->y + 4);
	}

	return -1;
}

static int isValidWaterBody(__int16 x, __int16 y) {
	return (XBITReturnIsSaltWater(x, y) && XBITReturnIsWater(x, y)) ? 1 : 0;
}

static int MilitaryBaseNavalYard(BOOL bForce) {
	WORD iBaseLevel;

	coords_w_t iStartCoords, iAdvanceBy;
	coords_w_t iFinalCoords, iIntermediateCoords;
	int nCurrPos, nNextPos;
	WORD iRotOne, iRotTwo, iRotThree;
	__int16 iNextX, iNextY;
	
	if (bCityHasOcean) {
		if ((rand() & 1) != 0 || bForce) {
			iStartCoords = cornerCoords[wViewRotation];
			iAdvanceBy = directionalSteps[wViewRotation];

			if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
				ConsoleLog(LOG_DEBUG, "(%u) (%d, %d) (%d, %d)\n", wViewRotation, iStartCoords.x, iStartCoords.y, iAdvanceBy.x, iAdvanceBy.y);

			if (XBITReturnIsWater(iStartCoords.x, iStartCoords.y)) {
				while (TRUE) {
					if (iStartCoords.x < MAP_EDGE_MIN || iStartCoords.x > MAP_EDGE_MAX || iStartCoords.y < MAP_EDGE_MIN || iStartCoords.y > MAP_EDGE_MAX)
						goto NONAVY;
					if (!XBITReturnIsWater(iStartCoords.x, iStartCoords.y))
						break;
					iStartCoords.x += iAdvanceBy.x;
					iStartCoords.y += iAdvanceBy.y;
				}

				if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
					ConsoleLog(LOG_DEBUG, "- (%d, %d) (%d, %d) (%d, %d)\n", iStartCoords.x, iStartCoords.y, iAdvanceBy.x, iAdvanceBy.y, advanceX[wViewRotation], advanceY[wViewRotation]);

				iRotOne   = wViewRotation;
				iRotTwo   = (iRotOne + 1) & 3;
				iRotThree = (iRotOne + 2) & 3;

				if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
					ConsoleLog(LOG_DEBUG, "-- (%d, %d) (%d, %d) (%u, %u, %u)\n", iStartCoords.x, iStartCoords.y, iAdvanceBy.x, iAdvanceBy.y,
						iRotOne, iRotTwo, iRotThree);

				iFinalCoords = iStartCoords;
				iIntermediateCoords = iStartCoords;
				nCurrPos = 0;
				nNextPos = 0;
				while (iStartCoords.x >= MAP_EDGE_MIN && iStartCoords.x <= MAP_EDGE_MAX && iStartCoords.y >= MAP_EDGE_MIN && iStartCoords.y <= MAP_EDGE_MAX) {
					iBaseLevel = ALTMReturnLandAltitude(iStartCoords.x, iStartCoords.y);
					if (GetTileID(iStartCoords.x, iStartCoords.y) >= TILE_SMALLPARK ||
						GetTerrainTileID(iStartCoords.x, iStartCoords.y) ||
						XBITReturnIsWater(iStartCoords.x, iStartCoords.y) ||
						GetUndergroundTileID(iStartCoords.x, iStartCoords.y) ||
						ALTMReturnLandAltitude(iStartCoords.x, iStartCoords.y) != iBaseLevel) {
						iFinalCoords = iStartCoords;
						nNextPos = 0;
					}
					else if (++nNextPos > nCurrPos) {
						iIntermediateCoords = iFinalCoords;
						nCurrPos = nNextPos;
					}
					iNextX = (iStartCoords.x + advanceX[iRotTwo]);
					iNextY = (iStartCoords.y + advanceY[iRotTwo]);
					if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
						if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
							ConsoleLog(LOG_DEBUG, "Break 0: (%d, %d)\n", iNextX, iNextY);
						break;
					}
					SetNewCoords(&iAdvanceBy, iNextX, iNextY);
					//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
					//	ConsoleLog(LOG_DEBUG, "0 (%d, %d)\n", iStartCoords.x, iStartCoords.y);
					if (isValidWaterBody(iAdvanceBy.x, iAdvanceBy.y)) {
						do {
							iNextX = (iAdvanceBy.x + advanceX[iRotOne]);
							iNextY = (iAdvanceBy.y + advanceY[iRotOne]);
							if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
								if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
									ConsoleLog(LOG_DEBUG, "Break 1: (%d, %d)\n", iNextX, iNextY);
								break;
							}
							SetNewCoords(&iStartCoords, iNextX, iNextY);
							iAdvanceBy = iStartCoords;
							//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
							//	ConsoleLog(LOG_DEBUG, "1 (%d, %d)\n", iStartCoords.x, iStartCoords.y);
						} while (isValidWaterBody(iStartCoords.x, iStartCoords.y));
					}
					else {
						do {
							iStartCoords = iAdvanceBy;
							iNextX = (iAdvanceBy.x + advanceX[iRotThree]);
							iNextY = (iAdvanceBy.y + advanceY[iRotThree]);
							if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
								if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
									ConsoleLog(LOG_DEBUG, "Break 2: (%d, %d)\n", iNextX, iNextY);
								break;
							}
							SetNewCoords(&iAdvanceBy, iNextX, iNextY);
							//BYTE iTile = (wViewRotation == VIEWROTATION_SOUTH) ? TILE_TREES7 : (wViewRotation == VIEWROTATION_WEST) ? TILE_TREES5 : (wViewRotation == VIEWROTATION_EAST) ? TILE_TREES3 : TILE_TREES1;
							// Temporary coastline drawing.
							//Game_PlaceTileWithMilitaryCheck(iAdvanceBy.x, iAdvanceBy.y, iTile);
							//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
							//	ConsoleLog(LOG_DEBUG, "2 (%d, %d)\n", iAdvanceBy.x, iAdvanceBy.y);
						} while (!isValidWaterBody(iAdvanceBy.x, iAdvanceBy.y));
					}
				}
				iStartCoords = iIntermediateCoords;
				if (nCurrPos >= 12) {
					if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
						ConsoleLog(LOG_DEBUG, "--- (%d, %d) (%d) (%d) (%d, %d)\n", iStartCoords.x, iStartCoords.y, nCurrPos, nNextPos,
							iIntermediateCoords.x, iIntermediateCoords.y);
					int nRow = (nCurrPos - 12) / 2;
					while (nRow <= --nCurrPos) {
						iNextX = (iStartCoords.x + advanceX[iRotTwo]);
						iNextY = (iStartCoords.y + advanceY[iRotTwo]);
						if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
							if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
								ConsoleLog(LOG_DEBUG, "Break 3: (%d, %d)\n", iNextX, iNextY);
							break;
						}
						SetNewCoords(&iAdvanceBy, iNextX, iNextY);
						//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
						//	ConsoleLog(LOG_DEBUG, "3 (%d, %d)\n", iAdvanceBy.x, iAdvanceBy.y);
						if (isValidWaterBody(iAdvanceBy.x, iAdvanceBy.y)) {
							do {
								iNextX = (iAdvanceBy.x + advanceX[iRotOne]);
								iNextY = (iAdvanceBy.y + advanceY[iRotOne]);
								if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
									if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
										ConsoleLog(LOG_DEBUG, "Break 4: (%d, %d)\n", iNextX, iNextY);
									break;
								}
								SetNewCoords(&iStartCoords, iNextX, iNextY);
								iAdvanceBy = iStartCoords;
								//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
								//	ConsoleLog(LOG_DEBUG, "4 (%d, %d)\n", iStartCoords.x, iStartCoords.y);
							} while (isValidWaterBody(iStartCoords.x, iStartCoords.y));
						}
						else {
							do {
								iStartCoords = iAdvanceBy;
								iNextX = (iAdvanceBy.x + advanceX[iRotThree]);
								iNextY = (iAdvanceBy.y + advanceY[iRotThree]);
								if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
									if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
										ConsoleLog(LOG_DEBUG, "Break 5: (%d, %d)\n", iNextX, iNextY);
									break;
								}
								SetNewCoords(&iAdvanceBy, iNextX, iNextY);
								//BYTE iTile = (wViewRotation == VIEWROTATION_SOUTH) ? TILE_RUBBLE1 : (wViewRotation == VIEWROTATION_WEST) ? TILE_RUBBLE3 : (wViewRotation == VIEWROTATION_EAST) ? TILE_RUBBLE2 : TILE_RUBBLE4;
								// Temporary drawing.
								//Game_PlaceTileWithMilitaryCheck(iAdvanceBy.x, iAdvanceBy.y, iTile);
								//if (military_debug & (MILITARY_DEBUG_PLACEMENT_NAVAL|MILITARY_DEBUG_PLACEMENT_NAVAL_VERBOSE))
								//	ConsoleLog(LOG_DEBUG, "5 (%d, %d)\n", iAdvanceBy.x, iAdvanceBy.y);
							} while (!isValidWaterBody(iAdvanceBy.x, iAdvanceBy.y));
						}
						if (nCurrPos < nRow + 10) {
							iAdvanceBy = iStartCoords;
							iBaseLevel = ALTMReturnLandAltitude(iAdvanceBy.x, iAdvanceBy.y);
							for (nNextPos = 0; nNextPos < 4; ++nNextPos) {
								BYTE iTileID = GetTileID(iAdvanceBy.x, iAdvanceBy.y);
								if (iTileID < TILE_SMALLPARK &&
									!GetTerrainTileID(iAdvanceBy.x, iAdvanceBy.y) &&
									!XBITReturnIsWater(iAdvanceBy.x, iAdvanceBy.y) &&
									!GetUndergroundTileID(iAdvanceBy.x, iAdvanceBy.y) && 
									ALTMReturnLandAltitude(iAdvanceBy.x, iAdvanceBy.y) == iBaseLevel) {
									Game_PlaceTileWithMilitaryCheck(iAdvanceBy.x, iAdvanceBy.y, 0);
									XZONSetNewZone(iAdvanceBy.x, iAdvanceBy.y, ZONE_MILITARY);
									--dwTileCount[iTileID];
									++dwMilitaryTiles[MILITARYTILE_OTHER];
								}
								iNextX = (iAdvanceBy.x + advanceX[iRotOne]);
								iNextY = (iAdvanceBy.y + advanceY[iRotOne]);
								if (iNextX < MAP_EDGE_MIN || iNextX > MAP_EDGE_MAX || iNextY < MAP_EDGE_MIN || iNextY > MAP_EDGE_MAX) {
									if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
										ConsoleLog(LOG_DEBUG, "Break 6: (%d, %d)\n", iNextX, iNextY);
									break;
								}
								SetNewCoords(&iAdvanceBy, iNextX, iNextY);
							}
							if (nCurrPos == nRow + 5)
								iFinalCoords = iStartCoords;
						}
					}
					bMilitaryBaseType = MILITARY_BASE_NAVY;
					Game_CenterOnTileCoords(iFinalCoords.x, iFinalCoords.y);
					return GameMain_AfxMessageBoxID(243, 0, -1);
				}
			}
		}
	}
NONAVY:
	return -1;
}

void ProposeMilitaryBaseDecline(void) {
	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to stop the development of existing military zones?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;

	if (bMilitaryBaseType <= MILITARY_BASE_DECLINED) {
		L_MessageBoxA(GameGetRootWindowHandle(), "Military base development has already been stopped.", "Clonk", MB_OK|MB_ICONASTERISK);
		return;
	}
	MilitaryBaseDecline();
}

void ProposeMilitaryBaseMissileSilos(void) {
	BOOL bForce = FALSE;
	int nSiloCnt;
	coords_w_t wSiloPos[MAX_SILOS];

	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn Missile Silos?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;

	unsigned int iMilitaryBaseTries = 0;

	nSiloCnt = 0;
	memset(wSiloPos, 0, sizeof(wSiloPos));

REATTEMPT:
	int iSiloRet = MilitaryBaseMissileSilos(0, 0, &nSiloCnt, wSiloPos, bForce);
	if (iSiloRet <= 0) {
		if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
			iMilitaryBaseTries++;
			// Force on the last attempt.
			if (iMilitaryBaseTries == MILITARY_RETRY_ATTEMPT_MAX - 1)
				bForce = TRUE;
			goto REATTEMPT;
		}
		MilitaryBaseDecline();
	}
}

void ProposeMilitaryBaseAirForceBase(void) {
	int iResult;
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	coords_w_t randPos;

	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	iResult = -1;
	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn an Air Force plot?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;
	
	unsigned int iMilitaryBaseTries = 0;
REATTEMPT:
	MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &randPos);
	
	iResult = MilitaryBaseAirForce(iValidTiles, iValidAltitudeTiles, &randPos);
	if (iResult <= 0) {
		if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
			iMilitaryBaseTries++;
			goto REATTEMPT;
		}
		MilitaryBaseDecline();
	}
}

void ProposeMilitaryBaseArmyBase(void) {
	int iResult;
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	coords_w_t randPos;

	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	iResult = -1;
	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn an Army Base plot?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;
	
	unsigned int iMilitaryBaseTries = 0;
REATTEMPT:
	MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &randPos);
	
	iResult = MilitaryBaseArmyBase(iValidTiles, iValidAltitudeTiles, &randPos);
	if (iResult <= 0) {
		if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
			iMilitaryBaseTries++;
			goto REATTEMPT;
		}
		MilitaryBaseDecline();
	}
}

void ProposeMilitaryBaseNavalYard(void) {
	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	if (!bCityHasOcean) {
		L_MessageBoxA(GameGetRootWindowHandle(), "A Naval Yard cannot be placed in a city without a neighbouring ocean.", "Clonk", MB_OK|MB_ICONSTOP);
		return;
	}

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn a Naval Yard plot?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES) 
		return;
	
	unsigned int iMilitaryBaseTries = 0;
REATTEMPT:
	int iResult = MilitaryBaseNavalYard(TRUE);
	if (iResult < 0) {
		if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
			iMilitaryBaseTries++;
			goto REATTEMPT;
		}
		MilitaryBaseDecline();
	}
}

extern "C" void __stdcall Hook_SimulationProposeMilitaryBase(void) {
	int iResult;
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	coords_w_t randPos;
	int nSiloCnt;
	coords_w_t wSiloPos[MAX_SILOS];
	
	unsigned int iMilitaryBaseTries = 0;

	nSiloCnt = 0;
	memset(wSiloPos, 0, sizeof(wSiloPos));

	if (GameMain_AfxMessageBoxID(240, MB_YESNO, -1) == IDNO)
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	else {
	REATTEMPT:
		if (MilitaryBaseNavalYard(FALSE) < 0) {
			MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &randPos);

			iResult = MilitaryBaseMissileSilos(iValidAltitudeTiles, iValidTiles, &nSiloCnt, wSiloPos, FALSE);
			if (iResult <= 0) {
				if (iResult < 0)
					iResult = MilitaryBaseAirForce(iValidTiles, iValidAltitudeTiles, &randPos);
				if (iResult <= 0) {
					if (iResult < 0)
						iResult = MilitaryBaseArmyBase(iValidTiles, iValidAltitudeTiles, &randPos);
					if (iResult <= 0) {
						iResult = 0;

						if (iResult == 0) {
							// During the base-type checking order it goes as follows:
							// 1) Naval
							// 2) Silo
							// 3) Air Force
							// 4) Army Base
							//
							// The Silo case also does a general area check when it comes to
							// general suitability, hence why the retry and decline cases are
							// present in this area.
							if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
								iMilitaryBaseTries++;
								goto REATTEMPT;
							}
							MilitaryBaseDecline();
						}
					}
				}
			}
		}
	}
}

void InstallMilitaryHooks_SC2K1996(void) {
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);
}
