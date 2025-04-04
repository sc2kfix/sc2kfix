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

// This function has been replicated from he equivalent that was found
// in the DOS version of the game.
static void FormArmyBaseGrid(int x1, int y1, __int16 x2, __int16 y2) {
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
			Game_CheckAndAdjustTransportTerrain(iX, iY);
			Game_PlaceRoadAtCoordinates(iX, iY);
			iX = iNewX;
			iY = iNewY;
		}
		Game_CheckAndAdjustTransportTerrain(iX, iY);
		Game_PlaceRoadAtCoordinates(iX, iY);
	}
	if (GetTileID(x1, y1) == TILE_ROAD_LR || GetTileID(x1, y1) == TILE_ROAD_TB) {
		// TERRAIN_00 check added here to avoid the runwaycross
		// being placed into a dip (likely replacing a slope or granite block).
		if (!dwMapXTER[x1]->iTileID[y1]) {
			dwMapXZON[x1]->b[y1].iZoneType = ZONE_MILITARY;
			dwMapXZON[x1]->b[y1].iCorners = 0x0F; // In the DOS build this is 0xF0, however that value here results in a blank area.
			Game_PlaceTileWithMilitaryCheck(x1, y1, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
	}
	if (GetTileID(iX, iY) == TILE_ROAD_LR || GetTileID(iX, iY) == TILE_ROAD_TB) {
		// TERRAIN_00 check added here to avoid the runwaycross
		// being placed into a dip (likely replacing a slope or granite block).
		if (!dwMapXTER[iX]->iTileID[iY]) {
			dwMapXZON[iX]->b[iY].iZoneType = ZONE_MILITARY;
			dwMapXZON[iX]->b[iY].iCorners = 0x0F; // In the DOS build this is 0xF0, however that value here results in a blank area.
			Game_PlaceTileWithMilitaryCheck(iX, iY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
	}
	wMaybeActiveToolGroup = wOldToolGroup;
	Game_GetLastViewRotation();
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

extern "C" int __stdcall Hook_SimulationProposeMilitaryBase(void) {
	int iResult;
	int iIterations;
	bool bMaxIteration;
	__int16 iPosCount;   // Indicative name, subject to change.
	int iRandOne[2];
	__int16 iRandTwo[2];
	unsigned __int16 uArrPos;
	__int16 iValidTiles;
	int iPosOffset;
	__int16 iBaseLevel;
	__int16 i, j;
	int iPos[2];
	int k;
	unsigned __int16 uPos[2];
	int iBuildingArea;
	DWORD dwSiloPos[12];

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
						while (1) {
							unsigned __int8 iMilitaryArea = dwMapXBLD[GetNearCoord(iTempCoords)]->iTileID[GetFarCoord(iTempCoords)];
							if (dwMapXBIT[GetNearCoord(iTempCoords)]->b[GetFarCoord(iTempCoords)].iWater == 0)
							{
								if (!((iMilitaryArea >= TILE_CLEAR && iMilitaryArea <= TILE_RUBBLE4) ||
									(iMilitaryArea >= TILE_TREES1 && iMilitaryArea < TILE_SMALLPARK)) && 
									dwMapXTER[GetNearCoord(iTempCoords)]->iTileID[GetFarCoord(iTempCoords)]) {
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
								--*((WORD *)&dwTileCount + iMilitaryArea);
								++*(WORD *)dwMilitaryTiles;
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
		iIterations = 24;
		iPosCount = (short)dwSiloPos[0];
		do {
			bMaxIteration = iIterations-- == 0;
			if (bMaxIteration)
				break;
			iRandOne[0] = Game_RandomWordLCGMod(119);
			iRandTwo[0] = Game_RandomWordLCGMod(119);
			uArrPos = iRandOne[0];
			iValidTiles = 0;
			iPosCount = 0;
			iPosOffset = iRandTwo[0];
			iBaseLevel = dwMapALTM[iRandOne[0]]->w[iRandTwo[0]].iLandAltitude;
			for (dwSiloPos[0] = iRandOne[0] + 8; (__int16)uArrPos < dwSiloPos[0]; ++uArrPos) {
				for (i = iRandTwo[0]; iRandTwo[0] + 8 > i; ++i) {
					if (
						dwMapXBLD[uArrPos]->iTileID[i] < TILE_SMALLPARK &&
						!dwMapXTER[uArrPos]->iTileID[i] &&
						(
							uArrPos >= 0x80u || // (Not present in the DOS-equivalent)
							(unsigned __int16)i >= 0x80u || // (Not present in the DOS-equivalent)
							dwMapXBIT[uArrPos]->b[i].iWater == 0
						) &&
						dwMapXZON[uArrPos]->b[i].iZoneType == ZONE_NONE // (The DOS version has an additional dwMapXZON & 0xF check as well)
					) {
						++iValidTiles;
						if (dwMapALTM[uArrPos]->w[i].iLandAltitude == iBaseLevel)
							++iPosCount;
					}
				}
			}
		} while (iValidTiles < 40);
		if (iPosCount < 40) {
			if (iValidTiles < 40) {
				iIterations = 24;
				iValidTiles = 0;
				do {
					bMaxIteration = iIterations-- == 0;
					if (bMaxIteration)
						break;
					iRandOne[1] = Game_RandomWordLCGMod(124);
					iRandTwo[1] = Game_RandomWordLCGMod(124);
					uArrPos = iRandOne[1];
					for (i = 0; (__int16)uArrPos < iRandOne[1] + 3; ++uArrPos) {
						for ( j = iRandTwo[1]; iRandTwo[1] + 3 > j; ++j ) {
							if (
								dwMapXBLD[uArrPos]->iTileID[j] < TILE_SMALLPARK &&
								!dwMapXTER[uArrPos]->iTileID[i] &&
								(
									uArrPos >= 0x80u ||
									(unsigned __int16)j >= 0x80u ||
									dwMapXBIT[uArrPos]->b[j].iWater == 0
								) &&
								dwMapALTM[uArrPos]->w[j].iLandAltitude == iBaseLevel &&
								dwMapXZON[uArrPos]->b[j].iZoneType != ZONE_MILITARY &&
								!dwMapXUND[iRandOne[1]]->iTileID[iRandTwo[1]]
							) {
								++i;
							}
						}
					}
					if (i == 9) {
						iPos[0] = iValidTiles++;
						dwSiloPos[2 * iPos[0]] = iRandOne[1];
						dwSiloPos[2 * iPos[0] + 1] = iRandTwo[1];
					}
				} while (iValidTiles < 6);
			}
			if (iValidTiles == 6) {
				bMilitaryBaseType = MILITARY_BASE_MISSILE_SILOS;
				for (i = 0; i < 6; i++) {
					iPos[0] = dwSiloPos[2 * i];
					iPos[1] = iPos[0];
					for (k = dwSiloPos[2 * i + 1]; iPos[1] + 3 > (__int16)iPos[0]; P_LOWORD(iPos[0]) = iPos[0] + 1) {
						for (uPos[0] = k; k + 3 > (__int16)uPos[0]; ++*(WORD *)dwMilitaryTiles) {
							iBuildingArea = dwMapXBLD[iPos[0]]->iTileID[uPos[0]];
							--*((WORD *)&dwTileCount + iBuildingArea);
							if ((unsigned __int16)iPos[0] < 0x80u && uPos[0] < 0x80u) {
								dwMapXZON[iPos[0]]->b[uPos[0]].iZoneType = ZONE_MILITARY;
								dwMapXZON[iPos[0]]->b[uPos[0]].iCorners = 0xF0;
							}
							++uPos[0];
						}
					}
				}
				Game_CenterOnTileCoords(iPos[1], k);
				return Game_AfxMessageBox(244, 0, -1);
			}
			else {
				if (iMilitaryBaseTries < 10) {
					iMilitaryBaseTries++;
					goto REATTEMPT;
				}
				iResult = Game_AfxMessageBox(411, 0, -1);
				bMilitaryBaseType = MILITARY_BASE_DECLINED;
			}
		}
		else {
			if (iValidTiles == iPosCount) {
				bMilitaryBaseType = MILITARY_BASE_AIR_FORCE;
				Game_AfxMessageBox(242, 0, -1);
			}
			else {
				bMilitaryBaseType = MILITARY_BASE_ARMY;
				Game_AfxMessageBox(241, 0, -1);
			}
			for (uPos[0] = iRandOne[0]; iRandOne[0] + 8 > (__int16)uPos[0]; ++uPos[0]) {
				for (uPos[1] = iPosOffset; iPosOffset + 8 > (__int16)uPos[1]; ++uPos[1]) {
					unsigned __int8 iMilitaryArea = dwMapXBLD[uPos[0]]->iTileID[uPos[1]];
					if (
						iMilitaryArea < TILE_SMALLPARK &&
						!dwMapXTER[uPos[0]]->iTileID[uPos[1]] &&
						(
							uPos[0] >= 0x80u ||
							uPos[1] >= 0x80u ||
							dwMapXBIT[uPos[0]]->b[uPos[1]].iWater == 0
						) &&
						dwMapXZON[uPos[0]]->b[uPos[1]].iZoneType == ZONE_NONE &&
						!dwMapXUND[iRandOne[0]]->iTileID[iPosOffset]
					) {
						--*((WORD *)&dwTileCount + iMilitaryArea);
						if (uPos[0] < 0x80u && uPos[1] < 0x80u) {
							dwMapXZON[uPos[0]]->b[uPos[1]].iZoneType = ZONE_MILITARY;
							dwMapXZON[uPos[0]]->b[uPos[1]].iCorners = 0xF0;
						}
						++*(WORD *)dwMilitaryTiles;
					}
				}
			}
			if (bMilitaryBaseType == MILITARY_BASE_ARMY) {
				// The '//' spacers below represent where an 'if' check and subsequent call to
				// 'FormArmyBaseGrid' is performed in the DOS version (clarity still needed).
				// Although it isn't confirmed, a possibility could be that the 'if' checks
				// were to do with failed runwaycross placements (either down to them ending
				// up on a slope and/or replacing a granite block).
				// Another possibility is that the check was to see whether the runwaycross
				// is present at each opposite end where possible.
				FormArmyBaseGrid(iRandOne[0] + 2, iPosOffset, iRandOne[0] + 2, iPosOffset + 7);
				//
				//
				FormArmyBaseGrid(iRandOne[0] + 5, iPosOffset, iRandOne[0] + 5, iPosOffset + 7);
				//
				//
				FormArmyBaseGrid(iRandOne[0], iPosOffset + 2, iRandOne[0] + 7, iPosOffset + 2);
				//
				//
				FormArmyBaseGrid(iRandOne[0], iPosOffset + 5, iRandOne[0] + 7, iPosOffset + 5);
				//
				//
			}
			return Game_CenterOnTileCoords(iRandOne[0] + 4, iPosOffset + 4);
		}
	}
	return iResult;
}

void InstallMilitaryHooks(void) {
	// Replicate the general functionality provided from the DOS version
	// to also include the Navy and Army Base spawning.
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);
}
