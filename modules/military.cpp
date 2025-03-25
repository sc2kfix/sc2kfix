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

#if 1
extern "C" int __stdcall Hook_SimulationProposeMilitaryBase(void) {
#if 1
	int iResult;
	int iIterations;
	bool bMaxIteration;
	__int16 iPosCount;   // Indicative name, subject to change.
	__int16 iDryTileFootprint; // Indicative name, subject to change.
	int iRandOne;
	__int16 iRandTwo;
	unsigned __int16 uArrPos;
	__int16 iValidTiles;
	int iPosOffset;
	__int16 iBaseLevel;
	DWORD dwSiloPos[12];
		
	iResult = Game_AfxMessageBox(240, MB_YESNO, -1);
	if (iResult == IDNO) {
		bMilitaryBaseType = MILITARY_BASE_DECLINED;
	}
	else {
		if (bCityHasOcean) {

		}
		iIterations = 24;
		iPosCount = dwSiloPos[0];
		do {
			bMaxIteration = iIterations-- == 0;
			if (bMaxIteration)
				break;
			iRandOne = Game_RandomWordLCGMod(119);
			iRandTwo = Game_RandomWordLCGMod(119);
			uArrPos = iRandOne;
			iValidTiles = 0;
			iPosCount = 0;
			iPosOffset = iRandTwo;
			iBaseLevel = *((WORD*)*(&dwMapALTM + (__int16)iRandOne) + iRandTwo) & 0x1F; // 31 - something
			for (dwSiloPos[0] = iRandOne + 8; (__int16)uArrPos < dwSiloPos[0]; ++uArrPos) {
				for (__int16 i = iRandTwo; iRandTwo + 8 > i; ++i) {

				}
			}
		} while (iDryTileFootprint < 40);
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