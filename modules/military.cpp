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

extern "C" int __cdecl Hook_ItemPlacementCheck(unsigned __int16 m_x, int m_y, __int16 iTileID, __int16 iTileArea) {
#if 1
	__int16 iArea;
	__int16 iMarinaCount;
	__int16 iX[2];
	__int16 iY[2];
	__int16 iTile;
	unsigned __int8 iBuilding;
	int iItemWidth;
	int iItemLength;
	int iItemDepth;
	__int16 iMapBit;
	__int16 iSection[3];
	char cMSimBit;
	BYTE *pZone;

	char(__cdecl *H_SimulationProvisionMicrosim)(__int16, int, __int16) = (char(__cdecl *)(__int16, int, __int16))0x401460;
	int(__cdecl *H_sub_4012C1)(__int16, __int16) = (int(__cdecl *)(__int16, __int16))0x4012C1;

	unsigned __int16 x = m_x;
	int y = P_LOWORD(m_y);

	iArea = iTileArea - 1;
	if (iArea > 1) {
		--x;
		P_LOWORD(y) = y - 1;
	}
	ConsoleLog(LOG_DEBUG, "DBG: 0x%08X -> ItemPlacementCheck(x: %u, y: %d, iTileID: %s, iTileArea: %d)\n", _ReturnAddress(), x, y, szTileNames[iTileID], iTileArea);
	iMarinaCount = 0;
	iX[0] = x;
	iItemWidth = (__int16)x + iArea;
	if (iItemWidth >= (__int16)x) {
		iTile = iTileID;
		iItemLength = iArea + (__int16)y;
		while (1) {
			iY[0] = y;
			if (iItemLength >= (__int16)y)
				break;
GOBACK:
			if (++iX[0] > iItemWidth)
				goto GOFORWARD;
		}
		while (1) {
			if (iArea <= 0) {
				if ((unsigned __int16)iX[0] >= 0x80u || (unsigned __int16)iY[0] >= 0x80u)
					return 0;
			}
			else if (iX[0] < 1 || iY[0] < 1 || iX[0] > 126 || iY[0] > 126) {
				return 0;
			}
			iBuilding = dwMapXBLD[iX[0]]->iTileID[iY[0]];
			if (iBuilding >= TILE_ROAD_LR) {
				return 0;
			}
			if (iBuilding == TILE_RADIOACTIVITY) {
				return 0;
			}
			if (iBuilding == TILE_SMALLPARK) {
				return 0;
			}
			//if (dwMapXZON[iX[0]]->b[iY[0]].iZoneType == ZONE_MILITARY) {
			//	return 0; // This is where it stops during the military zone checking process.
			//}
			if (iTileID == TILE_INFRASTRUCTURE_MARINA) {
				if ((unsigned __int16)iX[0] < 0x80u &&
					(unsigned __int16)iY[0] < 0x80u &&
					dwMapXBIT[iX[0]]->b[iY[0]].iWater != 0) {
					++iMarinaCount;
					goto GOSKIP;
				}
				if (dwMapXTER[iX[0]]->iTileID[iY[0]]) {
					return 0;
				}
			}
			if (dwMapXTER[iX[0]]->iTileID[iY[0]]) {
				return 0;
			}
			if ((unsigned __int16)iX[0] < 0x80u &&
				(unsigned __int16)iY[0] < 0x80u &&
				dwMapXBIT[iX[0]]->b[iY[0]].iWater != 0) {
				return 0;
			}
GOSKIP:
			if (++iY[0] > iItemLength) {
				goto GOBACK;
			}
		}
	}
	iTile = iTileID;
GOFORWARD:
	if (iTile == TILE_INFRASTRUCTURE_MARINA && (!iMarinaCount || iMarinaCount == 9)) {
		Game_AfxMessageBox(107, 0, -1);
		return 0;
	}
	else {
		if (iTile == TILE_SERVICES_BIGPARK || (iMapBit = -32, iTile == TILE_SMALLPARK)) { // The initial setting of iMapBit to -32 isn't present in the DOS version.
			iMapBit = 32;
		}
		else {
			iMapBit = 224; // Present in the DOS version.
		}
		if (iTile == TILE_SMALLPARK && dwMapXBLD[x]->iTileID[y] > TILE_SMALLPARK) {
			return 0;
		}
		else {
			iX[1] = x;
			cMSimBit = H_SimulationProvisionMicrosim(x, y, iTile);
			if (iItemWidth >= (__int16)x) {
				iItemDepth = (__int16)y + iArea;
				do {
					for (int i = y; i <= iItemDepth; ++i) {
						if (iX[1] > -1) {
							if (iX[1] < 128 && (unsigned __int16)i < 0x80u) {
								*(BYTE *)&dwMapXBIT[iX[1]]->b[i] &= 0x1Fu;
							}
							if ((unsigned __int16)iX[1] < 0x80u && (unsigned __int16)i < 0x80u) {
								*(BYTE *)&dwMapXBIT[iX[1]]->b[i] |= iMapBit;
							}
						}
						Game_PlaceTileWithMilitaryCheck(iX[1], i, iTile);
						if (iX[1] > -1) {
							if (iX[1] < 128 && (unsigned __int16)i < 0x80u) {
								*(BYTE *)&dwMapXZON[iX[1]]->b[i] &= 0xF0u;
							}
							if ((unsigned __int16)iX[1] < 0x80u && (unsigned __int16)i < 0x80u) {
								*(BYTE *)&dwMapXZON[iX[1]]->b[i] &= 0xFu;
							}
						}
						if (cMSimBit) {
							*(BYTE *)&dwMapXTXT[iX[1]]->bTextOverlay[i] = cMSimBit;
						}
					}
					++iX[1];
				} while (iX[1] <= iItemWidth);
			}
			if (iArea) {
				if (x < 0x80u && (unsigned __int16)y < 0x80u) {
					pZone = (BYTE *)&dwMapXZON[x]->b[y];
					*pZone = LOBYTE(wSomePositionalAngleOne[4 * wViewRotation]) | *pZone & 0xF;
				}
				iSection[0] = iArea + x;
				if ((__int16)(iArea + x) > -1 && iSection[0] < 128 && (unsigned __int16)y < 0x80u) {
					pZone = (BYTE *)&dwMapXZON[iSection[0]]->b[y];
					*pZone = LOBYTE(wSomePositionalAngleTwo[4 * wViewRotation]) | *pZone & 0xF;
				}
				if ((unsigned __int16)iSection[0] < 0x80u) {
					iSection[1] = y + iArea;
					if ((__int16)(y + iArea) > -1 && iSection[1] < 128) {
						pZone = (BYTE *)&dwMapXZON[iSection[0]]->b[iSection[1]];
						*pZone = LOBYTE(wSomePositionalAngleThree[4 * wViewRotation]) | *pZone & 0xF;
					}
				}
				if (x < 0x80u) {
					iSection[2] = iArea + y;
					if ((__int16)(iArea + y) > -1 && iSection[2] < 128) {
						pZone = (BYTE *)&dwMapXZON[x]->b[iSection[2]];
						*pZone = LOBYTE(wSomePositionalAngleFour[4 * wViewRotation]) | *pZone & 0xF;
					}
				}
			}
			else if (x < 0x80u && (unsigned __int16)y < 0x80u) {
				*(BYTE *)&dwMapXZON[x]->b[y] |= 0xF0u;
			}
			H_sub_4012C1(x, y + iArea);
			return 1;
		}
	}
#else
	// x - In the calling function it seems that iX (Tile X Coordinate) is set on P_LOWORD(a1)
	// y - iY (Tile Y Coordinate) set on v5 (with at leat one append case occurring) **

	// Observation with 'a2': Every now and then the printed value goes out of range, further investigation needed.

	int(__cdecl *H_ItemPlacementCheck)(unsigned __int16, int, __int16, __int16) = (int(__cdecl *)(unsigned __int16, int, __int16, __int16))0x440C50;

	int ret = H_ItemPlacementCheck(x, y, iTileID, iTileArea);
	ConsoleLog(LOG_DEBUG, "DBG: 0x%08X -> ItemPlacementCheck(x: %u, y: %d, iTileID: %s, iTileArea: %d) == %d\n", _ReturnAddress(), x, y, szTileNames[iTileID], iTileArea, ret);
	return ret;
#endif
}

static WORD *NavalBaseFunc(WORD *a1, __int16 a2, __int16 a3) {
	WORD *result;

	// Exact use-case not yet clear.
	result = a1;
	a1[1] = a2;
	*a1 = a3;
	return result;
}

static void FormArmyBaseGrid(int x1, int y1, __int16 x2, __int16 y2) {
	WORD wOldToolGroup;
	__int16 iX;
	__int16 iY;
	int iNewX;
	int iNewY;

	// These will be moved to the global section once demystified.
	char(__cdecl *sub_4019A1)(unsigned __int16, unsigned __int16) = (char(__cdecl *)(unsigned __int16, unsigned __int16))0x4019A1;
	int(*sub_401CCB)(void) = (int(*)(void))0x401CCB;

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
			sub_4019A1(iX, iY);
			Game_PlaceRoadAtCoordinates(iX, iY);
			iX = iNewX;
			iY = iNewY;
		}
		sub_4019A1(iX, iY);
		Game_PlaceRoadAtCoordinates(iX, iY);
	}
	if (GetTileID(x1, y1) == TILE_ROAD_LR || GetTileID(x1, y1) == TILE_ROAD_TB) {
		dwMapXZON[x1]->b[y1].iZoneType = ZONE_MILITARY;
		dwMapXZON[x1]->b[y1].iCorners = 0x0F; // In the DOS build this is 0xF0, however that value here results in a blank area.
		Game_PlaceTileWithMilitaryCheck(x1, y1, TILE_INFRASTRUCTURE_RUNWAYCROSS);

	}
	if (GetTileID(iX, iY) == TILE_ROAD_LR || GetTileID(iX, iY) == TILE_ROAD_TB) {
		dwMapXZON[iX]->b[iY].iZoneType = ZONE_MILITARY;
		dwMapXZON[iX]->b[iY].iCorners = 0x0F; // In the DOS build this is 0xF0, however that value here results in a blank area.
		Game_PlaceTileWithMilitaryCheck(iX, iY, TILE_INFRASTRUCTURE_RUNWAYCROSS);

	}
	wMaybeActiveToolGroup = wOldToolGroup;
	sub_401CCB();
}

#if 1
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

	int iPosX[4];
	int iPosY[4];
	int iX, iY;
	int iRotation;
	int iRotVal[4];
	int iOldResult;
	WORD wPosX[4], wPosY[4];
	
	UINT iMilitaryBaseTries = 0;

	iResult = Game_AfxMessageBox(240, MB_YESNO, -1);
	if (iResult == IDNO) {
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	}
	else {
REATTEMPT:
		if (bCityHasOcean) {
			ConsoleLog(LOG_DEBUG, "DBG - 1\n");
			iResult = rand();
			if ((iResult & 1) != 0) {
				ConsoleLog(LOG_DEBUG, "DBG - 2\n");
#if 1
				// X and Y pos with viewrotation, though this is likely indicative for now.
				iX = iPosX[4 * wViewRotation];
				iY = iPosY[4 * wViewRotation];
				ConsoleLog(LOG_DEBUG, "DBG - 2 (%d) (%d) (%d)\n", iX, iY, wViewRotation);
				if (dwMapXBIT[P_SHIWORD(iX)]->b[(__int16)iX].iWater) {
					ConsoleLog(LOG_DEBUG, "DBG - 3\n");
					while (1) {
						if (dwMapXBIT[P_SHIWORD(iX)]->b[(__int16)iX].iWater == 0)
							break;
						P_HIWORD(iX) += P_HIWORD(iY);
						P_LOWORD(iX) = iY + iX;
					}
					P_LOWORD(iRotation) = wViewRotation;
					iRotVal[0] = iRotation;
					iRotVal[1] = iRotation + 1;
					P_LOWORD(iRotVal[1]) = iRotVal[1] & 3;
					iRotVal[2] = iRotVal[1];
					iResult = iRotVal[0] + 2;
					P_LOWORD(iResult) = ((BYTE)iRotVal[0] + 2) & 3;
					iOldResult = iResult;
					int iOffX = iX;
					int iFOffX = iX;
					int iFOffY = 0;
					int iOffY = 0;
					while (P_HIWORD(iX) < 0x80u && (unsigned __int16)iX < 0x80u) {
						if (
						    dwMapXBLD[P_SHIWORD(iX)]->iTileID[(__int16)iX] >= TILE_SMALLPARK ||
							dwMapXTER[P_SHIWORD(iX)]->iTileID[(__int16)iX] ||
							dwMapXBIT[P_SHIWORD(iX)]->b[(__int16)iX].iWater != 0 ||
							dwMapXUND[P_SHIWORD(iX)]->iTileID[(__int16)iX]
						) {
							iFOffX = iX;
							iOffY = 0;
						}
						else if ((__int16)++iOffY > (__int16)iFOffY) {
							iOffX = iFOffX;
							iFOffY = iOffY;
						}
						NavalBaseFunc((WORD *)&iY, P_SHIWORD(iX) + wPosX[(__int16)iRotVal[2]], (__int16)iX + wPosY[(__int16)iRotVal[2]]);
						if ((*(BYTE *)(&dwMapXBIT[P_SHIWORD(iY)]->b[(__int16)iY]) & 5) == 5) {
							do {
								NavalBaseFunc((WORD *)&iX, P_SHIWORD(iY) + wPosX[(__int16)iRotVal[0]], (__int16)iY + wPosY[(__int16)iRotVal[0]]);
								iY = iX;
								P_LOBYTE(iResult) = *(BYTE *)&dwMapXBIT[P_SHIWORD(iX)]->b[(__int16)iX] & 5;
							} while ((BYTE)iResult != 5);
						}
						else {
							do {
								iX = iY;
								NavalBaseFunc((WORD *)&iY, P_SHIWORD(iY) + wPosX[(__int16)iOldResult], (__int16)iY + wPosY[(__int16)iOldResult]);
								P_LOBYTE(iResult) = *(BYTE *)&dwMapXBIT[P_SHIWORD(iY)]->b[(__int16)iY] & 5;
							} while ((BYTE)iResult != 5);
						}
					}
					ConsoleLog(LOG_DEBUG, "DBG - 4\n");
					iX = iOffX;
					if ((__int16)iFOffY >= 12) {
						ConsoleLog(LOG_DEBUG, "DBG - 5\n");
						__int16 iTileDepth = ((__int16)iFOffY - 12) / 2;
						while (iTileDepth <= (__int16)--iFOffY) {
							NavalBaseFunc((WORD *)&iY, P_SHIWORD(iX) + wPosX[(__int16)iRotVal[2]], (__int16)iX + wPosY[(__int16)iRotVal[2]]);
							if ((*(BYTE *)(&dwMapXBIT[P_SHIWORD(iY)]->b[(__int16)iY]) & 5) == 5) {
								do {
									NavalBaseFunc((WORD *)&iX, P_SHIWORD(iY) + wPosX[(__int16)iRotVal[0]], (__int16)iY + wPosY[(__int16)iRotVal[0]]);
									iY = iX;
								} while ((*(BYTE *)(&dwMapXBIT[P_SHIWORD(iX)]->b[(__int16)iX]) & 5) == 5);
							}
							else {
								do {
									iX = iY;
									NavalBaseFunc((WORD *)&iY, P_SHIWORD(iY) + wPosX[(__int16)iOldResult], (__int16)iY + wPosY[(__int16)iOldResult]);
								} while ((*(BYTE *)(&dwMapXBIT[P_SHIWORD(iY)]->b[(__int16)iY]) & 5) == 5);
							}
							if ((__int16)iFOffY < iTileDepth + 10) {
								iY = iX;
								for (i = 0; i < 4; ++i) {
									if (
										dwMapXBLD[P_SHIWORD(iY)]->iTileID[(__int16)iY] < TILE_SMALLPARK &&
										!dwMapXTER[P_SHIWORD(iY)]->iTileID[(__int16)iY] &&
										dwMapXBIT[P_SHIWORD(iY)]->b[(__int16)iY].iWater == 0
									) {
										Game_PlaceTileWithMilitaryCheck(P_SHIWORD(iY), iY, 0);
										dwMapXZON[P_SHIWORD(iY)]->b[(__int16)iY].iZoneType = ZONE_MILITARY;
										dwMapXZON[P_SHIWORD(iY)]->b[(__int16)iY].iCorners = 0xF0;
										--*(WORD *)dwTileCount;
										++*(WORD *)dwMilitaryTiles;
									}
									NavalBaseFunc((WORD *)&iY, P_SHIWORD(iY) + wPosX[(__int16)iRotVal[0]], (__int16)iY + wPosY[(__int16)iRotVal[0]]);
								}
							}
							if ((__int16)iFOffY == iTileDepth + 5) {
								iOffX = iX;
							}
						}
						bMilitaryBaseType = MILITARY_BASE_NAVY;
						Game_CenterOnTileCoords(P_SHIWORD(iOffX), iOffX);
						return Game_AfxMessageBox(243, 0, -1);
					}
					ConsoleLog(LOG_DEBUG, "DBG - -1\n");
				}
				ConsoleLog(LOG_DEBUG, "DBG - -2\n");
#else
				int iTileX = 127;
				int iTileY;
				int iCoastlineRetries = 0;

				// Make a few attempts to find a coastline that isn't immediately bad.
				// Could probably use some refining.
				while (iCoastlineRetries < 10) {
					iTileY = rand() & 0x7F;
					while (TRUE) {
						iTileX--;
						if (!dwMapXBIT[iTileX]->b[iTileY].iSaltWater || !dwMapXBIT[iTileX]->b[iTileY].iWater)
							break;
					}

					if (dwMapXTER[iTileX]->iTileID[iTileY]) {
						if (military_debug & MILITARY_DEBUG_PLACEMENT)
							ConsoleLog(LOG_DEBUG, "MIL:  Bad coastline. Trying again.\n");
						iCoastlineRetries++;
						continue;
					}
					break;
				}

				if (military_debug & MILITARY_DEBUG_PLACEMENT)
					ConsoleLog(LOG_DEBUG, "MIL:  Found potential edge of coast at %i, %i. Moving back four tiles.\n", iTileX + 1, iTileY);
				iTileX -= 3;

				int i = 0, j = 0, iValidTiles = 0;
				while (TRUE) {
					if (iTileX + i == 127) {
						i = 0;
						j++;
					}
					if (j == 12 || i == 0 && iValidTiles >= 48)
						break;

					if (dwMapXBLD[iTileX + i]->iTileID[iTileY + j] > TILE_TREES7 ||
						dwMapXBLD[iTileX + i]->iTileID[iTileY + j] == TILE_RADIOACTIVITY ||
						dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType ||
						dwMapXBIT[iTileX + i]->b[iTileY + j].iWater ||
						dwMapXTER[iTileX + i]->iTileID[iTileY + j]) {
						//printf("Tile at %i, %i no good", iTileX + i, iTileY + j);
						if (dwMapXBIT[iTileX + i]->b[iTileY + j].iWater && dwMapXBIT[iTileX + i]->b[iTileY + j].iSaltWater) {
							i = 0;
							j++;
							//printf(" (coastline hit)");
						}
						//printf(".\n");
					}
					else
						iValidTiles++;
					i++;
				}

				// Placement 
				if (iValidTiles >= 40)
					if (military_debug & MILITARY_DEBUG_PLACEMENT)
						ConsoleLog(LOG_DEBUG, "MIL:  Found zone for naval base at %i, %i: %i valid tiles total, %i height.\n", iTileX, iTileY, iValidTiles, j);
					else {
						if (military_debug & MILITARY_DEBUG_PLACEMENT)
							ConsoleLog(LOG_DEBUG, "MIL:  Failed to place naval base at %i, %i: Only found %i valid tiles, height %i.\n", iTileX, iTileY, iValidTiles, j);
						goto notnavalbase;
					}

					if (military_debug & MILITARY_DEBUG_PLACEMENT)
						ConsoleLog(LOG_DEBUG, "MIL:  Setting military base flag to 4 and zoning tiles...");
					bMilitaryBaseType = MILITARY_BASE_NAVY;
					i = 0;
					j = 0;
					iValidTiles = 0;

					// Zone valid tiles as ZONE_MILITARY
					while (TRUE) {
						if (iTileX + i == 127) {
							i = 0;
							j++;
						}
						if (j == 12 || i == 0 && iValidTiles >= 48)
							break;

						if (dwMapXBLD[iTileX + i]->iTileID[iTileY + j] > TILE_TREES7 ||
							dwMapXBLD[iTileX + i]->iTileID[iTileY + j] == TILE_RADIOACTIVITY ||
							dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType ||
							dwMapXBIT[iTileX + i]->b[iTileY + j].iWater ||
							dwMapXTER[iTileX + i]->iTileID[iTileY + j]) {
							if (dwMapXBIT[iTileX + i]->b[iTileY + j].iWater && dwMapXBIT[iTileX + i]->b[iTileY + j].iSaltWater) {
								i = 0;
								j++;
							}
						}
						else {
							dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType = ZONE_MILITARY;
							iValidTiles++;
						}
						i++;
					}

					Game_CenterOnTileCoords(iTileX, iTileY);
					return Game_AfxMessageBox(243, 0, -1);
#endif
			}
		}
#if 0
notnavalbase:
#endif
		iIterations = 24;
		iPosCount = dwSiloPos[0];
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
							--*((WORD *)dwTileCount + iBuildingArea);
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
				// Could the 'if' cases represent dimensions of the placed zone plot?
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
#else
// Fix military bases not growing.
// XXX - This could use a few extra lines as it's currently possible for a few placeable buildings
// to overwrite and effectively erase military zoned tiles, and I don't know what that will do to
// the simulation engine since it keeps meticulous track of things like that.
//
// We also might want to optionally add in a few more buildings to the growth algorithm for Army
// bases, as currently Army bases only ever build 0xE8 Small Hangar and 0xEF Military Parking Lot.
// Maybe add in 0xE3 Warehouse or 0xF1 Top Secret, since those seem to only grow on naval bases?
extern "C" void _declspec(naked) Hook_FixMilitaryBaseGrowth(void) {
	__asm {
		cmp bp, 0xDD
		jb bail
		cmp bp, 0xF9
		ja bail

		push 0x440D55					// Maxim 43:
		retn							// "If it's stupid and it works...

	bail:
		push 0x440E00					// ...it's still stupid and you're *lucky*."
		retn							//    - The Seventy Maxims of Maximally Effective Mercenaries
	}
}

// Hook to reset iMilitaryBaseTries if needed (new/loaded game, gilmartin)
extern "C" void _declspec(naked) Hook_SimulationProposeMilitaryBase(void) {
	if (military_debug & MILITARY_DEBUG_PLACEMENT)
		ConsoleLog(LOG_DEBUG, "MIL:  SimulationProposeMilitaryBase called, resetting iMilitaryBaseTries.\n");
	iMilitaryBaseTries = 0;
	GAMEJMP(0x4142C0)
}

// Fix the game giving up after one attempt at placing a military base.
// 10 tries was enough to get an army base to spawn in the smallest crags of a map with a maxed-
// out mountain slider, so that's what we're going with here.
extern "C" void _declspec(naked) Hook_AttemptMultipleMilitaryBases(void) {
	if (iMilitaryBaseTries++ <= 10) {
		if (military_debug & MILITARY_DEBUG_PLACEMENT)
			ConsoleLog(LOG_DEBUG, "MIL:  Military base placement attempt %i.\n", iMilitaryBaseTries);

		// Skip attempting to generate a naval base on oceanless maps.
		if (!bCityHasOcean) {
			if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
				ConsoleLog(LOG_DEBUG, "MIL:  Not attempting naval base, city has no ocean.\n");
			goto notnavalbase;
		}

		// Prioritize naval bases on ocean maps.
		if (!(rand() & 3)) {
			if (military_debug & MILITARY_DEBUG_PLACEMENT_NAVAL)
				ConsoleLog(LOG_DEBUG, "MIL:  Not attempting naval base, dice roll failed.\n");
			goto notnavalbase;
		}

		// Attempt to generate a naval base.
		{
			int iTileX = 127;
			int iTileY;
			int iCoastlineRetries = 0;

			// Make a few attempts to find a coastline that isn't immediately bad.
			// Could probably use some refining.
			while (iCoastlineRetries < 10) {
				iTileY = rand() & 0x7F;
				while (TRUE) {
					iTileX--;
					if (!dwMapXBIT[iTileX]->b[iTileY].iSaltWater || !dwMapXBIT[iTileX]->b[iTileY].iWater)
						break;
				}

				if (dwMapXTER[iTileX]->iTileID[iTileY]) {
					if (military_debug & MILITARY_DEBUG_PLACEMENT)
						ConsoleLog(LOG_DEBUG, "MIL:  Bad coastline. Trying again.\n");
					iCoastlineRetries++;
					continue;
				}
				break;
			}

			if (military_debug & MILITARY_DEBUG_PLACEMENT)
				ConsoleLog(LOG_DEBUG, "MIL:  Found potential edge of coast at %i, %i. Moving back four tiles.\n", iTileX + 1, iTileY);
			iTileX -= 3;

			int i = 0, j = 0, iValidTiles = 0;
			while (TRUE) {
				if (iTileX + i == 127) {
					i = 0;
					j++;
				}
				if (j == 12 || i == 0 && iValidTiles >= 48)
					break;

				if (dwMapXBLD[iTileX + i]->iTileID[iTileY + j] > TILE_TREES7 ||
					dwMapXBLD[iTileX + i]->iTileID[iTileY + j] == TILE_RADIOACTIVITY ||
					dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType ||
					dwMapXBIT[iTileX + i]->b[iTileY + j].iWater ||
					dwMapXTER[iTileX + i]->iTileID[iTileY + j]) {
					//printf("Tile at %i, %i no good", iTileX + i, iTileY + j);
					if (dwMapXBIT[iTileX + i]->b[iTileY + j].iWater && dwMapXBIT[iTileX + i]->b[iTileY + j].iSaltWater) {
						i = 0;
						j++;
						//printf(" (coastline hit)");
					}
					//printf(".\n");
				}
				else
					iValidTiles++;
				i++;
			}

			// Placement 
			if (iValidTiles >= 40)
				if (military_debug & MILITARY_DEBUG_PLACEMENT)
					ConsoleLog(LOG_DEBUG, "MIL:  Found zone for naval base at %i, %i: %i valid tiles total, %i height.\n", iTileX, iTileY, iValidTiles, j);
			else {
				if (military_debug & MILITARY_DEBUG_PLACEMENT)
					ConsoleLog(LOG_DEBUG, "MIL:  Failed to place naval base at %i, %i: Only found %i valid tiles, height %i.\n", iTileX, iTileY, iValidTiles, j);
				goto notnavalbase;
			}

			if (military_debug & MILITARY_DEBUG_PLACEMENT)
				ConsoleLog(LOG_DEBUG, "MIL:  Setting military base flag to 4 and zoning tiles...");
			bMilitaryBaseType = 4;
			i = 0;
			j = 0;
			iValidTiles = 0;

			// Zone valid tiles as ZONE_MILITARY
			while (TRUE) {
				if (iTileX + i == 127) {
					i = 0;
					j++;
				}
				if (j == 12 || i == 0 && iValidTiles >= 48)
					break;

				if (dwMapXBLD[iTileX + i]->iTileID[iTileY + j] > TILE_TREES7 ||
					dwMapXBLD[iTileX + i]->iTileID[iTileY + j] == TILE_RADIOACTIVITY ||
					dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType ||
					dwMapXBIT[iTileX + i]->b[iTileY + j].iWater ||
					dwMapXTER[iTileX + i]->iTileID[iTileY + j]) {
					if (dwMapXBIT[iTileX + i]->b[iTileY + j].iWater && dwMapXBIT[iTileX + i]->b[iTileY + j].iSaltWater) {
						i = 0;
						j++;
					}
				}
				else {
					dwMapXZON[iTileX + i]->b[iTileY + j].iZoneType = ZONE_MILITARY;
					iValidTiles++;
				}
				i++;
			}

			// Save the coordinates in case we need them later
			wMilitaryBaseX = iTileX;
			wMilitaryBaseY = iTileY;

			if (military_debug & MILITARY_DEBUG_PLACEMENT)
				ConsoleLog(LOG_DEBUG, "MIL:  Military base placed at %i, %i.\n", wMilitaryBaseX, wMilitaryBaseY);

			// Run the message box, center the view on the location, and return
			Game_AfxMessageBox(243, 4, 0xFFFFFFFF);
			Game_CenterOnTileCoords(wMilitaryBaseX, wMilitaryBaseY);

			__asm {
				pop ebp
				pop esi
				pop edi
				pop edx
				add esp, 0x4C
				retn
			}
		}

		// Pass through to original function
notnavalbase:
		GAMEJMP(0x4142E9)
	} else
		GAMEJMP(0x4147AF)
}

// Quick detour to pull the top-left corner coordinates of a spawned military base.
extern "C" void _declspec(naked) Hook_41442E(void) {
	__asm {
		call Game_AfxMessageBox

		mov edx, [esp + 0x5C - 0x38]
		mov word ptr [wMilitaryBaseX], dx
		mov edx, [esp + 0x5C - 0x34]
		mov word ptr [wMilitaryBaseY], dx
	}

	if (military_debug & MILITARY_DEBUG_PLACEMENT)
		ConsoleLog(LOG_DEBUG, "MIL:  Military base placed at %i, %i.\n", wMilitaryBaseX, wMilitaryBaseY);

	if (bMilitaryBaseType == MILITARY_BASE_ARMY) {
		// The DOS and Mac versions plant a grid of roads on army bases and then a set of runway
		// cross tiles at the ends to give the base some commercial demand cap bonuses.
		// 
		// This is more or less the exact algorithm from the DOS version. And it is equally as
		// terrifyingly ugly. My apologies (also, goddammit Maxis).
		PlaceRoadsAlongPath(wMilitaryBaseX + 2, wMilitaryBaseY, wMilitaryBaseX + 2, wMilitaryBaseY + 7);
		PlaceRoadsAlongPath(wMilitaryBaseX + 5, wMilitaryBaseY, wMilitaryBaseX + 5, wMilitaryBaseY + 7);
		PlaceRoadsAlongPath(wMilitaryBaseX, wMilitaryBaseY + 2, wMilitaryBaseX + 7, wMilitaryBaseY + 2);
		PlaceRoadsAlongPath(wMilitaryBaseX, wMilitaryBaseY + 5, wMilitaryBaseX + 7, wMilitaryBaseY + 5);

		if (GetTileID(wMilitaryBaseX + 2, wMilitaryBaseY) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 2, wMilitaryBaseY) == TILE_ROAD_LR)	{
			dwMapXZON[wMilitaryBaseX + 2]->b[wMilitaryBaseY].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 2]->b[wMilitaryBaseY].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 2, wMilitaryBaseY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
		if (GetTileID(wMilitaryBaseX + 5, wMilitaryBaseY) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 5, wMilitaryBaseY) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX + 5]->b[wMilitaryBaseY].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 5]->b[wMilitaryBaseY].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 5, wMilitaryBaseY, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}

		if (GetTileID(wMilitaryBaseX + 2, wMilitaryBaseY + 7) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 2, wMilitaryBaseY + 7) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX + 2]->b[wMilitaryBaseY + 7].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 2]->b[wMilitaryBaseY + 7].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 2, wMilitaryBaseY + 7, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
		if (GetTileID(wMilitaryBaseX + 5, wMilitaryBaseY + 7) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 5, wMilitaryBaseY + 7) == TILE_ROAD_LR)	{
			dwMapXZON[wMilitaryBaseX + 5]->b[wMilitaryBaseY + 7].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 5]->b[wMilitaryBaseY + 7].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 5, wMilitaryBaseY + 7, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}

		if (GetTileID(wMilitaryBaseX, wMilitaryBaseY + 2) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX, wMilitaryBaseY + 2) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX]->b[wMilitaryBaseY + 2].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX]->b[wMilitaryBaseY + 2].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX, wMilitaryBaseY + 2, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
		if (GetTileID(wMilitaryBaseX, wMilitaryBaseY + 5) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX, wMilitaryBaseY + 5) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX]->b[wMilitaryBaseY + 5].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX]->b[wMilitaryBaseY + 5].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX, wMilitaryBaseY + 5, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}

		if (GetTileID(wMilitaryBaseX + 7, wMilitaryBaseY + 2) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 7, wMilitaryBaseY + 2) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX + 7]->b[wMilitaryBaseY + 2].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 7]->b[wMilitaryBaseY + 2].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 7, wMilitaryBaseY + 2, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}
		if (GetTileID(wMilitaryBaseX + 7, wMilitaryBaseY + 5) == TILE_ROAD_TB || GetTileID(wMilitaryBaseX + 7, wMilitaryBaseY + 5) == TILE_ROAD_LR) {
			dwMapXZON[wMilitaryBaseX + 7]->b[wMilitaryBaseY + 5].iZoneType = ZONE_MILITARY;
			dwMapXZON[wMilitaryBaseX + 7]->b[wMilitaryBaseY + 5].iCorners = 0x0F;
			Game_PlaceTileWithMilitaryCheck(wMilitaryBaseX + 7, wMilitaryBaseY + 5, TILE_INFRASTRUCTURE_RUNWAYCROSS);
		}

		// Update the view to reflect the new placements
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
	}

	GAMEJMP(0x414433)
}
#endif

void InstallMilitaryHooks(void) {

	// Hook into what appears to be one of the item placement checking functions
	VirtualProtect((LPVOID)0x4027F2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4027F2, Hook_ItemPlacementCheck);
	
	// Fix military bases not growing
	//VirtualProtect((LPVOID)0x440D4F, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJZ((LPVOID)0x440D4F, Hook_FixMilitaryBaseGrowth);

	// Make multiple attempts at building a military base before giving up
	//VirtualProtect((LPVOID)0x4142D8, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJNZ((LPVOID)0x4142D8, Hook_AttemptMultipleMilitaryBases);
	//VirtualProtect((LPVOID)0x4146B5, 6, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJNZ((LPVOID)0x4146B5, Hook_AttemptMultipleMilitaryBases);

	// Fix declining military bases crashing after the above hooks are inserted
	//VirtualProtect((LPVOID)0x4142DE, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJMP((LPVOID)0x4142DE, 0x4147BD);

	// Restore the functionality to place naval bases on maps with coastlines
	VirtualProtect((LPVOID)0x403017, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x403017, Hook_SimulationProposeMilitaryBase);

	// Store the coordinates of the military base
	//VirtualProtect((LPVOID)0x41442E, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJMP((LPVOID)0x41442E, Hook_41442E);
}