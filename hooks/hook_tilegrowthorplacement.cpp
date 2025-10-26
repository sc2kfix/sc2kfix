// sc2kfix hooks/hook_tilegrowthorplacement.cpp: tile item placement and/or
//                                               growth handling.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define TILEBUILD_DEBUG_OTHER 1
#define TILEBUILD_DEBUG_SPRITES 2
#define TILEBUILD_DEBUG_TILESETS 4

#define TILEBUILD_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TILEBUILD_DEBUG
#define TILEBUILD_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT tilebuild_debug = TILEBUILD_DEBUG;

static DWORD dwDummy;

static void GetItemPlacementAreaAndFarPosition(__int16 m_x, __int16 m_y, __int16 iTileArea, __int16 *outX, __int16 *outY, __int16 *outFarX, __int16 *outFarY, __int16 *outArea) {
	__int16 x;
	__int16 y;
	__int16 iArea;

	x = m_x;
	y = m_y;

	iArea = iTileArea - 1;
	if (iArea > 1) {
		--x;
		--y;
	}

	*outX = x;
	*outY = y;
	*outFarX = iArea + x;
	*outFarY = iArea + y;
	*outArea = iArea;
}

static int IsValidGeneralPosPlacementMain(__int16 x, __int16 y, __int16 iFarX, __int16 iFarY, __int16 iArea, BYTE iTileID, BOOL bDoSilo, BOOL bSiloPlotCheck, __int16 *outMarinaWaterTileCount) {
	__int16 iCurX;
	__int16 iCurY;
	__int16 iMarinaWaterTileCount;
	BOOL bCanBeMarinaTile;
	BYTE iCurTile;

	iMarinaWaterTileCount = 0;
	for (iCurX = x; iCurX <= iFarX; ++iCurX) {
		for (iCurY = y; iCurY <= iFarY; ++iCurY) {
			// if the extended iArea is zero (or below..) and the current X or Y
			// tiles are equal to or exceed GAME_MAP_SIZE.. definitely abort.
			if (iArea <= 0 && (iCurX >= GAME_MAP_SIZE || iCurY >= GAME_MAP_SIZE))
				return 0;
			else if (iCurX < 1 || iCurY < 1 || iCurX > GAME_MAP_SIZE - 2 || iCurY > GAME_MAP_SIZE - 2) {
				// Added this due to legacy military plot drops,
				// this allows > 1x1 type buildings to develop
				// if the plot is on the edge of the map.
				if (!bDoSilo) {
					if (XZONReturnZone(iCurX, iCurY) == ZONE_MILITARY && (iCurX < 0 || iCurY < 0 || iCurX > GAME_MAP_SIZE - 1 || iCurY > GAME_MAP_SIZE - 1))
						return 0;
					else
						return 0;
				}
				else
					return 0;
			}

			// If the current tile has the referenced
			// item.
			iCurTile = GetTileID(iCurX, iCurY);
			if (iCurTile >= TILE_ROAD_LR)
				return 0;

			if (iCurTile == TILE_RADIOACTIVITY)
				return 0;

			if (iCurTile == TILE_SMALLPARK)
				return 0;

			// !bDoSilo case:
			// Originally in the Win95 version this check
			// only did a comparison regarding the current
			// tile zone being ZONE_MILITARY, as a result
			// it would return 0 and military bases wouldn't
			// grow; now it checks to see whether the current
			// tile zone is ZONE_MILITARY, and whether the
			// tile item is a runwaycross, or certain road tiles -
			// this then prevents either:
			// a) erroneous growth attempts if said tiles are
			//    destroyed (particularly on Army Base plots)
			// b) blank sections being left on Army Base plots
			//    as a result of the presence of said tiles -
			//    particular the road tiles - you'd then see
			//    the 'Hanger' (nice typing error there..)
			//    constantly spawn and despawn resulting
			//    in many unnecessary calls.
			if (!bDoSilo) {
				if (XZONReturnZone(iCurX, iCurY) == ZONE_MILITARY &&
					(iCurTile == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
						iCurTile == TILE_ROAD_LR ||
						iCurTile == TILE_ROAD_TB))
					return 0;
			}
			else {
				if (bSiloPlotCheck) {
					if (XZONReturnZone(iCurX, iCurY) != ZONE_NONE)
						return 0;
				}
				else {
					if (XZONReturnZone(iCurX, iCurY) != ZONE_MILITARY)
						return 0;
					if (XZONReturnZone(iCurX, iCurY) == ZONE_MILITARY && 
						(iCurTile == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
							iCurTile == TILE_ROAD_LR ||
							iCurTile == TILE_ROAD_TB ||
							iCurTile == TILE_MILITARY_MISSILESILO))
						return 0;
				}
			}

			// Marina being an exception, this 'if' block
			// checks to see whether the prospective area
			// is suitable for placement.
			bCanBeMarinaTile = FALSE;
			if (iTileID == TILE_INFRASTRUCTURE_MARINA) {
				if (iCurX < GAME_MAP_SIZE &&
					iCurY < GAME_MAP_SIZE &&
					XBITReturnIsWater(iCurX, iCurY)) {
					++iMarinaWaterTileCount;
					bCanBeMarinaTile = TRUE;
				}
			}

			// This check shouldn't occur if 'bCanBeMarinaTile'
			// is true, since the Marina needs to be placed
			// across shorelines, and a block to prevent
			// placement on shores or water bearing tiles
			// would negate that entirely.
			if (!bCanBeMarinaTile) {
				if (GetTerrainTileID(iCurX, iCurY))
					return 0;

				if (bDoSilo) {
					if (GetUndergroundTileID(iCurX, iCurY))
						return 0;
				}

				if (iCurX < GAME_MAP_SIZE &&
					iCurY < GAME_MAP_SIZE &&
					XBITReturnIsWater(iCurX, iCurY))
					return 0;
			}
		}
	}

	*outMarinaWaterTileCount = iMarinaWaterTileCount;
	return 1;
}

int IsValidSiloPosCheck(__int16 m_x, __int16 m_y) {
	__int16 x;
	__int16 y;
	__int16 iArea;
	__int16 iFarX;
	__int16 iFarY;
	__int16 iDummy;

	GetItemPlacementAreaAndFarPosition(m_x, m_y, AREA_3x3, &x, &y, &iFarX, &iFarY, &iArea);

	return IsValidGeneralPosPlacementMain(x, y, iFarX, iFarY, iArea, TILE_MILITARY_MISSILESILO, TRUE, TRUE, &iDummy);
}

static int IsValidGeneralPosPlacement(__int16 x, __int16 y, __int16 iFarX, __int16 iFarY, __int16 iArea, BYTE iTileID, BOOL bDoSilo, __int16 *outMarinaWaterTileCount) {
	return IsValidGeneralPosPlacementMain(x, y, iFarX, iFarY, iArea, iTileID, bDoSilo, FALSE, outMarinaWaterTileCount);
}

static int L_ItemPlacementCheck(__int16 m_x, __int16 m_y, BYTE iTileID, __int16 iTileArea, BOOL bDoSilo) {
	__int16 x;
	__int16 y;
	__int16 iArea;
	__int16 iFarX;
	__int16 iFarY;
	__int16 iMarinaWaterTileCount;
	__int16 iCurX;
	__int16 iCurY;
	BYTE iTileBitMask;
	BYTE bTextOverlay;

	GetItemPlacementAreaAndFarPosition(m_x, m_y, iTileArea, &x, &y, &iFarX, &iFarY, &iArea);

	iMarinaWaterTileCount = 0;
	if (!IsValidGeneralPosPlacement(x, y, iFarX, iFarY, iArea, iTileID, bDoSilo, &iMarinaWaterTileCount))
		return 0;

	if (iTileID == TILE_INFRASTRUCTURE_MARINA && (iMarinaWaterTileCount == MARINA_TILES_ALLDRY || iMarinaWaterTileCount == MARINA_TILES_ALLWET)) {
		GameMain_AfxMessageBoxID(107, 0, -1);
		return 0;
	}
	else {
		iTileBitMask = 0;
		if (!bDoSilo) {
			iTileBitMask = (XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
			if (iTileID == TILE_SERVICES_BIGPARK || iTileID == TILE_SMALLPARK)
				iTileBitMask = (XBIT_PIPED);
		}
		if (iTileID == TILE_SMALLPARK && GetTileID(x, y) >= TILE_SMALLPARK)
			return 0;
		else {
			if (!bDoSilo)
				bTextOverlay = Game_SimulationProvisionMicrosim(x, y, iTileID);
			if (iFarX >= x) {
				for (iCurX = x; iCurX <= iFarX; ++iCurX) {
					for (iCurY = y; iCurY <= iFarY; ++iCurY) {
						if (iCurX >= 0) {
							if (bDoSilo) {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITClearBits(iCurX, iCurY, XBIT_WATERED | XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
							}
							else {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITClearBits(iCurX, iCurY, XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITSetBits(iCurX, iCurY, iTileBitMask);
							}
						}
						Game_PlaceTileWithMilitaryCheck(iCurX, iCurY, iTileID);
						if (bDoSilo)
							Game_PlaceUndergroundTiles(iCurX, iCurY, UNDER_TILE_MISSILESILO);
						else {
							if (iCurX >= 0) {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XZONClearZone(iCurX, iCurY);
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XZONClearCorners(iCurX, iCurY);
							}
							if (bTextOverlay)
								XTXTSetTextOverlayID(iCurX, iCurY, bTextOverlay);
						}
					}
				}
			}
			if (iArea) {
				if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					XZONSetCornerAngle(x, y, wTileAreaBottomLeftCorner[wViewRotation]);
				if (iFarX >= 0 && iFarX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					XZONSetCornerAngle(iFarX, y, wTileAreaBottomRightCorner[wViewRotation]);
				if (iFarX < GAME_MAP_SIZE && iFarY >= 0 && iFarY < GAME_MAP_SIZE)
					XZONSetCornerAngle(iFarX, iFarY, wTileAreaTopLeftCorner[wViewRotation]);
				if (x < GAME_MAP_SIZE && iFarY >= 0 && iFarY < GAME_MAP_SIZE)
					XZONSetCornerAngle(x, iFarY, wTileAreaTopRightCorner[wViewRotation]);
			}
			else if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XZONSetCornerMask(x, y, CORNER_ALL);
			Game_SpawnItem(x, iFarY);
			return 1;
		}
	}
}

#define CMP_LESSTHAN 0
#define CMP_GREATERTHAN 1
#define CMP_GREATEROREQUAL 2
#define CMP_LESSOREQUAL 3

static bool IsTileThresholdReached(BYTE iTileID, DWORD nTarget, BOOL bMilitary, unsigned uComparator, DWORD nDiv, DWORD nMult) {
	WORD wTileIDCount;
	DWORD nCount;

	// Key:
	//
	// wTileIDCount: This is the returned number of 1x1 tiles covered by iTileID (the bMilitary flag when true
	//               will get you the count of military-specific tiles for the target iTileID).
	//
	// nDiv: This factor you divide against dwTileIDCount in order to get the number of groups of dwTileIDCount.
	//
	// nMult: This factor you multiply against (wTileIDCount / nDiv) in order to get the expected
	//                   multiplied returned count.
	//
	// uComparator: Specify whether you want to compare nCount against nTarget as greater than,
	//              greater or equal, less or equal, or less than.

	wTileIDCount = GetFlaggedTileCount(iTileID, bMilitary);

	if (nDiv < 1)
		nDiv = 1;

	if (nMult < 1)
		nMult = 1;

	nCount = (wTileIDCount / nDiv) * nMult;
	if (uComparator == CMP_GREATERTHAN)
		return (nCount > nTarget);
	else if (uComparator == CMP_GREATEROREQUAL)
		return (nCount >= nTarget);
	else if (uComparator == CMP_LESSOREQUAL)
		return (nCount <= nTarget);
	else
		return (nCount < nTarget);
}

// Use case:
//
// IsTileMultipliedThresholdReached() - Use this one if you want N iTileIDs to be multiplied in order to get a higher expected count
//                                      prior to comparing against the target.
//
// IsTileDividedThresholdReached() - Use this one if you want to divide the returned iTileID count (usually for grouping purposes)
//                                   before then comparing against the target.
//
// IsTileNormalThresholdReached() - Use this one to check the direct iTileID count against the target.

static bool IsTileMultipliedThresholdReached(BYTE iTileID, DWORD nTarget, BOOL bMilitary, unsigned uComparator, DWORD nMult) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, 1, nMult);
}

static bool IsTileDividedThresholdReached(BYTE iTileID, DWORD nTarget, BOOL bMilitary, unsigned uComparator, DWORD nDiv) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, nDiv, 1);
}

static bool IsTileNormalThresholdReached(BYTE iTileID, DWORD nTarget, BOOL bMilitary, unsigned uComparator) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, 1, 1);
}

static void DoArmyBaseGrowth(__int16 iX, __int16 iY, __int16 iCurrZoneType) {
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	if ((rand() & 3) == 0) {
		wFlaggedTileCount = GetFlaggedTileCount(TILE_MILITARY_PARKINGLOT, TRUE) / GetTileArea(AREA_2x2);
		iSelectedTileID = TILE_MILITARY_PARKINGLOT;
		iFirstCheckedTileID = TILE_MILITARY_TOPSECRET;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, TRUE, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = TILE_MILITARY_HANGAR1;
			if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, TRUE, CMP_GREATERTHAN, 8))
				iSelectedTileID = iFirstCheckedTileID;
		}
		if (!Game_SimulationGrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType))
			Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_HANGAR1, iCurrZoneType);
	}
}

static void DoAirPortGrowth(__int16 iX, __int16 iY, BYTE iCurrentTileID, __int16 iCurrZoneType) {
	BOOL bMilitary, bAeroplaneLiftOff;
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	bMilitary = (iCurrZoneType == ZONE_MILITARY) ? TRUE : FALSE;

	if ((rand() & 3) != 0) {
		// This section is for handling the spawning of aeroplanes and helicopters.
		// Only executed for civilian airports.
		if (!bMilitary) {
			if (iCurrentTileID == TILE_INFRASTRUCTURE_RUNWAY &&
				!(rand() % 30) &&
				iX < GAME_MAP_SIZE &&
				iY < GAME_MAP_SIZE) {
				if (XBITReturnIsPowered(iX, iY)) {
					if (rand() % 10 < 4) {
						Game_SpawnHelicopter(iX, iY);
						return;
					}
					bAeroplaneLiftOff = FALSE;
					if (!IsEven(wViewRotation)) {
						if (XBITReturnIsFlipped(iX, iY))
							bAeroplaneLiftOff = TRUE;
					}
					else {
						if (!XBITReturnIsFlipped(iX, iY))
							bAeroplaneLiftOff = TRUE;
					}
					Game_SpawnAeroplane(iX, iY, (bAeroplaneLiftOff) ? 0 : 2);
				}
			}
		}
	}
	else {
		// Aside from certain selected building substitions, the tile selection criteria are the same.
		wFlaggedTileCount = (GetFlaggedTileCount(TILE_INFRASTRUCTURE_RUNWAY, bMilitary) + GetFlaggedTileCount(TILE_INFRASTRUCTURE_RUNWAYCROSS, bMilitary)) / 5;
		iSelectedTileID = TILE_INFRASTRUCTURE_RUNWAY;
		iFirstCheckedTileID = (bMilitary) ? TILE_MILITARY_PARKINGLOT : TILE_INFRASTRUCTURE_PARKINGLOT;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, bMilitary, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = (bMilitary) ? TILE_MILITARY_CONTROLTOWER : TILE_INFRASTRUCTURE_CONTROLTOWER_CIV;
			if (IsTileMultipliedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
				iSelectedTileID = TILE_MILITARY_RADAR;
				if (IsTileMultipliedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
					iSelectedTileID = (bMilitary) ? TILE_MILITARY_F15B : TILE_MILITARY_TARMAC;
					if (IsTileNormalThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL)) {
						iSelectedTileID = TILE_INFRASTRUCTURE_BUILDING1;
						if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
							iSelectedTileID = TILE_INFRASTRUCTURE_BUILDING2;
							if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
								iSelectedTileID = TILE_INFRASTRUCTURE_HANGAR2;
								if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, GetTileArea(AREA_2x2)))
									iSelectedTileID = iFirstCheckedTileID;
							}
						}
					}	
				}	
			}
		}
		Game_SimulationGrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType);
	}
}

static void DoSeaPortGrowth(__int16 iX, __int16 iY, BYTE iCurrentTileID, __int16 iCurrZoneType) {
	BOOL bMilitary;
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	bMilitary = (iCurrZoneType == ZONE_MILITARY) ? TRUE : FALSE;

	if ((rand() & 3) != 0) {
		if (iCurrentTileID == TILE_INFRASTRUCTURE_CRANE && (rand() & 3) == 0)
			Game_SpawnShip(iX, iY);
	}
	else {
		wFlaggedTileCount = GetFlaggedTileCount(TILE_INFRASTRUCTURE_CRANE, bMilitary);
		iSelectedTileID = TILE_INFRASTRUCTURE_CRANE;
		iFirstCheckedTileID = TILE_INFRASTRUCTURE_CARGOYARD;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, bMilitary, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = (bMilitary) ? TILE_MILITARY_TOPSECRET : TILE_MILITARY_LOADINGBAY;
			if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, GetTileArea(AREA_2x2))) {
				iSelectedTileID = TILE_MILITARY_WAREHOUSE;
				if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 3))
					iSelectedTileID = iFirstCheckedTileID;
			}
		}
		if (!Game_SimulationGrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType))
			Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_WAREHOUSE, iCurrZoneType);
	}
}

static void DoSiloGrowth(__int16 iX, __int16 iY, BYTE iCurrentTileID, __int16 iCurrZoneType) {
	if (iCurrentTileID != TILE_MILITARY_MISSILESILO)
		Game_SimulationGrowSpecificZone(iX, iY, TILE_MILITARY_MISSILESILO, iCurrZoneType);
}

static void DoBudgetSubwayCheck(CSimcityView *pSCView, __int16 iX, __int16 iY) {
	BOOL bRemoveUndergroundTile;
	BYTE iCurrentUndergroundTileID;
	BYTE iReplaceTile;
	signed __int16 iFundingPercent;

	if (!Game_RandomWordLFSRMod128()) {
		iCurrentUndergroundTileID = GetUndergroundTileID(iX, iY);
		if (iCurrentUndergroundTileID >= UNDER_TILE_SUBWAY_LR && iCurrentUndergroundTileID < UNDER_TILE_PIPES_LR ||
			iCurrentUndergroundTileID == UNDER_TILE_SUBWAYENTRANCE ||
			iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
			iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB) {
			iFundingPercent = pBudgetArr[BUDGET_SUBWAY].iFundingPercent;
			if (iFundingPercent != 100 && ((unsigned __int16)rand() % 100) >= iFundingPercent) {
				//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Subway. Item(%s) / Underground Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID], (iCurrentUndergroundTileID > UNDER_TILE_SUBWAYENTRANCE) ? "** Unknown **" : szUndergroundNames[iCurrentUndergroundTileID]);
				bRemoveUndergroundTile = FALSE;
				iReplaceTile = UNDER_TILE_CLEAR;
				if (iCurrentUndergroundTileID == UNDER_TILE_SUBWAYENTRANCE) {
					Game_SimcityView_DestroyStructure(pSCView, iX, iY, 0);
					bRemoveUndergroundTile = TRUE;
				}
				else {
					if (iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR)
						iReplaceTile = UNDER_TILE_PIPES_TB;
					else if (iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB)
						iReplaceTile = UNDER_TILE_PIPES_LR;
					bRemoveUndergroundTile = TRUE;
				}
				if (bRemoveUndergroundTile)
					Game_PlaceUndergroundTiles(iX, iY, iReplaceTile);
			}
		}
	}
}

extern int iChurchVirus;

extern "C" void __cdecl Hook_SimulationGrowthTick(signed __int16 iStep, signed __int16 iSubStep) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iX;
	__int16 iY;
	BOOL bPlaceChurch;
	__int16 iCurrZoneType;
	BYTE iCurrentTileID;
	BYTE iTileAreaState;
	// 'iBuildingCommitThreshold' must be 'int' (or a 32-bit integer at the very least),
	// otherwise building growth will not correctly occur and you'll end up with a very
	// high number of 1x1 abandonded buildings.
	int iBuildingCommitThreshold;
	signed __int16 iFundingPercent;
	WORD iBuildingPopLevel;
	WORD iPopulatedAreaTile;
	__int16 iCurrentDemand;
	__int16 iRemainderDemand;
	BYTE iTextOverlay;
	BYTE iMicrosimIdx;
	BYTE iMicrosimDataStat0;
	BYTE iReplaceTile;
	__int16 iNextX;
	__int16 iNextY;

	// Key:
	// iStep: iX += 4 with evert loop until >= GAME_MAP_SIZE is reached.
	// iSubStep: iY += 4 with each loop until >= GAME_MAP_SIZE is reached.

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	// The calculation here is otherwise 2500 population multiplied by
	// number of church tiles is less than the city population, in which
	// case build a church (when the Church virus isn't active...).
	bPlaceChurch = (iChurchVirus > 0) ? 1 : IsTileMultipliedThresholdReached(TILE_INFRASTRUCTURE_CHURCH, dwCityPopulation, FALSE, CMP_LESSTHAN, 2500);
	wCurrentAngle = wPositionAngle[wViewRotation];
	iX = iStep;
	for (;;) {
		if (iX >= GAME_MAP_SIZE)
			break;
		iY = iSubStep;
		for (;;) {
			if (iY >= GAME_MAP_SIZE)
				break;
			iCurrZoneType = XZONReturnZone(iX, iY);
			iCurrentTileID = GetTileID(iX, iY);
			if (iCurrZoneType != ZONE_NONE) {
				if (iCurrZoneType > ZONE_DENSE_INDUSTRIAL) {
					switch (iCurrZoneType) {
					case ZONE_MILITARY:
						switch (bMilitaryBaseType) {
						case MILITARY_BASE_ARMY:
							DoArmyBaseGrowth(iX, iY, iCurrZoneType);
							break;
						case MILITARY_BASE_AIR_FORCE:
							DoAirPortGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
							break;
						case MILITARY_BASE_NAVY:
							DoSeaPortGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
							break;
						case MILITARY_BASE_MISSILE_SILOS:
							DoSiloGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
							break;
						default:
							goto GOTOEND;
						}
						break;
					case ZONE_SEAPORT:
						DoSeaPortGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
						break;
					case ZONE_AIRPORT:
						DoAirPortGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
						break;
					default:
						break;
					}
				}
				else {
					if (iCurrentTileID >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
						if (!XZONCornerCheck(iX, iY, wCurrentAngle)) {
							// This case appears to be hit with >= 2x2 zoned items.
							goto GOTOEND;
						}
						iPopulatedAreaTile = iCurrentTileID - TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1;
						iBuildingPopLevel = wBuildingPopLevel[iPopulatedAreaTile];
					}
					else {
						if (iCurrentTileID >= TILE_ROAD_LR || !Game_IsValidTransitItems(iX, iY)) {
							goto GOTOEND;
						}
						iPopulatedAreaTile = 0;
						iBuildingPopLevel = 0;
					}
					iCurrentDemand = 0;
					iRemainderDemand = 4000;
					if (Game_IsZonedTilePowered(iX, iY)) {
						if (Game_RunTripGenerator(iX, iY, iCurrZoneType, iBuildingPopLevel, 100)) {
							iCurrentDemand = wCityDemand[((iCurrZoneType - 1) / 2)] + 2000;
							iRemainderDemand = 4000 - iCurrentDemand;
						}
					}
					// This block is encountered when a given area is not "under construction" and not "abandonded".
					// A building is then randomly selected in subsequent calls.
					iTileAreaState = bAreaState[iPopulatedAreaTile];
					if (iBuildingPopLevel > 0 && !iTileAreaState) {
						pZonePops[iCurrZoneType] += wBuildingPopulation[iBuildingPopLevel]; // Values appear to be: 1[1], 8[2], 12[3], 36[4] (wBuildingPopulation[iBuildingPopLevel] format.
						if ((unsigned __int16)rand() < (iRemainderDemand / iBuildingPopLevel)) {
							iReplaceTile = rand() & 1;
							Game_PerhapsGeneralZoneChangeBuilding(iX, iY, iBuildingPopLevel, iReplaceTile);
							goto GOTOEND;
						}
					}
					//ConsoleLog(LOG_DEBUG, "[%s] iCurrentTileID[%u] iCurrZoneType[%d](%d) iPopulatedAreaTile(%u) Coords(%d/%d) iBuildingPopLevel[%u] iCurrentDemand[%d] iRemainderDemand[%d] bAreaState(%u/%u)\n", szTileNames[iCurrentTileID], iCurrentTileID, iCurrZoneType, ((iCurrZoneType - 1) / 2), iPopulatedAreaTile, iX, iY, iBuildingPopLevel, iCurrentDemand, iRemainderDemand, iTileAreaState, bAreaState[iPopulatedAreaTile]);
					if (iTileAreaState == 1 && (unsigned __int16)rand() < 0x4000 / iBuildingPopLevel) {
						if (bPlaceChurch && (iBuildingPopLevel & 2) != 0 && iCurrZoneType < ZONE_LIGHT_COMMERCIAL) {
							Game_PlaceChurch(iX, iY);
							goto GOTOEND;
						}
					GOGENERALZONEITEMPLACE:
						Game_PerhapsGeneralZoneChooseAndPlaceBuilding(iX, iY, iBuildingPopLevel, (iCurrZoneType - 1) / 2);
						goto GOTOEND;
					}
					if (iTileAreaState == 2) {
						// Abandoned buildings.
						pZonePops[ZONEPOP_ABANDONED] += wBuildingPopulation[iBuildingPopLevel];
						iBuildingCommitThreshold = 15 * iCurrentDemand / iBuildingPopLevel;
						if ((unsigned __int16)rand() >= iBuildingCommitThreshold)
							goto GOTOEND;
						goto GOGENERALZONEITEMPLACE;
					}
					// This block is where construction will start.
					if (iBuildingPopLevel != 4 &&
						(IsEven(iCurrZoneType) || iBuildingPopLevel <= 0) &&
						(iCurrZoneType >= ZONE_LIGHT_INDUSTRIAL ||
						(iBuildingPopLevel != 1 || GetXVALByteDataWithNormalCoordinates(iX, iY) >= 0x20u) &&
							(iBuildingPopLevel != 2 || GetXVALByteDataWithNormalCoordinates(iX, iY) >= 0x60u) &&
							(iBuildingPopLevel != 3 || GetXVALByteDataWithNormalCoordinates(iX, iY) >= 0xC0u))) {
						iBuildingCommitThreshold = 3 * iCurrentDemand / (iBuildingPopLevel + 1);
						if (iBuildingCommitThreshold > (unsigned __int16)rand())
							Game_PerhapsGeneralZoneStartBuilding(iX, iY, iBuildingPopLevel, iCurrZoneType);
					}
				}
			}
			else {
				if (iCurrentTileID < TILE_ROAD_LR)
					goto GOTOEND;
				if (Game_RandomWordLFSRMod128())
					goto GOAFTERSETXBIT;
				if (iCurrentTileID >= TILE_ROAD_LR && iCurrentTileID < TILE_RAIL_LR ||
					iCurrentTileID >= TILE_CROSSOVER_POWERTB_ROADLR && iCurrentTileID < TILE_CROSSOVER_POWERTB_RAILLR ||
					iCurrentTileID == TILE_CROSSOVER_HIGHWAYLR_ROADTB ||
					iCurrentTileID == TILE_CROSSOVER_HIGHWAYTB_ROADLR ||
					iCurrentTileID >= TILE_ONRAMP_TL && iCurrentTileID < TILE_HIGHWAY_HTB) {
					// Transportation budget, roads - if below 100% related tiles will be replaced with rubble.
					iFundingPercent = pBudgetArr[BUDGET_ROAD].iFundingPercent;
					if (iFundingPercent != 100 && ((unsigned __int16)rand() % 100) >= iFundingPercent) {
						iReplaceTile = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iReplaceTile);
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							goto GOAFTERSETXBIT;
						goto GOBEFORESETXBIT;
					}
				}
				else if (iCurrentTileID >= TILE_RAIL_LR && iCurrentTileID < TILE_TUNNEL_T ||
					iCurrentTileID >= TILE_CROSSOVER_ROADLR_RAILTB && iCurrentTileID < TILE_HIGHWAY_LR ||
					iCurrentTileID >= TILE_SUBTORAIL_T && iCurrentTileID < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 ||
					iCurrentTileID == TILE_CROSSOVER_HIGHWAYLR_RAILTB ||
					iCurrentTileID == TILE_CROSSOVER_HIGHWAYTB_RAILLR) {
					// Transportation budget, rails - if below 100% related tiles will be replaced with rubble.
					iFundingPercent = pBudgetArr[BUDGET_RAIL].iFundingPercent;
					if (iFundingPercent != 100 && ((unsigned __int16)rand() % 100) >= iFundingPercent) {
						iReplaceTile = (rand() & 3) + 1;
						Game_PlaceTileWithMilitaryCheck(iX, iY, iReplaceTile);
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							goto GOAFTERSETXBIT;
					GOBEFORESETXBIT:
						XBITClearBits(iX, iY, XBIT_POWERABLE);
					GOAFTERSETXBIT:
						if (iCurrentTileID >= TILE_INFRASTRUCTURE_RAILSTATION) {
							if (iCurrentTileID == TILE_INFRASTRUCTURE_RAILSTATION &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								XBITReturnIsPowered(iX, iY) &&
								!Game_RandomWordLFSRMod4()) {
								if (dwTileCount[TILE_INFRASTRUCTURE_RAILSTATION] / 4 > wActiveTrains)
									Game_SpawnTrain(iX, iY);
							}
							else if (iCurrentTileID == TILE_INFRASTRUCTURE_MARINA &&
								iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								XBITReturnIsPowered(iX, iY) &&
								!Game_RandomWordLFSRMod4()) {
								if (dwTileCount[TILE_INFRASTRUCTURE_MARINA] / 9 > wSailingBoats)
									Game_SpawnSailBoat(iX, iY);
							}
							else if (iCurrentTileID >= TILE_ARCOLOGY_PLYMOUTH &&
								iCurrentTileID <= TILE_ARCOLOGY_LAUNCH &&
								XZONCornerAbsoluteCheckMask(iX, iY, CORNER_TRIGHT)) {
								iTextOverlay = XTXTGetTextOverlayID(iX, iY);
								iMicrosimIdx = iTextOverlay - MAX_USER_TEXT_ENTRIES;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - iTextOverlay(%u), iMicrosimIdx(%u), Item(%s)\n", iStep, iSubStep, iTextOverlay, iMicrosimIdx, szTileNames[iCurrentTileID]);
								if (iTextOverlay >= MAX_USER_TEXT_ENTRIES &&
									iTextOverlay < 201 &&
									GetMicroSimulatorTileID(iMicrosimIdx) >= TILE_ARCOLOGY_PLYMOUTH &&
									GetMicroSimulatorTileID(iMicrosimIdx) <= TILE_ARCOLOGY_LAUNCH) {
									iMicrosimDataStat0 = (GetXVALByteDataWithNormalCoordinates(iX, iY) >> 5)
										- (GetXCRMByteDataWithNormalCoordinates(iX,iY) >> 5)
										- (GetXPLTByteDataWithNormalCoordinates(iX,iY) >> 5)
										+ 12;
									if (iX >= GAME_MAP_SIZE ||
										iY >= GAME_MAP_SIZE ||
										!XBITReturnIsPowered(iX, iY))
										iMicrosimDataStat0 /= 2;
									if (iX >= GAME_MAP_SIZE ||
										iY >= GAME_MAP_SIZE ||
										!XBITReturnIsWatered(iX, iY))
										iMicrosimDataStat0 /= 2;
									if (iMicrosimDataStat0 < 0)
										iMicrosimDataStat0 = 0;
									if (iMicrosimDataStat0 > 12)
										iMicrosimDataStat0 = 12;
									//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - iTextOverlay(%u), iMicrosimIdx(%u), iMicrosimDataStat0(%u), Item(%s) (%u, %u, %u, %d - %d)\n", iStep, iSubStep, iTextOverlay, iMicrosimIdx, iMicrosimDataStat0, szTileNames[iCurrentTileID], *(BYTE *)&dwMapXZON[iX][iY].b & 0xF0, 0x80, dwMapXZON[iX][iY].b.iCorners, CORNER_TRIGHT, (CORNER_TRIGHT << 4));
									SetMicroSimulatorStat0(iMicrosimIdx, iMicrosimDataStat0);
								}
							}
						}
					}
				}
				else if (iCurrentTileID >= TILE_SUSPENSION_BRIDGE_START_B && iCurrentTileID < TILE_ONRAMP_TL ||
					iCurrentTileID == TILE_REINFORCED_BRIDGE_PYLON ||
					iCurrentTileID == TILE_REINFORCED_BRIDGE) {
					iFundingPercent = pBudgetArr[BUDGET_BRIDGE].iFundingPercent;
					// Transportation budget, bridges - if below 100% and the weather isn't favourable, there's a chance of destruction.
					if (iFundingPercent != 100 && (int)(bWeatherWind + (unsigned __int16)rand() % 50) >= iFundingPercent) {
						//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Bridge. Weather Vulnerable\n", iStep, iSubStep);
						Game_CenterOnTileCoords(iX, iY);
						Game_SimcityView_DestroyStructure(pSCView, iX, iY, 1);
						Game_NewspaperStoryGenerator(39, 0);
						goto GOAFTERSETXBIT;
					}
				}
				else if (iCurrentTileID < TILE_TUNNEL_T || iCurrentTileID >= TILE_CROSSOVER_POWERTB_ROADLR) {
					if ((iCurrentTileID < TILE_HIGHWAY_HTB || iCurrentTileID >= TILE_SUBTORAIL_T) &&
						(iCurrentTileID < TILE_HIGHWAY_LR || iCurrentTileID >= TILE_SUSPENSION_BRIDGE_START_B))
						goto GOAFTERSETXBIT;
					if (IsEven(iX) && IsEven(iY)) {
						iFundingPercent = pBudgetArr[BUDGET_HIGHWAY].iFundingPercent;
						if (iFundingPercent != 100 && ((unsigned __int16)rand() % 100) >= iFundingPercent) {
							if (iX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								XBITReturnIsWater(iX, iY)) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #1. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iX, iY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #1 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iReplaceTile, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iX, iY, iReplaceTile);
							}
							iNextX = iX + 1;
							if (iNextX >= 0 &&
								iNextX < GAME_MAP_SIZE &&
								iY < GAME_MAP_SIZE &&
								XBITReturnIsWater(iNextX, iY)) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #2. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iNextX, iY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #2 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iReplaceTile, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iNextX, iY, iReplaceTile);
							}
							iNextY = iY + 1;
							if (iX < GAME_MAP_SIZE &&
								iNextY >= 0 &&
								iNextY < GAME_MAP_SIZE &&
								XBITReturnIsWater(iX, iNextY)) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #3. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iX, iNextY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #3 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iReplaceTile, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iX, iNextY, iReplaceTile);
							}
							if (iNextX < GAME_MAP_SIZE &&
								iNextY < GAME_MAP_SIZE &&
								XBITReturnIsWater(iNextX, iNextY)) {
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #4. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iNextX, iNextY, 0);
							}
							else {
								iReplaceTile = (rand() & 3) + 1;
								//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Transit #4 (else). iRandSelect(%d). Item(%s)\n", iStep, iSubStep, iReplaceTile, szTileNames[iCurrentTileID]);
								Game_PlaceTileWithMilitaryCheck(iNextX, iNextY, iReplaceTile);
							}
							goto GOAFTERSETXBIT;
						}
					}
				}
				else if (iCurrentTileID >= TILE_TUNNEL_T && iCurrentTileID <= TILE_TUNNEL_L) {
					iFundingPercent = pBudgetArr[BUDGET_TUNNEL].iFundingPercent;
					if (iFundingPercent != 100 && ((unsigned __int16)rand() % 100) >= iFundingPercent) {
						//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Tunnel. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
						Game_CenterOnTileCoords(iX, iY);
						Game_SimcityView_DestroyStructure(pSCView, iX, iY, 1);
						goto GOAFTERSETXBIT;
					}
				}
			}
		GOTOEND:
			DoBudgetSubwayCheck(pSCView, iX, iY);
			iY += 4;
		}
		iX += 4;
	}
	rcDst.top = -1000;
}

static void DeleteTilePortion(__int16 x, __int16 y) {
	if (GetTileID(x, y) >= TILE_SMALLPARK)
		Game_ZonedBuildingTileDeletion(x, y);
}

static void SetNewZoneOnTilePortion(__int16 x, __int16 y, __int16 iZoneType) {
	if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
		XZONSetNewZone(x, y, iZoneType);
}

static void MilitaryUnsetBitsOnTilePortion(__int16 x, __int16 y, __int16 iZoneType) {
	if (iZoneType == ZONE_MILITARY) {
		if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
			XBITClearBits(x, y, XBIT_WATERED | XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
	}
}

static BOOL SetMoveRunwayTileAxis(__int16 primaryAxis, __int16 secondaryAxis, BOOL *bMovePrimaryAxis, BOOL *bMoveSecondaryAxis) {
	if (!IsEven(primaryAxis))
		*bMovePrimaryAxis = TRUE;
	if (!*bMovePrimaryAxis) {
		if (IsEven(secondaryAxis))
			return FALSE;

		*bMoveSecondaryAxis = TRUE;
	}
	return TRUE;
}

static BOOL GetRunwayTilePositionalOffset(__int16 x, __int16 y, __int16 iZoneType, __int16 *iMoveX, __int16 *iMoveY) {
	BOOL bMoveXAxis;
	BOOL bMoveYAxis;
	WORD wTileCountType;

	bMoveXAxis = FALSE;
	bMoveYAxis = FALSE;
	// Slight change here: distinguish between military and standard runway tiles.
	wTileCountType = (iZoneType == ZONE_MILITARY) ? dwMilitaryTiles[MILITARYTILE_RUNWAY] : dwTileCount[TILE_INFRASTRUCTURE_RUNWAY];
	if (IsEven(wTileCountType)) {
		if (!SetMoveRunwayTileAxis(x, y, &bMoveXAxis, &bMoveYAxis))
			return FALSE;
	}
	else {
		if (!SetMoveRunwayTileAxis(y, x, &bMoveYAxis, &bMoveXAxis))
			return FALSE;
	}

	*iMoveX = (bMoveXAxis) ? 1 : 0;
	*iMoveY = (bMoveYAxis) ? 1 : 0;
	return TRUE;
}

static int ShouldRunwayTileFlip(__int16 iMoveY) {
	int iToFlip;

	iToFlip = 0;
	if (!iMoveY) {
		if (!IsEven(wViewRotation))
			iToFlip = 1;
	}
	else {
		if (IsEven(wViewRotation))
			iToFlip = 1;
	}

	return iToFlip;
}

static BOOL RunwayTileMilitaryCheck(__int16 x, __int16 y, __int16 iZoneType) {
	if (iZoneType == ZONE_MILITARY) {
		if ((GetTileID(x, y) >= TILE_ROAD_LR && GetTileID(x, y) <= TILE_ROAD_LTBR) ||
			GetTileID(x, y) == TILE_INFRASTRUCTURE_CRANE || GetTileID(x, y) == TILE_MILITARY_MISSILESILO)
			return FALSE;
		if (GetTerrainTileID(x, y))
			return FALSE;
		if (GetUndergroundTileID(x, y))
			return FALSE;
	}
	return TRUE;
}

static BOOL RunwayStripLengthCheck(__int16 iRunwayStripTileCount) {
	// Does the runway strip equal or exceed the defined number of max tiles?
	return (iRunwayStripTileCount >= RUNWAYSTRIP_MAXTILES) ? TRUE : FALSE;
}

static BOOL IsRunwayTypeTile(__int16 x, __int16 y) {
	return (GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAY || GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAYCROSS) ? TRUE : FALSE;
}

static int ShouldPierTileFlip(__int16 iMoveX) {
	__int16 iToFlip;

	iToFlip = 0;
	if (!iMoveX) {
		if (!IsEven(wViewRotation))
			iToFlip = 1;
	}
	else {
		if (IsEven(wViewRotation))
			iToFlip = 1;
	}

	return iToFlip;
}

static BOOL TwoByTwoGeneralBlockTileCheck(__int16 x, __int16 y) {
	BYTE iTileID;

	iTileID = GetTileID(x, y);
	return (iTileID == TILE_INFRASTRUCTURE_RUNWAY || iTileID == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
		iTileID == TILE_INFRASTRUCTURE_CRANE || iTileID == TILE_MILITARY_MISSILESILO) ? TRUE : FALSE;
}

static BOOL TwoByTwoMismatchAndMilitaryBlockTileCheck(__int16 x, __int16 y, __int16 iZoneType) {
	if (XZONReturnZone(x, y) != iZoneType)
		return TRUE;
	if (iZoneType == ZONE_MILITARY) {
		if (XZONReturnZone(x, y) == ZONE_MILITARY) {
			if (GetTileID(x, y) >= TILE_ROAD_LR && GetTileID(x, y) <= TILE_ROAD_LTBR)
				return TRUE;
		}
		if (GetUndergroundTileID(x, y))
			return TRUE;
	}
	return FALSE;
}

extern "C" int __cdecl Hook_SimulationGrowSpecificZone(__int16 iX, __int16 iY, BYTE iTileID, __int16 iZoneType) {
	__int16 x, y;
	__int16 iMoveX, iMoveY;
	__int16 iCurrX, iCurrY;
	__int16 iNextX, iNextY;
	__int16 iInitialRunwayStripTileCount;
	__int16 iBranchingRunwayStripTileCount;
	__int16 iToFlip;
	__int16 iTileFlipped;
	__int16 iPierTileCount;
	__int16 iPierPathTileCount;
	__int16 iPierLength;

	x = iX;
	y = iY;
	if (iZoneType != ZONE_MILITARY)
		if (!Game_IsZonedTilePowered(x, y))
			return 0;

	switch (iTileID) {
	case TILE_INFRASTRUCTURE_RUNWAY:
		iMoveX = 0;
		iMoveY = 0;

		if (!GetRunwayTilePositionalOffset(x, y, iZoneType, &iMoveX, &iMoveY))
			return 0;

		iCurrX = x;
		iCurrY = y;
		iInitialRunwayStripTileCount = 0;
		while (iCurrX < GAME_MAP_SIZE && iCurrY < GAME_MAP_SIZE) {
			if (XZONReturnZone(iCurrX, iCurrY) != iZoneType)
				return 0;
			if (!RunwayTileMilitaryCheck(iCurrX, iCurrY, iZoneType))
				return 0;
			// With this check if there's a hit on an existing runway
			// tile then we want to decrease the count until it reaches
			// the first vacant tile.
			if (IsRunwayTypeTile(iCurrX, iCurrY))
				--iInitialRunwayStripTileCount;
			iCurrX += iMoveY;
			iCurrY += iMoveX;
			++iInitialRunwayStripTileCount;
			if (RunwayStripLengthCheck(iInitialRunwayStripTileCount)) {
				iToFlip = ShouldRunwayTileFlip(iMoveY);

				iBranchingRunwayStripTileCount = 0;
				while (1) {
					if (x >= 0) {
						// With this check if true and there's a hit on
						// an existing runway tile then the branching strip
						// counter is decreased, if the tile is specifically
						// the standard runway-type (not cross) do a tile-flip
						// check, if the result is not equivalent to iToFlip
						// then replace the tile in question with the
						// runwaycross type and unset the flipped bit at the end.
						if (IsRunwayTypeTile(x, y)) {
							--iBranchingRunwayStripTileCount;
							if (GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAY) {
								iTileFlipped = (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE && XBITReturnIsFlipped(x, y));
								if (iTileFlipped != iToFlip) {
									Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAYCROSS);
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XZONSetCornerMask(x, y, CORNER_ALL);
									if (iZoneType != ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XBITSetBits(x, y, XBIT_POWERED | XBIT_POWERABLE);
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XBITClearBits(x, y, XBIT_FLIPPED);
								}
							}
						}
						else {
							if (!RunwayTileMilitaryCheck(x, y, iZoneType))
								return 0;
							DeleteTilePortion(x, y);
							Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_RUNWAY);
							if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XZONSetCornerMask(x, y, CORNER_ALL);
							if (iZoneType != ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XBITSetBits(x, y, XBIT_POWERED | XBIT_POWERABLE);
							if (iToFlip && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XBITSetBits(x, y, XBIT_FLIPPED);
						}
					}
					x += iMoveY;
					y += iMoveX;
					++iBranchingRunwayStripTileCount;
					if (RunwayStripLengthCheck(iBranchingRunwayStripTileCount))
						return 1;
					continue;
				}
			}
		}
		return 1;
	case TILE_INFRASTRUCTURE_CRANE:
		// This check has been added to prevent a condition whereas the crane fails
		// to be placed as a result of it being on the edge of the map but the pier
		// itself is still spawned (REVISIT if the various literal edge-cases end
		// up being overcome!).
		if ((x <= 0 || x >= GAME_MAP_SIZE - 1) || (y <= 0 || y >= GAME_MAP_SIZE - 1))
			return 0;
		for (iPierTileCount = 0; iPierTileCount < PIER_MAXTILES; iPierTileCount++) {
			iMoveX = x + wTilePierLengthWays[iPierTileCount];
			if (iMoveX < GAME_MAP_SIZE) {
				iMoveY = y + wTilePierDepthWays[iPierTileCount];
				if (iMoveY < GAME_MAP_SIZE && XBITReturnIsWater(iMoveX, iMoveY))
					break;
			}
		}
		if (iPierTileCount == PIER_MAXTILES)
			return 0;
		iMoveY = wTilePierDepthWays[iPierTileCount];
		if (iMoveY && !IsEven(x))
			return 0;
		iMoveX = wTilePierLengthWays[iPierTileCount];
		if (iMoveX && !IsEven(y))
			return 0;
		iPierPathTileCount = 0;
		iCurrX = x;
		iCurrY = y;
		do {
			iCurrX += iMoveX;
			iCurrY += iMoveY;
			if (iCurrX >= GAME_MAP_SIZE ||
				iCurrY >= GAME_MAP_SIZE ||
				!XBITReturnIsWater(iCurrX, iCurrY))
				return 0;
			if (GetTileID(iCurrX, iCurrY))
				return 0;
			++iPierPathTileCount;
		} while (iPierPathTileCount <= PIER_MAXTILES);
		if (ALTMReturnWaterLevel(iCurrX, iCurrY) < ALTMReturnLandAltitude(iCurrX, iCurrY) + 2)
			return 0;
		DeleteTilePortion(x, y);
		Game_ItemPlacementCheck(x, y, TILE_INFRASTRUCTURE_CRANE, AREA_1x1);
		SetNewZoneOnTilePortion(x, y, iZoneType);
		MilitaryUnsetBitsOnTilePortion(x, y, iZoneType);

		iToFlip = ShouldPierTileFlip(iMoveX);

		iPierLength = PIER_MAXTILES;
		do {
			x += wTilePierLengthWays[iPierTileCount];
			y += wTilePierDepthWays[iPierTileCount];
			Game_PlaceTileWithMilitaryCheck(x, y, TILE_INFRASTRUCTURE_PIER);
			if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XZONSetCornerMask(x, y, CORNER_ALL);
			if (iToFlip && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XBITSetBits(x, y, XBIT_FLIPPED);
			--iPierLength;
		} while (iPierLength);
		return 1;
	case TILE_INFRASTRUCTURE_CONTROLTOWER_CIV:
	case TILE_MILITARY_CONTROLTOWER:
	case TILE_MILITARY_WAREHOUSE:
	case TILE_INFRASTRUCTURE_BUILDING1:
	case TILE_INFRASTRUCTURE_BUILDING2:
	case TILE_MILITARY_TARMAC:
	case TILE_MILITARY_F15B:
	case TILE_MILITARY_HANGAR1:
	case TILE_MILITARY_RADAR:
		if (GetTileID(x, y) < TILE_SMALLPARK) {
			Game_ItemPlacementCheck(x, y, iTileID, AREA_1x1);
			SetNewZoneOnTilePortion(x, y, iZoneType);
			MilitaryUnsetBitsOnTilePortion(x, y, iZoneType);
		}
		return 1;
	case TILE_INFRASTRUCTURE_PARKINGLOT:
	case TILE_MILITARY_PARKINGLOT:
	case TILE_MILITARY_LOADINGBAY:
	case TILE_MILITARY_TOPSECRET:
	case TILE_INFRASTRUCTURE_CARGOYARD:
	case TILE_INFRASTRUCTURE_HANGAR2:
		//__int16 iSX, iSY;
		// Odd numbered x/y coordinates are subtracted by 1.
		// If this isn't done then buildings of this nature
		// will end up overlapping.
		iCurrX = (IsEven(x)) ? x : x - 1;
		iCurrY = (IsEven(y)) ? y : y - 1;

		// Old method, kept here for comparison cases as a precaution.
		// iSX == iCurrX
		// iSY == iCurrY
		//iSX = x;
		//P_LOBYTE(iSX) = x & 0xFE;
		//iSY = y;
		//P_LOBYTE(iSY) = y & 0xFE;
		//ConsoleLog(LOG_DEBUG, "0: [%s] (%d, (%d, %d))==%c (%d, (%d, %d))==%c\n", szTileNames[iTileID], x, iCurrX, iSX, ((iCurrX==iSX) ? 'Y' : 'N'), y, iCurrY, iSY, ((iCurrY==iSY) ? 'Y' : 'N'));

		iNextX = iCurrX + 1;
		iNextY = iCurrY + 1;
		if (GetTileID(iCurrX, iCurrY) >= TILE_INFRASTRUCTURE_WATERTOWER)
			return 0;
		if (TwoByTwoGeneralBlockTileCheck(iCurrX, iCurrY) ||
			TwoByTwoGeneralBlockTileCheck(iNextX, iCurrY) ||
			TwoByTwoGeneralBlockTileCheck(iCurrX, iNextY) ||
			TwoByTwoGeneralBlockTileCheck(iNextX, iNextY))
			return 0;
		if (TwoByTwoMismatchAndMilitaryBlockTileCheck(iCurrX, iCurrY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iNextX, iCurrY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iCurrX, iNextY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iNextX, iNextY, iZoneType))
			return 0;
		DeleteTilePortion(iCurrX, iCurrY);
		DeleteTilePortion(iNextX, iCurrY);
		DeleteTilePortion(iCurrX, iNextY);
		DeleteTilePortion(iNextX, iNextY);
		Game_ItemPlacementCheck(iCurrX, iCurrY, iTileID, AREA_2x2);
		SetNewZoneOnTilePortion(iCurrX, iCurrY, iZoneType);
		SetNewZoneOnTilePortion(iNextX, iCurrY, iZoneType);
		SetNewZoneOnTilePortion(iCurrX, iNextY, iZoneType);
		SetNewZoneOnTilePortion(iNextX, iNextY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iCurrX, iCurrY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iNextX, iCurrY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iCurrX, iNextY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iNextX, iNextY, iZoneType);
		return 1;
	case TILE_MILITARY_MISSILESILO:
		L_ItemPlacementCheck(x, y, TILE_MILITARY_MISSILESILO, AREA_3x3, TRUE);
		return 1;
	default:
		return 1;
	}
}

static void PlacePowerLineTile(__int16 x, __int16 y, BYTE iTileID) {
	Game_PlaceTileWithMilitaryCheck(x, y, iTileID);
	if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
		XBITSetBits(x, y, XBIT_POWERABLE);
	Game_CheckAdjustTerrainAndPlacePowerLines(x, y);
	if (x > 0)
		Game_CheckAdjustTerrainAndPlacePowerLines(x - 1, y);
	if (x < GAME_MAP_SIZE-1)
		Game_CheckAdjustTerrainAndPlacePowerLines(x + 1, y);
	if (y > 0)
		Game_CheckAdjustTerrainAndPlacePowerLines(x, y - 1);
	if (y < GAME_MAP_SIZE-1)
		Game_CheckAdjustTerrainAndPlacePowerLines(x, y + 1);
}

extern "C" void __cdecl Hook_PlacePowerLinesAtCoordinates(__int16 x, __int16 y) {
	BYTE iTileID;
	BYTE iBuildTileID;

	if ((x >= 0 || x < GAME_MAP_SIZE) && (y >= 0 || y < GAME_MAP_SIZE) && XBITReturnMask(x, y) >= 0) {
		iTileID = GetTileID(x, y);
		if (iTileID < TILE_POWERLINES_LR)
			iBuildTileID = TILE_POWERLINES_LR;
		else if (iTileID == TILE_ROAD_LR)
			iBuildTileID = TILE_CROSSOVER_POWERTB_ROADLR;
		else if (iTileID == TILE_ROAD_TB)
			iBuildTileID = TILE_CROSSOVER_POWERLR_ROADTB;
		else if (iTileID == TILE_RAIL_LR)
			iBuildTileID = TILE_CROSSOVER_POWERTB_RAILLR;
		else if (iTileID == TILE_RAIL_TB)
			iBuildTileID = TILE_CROSSOVER_POWERLR_RAILTB;
		else if (iTileID == TILE_HIGHWAY_LR)
			iBuildTileID = TILE_CROSSOVER_HIGHWAYLR_POWERTB;
		else if (iTileID == TILE_HIGHWAY_TB)
			iBuildTileID = TILE_CROSSOVER_HIGHWAYTB_POWERLR;
		else
			return;

		PlacePowerLineTile(x, y, iBuildTileID);
	}
}

extern "C" int __cdecl Hook_ItemPlacementCheck(__int16 m_x, __int16 m_y, BYTE iTileID, __int16 iTileArea) {
	return L_ItemPlacementCheck(m_x, m_y, iTileID, iTileArea, FALSE);
}

void InstallTileGrowthOrPlacementHandlingHooks_SC2K1996(void) {
	// Hook into the SimulationGrowthTick function
	VirtualProtect((LPVOID)0x4022FC, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4022FC, Hook_SimulationGrowthTick);

	// Hook into the SimulationGrowSpecificZone function
	VirtualProtect((LPVOID)0x4026B2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4026B2, Hook_SimulationGrowSpecificZone);

	// Hook into the PlacePowerLinesAtCoordinates function
	VirtualProtect((LPVOID)0x402725, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402725, Hook_PlacePowerLinesAtCoordinates);

	// Hook into the ItemPlacementCheck function
	VirtualProtect((LPVOID)0x4027F2, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4027F2, Hook_ItemPlacementCheck);

	// Military base hooks
	InstallMilitaryHooks_SC2K1996();
}
