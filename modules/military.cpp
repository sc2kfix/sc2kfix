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

#ifdef DEBUGALL
#undef MILITARY_DEBUG
#define MILITARY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT military_debug = MILITARY_DEBUG;

static DWORD dwDummy;

// This function has been replicated from he equivalent that was found
// in the DOS version of the game.
static void FormArmyBaseStrip(__int16 x1, __int16 y1, __int16 x2, __int16 y2) {
	WORD wOldToolGroup;
	__int16 nCnt;
	__int16 iX;
	__int16 iY;
	__int16 iNewX;
	__int16 iNewY;
	BYTE iTileID;

	wOldToolGroup = wMaybeActiveToolGroup;
	iX = x2;
	iY = y2;
	wMaybeActiveToolGroup = CITYTOOL_GROUP_ROADS;
	nCnt = 0;
	if (Game_MaybeCheckViablePlacementPath(x1, y1, x2, y2)) {
		iNewX = iX = x1;
		iNewY = iY = y1;
		while (Game_MaybeRoadViabilityAlongPath(&iNewX, &iNewY)) {
			Game_CheckAndAdjustTraversableTerrain(iX, iY);
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
		Game_CheckAndAdjustTraversableTerrain(iX, iY);
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
		if (iTileID >= TILE_ROAD_LR && iTileID <= TILE_ROAD_LTBR) {
			// TERRAIN_00 check added here to avoid the runwaycross
			// being placed into a dip (likely replacing a slope or granite block).
			if (!GetTerrainTileID(x1, y1) && XZONReturnZone(x1, y1) == ZONE_MILITARY) {
				XZONSetCornerMask(x1, y1, CORNER_ALL);
				Game_PlaceTileWithMilitaryCheck(x1, y1, TILE_INFRASTRUCTURE_RUNWAYCROSS);
			}
		}
		if (nCnt > 1) {
			iTileID = GetTileID(iX, iY);
			if (iTileID >= TILE_ROAD_LR && iTileID <= TILE_ROAD_LTBR) {
				// TERRAIN_00 check added here to avoid the runwaycross
				// being placed into a dip (likely replacing a slope or granite block).
				if (!GetTerrainTileID(iX, iY) && XZONReturnZone(iX, iY) == ZONE_MILITARY) {
					XZONSetCornerMask(iX, iY, CORNER_ALL);
					Game_PlaceTileWithMilitaryCheck(iX, iY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
				}
			}
		}
	}
	wMaybeActiveToolGroup = wOldToolGroup;
	Game_ResetTileDirection();
}

static bool FindArmyBaseCrossingDepth(__int16 iX, __int16 iYA, __int16 iYB) {
	if (GetTileID(iX, iYA) == TILE_INFRASTRUCTURE_RUNWAYCROSS && GetTileID(iX, iYB) == TILE_INFRASTRUCTURE_RUNWAYCROSS)
		return true;
	return false;
}

static bool FindArmyBaseCrossingLength(__int16 iY, __int16 iXA, __int16 iXB) {
	if (GetTileID(iXA, iY) == TILE_INFRASTRUCTURE_RUNWAYCROSS && GetTileID(iXB, iY) == TILE_INFRASTRUCTURE_RUNWAYCROSS)
		return true;
	return false;
}

static coords_w_t SetTileCoords(int iPart) {
	coords_w_t val;
	switch (wViewRotation) {
		case VIEWROTATION_EAST:
			val.x = (iPart == 0) ? MAP_EDGE_MIN : MAP_EDGE_MAX;
			val.y = MAP_EDGE_MIN;
			break;
		case VIEWROTATION_SOUTH:
			val.x = MAP_EDGE_MIN;
			val.y = (iPart == 0) ? MAP_EDGE_MAX : MAP_EDGE_MIN;
			break;
		case VIEWROTATION_WEST:
			val.x = (iPart == 0) ? MAP_EDGE_MAX : MAP_EDGE_MIN;
			val.y = MAP_EDGE_MAX;
			break;
		default:
			val.x = MAP_EDGE_MAX;
			val.y = (iPart == 0) ? MAP_EDGE_MIN : MAP_EDGE_MAX;
	}

	return val;
}

static coords_w_t SetRandomPointCoords() {
	coords_w_t val;
	__int16 iRandPos = Game_RandomWordLCGMod(MAP_EDGE_MAX);
	switch (wViewRotation) {
	case VIEWROTATION_EAST:
		val.x = iRandPos;
		val.y = MAP_EDGE_MIN;
		break;
	case VIEWROTATION_SOUTH:
		val.x = MAP_EDGE_MIN;
		val.y = iRandPos;
		break;
	case VIEWROTATION_WEST:
		val.x = iRandPos;
		val.y = MAP_EDGE_MAX;
		break;
	default:
		val.x = MAP_EDGE_MAX;
		val.y = iRandPos;
	}

	return val;
}

static __int16 GetTileDepth(__int16 iPosA, __int16 iPosB, int iPlus) {
	__int16 iVal = iPosA;
	__int16 n = -1;
	__int16 iBaseLevel = ALTMReturnLandAltitude(iPosA, iPosB);
	while (1) {
		n++;
		if (n >= 5)
			break;
		if (iPlus) {
			if (GetTerrainTileID(iPosA + n, iPosB) || ALTMReturnLandAltitude(iPosA + n, iPosB) > iBaseLevel) {
				n = 0;
				break;
			}
		}
		else {
			if (GetTerrainTileID(iPosA - n, iPosB) || ALTMReturnLandAltitude(iPosA - n, iPosB) > iBaseLevel) {
				n = 0;
				break;
			}
		}
	}
	if (iPlus)
		iVal += n;
	else
		iVal -= n;
	return iVal;
}

static __int16 GetTileLength(__int16 iPosA, __int16 iPosB, int iPlus) {
	__int16 iVal = iPosB;
	__int16 n = -1;
	__int16 iRandMaxLength = Game_RandomWordLCGMod(6);
	if (iRandMaxLength < 3)
		iRandMaxLength = 3;
	__int16 iBaseLevel = ALTMReturnLandAltitude(iPosA, iPosB);
	while (1) {
		n++;
		if (n >= iRandMaxLength)
			break;
		if (iPlus) {
			if (GetTerrainTileID(iPosA, iPosB + n) || ALTMReturnLandAltitude(iPosA, iPosB + n) > iBaseLevel) {
				n = 0;
				break;
			}
		}
		else {
			if (GetTerrainTileID(iPosA, iPosB - n) || ALTMReturnLandAltitude(iPosA, iPosB - n) > iBaseLevel) {
				n = 0;
				break;
			}
		}
	}
	if (n != iRandMaxLength)
		return -1;
	if (iPlus)
		iVal += n;
	else
		iVal -= n;
	return iVal;
}

static __int16 GetStartPoint(coords_w_t *iCoords) {
	if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST)
		return iCoords->x;
	else
		return iCoords->y;
}

static __int16 GetDepthPoint(coords_w_t *iCoords) {
	if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST)
		return iCoords->y;
	else
		return iCoords->x;
}

static int isValidWaterBody(__int16 x, __int16 y) {
	return (XBITReturnIsSaltWater(x, y) && XBITReturnIsWater(x, y)) ? 1 : 0;
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

static void MilitaryBasePlotCheck(__int16 *iVAltitudeTiles, __int16 *iVTiles, __int16 *iRXPos, __int16 *iRYPos) {
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	__int16 iRandXPos;
	__int16 iRandYPos;

	int iIterations = 24;
	iValidAltitudeTiles = 0;
	iValidTiles = 0;
	do {
		BOOL bMaxIteration = iIterations-- == 0;
		if (bMaxIteration)
			break;
		iRandXPos = Game_RandomWordLCGMod(119);
		if (iRandXPos < MAP_EDGE_MIN)
			iRandXPos = MAP_EDGE_MIN;
		else if (iRandXPos > MAP_EDGE_MAX)
			iRandXPos = MAP_EDGE_MAX - 8;
		iRandYPos = Game_RandomWordLCGMod(119);
		if (iRandYPos < MAP_EDGE_MIN)
			iRandYPos = MAP_EDGE_MIN;
		else if (iRandYPos > MAP_EDGE_MAX)
			iRandYPos = MAP_EDGE_MAX - 8;
		iValidAltitudeTiles = 0;
		iValidTiles = 0;
		__int16 iBaseLevel = ALTMReturnLandAltitude(iRandXPos, iRandYPos);
		for (__int16 iCurrXPos = iRandXPos; iCurrXPos < iRandXPos + 8; ++iCurrXPos) {
			for (__int16 iCurrYPos = iRandYPos; iCurrYPos < iRandYPos + 8; ++iCurrYPos) {
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
	*iRXPos = iRandXPos;
	*iRYPos = iRandYPos;
}

static int MilitaryBaseMissileSilos(int iValidAltitudeTiles, int iValidTiles, bool force) {
	__int16 iRandXPos, iRandYPos;
	__int16 iLengthWays, iDepthWays;
	int iSiloIdx;
	__int16 iCurrLengthPos;
	__int16 iCurrDepthPos;
	coords_w_t wSiloPos[6];
	
	if (iValidAltitudeTiles < 40 || force) {
		int iSiloAttempt = 0;
		int iValidPositions = 0;
		if (iValidTiles < 40 || force) {
RETRYFROMBEGINNING:
			int iRetrySiloPos = 0;
RETRYFROMCURRENT:
			int iIterations = 24;
			if (iSiloAttempt == 0)
				iValidPositions = 0;
			do {
				BOOL bMaxIteration = iIterations-- == 0;
				if (bMaxIteration)
					break;
				iRandXPos = Game_RandomWordLCGMod(GAME_MAP_SIZE-4);
				iRandYPos = Game_RandomWordLCGMod(GAME_MAP_SIZE-4);
				if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST) {
					iLengthWays = iRandYPos;
					iDepthWays = iRandXPos;
				}
				else {
					iLengthWays = iRandXPos;
					iDepthWays = iRandYPos;
				}
				__int16 iBaseLevel = ALTMReturnLandAltitude(iLengthWays, iDepthWays);
				if (!force) {
					if (iBaseLevel <= 5) {
						if (iRetrySiloPos < MILITARY_RETRY_SILO_REPOSIT) {
							iRetrySiloPos++;
							goto RETRYFROMCURRENT;
						}
						else
							return -1;
					}
				}
				__int16 iTileArea = 0;
				for (iCurrLengthPos = iLengthWays; iCurrLengthPos < iLengthWays + 3; ++iCurrLengthPos) {
					for (iCurrDepthPos = iDepthWays; iCurrDepthPos < iDepthWays + 3; ++iCurrDepthPos) {
						if (GetTileID(iCurrLengthPos, iCurrDepthPos) < TILE_SMALLPARK &&
							!GetTerrainTileID(iCurrLengthPos, iCurrDepthPos) &&
							(iCurrLengthPos >= GAME_MAP_SIZE || iCurrDepthPos >= GAME_MAP_SIZE || !XBITReturnIsWater(iCurrLengthPos, iCurrDepthPos)) &&
							ALTMReturnLandAltitude(iCurrLengthPos, iCurrDepthPos) == iBaseLevel &&
							XZONReturnZone(iCurrLengthPos, iCurrDepthPos) != ZONE_MILITARY &&
							!GetUndergroundTileID(iCurrLengthPos, iCurrDepthPos)) {
							if (IsValidSiloPosCheck(iCurrLengthPos, iCurrDepthPos)) {
								if (!CheckForOverlappingSiloPositions(wSiloPos, iValidPositions, iCurrLengthPos, iCurrDepthPos)) {
									if (iTileArea >= 9)
										break;
									++iTileArea;
								}
								else {
									if (iRetrySiloPos < MILITARY_RETRY_SILO_REPOSIT) {
										iRetrySiloPos++;
										//ConsoleLog(LOG_DEBUG, "(VT: %d) Overlapping case found, trying again (%d).\n", iValidPositions, iRetrySiloPos);
										goto RETRYFROMCURRENT;
									}
								}
							}
							else {
								if (iRetrySiloPos < MILITARY_RETRY_SILO_REPOSIT) {
									iRetrySiloPos++;
									//ConsoleLog(LOG_DEBUG, "(VT: %d) Invalid case found, trying again (%d).\n", iValidPositions, iRetrySiloPos);
									goto RETRYFROMCURRENT;
								}
							}
						}
					}
				}
				if (iTileArea == 9) {
					iSiloIdx = iValidPositions++;
					wSiloPos[iSiloIdx].x = iLengthWays;
					wSiloPos[iSiloIdx].y = iDepthWays;
					//ConsoleLog(LOG_DEBUG, "DBG: iTileArea == 9: (%d) (%u, %d)\n", iSiloIdx, wSiloPos[iSiloIdx].x, wSiloPos[iSiloIdx].y);
				}
			} while (iValidPositions < 6);
		}
		if (iValidPositions == 6) {
			__int16 iSiloStartXPos = 0;
			__int16 iSiloStartYPos = 0;
			bMilitaryBaseType = MILITARY_BASE_MISSILE_SILOS;
			for (iSiloIdx = 0; iSiloIdx < 6; iSiloIdx++) {
				iSiloStartXPos = wSiloPos[iSiloIdx].x;
				iSiloStartYPos = wSiloPos[iSiloIdx].y;
				for (__int16 iSiloXPos = iSiloStartXPos; iSiloXPos < iSiloStartXPos + 3; ++iSiloXPos) {
					for (__int16 iSiloYPos = iSiloStartYPos; iSiloYPos < iSiloStartYPos + 3; ++iSiloYPos) {
						BYTE iTileID = GetTileID(iSiloXPos, iSiloYPos);
						--dwTileCount[iTileID];
						if (iSiloXPos < GAME_MAP_SIZE && iSiloYPos < GAME_MAP_SIZE)
							XZONSetNewZone(iSiloXPos, iSiloYPos, ZONE_MILITARY);
						++dwMilitaryTiles[MILITARYTILE_OTHER];
					}
				}
			}
			Game_CenterOnTileCoords(iSiloStartXPos, iSiloStartYPos);
			return GameMain_AfxMessageBoxID(244, 0, -1);
		}
		else {
			if (iSiloAttempt < MILITARY_RETRY_SILO_SPOTFIND) {
				iSiloAttempt++;
				goto RETRYFROMBEGINNING;
			}
		}
	}
	// When 'force' is set to false, this will lead to the normal
	// Air Force or Army Base cases.
	return (force) ? 0 : -1;
}

static void MilitaryBasePlotPlacement(__int16 iRandXPos, __int16 iRandYPos) {
	for (__int16 iCurrXPos = iRandXPos; iCurrXPos < iRandXPos + 8; ++iCurrXPos) {
		for (__int16 iCurrYPos = iRandYPos; iCurrYPos < iRandYPos + 8; ++iCurrYPos) {
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

static int MilitaryBaseAirForce(int iValidTiles, int iValidAltitudeTiles, __int16 iRandXPos, __int16 iRandYPos) {
	if (iValidAltitudeTiles < 40)
		return 0;

	if (iValidTiles == iValidAltitudeTiles) {
		bMilitaryBaseType = MILITARY_BASE_AIR_FORCE;
		GameMain_AfxMessageBoxID(242, 0, -1);

		MilitaryBasePlotPlacement(iRandXPos, iRandYPos);

		return Game_CenterOnTileCoords(iRandXPos + 4, iRandYPos + 4);
	}

	return -1;
}

static int MilitaryBaseArmyBase(int iValidTiles, int iValidAltitudeTiles, __int16 iRandXPos, __int16 iRandYPos) {
	if (iValidTiles < 40)
		return 0;

	if (iValidTiles != iValidAltitudeTiles) {
		bMilitaryBaseType = MILITARY_BASE_ARMY;
		GameMain_AfxMessageBoxID(241, 0, -1);

		MilitaryBasePlotPlacement(iRandXPos, iRandYPos);

		// Explanation:
		// First it lays down the depth-way roads and runwaycross.
		// Second it lays down the length-way roads and runwaycross.
		// If during each attempt it fails at laying down the roadway, it will
		// check to see whether both runwaycross items are present, if they're
		// not then it'll attempt to place down the respective crossing once more
		// but from the opposite direction.
		FormArmyBaseStrip(iRandXPos + 2, iRandYPos, iRandXPos + 2, iRandYPos + 7);
		if (!FindArmyBaseCrossingDepth(iRandXPos + 2, iRandYPos, iRandYPos + 7))
			FormArmyBaseStrip(iRandXPos + 2, iRandYPos + 7, iRandXPos + 2, iRandYPos);
		FormArmyBaseStrip(iRandXPos + 5, iRandYPos, iRandXPos + 5, iRandYPos + 7);
		if (!FindArmyBaseCrossingDepth(iRandXPos + 5, iRandYPos, iRandYPos + 7))
			FormArmyBaseStrip(iRandXPos + 5, iRandYPos + 7, iRandXPos + 5, iRandYPos);
		FormArmyBaseStrip(iRandXPos, iRandYPos + 2, iRandXPos + 7, iRandYPos + 2);
		if (!FindArmyBaseCrossingLength(iRandYPos + 2, iRandXPos + 7, iRandXPos))
			FormArmyBaseStrip(iRandXPos + 7, iRandYPos + 2, iRandXPos, iRandYPos + 2);
		FormArmyBaseStrip(iRandXPos, iRandYPos + 5, iRandXPos + 7, iRandYPos + 5);
		if (!FindArmyBaseCrossingLength(iRandYPos + 5, iRandXPos + 7, iRandXPos))
			FormArmyBaseStrip(iRandXPos + 7, iRandYPos + 5, iRandXPos, iRandYPos + 5);
		return Game_CenterOnTileCoords(iRandXPos + 4, iRandYPos + 4);
	}

	return -1;
}

static coords_w_t startNavalCoords[4] = {
	{ MAP_EDGE_MAX, MAP_EDGE_MIN },
	{ MAP_EDGE_MIN, MAP_EDGE_MIN },
	{ MAP_EDGE_MIN, MAP_EDGE_MAX },
	{ MAP_EDGE_MAX, MAP_EDGE_MAX }
};

static coords_w_t distNavalCoords[4] = {
	{ -1,  0  },
	{  0,  1  },
	{  1,  0  },
	{  0, -1  }
};

static __int16 advanceX[4] = {
	-1, 0, 1, 0
};

static __int16 advanceY[4] = {
	0, 1, 0, -1
};

static void SetNewCoords(coords_w_t *pCoords, __int16 x, __int16 y) {
	pCoords->x = x;
	pCoords->y = y;
}

static int MilitaryBaseNavalYard(bool force) {
	__int16 iBaseLevel;

	coords_w_t iStartCoords, iAdvanceBy;
	coords_w_t iFinalCoords, iIntermediateCoords;
	int nCurrPos, nNextPos;
	WORD iRotOne, iRotTwo, iRotThree;
	__int16 iNextX, iNextY;
	int iNavyLandingAttempts = 0;
	
	if (bCityHasOcean) {
		if ((rand() & 1) != 0 || force) {
#if 1
			iStartCoords = startNavalCoords[wViewRotation];
			iAdvanceBy = distNavalCoords[wViewRotation];

			if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
				ConsoleLog(LOG_DEBUG, "(%u) (%d, %d) (%d, %d) | (%d, %d)\n", wViewRotation, iStartCoords.x, iStartCoords.y, iAdvanceBy.x, iAdvanceBy.y,
					SetTileCoords(0).x, SetTileCoords(0).y);

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
#else
BACKTOSPOTREROLL:
			iTileCoords[0] = SetTileCoords(0); // First Corner
			iTileCoords[1] = SetTileCoords(1); // Second Corner
			if (XBITReturnIsWater(iTileCoords[0].x, iTileCoords[0].y)) { // First Corner
				if (XBITReturnIsWater(iTileCoords[1].x, iTileCoords[1].y)) { // Second Corner
																										 // Calculate a random point along the coastal area and then use that to plot the path
																										 // towards dry land, if this fails then retry N number of attempts.
				REROLLCOASTALSPOT:
					if (iNavyLandingAttempts >= 20)
						goto NONAVY;
					coords_w_t iTempCoords = SetRandomPointCoords();
					if ((iTempCoords.x < MAP_EDGE_MIN || iTempCoords.x > MAP_EDGE_MAX) ||
						(iTempCoords.y < MAP_EDGE_MIN || iTempCoords.y > MAP_EDGE_MAX)) {
						goto NONAVY;
					}
					while (1) {
						if ((iTempCoords.x < MAP_EDGE_MIN || iTempCoords.x > MAP_EDGE_MAX) ||
							(iTempCoords.y < MAP_EDGE_MIN || iTempCoords.y > MAP_EDGE_MAX)) {
							goto NONAVY;
						}
						BYTE iTileID = GetTileID(iTempCoords.x, iTempCoords.y);
						if (!XBITReturnIsWater(iTempCoords.x, iTempCoords.y))
						{
							if (iTileID >= TILE_SMALLPARK || 
								GetTerrainTileID(iTempCoords.x, iTempCoords.y)) {
								iNavyLandingAttempts++;
								goto REROLLCOASTALSPOT;
							}
							iTileCoords[0] = iTempCoords;
							break;
						}
						if (wViewRotation == VIEWROTATION_EAST)
							iTempCoords.y += 1;
						else if (wViewRotation == VIEWROTATION_SOUTH)
							iTempCoords.x += 1;
						else if (wViewRotation == VIEWROTATION_WEST)
							iTempCoords.y -= 1;
						else
							iTempCoords.x -= 1;
					}
				}

				__int16 iStartLengthPoint = GetStartPoint(&iTileCoords[0]); // Lengthway coordinate
				__int16 iFarthestDepth = GetDepthPoint(&iTileCoords[1]); // Edge of the map out at sea.

				// Landing Zone.
				__int16 iStartDepthPoint = GetDepthPoint(&iTileCoords[0]);
				// Depth of the base from landfall to further in-land.
				__int16 iDepthPoint = GetTileDepth(iStartDepthPoint, iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 1 : 0));

				// Determine relative "left"
				__int16 iLengthPointA = GetTileLength(iDepthPoint, iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 1 : 0));
				if (iLengthPointA <= 0)
					goto BACKTOSPOTREROLL;

				// Determine relative "right"
				__int16 iLengthPointB = GetTileLength(iDepthPoint, iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 0 : 1));
				if (iLengthPointB <= 0)
					goto BACKTOSPOTREROLL;

				int iNumTiles = 0;
				int iPass = 0;
PLACENAVAL:
				// Let's avoid bases that are too small.
				if (!iPass || (iNumTiles >= 50 && iNumTiles <= 60)) {
					iBaseLevel = ALTMReturnLandAltitude(iStartLengthPoint, iStartDepthPoint);
					for (__int16 iLengthWay = iLengthPointA;;) {
						if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) {
							if (iLengthWay <= iLengthPointB)
								break;
						}
						else {
							if (iLengthWay >= iLengthPointB)
								break;
						}

						int iNonContiguousDepth = 0;
						for (__int16 iDepthWay = iDepthPoint;;) {
							if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) {
								if (iDepthWay <= iFarthestDepth)
									break;
							}
							else {
								if (iDepthWay >= iFarthestDepth)
									break;
							}

							__int16 iDirectionOne, iDirectionTwo;
							if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST) {
								iDirectionOne = iLengthWay;
								iDirectionTwo = iDepthWay;
							}
							else {

								iDirectionOne = iDepthWay;
								iDirectionTwo = iLengthWay;
							}

							BYTE iTileID = GetTileID(iDirectionOne, iDirectionTwo);
							if (iTileID < TILE_SMALLPARK &&
								XZONReturnZone(iDirectionOne, iDirectionTwo) == ZONE_NONE &&
								!GetTerrainTileID(iDirectionOne, iDirectionTwo) &&
								!isValidWaterBody(iDirectionOne, iDirectionTwo) &&
								!GetUndergroundTileID(iDirectionOne, iDirectionTwo) &&
								ALTMReturnLandAltitude(iDirectionOne, iDirectionTwo) == iBaseLevel) {
								if (!iNonContiguousDepth) {
									if (iPass) {
										Game_PlaceTileWithMilitaryCheck(iDirectionOne, iDirectionTwo, 0);
										XZONSetNewZone(iDirectionOne, iDirectionTwo, ZONE_MILITARY);
										--dwTileCount[iTileID];
										++dwMilitaryTiles[MILITARYTILE_OTHER];
									}
									else {
										iNumTiles++;
									}
								}
							}
							else {
								if (iTileID >= TILE_SMALLPARK ||
									GetTerrainTileID(iDirectionOne, iDirectionTwo) < SUBMERGED_00 ||
									GetTerrainTileID(iDirectionOne, iDirectionTwo) > COAST_13 ||
									GetUndergroundTileID(iDirectionOne, iDirectionTwo) ||
									ALTMReturnLandAltitude(iDirectionOne, iDirectionTwo) > iBaseLevel) {
									iNonContiguousDepth = 1;
								}
							}

							if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH)
								--iDepthWay;
							else
								++iDepthWay;
						}

						if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH)
							--iLengthWay;
						else
							++iLengthWay;
					}

					if (iPass) {
						bMilitaryBaseType = MILITARY_BASE_NAVY;
						Game_CenterOnTileCoords(iTileCoords[0].x, iTileCoords[0].y);
						return GameMain_AfxMessageBoxID(243, 0, -1);
					}
					else {
						iPass = 1;
						goto PLACENAVAL;
					}
				}
				else if (iNumTiles < 50) {
					goto BACKTOSPOTREROLL;
				}
			}
#endif
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
	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn Missile Silos?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;

	unsigned int iMilitaryBaseTries = 0;

REATTEMPT:
	int iSiloRet = MilitaryBaseMissileSilos(0, 0, true);
	if (iSiloRet <= 0) {
		if (iMilitaryBaseTries < MILITARY_RETRY_ATTEMPT_MAX) {
			iMilitaryBaseTries++;
			goto REATTEMPT;
		}
		MilitaryBaseDecline();
	}
}

void ProposeMilitaryBaseAirForceBase(void) {
	int iResult;
	__int16 iValidAltitudeTiles;
	__int16 iValidTiles;
	__int16 iRandXPos;
	__int16 iRandYPos;

	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	iResult = -1;
	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn an Air Force plot?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;
	
	unsigned int iMilitaryBaseTries = 0;

REATTEMPT:
	MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &iRandXPos, &iRandYPos);
	
	iResult = MilitaryBaseAirForce(iValidTiles, iValidAltitudeTiles, iRandXPos, iRandYPos);
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
	__int16 iRandXPos;
	__int16 iRandYPos;

	if (!Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis) || !wCityMode)
		return;

	iResult = -1;
	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want an attempt to be made to spawn an Army Base plot?", "Ominous sounds of danger...", MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES)
		return;
	
	unsigned int iMilitaryBaseTries = 0;
REATTEMPT:
	MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &iRandXPos, &iRandYPos);
	
	iResult = MilitaryBaseArmyBase(iValidTiles, iValidAltitudeTiles, iRandXPos, iRandYPos);
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
	int iResult = MilitaryBaseNavalYard(true);
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
	__int16 iRandXPos;
	__int16 iRandYPos;
	
	unsigned int iMilitaryBaseTries = 0;

	if (GameMain_AfxMessageBoxID(240, MB_YESNO, -1) == IDNO)
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	else {
	REATTEMPT:
		if (MilitaryBaseNavalYard(false) < 0) {
			MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &iRandXPos, &iRandYPos);

			iResult = MilitaryBaseMissileSilos(iValidAltitudeTiles, iValidTiles, false);
			if (iResult <= 0) {
				if (iResult < 0)
					iResult = MilitaryBaseAirForce(iValidTiles, iValidAltitudeTiles, iRandXPos, iRandYPos);
				if (iResult <= 0) {
					if (iResult < 0)
						iResult = MilitaryBaseArmyBase(iValidTiles, iValidAltitudeTiles, iRandXPos, iRandYPos);
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
