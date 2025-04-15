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

#define MILITARY_DEBUG_PLACEMENT 1
#define MILITARY_DEBUG_PLACEMENT_NAVAL 2

#define MILITARY_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MILITARY_DEBUG
#define MILITARY_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT military_debug = MILITARY_DEBUG;

static DWORD dwDummy;

extern "C" __int16 __cdecl Hook_PlaceTileWithMilitaryCheck(__int16 x, __int16 y, __int16 iTileID) {
#if 0
	int result;
	BYTE iCurrentBuilding;
	BYTE *pCurrentBuilding;

	result = iTileID;
	if ( x < 0x80 && y < 0x80 ) {
		pCurrentBuilding = &dwMapXBLD[x]->iTileID[y];
		iCurrentBuilding = *pCurrentBuilding;
		if ( dwMapXZON[x]->b[y].iZoneType != ZONE_MILITARY ) {
			--dwTileCount[iCurrentBuilding];
			++dwTileCount[iTileID];
			*pCurrentBuilding = (BYTE)iTileID;
			return result;
		}
		if ( iCurrentBuilding < TILE_MILITARY_F15B ) {
			if ( iCurrentBuilding < TILE_MILITARY_CONTROLTOWER ) {
				if ( iCurrentBuilding < TILE_INFRASTRUCTURE_RUNWAYCROSS ) {
					if ( iCurrentBuilding == TILE_INFRASTRUCTURE_RUNWAY ) {
						--dwMilitaryTiles[1];
						goto GOCHECKCURRENTTILE;
					}
				}
				else {
					if ( iCurrentBuilding <= TILE_INFRASTRUCTURE_RUNWAYCROSS ) {
						--dwMilitaryTiles[2];
						goto GOCHECKCURRENTTILE;
					}
					if ( iCurrentBuilding == TILE_INFRASTRUCTURE_CRANE ) {
						--dwMilitaryTiles[10];
						goto GOCHECKCURRENTTILE;
					}
				}
			}
			else {
				if ( iCurrentBuilding <= TILE_MILITARY_CONTROLTOWER ) {
					--dwMilitaryTiles[11];
					goto GOCHECKCURRENTTILE;
				}
				if ( iCurrentBuilding < TILE_INFRASTRUCTURE_BUILDING1 ) {
					--dwMilitaryTiles[6];
					goto GOCHECKCURRENTTILE;
				}
				if ( iCurrentBuilding <= TILE_INFRASTRUCTURE_BUILDING1 ) {
					--dwMilitaryTiles[7];
					goto GOCHECKCURRENTTILE;
				}
				if ( iCurrentBuilding == TILE_INFRASTRUCTURE_BUILDING2 ) {
					--dwMilitaryTiles[8];
					goto GOCHECKCURRENTTILE;
				}
			}
		}
		else {
			if ( iCurrentBuilding <= TILE_MILITARY_F15B ) {
				--dwMilitaryTiles[12];
				goto GOCHECKCURRENTTILE;
			}
			if ( iCurrentBuilding < TILE_MILITARY_TOPSECRET ) {
				if ( iCurrentBuilding < TILE_MILITARY_RADAR ) {
					if ( iCurrentBuilding == TILE_MILITARY_HANGAR1 ) {
						--dwMilitaryTiles[13];
						goto GOCHECKCURRENTTILE;
					}
				}
				else {
					if ( iCurrentBuilding <= TILE_MILITARY_RADAR ) {
						--dwMilitaryTiles[5];
						goto GOCHECKCURRENTTILE;
					}
					if ( iCurrentBuilding == TILE_MILITARY_PARKINGLOT ) {
						--dwMilitaryTiles[3];
						goto GOCHECKCURRENTTILE;
					}
				}
			}
			else {
				if ( iCurrentBuilding <= TILE_MILITARY_TOPSECRET ) {
					--dwMilitaryTiles[9];
					goto GOCHECKCURRENTTILE;
				}
				if ( iCurrentBuilding < TILE_INFRASTRUCTURE_HANGAR2 ) {
					if ( iCurrentBuilding == TILE_INFRASTRUCTURE_CARGOYARD ) {
						--dwMilitaryTiles[4];
						goto GOCHECKCURRENTTILE;
					}
				}
				else {
					if ( iCurrentBuilding <= TILE_INFRASTRUCTURE_HANGAR2 ) {
						--dwMilitaryTiles[14];
						goto GOCHECKCURRENTTILE;
					}
					if ( iCurrentBuilding == TILE_MILITARY_MISSILESILO ) {
						--dwMilitaryTiles[15];
						goto GOCHECKCURRENTTILE;
					}
				}
			}
		}
		--*dwMilitaryTiles;
	GOCHECKCURRENTTILE:
		if ( iTileID < TILE_MILITARY_F15B ) {
			if ( iTileID < TILE_MILITARY_CONTROLTOWER ) {
				if ( iTileID < TILE_INFRASTRUCTURE_RUNWAYCROSS ) {
					if ( iTileID != TILE_INFRASTRUCTURE_RUNWAY ) {
					GOBACKCHECKTILE:
						++*dwMilitaryTiles;
						goto GOFORWARDGETOUT;
					}
					++dwMilitaryTiles[1];
				}
				else if ( iTileID <= TILE_INFRASTRUCTURE_RUNWAYCROSS ) {
					++dwMilitaryTiles[2];
				}
				else {
					if ( iTileID != TILE_INFRASTRUCTURE_CRANE )
						goto GOBACKCHECKTILE;
					++dwMilitaryTiles[10];
				}
			}
			else if ( iTileID <= TILE_MILITARY_CONTROLTOWER ) {
				++dwMilitaryTiles[11];
			}
			else if ( iTileID < TILE_INFRASTRUCTURE_BUILDING1 ) {
				++dwMilitaryTiles[6];
			}
			else if ( iTileID <= TILE_INFRASTRUCTURE_BUILDING1 ) {
				++dwMilitaryTiles[7];
			}
			else {
				if ( iTileID != TILE_INFRASTRUCTURE_BUILDING2 )
					goto GOBACKCHECKTILE;
				++dwMilitaryTiles[8];
			}
		}
		else if ( iTileID <= TILE_MILITARY_F15B ) {
			++dwMilitaryTiles[12];
		}
		else if ( iTileID < TILE_MILITARY_TOPSECRET ) {
			if ( iTileID < TILE_MILITARY_RADAR ) {
				if ( iTileID != TILE_MILITARY_HANGAR1 )
					goto GOBACKCHECKTILE;
				++dwMilitaryTiles[13];
			}
			else if ( iTileID <= TILE_MILITARY_RADAR ) {
				++dwMilitaryTiles[5];
			}
			else {
				if ( iTileID != TILE_MILITARY_PARKINGLOT )
					goto GOBACKCHECKTILE;
				++dwMilitaryTiles[3];
			}
		}
		else if ( iTileID <= TILE_MILITARY_TOPSECRET ) {
			++dwMilitaryTiles[9];
		}
		else if ( iTileID < TILE_INFRASTRUCTURE_HANGAR2 ) {
			if ( iTileID != TILE_INFRASTRUCTURE_CARGOYARD )
				goto GOBACKCHECKTILE;
			++dwMilitaryTiles[4];
		}
		else if ( iTileID <= TILE_INFRASTRUCTURE_HANGAR2 ) {
			++dwMilitaryTiles[14];
		}
		else {
			if ( iTileID != TILE_MILITARY_MISSILESILO )
				goto GOBACKCHECKTILE;
			++dwMilitaryTiles[15];
		}
	GOFORWARDGETOUT:
		*pCurrentBuilding = (BYTE)iTileID;
	}
	return result;
#else
	__int16(__cdecl *H_PlaceTileWithMilitaryCheck)(__int16, __int16, __int16) = (__int16(__cdecl *)(__int16, __int16, __int16))0x441F00;

	__int16 ret = H_PlaceTileWithMilitaryCheck(x, y, iTileID);

	return ret;
#endif
}

// This function has been replicated from he equivalent that was found
// in the DOS version of the game.
static void FormArmyBaseStrip(int x1, int y1, __int16 x2, __int16 y2) {
	WORD wOldToolGroup;
	__int16 iX;
	__int16 iY;
	int iNewX;
	int iNewY;

	wOldToolGroup = wMaybeActiveToolGroup;
	iX = x2;
	iY = y2;
	wMaybeActiveToolGroup = TOOL_GROUP_ROADS;
	if (Game_MaybeCheckViablePlacementPath(x1, y1, x2, y2)) {
		iNewX = x1;
		iX = x1;
		iNewY = y1;
		iY = y1;
		while (Game_MaybeRoadViabilityAlongPath((__int16 *)&iNewX, (__int16 *)&iNewY)) {
			dwMapXZON[iX]->b[iY].iZoneType = ZONE_MILITARY;
			Game_CheckAndAdjustTransportTerrain(iX, iY);
			Game_PlaceRoadAtCoordinates(iX, iY);
			iX = iNewX;
			iY = iNewY;
		}
		dwMapXZON[iX]->b[iY].iZoneType = ZONE_MILITARY;
		Game_CheckAndAdjustTransportTerrain(iX, iY);
		Game_PlaceRoadAtCoordinates(iX, iY);
	}
	if (GetTileID(x1, y1) == TILE_ROAD_LR || GetTileID(x1, y1) == TILE_ROAD_TB) {
		// TERRAIN_00 check added here to avoid the runwaycross
		// being placed into a dip (likely replacing a slope or granite block).
		if (!dwMapXTER[x1]->iTileID[y1]) {
			dwMapXZON[x1]->b[y1].iCorners = 0xF;
			Game_PlaceTileWithMilitaryCheck(x1, y1, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
	}
	if (GetTileID(iX, iY) == TILE_ROAD_LR || GetTileID(iX, iY) == TILE_ROAD_TB) {
		// TERRAIN_00 check added here to avoid the runwaycross
		// being placed into a dip (likely replacing a slope or granite block).
		if (!dwMapXTER[iX]->iTileID[iY]) {
			dwMapXZON[iX]->b[iY].iCorners = 0xF;
			Game_PlaceTileWithMilitaryCheck(iX, iY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
	}
	wMaybeActiveToolGroup = wOldToolGroup;
	Game_GetLastViewRotation();
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

static int SetTileCoords(int iPart) {
	int val;
	switch (wViewRotation) {
		case VIEWROTATION_EAST:
			P_HIWORD(val) = (iPart == 0) ? 0 : 127;
			P_LOWORD(val) = 0;
			break;
		case VIEWROTATION_SOUTH:
			P_HIWORD(val) = 0;
			P_LOWORD(val) = (iPart == 0) ? 127 : 0;
			break;
		case VIEWROTATION_WEST:
			P_HIWORD(val) = (iPart == 0) ? 127 : 0;
			P_LOWORD(val) = 127;
			break;
		default:
			P_HIWORD(val) = 127;
			P_LOWORD(val) = (iPart == 0) ? 0 : 127;
	}

	return val;
}

static int SetRandomPointCoords() {
	int val;
	switch (wViewRotation) {
	case VIEWROTATION_EAST:
		P_HIWORD(val) = rand() & 127;
		P_LOWORD(val) = 0;
		break;
	case VIEWROTATION_SOUTH:
		P_HIWORD(val) = 0;
		P_LOWORD(val) = rand() & 127;
		break;
	case VIEWROTATION_WEST:
		P_HIWORD(val) = rand() & 127;
		P_LOWORD(val) = 127;
		break;
	default:
		P_HIWORD(val) = 127;
		P_LOWORD(val) = rand() & 127;
	}

	return val;
}

static __int16 GetTileDepth(__int16 iPosA, __int16 iPosB, int iPlus) {
	__int16 iVal = iPosA;
	__int16 n = -1;
	int iBaseLevel = dwMapALTM[iPosA]->w[iPosB].iLandAltitude;
	while (1) {
		n++;
		if (n >= 4)
			break;
		if (iPlus) {
			if (dwMapXTER[iPosA + n]->iTileID[iPosB] || dwMapALTM[iPosA + n]->w[iPosB].iLandAltitude != iBaseLevel) {
				n = 0;
				break;
			}
		}
		else {
			if (dwMapXTER[iPosA - n]->iTileID[iPosB] || dwMapALTM[iPosA - n]->w[iPosB].iLandAltitude != iBaseLevel) {
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
	int iBaseLevel = dwMapALTM[iPosA]->w[iPosB].iLandAltitude;
	while (1) {
		n++;
		if (n >= 6)
			break;
		if (iPlus) {
			if (dwMapXTER[iPosA]->iTileID[iPosB + n] || dwMapALTM[iPosA]->w[iPosB + n].iLandAltitude != iBaseLevel) {
				n = 0;
				break;
			}
		}
		else {
			if (dwMapXTER[iPosA]->iTileID[iPosB - n] || dwMapALTM[iPosA]->w[iPosB - n].iLandAltitude != iBaseLevel) {
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

static __int16 GetNearCoord(int iCoords) {
	return P_SHIWORD(iCoords);
}

static __int16 GetFarCoord(int iCoords) {
	return (__int16)iCoords;
}

static __int16 GetStartPoint(int iCoords) {
	if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST)
		return GetNearCoord(iCoords);
	else
		return GetFarCoord(iCoords);
}

static __int16 GetDepthPoint(int iCoords) {
	if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST)
		return GetFarCoord(iCoords);
	else
		return GetNearCoord(iCoords);
}

static int isValidSiloPos(__int16 m_x, __int16 m_y, __int16 iTileArea) {
	__int16 x;
	__int16 y;
	__int16 iArea;
	__int16 iX;
	__int16 iY;
	__int16 iItemWidth;
	__int16 iItemLength;
	BYTE iBuilding;

	x = m_x;
	if (x > 1)
		x--;
	y = m_y;
	if (y > 1)
		y--;

	iArea = iTileArea;
	if (iArea > 1) {
		--x;
		--y;
	}
	iX = x;
	iItemWidth = x + iArea;
	if (iItemWidth >= x) {
		iItemLength = iArea + y;
		while (1) {
			iY = y;
			if (iItemLength >= y)
				break;
		GOBACK:
			if (++iX > iItemWidth)
				goto GOFORWARD;
		}
		while (1) {
			if (iArea <= 0) {
				if (iX >= 0x80 || iY >= 0x80)
					return 0;
			}
			else if (iX < 1 || iY < 1 || iX > 126 || iY > 126) {
				return 0;
			}
			iBuilding = dwMapXBLD[iX]->iTileID[iY];
			if (iBuilding >= TILE_ROAD_LR) {
				return 0;
			}
			if (iBuilding == TILE_RADIOACTIVITY) {
				return 0;
			}
			if (iBuilding == TILE_SMALLPARK) {
				return 0;
			}
			if (dwMapXZON[iX]->b[iY].iZoneType != ZONE_NONE) {
				return 0;
			}
			if (dwMapXTER[iX]->iTileID[iY]) {
				return 0;
			}
			if (dwMapXUND[iX]->iTileID[iY]) {
				return 0;
			}
			if (iX < 0x80 &&
				iY < 0x80 &&
				dwMapXBIT[iX]->b[iY].iWater != 0) {
				return 0;
			}
			if (++iY > iItemLength) {
				goto GOBACK;
			}
		}
	}
GOFORWARD:
	return 1;
}

static int CheckOverlappingSiloPosition(__int16 x1, __int16 y1, int x2, int y2) {
	int i1, j1, i2, j2;

	for (i1 = 0; i1 < 3; i1++) {
		__int16 xPos1 = x1 + i1;
		if (xPos1 > 127)
			return 1;
		for (j1 = 0; j1 < 3; j1++) {
			__int16 yPos1 = y1 + j1;
			if (yPos1 > 127)
				return 1;
			for (i2 = 0; i2 < 3; i2++) {
				int xPos2 = x2 + i2;
				if (xPos2 > 127)
					return 1;
				for (j2 = 0; j2 < 3; j2++) {
					int yPos2 = y2 + j2;
					if (yPos2 > 127)
						return 1;
					if (xPos1 == xPos2 || (xPos1 >= xPos2 - 1 && xPos1 <= xPos2 + 4)) {
						if (yPos1 == yPos2 || (yPos1 >= yPos2 - 1 && yPos1 <= yPos2 + 4)) {
							//ConsoleLog(LOG_DEBUG, "CheckOverlappingSiloPosition (X-CHECK) (%d, %d, %d, %d) (%d, %d, %d, %d) xPos1(%d), yPos1(%d), xPos2(%d), yPos2(%d)\n", x1, y1, x2, y2, i1, j1, i2, j2, xPos1, yPos1, xPos2, yPos2);
							return 1;
						}
					}
					if (yPos1 == yPos2 || (yPos1 >= yPos2 - 1 && yPos1 <= yPos2 + 4)) {
						if (xPos1 == xPos2 || (xPos1 >= xPos2 - 1 && xPos1 <= xPos2 + 4)) {
							//ConsoleLog(LOG_DEBUG, "CheckOverlappingSiloPosition (Y-CHECK) (%d, %d, %d, %d) (%d, %d, %d, %d) xPos1(%d), yPos1(%d), xPos2(%d), yPos2(%d)\n", x1, y1, x2, y2, i1, j1, i2, j2, xPos1, yPos1, xPos2, yPos2);
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

static int CheckForOverlappingSiloPositions(DWORD *wSiloPos, int iPos, __int16 x1, __int16 y1) {
	int i;
	int x2, y2;

	for (i = 0; i < iPos; i++) {
		x2 = wSiloPos[2 * i];
		y2 = wSiloPos[2 * i + 1];
		if (CheckOverlappingSiloPosition(x1, y1, x2, y2))
			return 1;
		//ConsoleLog(LOG_DEBUG, "CheckForOverlappingSiloPositions(): (%d/%d) X/Y1(%d,%d), X/Y2(%d,%d)\n", i+1, iPos, x1, y1, x2, y2);
	}
	//ConsoleLog(LOG_DEBUG, "CheckForOverlappingSiloPositions(NO OVERLAP): X/Y1(%d,%d)\n", x1, y1);
	return 0;
}

static void MilitaryBasePlotCheck(__int16 *iVAltitudeTiles, __int16 *iVTiles, int *iRXPos, __int16 *iRYPos, int *iRStoredXPos, int *iRStoredYPos) {
	__int16 iValidAltitudeTiles;
	int iRandXPos;
	__int16 iRandYPos;
	__int16 iXPosStep;
	__int16 iValidTiles;
	int iRandStoredYPos;

	int iIterations = 24;
	iValidAltitudeTiles = 0;
	int iRandStoredXPos = 0;
	do {
		BOOL bMaxIteration = iIterations-- == 0;
		if (bMaxIteration)
			break;
		iRandXPos = Game_RandomWordLCGMod(119);
		iRandYPos = Game_RandomWordLCGMod(119);
		iXPosStep = iRandXPos;
		iValidTiles = 0;
		iValidAltitudeTiles = 0;
		iRandStoredYPos = iRandYPos;
		__int16 iBaseLevel = dwMapALTM[iRandXPos]->w[iRandYPos].iLandAltitude;
		for (iRandStoredXPos = iRandXPos + 8; iXPosStep < (__int16)iRandStoredXPos; ++iXPosStep) {
			for (__int16 i = iRandYPos; iRandYPos + 8 > i; ++i) {
				if (
					dwMapXBLD[iXPosStep]->iTileID[i] < TILE_SMALLPARK &&
					!dwMapXTER[iXPosStep]->iTileID[i] &&
					(
						iXPosStep >= 0x80 || // (Not present in the DOS-equivalent)
						i >= 0x80 || // (Not present in the DOS-equivalent)
						dwMapXBIT[iXPosStep]->b[i].iWater == 0
						) &&
					dwMapXZON[iXPosStep]->b[i].iZoneType == ZONE_NONE
					) {
					++iValidTiles;
					if (dwMapALTM[iXPosStep]->w[i].iLandAltitude == iBaseLevel)
						++iValidAltitudeTiles;
				}
			}
		}
	} while (iValidTiles < 40);

	*iVAltitudeTiles = iValidAltitudeTiles;
	*iVTiles = iValidTiles;
	*iRXPos = iRandXPos;
	*iRYPos = iRandYPos;
	*iRStoredXPos = iRandStoredXPos;
	*iRStoredYPos = iRandStoredYPos;
}

static int MilitaryBaseMissileSilos(int iValidAltitudeTiles, int iValidTiles, bool force) {
	int iRandXPos, iRandYPos;
	int iSiloIdx;
	__int16 i, j;
	WORD uArrPos;
	DWORD dwSiloPos[12];
	
	int iVAltitudeTiles = iValidAltitudeTiles;
	int iVTiles = iValidTiles;
	if (force) {
		if (iVAltitudeTiles >= 40)
			iVAltitudeTiles = 39;
		if (iVTiles >= 40)
			iVTiles = 39;
	}
	if (iVAltitudeTiles < 40) {
		int iSiloAttempt = 0;
		if (iVTiles < 40) {
RETRY_CHECK2:
			int iRetrySiloPos = 0;
RETRY_CHECK1:
			int iIterations = 24;
			if (iSiloAttempt == 0)
				iVTiles = 0;
			do {
				BOOL bMaxIteration = iIterations-- == 0;
				if (bMaxIteration)
					break;
				int iDirectionOne;
				int iDirectionTwo;
				WORD uAltPosOne;
				WORD uAltPosTwo;
				iRandXPos = Game_RandomWordLCGMod(124);
				iRandYPos = Game_RandomWordLCGMod(124);
				iDirectionOne = iRandXPos;
				iDirectionTwo = iRandYPos;
				if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_WEST) {
					uAltPosOne = iDirectionTwo;
					uAltPosTwo = iDirectionOne;
				}
				else {
					uAltPosOne = iDirectionOne;
					uAltPosTwo = iDirectionTwo;
				}
				uArrPos = uAltPosOne;
				__int16 iBaseLevel = dwMapALTM[uAltPosOne]->w[uAltPosTwo].iLandAltitude;
				for (i = 0; (__int16)uArrPos < uAltPosOne + 3; ++uArrPos) {
					for (j = uAltPosTwo; uAltPosTwo + 3 > j; ++j) {
						__int16 iLengthWays = (__int16)uArrPos;
						__int16 iDepthWays = j;
						if (
							dwMapXBLD[iLengthWays]->iTileID[iDepthWays] < TILE_SMALLPARK &&
							!dwMapXTER[iLengthWays]->iTileID[iDepthWays] &&
							(
								iLengthWays >= 0x80 ||
								iDepthWays >= 0x80 ||
								dwMapXBIT[iLengthWays]->b[iDepthWays].iWater == 0
								) &&
							dwMapALTM[iLengthWays]->w[iDepthWays].iLandAltitude == iBaseLevel &&
							dwMapXZON[iLengthWays]->b[iDepthWays].iZoneType != ZONE_MILITARY &&
							!dwMapXUND[uAltPosOne]->iTileID[uAltPosTwo]
							) {
							if (isValidSiloPos(iLengthWays, iDepthWays, 3)) {
								if (!CheckForOverlappingSiloPositions(dwSiloPos, iVTiles, iLengthWays, iDepthWays)) {
									if (i >= 9)
										break;
									++i;
								}
								else {
									if ((iRetrySiloPos < 32/* && iVTiles < 5*/)/* || (iRetrySiloPos < 64 && iVTiles == 5)*/) {
										iRetrySiloPos++;
										//ConsoleLog(LOG_DEBUG, "(VT: %d) Overlapping case found, trying again (%d).\n", iVTiles, iRetrySiloPos);
										goto RETRY_CHECK1;
									}
								}
							}
							else {
								if ((iRetrySiloPos < 32/* && iVTiles < 5*/)/* || (iRetrySiloPos < 64 && iVTiles == 5)*/) {
									iRetrySiloPos++;
									//ConsoleLog(LOG_DEBUG, "(VT: %d) Invalid case found, trying again (%d).\n", iVTiles, iRetrySiloPos);
									goto RETRY_CHECK1;
								}
							}
						}
					}
				}
				if (i == 9) {
					iSiloIdx = iVTiles++;
					dwSiloPos[2 * iSiloIdx] = uAltPosOne;
					dwSiloPos[2 * iSiloIdx + 1] = uAltPosTwo;
					//ConsoleLog(LOG_DEBUG, "DBG: i==9: (%d) (%u, %d)\n", iSiloIdx, dwSiloPos[2 * iSiloIdx], dwSiloPos[2 * iSiloIdx + 1]);
				}
			} while (iVTiles < 6);
		}
		if (iVTiles != 6) {
			if (iSiloAttempt < 20) {
				iSiloAttempt++;
				goto RETRY_CHECK2;
			}
		}
		if (iVTiles == 6) {
			__int16 iSiloStartXPos = 0;
			__int16 iSiloStartYPos = 0;
			bMilitaryBaseType = MILITARY_BASE_MISSILE_SILOS;
			for (i = 0; i < 6; i++) {
				__int16 iSiloXPos = LOWORD(dwSiloPos[2 * i]);
				iSiloStartXPos = iSiloXPos;
				__int16 iSiloYPos;
				for (iSiloStartYPos = LOWORD(dwSiloPos[2 * i + 1]); iSiloStartXPos + 3 > iSiloXPos; iSiloXPos = iSiloXPos + 1) {
					for (iSiloYPos = iSiloStartYPos; iSiloStartYPos + 3 > iSiloYPos; ++*dwMilitaryTiles) {
						BYTE iBuildingArea = dwMapXBLD[iSiloXPos]->iTileID[iSiloYPos];
						--dwTileCount[iBuildingArea];
						if (iSiloXPos < 0x80 && iSiloYPos < 0x80) {
							dwMapXZON[iSiloXPos]->b[iSiloYPos].iZoneType = ZONE_MILITARY;
							dwMapXZON[iSiloXPos]->b[iSiloYPos].iCorners = 0xF0;
						}
						++iSiloYPos;
					}
				}
			}
			Game_CenterOnTileCoords(iSiloStartXPos, iSiloStartYPos);
			return Game_AfxMessageBox(244, 0, -1);
		}
		else
			return 0;
	}
	// When 'force' is set to false, this will lead to the normal
	// Air Force or Army Base cases.
	return (force) ? 0 : -1;
}

static int MilitaryBaseDecline(bool bMsgbox_always) {
	if (!bMsgbox_always) {
		if (bMilitaryBaseType == MILITARY_BASE_DECLINED) {
			ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> MilitaryBaseDecline() - IDNO\n", _ReturnAddress());
			return IDNO;
		}
	}

	int iRes = Game_AfxMessageBox(411, 0, -1);
	bMilitaryBaseType = MILITARY_BASE_DECLINED;
	ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> MilitaryBaseDecline() - (%d) (%d)\n", _ReturnAddress(), iRes, bMilitaryBaseType);
	return iRes;
}

void ProposeMilitaryBaseDecline(void) {
	ConsoleLog(LOG_DEBUG, "DBG: 0x%06X -> ProposeMilitaryBaseDecline()\n", _ReturnAddress());
	MilitaryBaseDecline(false);
}

extern "C" int __stdcall Hook_SimulationProposeMilitaryBase(void) {
	int iResult;
	__int16 iValidAltitudeTiles;
	int iRandXPos;
	__int16 iRandYPos;
	__int16 iValidTiles;
	int iRandStoredXPos;
	int iRandStoredYPos;
	__int16 iBaseLevel;
	__int16 iCurrXPos;
	__int16 iCurrYPos;

	int iTileCoords[2];
	int iNavyLandingAttempts;
	
	UINT iMilitaryBaseTries = 0;

	iResult = Game_AfxMessageBox(240, MB_YESNO, -1);
	if (iResult == IDNO) {
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	}
	else {
REATTEMPT:
		iNavyLandingAttempts = 0;
		if (bCityHasOcean) {
			iResult = rand();
			if ((iResult & 1) != 0) {
				iTileCoords[0] = SetTileCoords(0); // First Corner
				iTileCoords[1] = SetTileCoords(1); // Second Corner
				if (dwMapXBIT[GetNearCoord(iTileCoords[0])]->b[GetFarCoord(iTileCoords[0])].iSaltWater) { // First Corner
					if (dwMapXBIT[GetNearCoord(iTileCoords[1])]->b[GetFarCoord(iTileCoords[1])].iSaltWater) { // Second Corner
						// Calculate a random point along the coastal area and then use that to plot the path
						// towards dry land, if this fails then retry N number of attempts.
REROLLCOASTALSPOT:
						if (iNavyLandingAttempts >= 20)
							goto NONAVY;
						int iTempCoords = SetRandomPointCoords();
						__int16 iTempNear = GetNearCoord(iTempCoords);
						__int16 iTempFar = GetFarCoord(iTempCoords);
						if ((iTempNear < 0 || iTempNear > 127) ||
							(iTempFar < 0 || iTempFar > 127)) {
							goto NONAVY;
						}
						while (1) {
							iTempNear = GetNearCoord(iTempCoords);
							iTempFar = GetFarCoord(iTempCoords);
							if ((iTempNear < 0 || iTempNear > 127) ||
								(iTempFar < 0 || iTempFar > 127)) {
								goto NONAVY;
							}
							unsigned __int8 iMilitaryArea = dwMapXBLD[iTempNear]->iTileID[iTempFar];
							if (dwMapXBIT[iTempNear]->b[iTempFar].iWater == 0)
							{
								if (!((iMilitaryArea >= TILE_CLEAR && iMilitaryArea <= TILE_RUBBLE4) ||
									(iMilitaryArea >= TILE_TREES1 && iMilitaryArea < TILE_SMALLPARK)) || 
									dwMapXTER[iTempNear]->iTileID[iTempFar]) {
									iNavyLandingAttempts++;
									goto REROLLCOASTALSPOT;
								}
								iTileCoords[0] = iTempCoords;
								break;
							}
							if (wViewRotation == VIEWROTATION_EAST) {
								P_LOWORD(iTempCoords) += 1;
							}
							else if (wViewRotation == VIEWROTATION_SOUTH) {
								P_HIWORD(iTempCoords) += 1;
							}
							else if (wViewRotation == VIEWROTATION_WEST) {
								P_LOWORD(iTempCoords) -= 1;
							}
							else {
								P_HIWORD(iTempCoords) -= 1;
							}
						}
					}

					__int16 iStartLengthPoint = GetStartPoint(iTileCoords[0]); // Lengthway coordinate
					__int16 iFarthestDepth = GetDepthPoint(iTileCoords[1]); // Edge of the map out at sea.
					
					// Depth of the base from landfall to further in-land.
					__int16 iDepthPoint = GetTileDepth(GetDepthPoint(iTileCoords[0]), iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 1 : 0));

					// Determine relative "left"
					__int16 iLengthPointA = GetTileLength(iDepthPoint, iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 1 : 0));

					// Determine relative "right"
					__int16 iLengthPointB = GetTileLength(iDepthPoint, iStartLengthPoint, ((wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) ? 0 : 1));

					int iNumTiles = 0;
					iBaseLevel = dwMapALTM[iStartLengthPoint]->w[iDepthPoint].iLandAltitude;
					for (__int16 iLengthWay = iLengthPointA;;) {
						if (wViewRotation == VIEWROTATION_EAST || wViewRotation == VIEWROTATION_SOUTH) {
							if (iLengthWay <= iLengthPointB)
								break;
						}
						else {
							if (iLengthWay >= iLengthPointB)
								break;
						}

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

							unsigned __int8 iMilitaryArea = dwMapXBLD[iDirectionOne]->iTileID[iDirectionTwo];
							if (
								((iMilitaryArea >= TILE_CLEAR && iMilitaryArea <= TILE_RUBBLE4) ||
								(iMilitaryArea >= TILE_TREES1 && iMilitaryArea < TILE_SMALLPARK)) &&
								dwMapXZON[iDirectionOne]->b[iDirectionTwo].iZoneType == ZONE_NONE &&
								!dwMapXTER[iDirectionOne]->iTileID[iDirectionTwo] &&
								dwMapXBIT[iDirectionOne]->b[iDirectionTwo].iWater == 0 &&
								!dwMapXUND[iDirectionOne]->iTileID[iDirectionTwo] &&
								dwMapALTM[iDirectionOne]->w[iDirectionTwo].iLandAltitude == iBaseLevel
								) {
								Game_PlaceTileWithMilitaryCheck(iDirectionOne, iDirectionTwo, 0);
								dwMapXZON[iDirectionOne]->b[iDirectionTwo].iZoneType = ZONE_MILITARY;
								dwMapXZON[iDirectionOne]->b[iDirectionTwo].iCorners = 0xF0;
								--dwTileCount[iMilitaryArea];
								++*dwMilitaryTiles;
								iNumTiles++;
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

					if (iNumTiles > 0) {
						bMilitaryBaseType = MILITARY_BASE_NAVY;
						Game_CenterOnTileCoords(GetNearCoord(iTileCoords[0]), GetFarCoord(iTileCoords[0]));
						return Game_AfxMessageBox(243, 0, -1);
					}
				}
			}
		}
NONAVY:
		MilitaryBasePlotCheck(&iValidAltitudeTiles, &iValidTiles, &iRandXPos, &iRandYPos, &iRandStoredXPos, &iRandStoredYPos);

		int iSiloRet = MilitaryBaseMissileSilos(iValidAltitudeTiles, iValidTiles, false); // The 'force' attribute here should be false.
		if (iSiloRet >= 0) {
			if (iSiloRet > 0)
				return iSiloRet;
			else {
				if (iMilitaryBaseTries < 10) {
					iMilitaryBaseTries++;
					goto REATTEMPT;
				}
				iResult = MilitaryBaseDecline(true);
			}
		}
		else {
			if (iValidTiles == iValidAltitudeTiles) {
				bMilitaryBaseType = MILITARY_BASE_AIR_FORCE;
				Game_AfxMessageBox(242, 0, -1);
			}
			else {
				bMilitaryBaseType = MILITARY_BASE_ARMY;
				Game_AfxMessageBox(241, 0, -1);
			}
			for (iCurrXPos = iRandXPos; iRandXPos + 8 > iCurrXPos; ++iCurrXPos) {
				for (iCurrYPos = iRandStoredYPos; iRandStoredYPos + 8 > iCurrYPos; ++iCurrYPos) {
					unsigned __int8 iMilitaryArea = dwMapXBLD[iCurrXPos]->iTileID[iCurrYPos];
					if (
						iMilitaryArea < TILE_SMALLPARK &&
						!dwMapXTER[iCurrXPos]->iTileID[iCurrYPos] &&
						(
							iCurrXPos >= 0x80 ||
							iCurrYPos >= 0x80 ||
							dwMapXBIT[iCurrXPos]->b[iCurrYPos].iWater == 0
						) &&
						dwMapXZON[iCurrXPos]->b[iCurrYPos].iZoneType == ZONE_NONE &&
						!dwMapXUND[iRandXPos]->iTileID[iRandStoredYPos]
					) {
						--dwTileCount[iMilitaryArea];
						if (iCurrXPos < 0x80 && iCurrYPos < 0x80) {
							dwMapXZON[iCurrXPos]->b[iCurrYPos].iZoneType = ZONE_MILITARY;
							dwMapXZON[iCurrXPos]->b[iCurrYPos].iCorners = 0xF0;
						}
						++*dwMilitaryTiles;
					}
				}
			}
			if (bMilitaryBaseType == MILITARY_BASE_ARMY) {
				// Explanation:
				// First it lays down the depth-way roads and runwaycross.
				// Second it lays down the length-way roads and runwaycross.
				// If during each attempt it fails at laying down the roadway, it will
				// check to see whether both runwaycross items are present, if they're
				// not then it'll attempt to place down the respective crossing once more
				// but from the opposite direction.
				FormArmyBaseStrip(iRandXPos + 2, iRandStoredYPos, iRandXPos + 2, iRandStoredYPos + 7);
				if (!FindArmyBaseCrossingDepth(iRandXPos + 2, iRandStoredYPos, iRandStoredYPos + 7))
					FormArmyBaseStrip(iRandXPos + 2, iRandStoredYPos + 7, iRandXPos + 2, iRandStoredYPos);
				FormArmyBaseStrip(iRandXPos + 5, iRandStoredYPos, iRandXPos + 5, iRandStoredYPos + 7);
				if (!FindArmyBaseCrossingDepth(iRandXPos + 5, iRandStoredYPos, iRandStoredYPos + 7))
					FormArmyBaseStrip(iRandXPos + 5, iRandStoredYPos + 7, iRandXPos + 5, iRandStoredYPos);
				FormArmyBaseStrip(iRandXPos, iRandStoredYPos + 2, iRandXPos + 7, iRandStoredYPos + 2);
				if (!FindArmyBaseCrossingLength(iRandStoredYPos + 2, iRandXPos + 7, iRandXPos))
					FormArmyBaseStrip(iRandXPos + 7, iRandStoredYPos + 2, iRandXPos, iRandStoredYPos + 2);
				FormArmyBaseStrip(iRandXPos, iRandStoredYPos + 5, iRandXPos + 7, iRandStoredYPos + 5);
				if (!FindArmyBaseCrossingLength(iRandStoredYPos + 5, iRandXPos + 7, iRandXPos))
					FormArmyBaseStrip(iRandXPos + 7, iRandStoredYPos + 5, iRandXPos, iRandStoredYPos + 5);
			}
			return Game_CenterOnTileCoords(iRandXPos + 4, iRandStoredYPos + 4);
		}
	}
	return iResult;
}

void InstallMilitaryHooks(void) {
	VirtualProtect((LPVOID)0x40178F, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40178F, Hook_PlaceTileWithMilitaryCheck);

	// Replicate the general functionality provided from the DOS version
	// to also include the Navy and Army Base spawning.
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);
}
