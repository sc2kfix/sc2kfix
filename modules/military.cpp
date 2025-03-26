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

UINT iMilitaryBaseTries = 0;
WORD wMilitaryBaseX = 0, wMilitaryBaseY = 0;

extern "C" int __cdecl Hook_ItemPlacementCheck(unsigned __int16 a1, int a2, __int16 iTileID, __int16 iTileArea) {

	// a1 - In the calling function it seems that iX (Tile X Coordinate) is set on P_LOWORD(a1)
	// a2 - iY (Tile Y Coordinate) set on v5 (with at leat one append case occurring) **

	// Observation with 'a2': Every now and then the printed value goes out of range, further investigation needed.

	int(__cdecl *H_ItemPlacementCheck)(unsigned __int16, int, __int16, __int16) = (int(__cdecl *)(unsigned __int16, int, __int16, __int16))0x440C50;

	int ret = H_ItemPlacementCheck(a1, a2, iTileID, iTileArea);
	ConsoleLog(LOG_DEBUG, "DBG: 0x%08X -> ItemPlacementCheck(a1: %u, a2: 0x%08X, iTileID: %s, iTileArea: %d) == %d\n", _ReturnAddress(), a1, a2, szTileNames[iTileID], iTileArea, ret);
	return ret;
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
#if 1
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
	
	iMilitaryBaseTries = 0;

	iResult = Game_AfxMessageBox(240, MB_YESNO, -1);
	if (iResult == IDNO) {
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	}
	else {
REATTEMPT:
		if (bCityHasOcean) {

		}
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
#else
	int(__stdcall *SimulationProposeMilitaryBase)(void) = (int(__stdcall *)(void))0x4142C0;

	int ret = SimulationProposeMilitaryBase();

	ConsoleLog(LOG_DEBUG, "DBG: 0x%8X -> SimulationProposeMilitaryBase() - %d\n", _ReturnAddress(), ret);

	return ret;
#endif
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