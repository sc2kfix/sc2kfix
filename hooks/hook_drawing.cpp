// sc2kfix hooks/hook_drawing.cpp: map drawing hooks
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

enum {
	SIZE_TINY,
	SIZE_SMALL,
	SIZE_LARGE,

	SIZE_LEVELS
};

enum {
	AXIS_HORZ,
	AXIS_VERT,

	AXIS_COUNT
};

#define SPRITE_BOUNDARY_MULTIPLIER 500

#define COORDSCALE_VAL(x) (2 << x)
#define LANDALTSCALE_VAL(x) (3 << x)
#define SCALE_VAL(x) (4 << x)

#define SHIP_MIN_DIST 8

#define WATEREDPIPES_SPRITE_OFFSET 116

#define MDRAWING_DEBUG_OTHER 1

#define MDRAWING_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MDRAWING_DEBUG
#define MDRAWING_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

extern BOOL bMapWireFrame;

UINT mdrawing_debug = MDRAWING_DEBUG;

static DWORD dwDummy;

static int DoWaterfallEdge(__int16 iMapOffSetX, int iX, int iY, __int16 iBottom, __int16 iWaterFallSpriteID, __int16 nLandAltScale) {
	__int16 iTopog;

	if (iX < GAME_MAP_SIZE &&
		iY < GAME_MAP_SIZE &&
		XBITReturnIsWater(iX, iY)) {
		iTopog = ALTMReturnWaterLevel(iX, iY) - ALTMReturnLandAltitude(iX, iY);
		if (iTopog > 0) {
			while (rcDst.top <= iBottom) {
				if (rcDst.bottom > iBottom)
					Game_DrawProcessObject(iWaterFallSpriteID, iMapOffSetX, iBottom, 0, 0);
				iBottom -= nLandAltScale;
				if (--iTopog <= 0)
					return 2;
			}
			return 0;
		}
	}
	return 1;
}

static void DoMapEdge(__int16 iMapOffSetX, int iX, int iY, __int16 iBottom, WORD iLandAlt, __int16 iSpriteID, __int16 iWaterFallSpriteID, __int16 nLandAltScale) {
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		if (iLandAlt > 0) {
			while (rcDst.top <= iBottom) {
				if (rcDst.bottom > iBottom)
					Game_DrawProcessObject(iSpriteID, iMapOffSetX, iBottom, 0, 0);
				iBottom -= nLandAltScale;
				if (--iLandAlt <= 0) {
					DoWaterfallEdge(iMapOffSetX, iX, iY, iBottom, iWaterFallSpriteID, nLandAltScale);
					break;
				}
			}
		}
	}
}

static void DoEdgeCoveragePortion(__int16 iX, __int16 iY, __int16 iSpriteID, __int16 iWaterFallSpriteID, __int16 nCoordsScale, __int16 nLandAltScale, __int16 nScale) {
	WORD iLandAlt;
	__int16 iMapOffSetX;
	__int16 iBottom;
	
	iLandAlt = ALTMReturnLandAltitude(iX, iY);
	iMapOffSetX = iScreenOffSetX + nScale * (iX - iY);
	iBottom = iScreenOffSetY + nCoordsScale * (iX + iY) - pArrSpriteHeaders[iSpriteID].wHeight;
	DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, iSpriteID, iWaterFallSpriteID, nLandAltScale);
}

static BOOL DoSpecificEdge(int iX, int iY, BYTE iCoverage, BYTE iZone, BYTE iTile) {
	if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					// This accounts for the >= 2x2 building cases.
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						if (iCoverage)
							return TRUE;
					}
					else {
						// No edge drawing should occur here, otherwise
						// you again end up with the original bleed case.
						return -1;
					}
				}
			}
		}
		else {
			if (DisplayLayer[LAYER_INFRANATURE]) {
				if (iTile >= TILE_HIGHWAY_HTB && iTile < TILE_SUBTORAIL_T) {
					// This accounts for the explicit 2x2 highway-type tile
					// cases.
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						if (iCoverage)
							return TRUE;
					}
					else {
						// No edge drawing should occur here, otherwise
						// you again end up with the original bleed case.
						return -1;
					}
				}
			}
		}
	}
	return FALSE;
}

static void DoCoverageMapEdgeFill(int iX, int iY, __int16 nSprBedrock, __int16 nSprWaterfall, __int16 nCoordsScale, __int16 nLandAltScale, __int16 nScale, BYTE iCoverage, BYTE iTile) {
	__int16 iCoverageSize;
	int iCoverX;

	if (iCoverage >= COVERAGE_2x2 && iCoverage < COVERAGE_COUNT) {
		iCoverageSize = iCoverage + 1;
		iCoverX = iX + iCoverage;
		if (iCoverX == MAP_EDGE_MAX) {
			for (int iOffY = iCoverageSize - 1; iOffY >= 0; iOffY--)
				DoEdgeCoveragePortion(iCoverX, iY - iOffY, nSprBedrock, nSprWaterfall, nCoordsScale, nLandAltScale, nScale);
		}
		if (iY == MAP_EDGE_MAX) {
			for (int iOffX = 0; iOffX < iCoverageSize; iOffX++)
				DoEdgeCoveragePortion(iX + iOffX, iY, nSprBedrock, nSprWaterfall, nCoordsScale, nLandAltScale, nScale);
		}
	}
}

static __int16 GetTerrainSprite(BYTE iTerrainTile, __int16 nSprStart) {
	__int16 iSprite;

	if (iTerrainTile >= TERRAIN_13 || !bMapWireFrame)
		iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
	else
		iSprite = wXTERToXUNDSpriteIDMap[iTerrainTile] + nSprStart;

	return iSprite;
}

static void DoHighwayCoverageFill(int iX, int iY, __int16 nSprStart, __int16 nCoordsScale, __int16 nLandAltScale, __int16 nScale) {
	__int16 iRight;
	__int16 iBottom;
	__int16 iAltTop;
	__int16 iTop;
	__int16 iSprite;
	BYTE iTerrainTile;

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iRight = iScreenOffSetX + nScale * (iX - iY);
	iBottom = iScreenOffSetY + nCoordsScale * (iX + iY) - nLandAltScale * iAltTop;
	
	iSprite = GetTerrainSprite(iTerrainTile, nSprStart);
	iTop = iBottom - pArrSpriteHeaders[iSprite].wHeight;
	Game_DrawProcessObject(iSprite, iRight, iTop, 0, 0);
}

static BOOL IsSpecificUnderDraw(int iX, int iY, BYTE iTile) {
	if (GetTileCoverage(iTile)) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !XZONReturnZone(iX, iY)) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle))
						return TRUE;
					else
						return -1;
				}
			}
		}
		else {
			if (DisplayLayer[LAYER_INFRANATURE]) {
				if (iTile >= TILE_HIGHWAY_HTB && iTile < TILE_SUBTORAIL_T) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle))
						return TRUE;
					else
						return -1;
				}
			}
		}
	}

	return FALSE;
}

static BYTE GetUnderlayTileID(__int16 iX, __int16 iY) {
	BYTE iUnderTile = GetUndergroundTileID(iX, iY);
	if (iUnderTile >= UNDER_TILE_PIPES_LR && iUnderTile < UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
		iUnderTile == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
		iUnderTile == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB) {
		if (iUnderTile == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR)
			iUnderTile = UNDER_TILE_SUBWAY_LR;
		else if (iUnderTile == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB)
			iUnderTile = UNDER_TILE_SUBWAY_TB;
		else
			return 0;
		return iUnderTile;
	}
	else if (iUnderTile >= UNDER_TILE_SUBWAY_LR && iUnderTile <= UNDER_TILE_SUBWAY_LTBR || iUnderTile >= UNDER_TILE_UNKNOWN) {
		if (iUnderTile == UNDER_TILE_MISSILESILO) {
			if (!DisplayLayer[LAYER_BUILDINGS] && !DisplayLayer[LAYER_ZONES])
				return 0;
		}
		return iUnderTile;
	}
	return 0;
}

static void DrawUnderCoveragePortion(int iX, int iY, __int16 nSprStart, __int16 nCoordsScale, __int16 nLandAltScale, __int16 nScale) {
	__int16 iRight;
	__int16 iBottom;
	__int16 iSprBottom;
	__int16 iUndTrnSpr;
	__int16 iSprite;
	BYTE iTerrainTile;
	BYTE iUnderTile;

	iTerrainTile = GetTerrainTileID(iX, iY);
	iUndTrnSpr = wXTERToXUNDSpriteIDMap[iTerrainTile];
	iRight = iScreenOffSetX + nScale * (iX - iY);
	iBottom = iScreenOffSetY + nCoordsScale * (iX + iY) - nLandAltScale * ALTMReturnLandAltitude(iX, iY);
	if (iBottom + nScale >= rcDst.top && rcDst.bottom >= iBottom) {
		iSprBottom = iBottom - pArrSpriteHeaders[nSprStart + iUndTrnSpr].wHeight;
		iUnderTile = GetUnderlayTileID(iX, iY);
		if (iUnderTile) {
			iSprite = iUnderTile + nSprStart + SPRITE_SMALL_BEDROCK_OUTLINE;
			Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
		}
	}
}

// This accounts for the tunnels, subways (underground station portion and tunnels)
// and the missile silo case, the rest is only available in the normal underground mode.
static void DoUndergroundAspects(int iX, int iY, __int16 nSprStart, __int16 nSizeLevel) {
	__int16 nCoordsScale;
	__int16 nLandAltScale;
	__int16 nScale;
	__int16 iRight;
	__int16 iBottom;
	__int16 iSprBottom;
	WORD iTunnelLvl;
	__int16 iUndTrnSpr;
	__int16 iSprite;
	BYTE iTerrainTile;
	BYTE iUnderTile;
	BYTE iTile;
	BYTE iCoverage;
	BOOL bSpecificUnderDraw;

	if (!bMapWireFrame)
		return;

	nCoordsScale = COORDSCALE_VAL(nSizeLevel);
	nLandAltScale = LANDALTSCALE_VAL(nSizeLevel);
	nScale = SCALE_VAL(nSizeLevel);

	iTerrainTile = GetTerrainTileID(iX, iY);
	iUndTrnSpr = wXTERToXUNDSpriteIDMap[iTerrainTile];
	iTunnelLvl = ALTMReturnTunnelLevels(iX, iY);
	iRight = iScreenOffSetX + nScale * (iX - iY);
	iBottom = iScreenOffSetY + nCoordsScale * (iX + iY) - nLandAltScale * ALTMReturnLandAltitude(iX, iY);

	// Note: There used to be an if check here for:
	// (iBottom + nScale >= scDst.top && rcDst.bottom >= iBottom)
	//
	// It used to contain all of the following code.
	//
	// It was removed in-order to account for certain sprite
	// despawning cases.

	iSprBottom = iBottom - pArrSpriteHeaders[nSprStart + iUndTrnSpr].wHeight;
	if (iTunnelLvl) {
		if (DisplayLayer[LAYER_INFRANATURE]) {
			if (iTunnelLvl == 1) {
				iSprite = iTerrainTile + nSprStart + WATERFALL;
				Game_DrawProcessObject(iSprite, iRight, iBottom - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
			else {
				iSprite = nSprStart + SPRITE_SMALL_MISSILESILO;
				Game_DrawProcessObject(iSprite, iRight, iBottom + nLandAltScale * (iTunnelLvl - 1) - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
	}
	iTile = GetTileID(iX, iY);
	bSpecificUnderDraw = IsSpecificUnderDraw(iX, iY, iTile);
	iUnderTile = GetUndergroundTileID(iX, iY);
	if (iUnderTile >= UNDER_TILE_PIPES_LR && iUnderTile < UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
		iUnderTile == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
		iUnderTile == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB) {
		if (iUnderTile == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR)
			iUnderTile = UNDER_TILE_SUBWAY_LR;
		else if (iUnderTile == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB)
			iUnderTile = UNDER_TILE_SUBWAY_TB;
		else
			return;

		if (bSpecificUnderDraw == 0) {
			iSprite = iUnderTile + nSprStart + SPRITE_SMALL_BEDROCK_OUTLINE;
			Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
		}
	}
	else if (iUnderTile) {
		iSprite = iUnderTile + nSprStart + SPRITE_SMALL_BEDROCK_OUTLINE;
		if (iUnderTile == UNDER_TILE_MISSILESILO) {
			if (DisplayLayer[LAYER_ZONES]) {
				if (bSpecificUnderDraw == 0)
					Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
			}
		}
		else {
			if (bSpecificUnderDraw == 0)
				Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
		}
	}

	if (bSpecificUnderDraw > 0) {
		iCoverage = GetTileCoverage(iTile);
		if (iCoverage >= COVERAGE_2x2 && iCoverage < COVERAGE_COUNT) {
			BYTE iCoverageSize = iCoverage + 1;
			for (int iOffX = 0; iOffX < iCoverageSize; iOffX++) {
				for (int iOffY = iCoverageSize - 1; iOffY >= 0; iOffY--)
					DrawUnderCoveragePortion(iX + iOffX, iY - iOffY, nSprStart, nCoordsScale, nLandAltScale, nScale);
			}
		}
	}
}

static void L_DrawTile_SC2K1996(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	__int16 nSizeLevel;
	__int16 nLandAltScale;
	__int16 nCoordsScale, nScale;
	__int16 nSprBedrock, nSprWaterfall;
	__int16 nSprStart, nSprWaterTer;
	__int16 nSprGreenTile, nSprFireStart;
	__int16 nSprPowerInd;
	__int16 nSprHighway, nSprSuspBridge;

	__int16 iBottom;
	__int16 iTop;
	__int16 iIndTop;
	__int16 iSprTop;
	WORD iAltTop;
	WORD iLandAlt;
	__int16 iSprite;
	__int16 iTrafficSprite, iTrafficSpriteOffset;
	__int16 iThing;
	BOOL bSpecificEdge;
	BOOL bIsFlipped;
	BYTE iTerrainTile;
	BYTE iTile;
	BYTE iZone;
	BYTE iCoverage;
	BYTE iTraffic, iLowTrfThreshold, iHeavyTrfThreshold;
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView = NULL;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp)
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (!pSCView)
		return;

	nSizeLevel = pSCView->wSCVZoomLevel;

	nCoordsScale = COORDSCALE_VAL(nSizeLevel);
	nLandAltScale = LANDALTSCALE_VAL(nSizeLevel);
	nScale = SCALE_VAL(nSizeLevel);

	nSprStart = SPRITE_BOUNDARY_MULTIPLIER * nSizeLevel;
	nSprBedrock = nSprStart + SPRITE_SMALL_BEDROCK;
	nSprWaterfall = nSprStart + SPRITE_SMALL_WATERFALL;
	nSprWaterTer = nSprStart + SPRITE_SMALL_WATER_R_TERRAIN_TBL;
	nSprGreenTile = nSprStart + SPRITE_SMALL_GREENTILE;
	nSprFireStart = nSprStart + SPRITE_SMALL_FIRE4;
	nSprPowerInd = nSprStart + SPRITE_SMALL_POWEROUTAGEINDICATOR;
	nSprHighway = nSprStart + SPRITE_SMALL_HIGHWAY_LR;
	nSprSuspBridge = nSprStart + SPRITE_SMALL_SUSPENSION_BRIDGE_START_B;

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = iMapOffSetY - pArrSpriteHeaders[nSprBedrock].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(iMapOffSetX, iX, iY, iBottom, nSprWaterfall, nLandAltScale))
				return;
		}
	}

	// Note: Originally there was a general 'DoMapEdge' processing
	// call here, however it has now been moved so the calls only
	// occur where they're needed for greater control - such as
	// when an alternative call is required in order to avoid
	// map-edge bleed on >= 2x2 buildings.

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = iMapOffSetY - nLandAltScale * iAltTop;
	iTile = GetTileID(iX, iY);
	iZone = XZONReturnZone(iX, iY);
	iCoverage = GetTileCoverage(iTile);

	DoUndergroundAspects(iX, iY, nSprStart, nSizeLevel);

	// -------- Move the edge-drawing here with a specific check for the coverage version.
	bSpecificEdge = DoSpecificEdge(iX, iY, iCoverage, iZone, iTile);
	if (bSpecificEdge >= 0) {
		if (bSpecificEdge) {
			// The following call is to account for >= 2x2 buildings and avoid the map-edge
			// bedrock-bleed that was previously occurring.
			DoCoverageMapEdgeFill(iX, iY, nSprBedrock, nSprWaterfall, nCoordsScale, nLandAltScale, nScale, iCoverage, iTile);
		}
		else
			DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nLandAltScale);
	}
	// ^ -------- Move the edge-drawing here with a specific check for the coverage version.

	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;

	if (iTile == TILE_CLEAR) {
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = GetTerrainSprite(iTerrainTile, nSprStart);
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
	}
	else if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						// Point of interest here when it comes to
						// the bIsFlipped negation depending on
						// view rotation; if this is adjusted then
						// certain tiles such as runway and pier cases
						// will need to be excluded to avoid a rather
						// bizarre view.
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						iSprite = iTile + nSprStart;
						Game_DrawProcessObject(iSprite, iMapOffSetX, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - nCoordsScale, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nScale, iTop - nScale, 0, 0);
						}
					}
				}
				else {
					iSprite = iZone + nSprWaterTer;
					Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + nSprGreenTile;
				else
					iSprite = iZone + nSprWaterTer;
				Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			iSprite = GetTerrainSprite(iTerrainTile, nSprStart);
			if (DisplayLayer[LAYER_INFRANATURE]) {
				if (iTile >= TILE_HIGHWAY_HTB && iTile < TILE_SUBTORAIL_T) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
						BYTE iCoverageSize = iCoverage + 1;
						for (int iOffX = 0; iOffX < iCoverageSize; iOffX++) {
							for (int iOffY = iCoverageSize - 1; iOffY >= 0; iOffY--)
								DoHighwayCoverageFill(iX + iOffX, iY - iOffY, nSprStart, nCoordsScale, nLandAltScale, nScale);
						}
						// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
						iSprite = iTile + nSprStart;
						iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + nCoordsScale;
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop, bIsFlipped, 0);
						iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
						iLowTrfThreshold = 28;
						iHeavyTrfThreshold = iLowTrfThreshold * 2;
						if (iTraffic > iLowTrfThreshold) {
							iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
							if (iTraffic > iHeavyTrfThreshold)
								iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
							if (iTrafficSpriteOffset)
								iTrafficSprite = iTrafficSpriteOffset + nSprFireStart;
							Game_DrawProcessMaskObject(iTrafficSprite, iMapOffSetX, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
						}
					}
				}
				else {
					Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					if (iTile == TILE_RAISING_BRIDGE_LOWERED) {
						if (wActiveShips) {
							for (iThing = MIN_THING_IDX; iThing < MAX_THING_COUNT; ++iThing) {
								if (XTHGGetType(iThing) == XTHG_CARGO_SHIP)
									break;
							}
							if (iThing != MAX_THING_COUNT) {
								map_XTHG_t *pThing = GetXTHG(iThing);

								if (pThing) {
									__int16 iDestDist = Game_GetDestDistance(iX, iY, pThing->iX, pThing->iY);
									if (iDestDist < SHIP_MIN_DIST)
										iTile = TILE_RAISING_BRIDGE_RAISED;
								}
							}
						}
					}
					// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					iSprite = iTile + nSprStart;
					iSprTop = iMapOffSetY - nLandAltScale * iAltTop;
					if (iTerrainTile == TERRAIN_13)
						iSprTop = iTop - nLandAltScale;
					iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
					if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
						bIsFlipped = FALSE;
					else
						bIsFlipped = XBITReturnIsFlipped(iX, iY);
					Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop, bIsFlipped, 0);
					iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
					if (iSprite < nSprHighway || iSprite >= nSprSuspBridge)
						iLowTrfThreshold = 85;
					else
						iLowTrfThreshold = 28;
					iHeavyTrfThreshold = iLowTrfThreshold * 2;
					if (iTraffic > iLowTrfThreshold) {
						iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
						if (iTrafficSpriteOffset) {
							if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
								bIsFlipped = FALSE;
							else
								bIsFlipped = XBITReturnIsFlipped(iX, iY);
							if (iTrafficSpriteOffset == 11) {
								if (!IsEven(iX))
									iTrafficSpriteOffset = 12;
							}
							else if (iTrafficSpriteOffset == 12) {
								bIsFlipped = TRUE;
								if (!IsEven(iY))
									iTrafficSpriteOffset = 11;
							}
							if (iTraffic > iHeavyTrfThreshold)
								iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
							iTrafficSprite = iTrafficSpriteOffset + nSprFireStart;
							Game_DrawProcessMaskObject(iTrafficSprite, iMapOffSetX, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
						}
					}
					if (iX < GAME_MAP_SIZE &&
						iY < GAME_MAP_SIZE &&
						XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
						iIndTop = iTop;
						if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iY) && iTile == TILE_ELEVATED_POWERLINES)
							iIndTop -= nLandAltScale;
						iIndTop = iIndTop - nScale;
						Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nScale, iIndTop, 0, 0);
					}
				}
			}
			else
				Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		}
	}
	else {
		iSprTop = iMapOffSetY - nLandAltScale * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - nLandAltScale;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = GetTerrainSprite(iTerrainTile, nSprStart);
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (DisplayLayer[LAYER_INFRANATURE]) {
			iSprite = iTile + nSprStart;
			Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			if (iX < GAME_MAP_SIZE &&
				iY < GAME_MAP_SIZE &&
				XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
				iIndTop = iTop;
				if (iTerrainTile >= TERRAIN_01 && iTerrainTile <= TERRAIN_13)
					iIndTop -= nLandAltScale;
				iIndTop = iIndTop - nScale;
				Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nScale, iIndTop, 0, 0);
			}
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabelsAndObjects(iX, iY, iMapOffSetX, iTop);
}

extern "C" void __stdcall Hook_DrawAllLarge() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iMapOffSetX, iMapOffSetY;
	__int16 iScan, iX, iY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		rcDst.left -= 128;
		rcDst.bottom += 220;
		rcDst.top -= 32;

		// Top half
		iScan = MAP_EDGE_MIN;
		iX = MAP_EDGE_MIN;
		iY = MAP_EDGE_MIN;
		iMapOffSetX = iScreenOffSetX;
		iMapOffSetY = iScreenOffSetY;
		do {
			if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
				Game_DrawLargeTile(iMapOffSetX, iMapOffSetY, iX, iY);
			}
			if (iY) {
				++iX;
				--iY;
				iMapOffSetX += 32;
			}
			else {
				++iScan;
				iX = MAP_EDGE_MIN;
				iY = iScan;
				iMapOffSetX = iScreenOffSetX - 16 * iScan;
				iMapOffSetY += 8;
			}
		} while (iScan < GAME_MAP_SIZE);

		// Bottom half
		iScan = MAP_EDGE_MIN + 1;
		iX = MAP_EDGE_MIN + 1;
		iY = MAP_EDGE_MAX;
		iMapOffSetX = iScreenOffSetX - 2016;
		iMapOffSetY = iScreenOffSetY + 1024;
		do {
			if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
				Game_DrawLargeTile(iMapOffSetX, iMapOffSetY, iX, iY);
			}
			if (iX == MAP_EDGE_MAX) {
				++iScan;
				iX = iScan;
				iY = MAP_EDGE_MAX;
				iMapOffSetX = 16 * (iScan - MAP_EDGE_MAX) + iScreenOffSetX;
				iMapOffSetY += 8;
			}
			else {
				++iX;
				--iY;
				iMapOffSetX += 32;
			}
		} while (iScan < GAME_MAP_SIZE);
	}
}

extern "C" void __cdecl Hook_DrawLargeTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY);
}

extern "C" void __cdecl Hook_DrawSmallTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY);
}

extern "C" void __cdecl Hook_DrawTinyTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY);
}

extern "C" void __stdcall Hook_DrawAllUnder() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 nBottomScale;
	__int16 nBottomRcScale;
	__int16 nLeftScale;
	__int16 iMapOffSetX;
	__int16 iScan, iX, iY;

	// Notes:
	// 1) The nLeftScale multiplier was previously 4.
	// 2) There was no nBottomRcScale - however we
	//    don't want to use this everywhere.
	// 3) All zoom levels originally used the same
	//    calculation for nLeftScale.
	// 4) iMapOffSetX was previously incremented with
	//    nLeftScale.
	//
	// Certain adjustments here have been made to avoid
	// various sprite despawning cases (bridges, tunnels).

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			nBottomScale = 2 * COORDSCALE_VAL(pSCView->wSCVZoomLevel);
			if (pSCView->wSCVZoomLevel && pSCView->wSCVZoomLevel != 1) {
				nBottomRcScale = 220;
				nLeftScale = 128;
			}
			else {
				nBottomRcScale = 2 * COORDSCALE_VAL(pSCView->wSCVZoomLevel);
				nLeftScale = 6 * COORDSCALE_VAL(pSCView->wSCVZoomLevel);
			}

			rcDst.left -= nLeftScale;
			rcDst.bottom += nBottomRcScale;

			// Top half
			iScan = MAP_EDGE_MIN;
			iX = MAP_EDGE_MIN;
			iY = MAP_EDGE_MIN;
			iMapOffSetX = iScreenOffSetX;
			do {
				if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
					Game_DrawUnderTile(iX, iY);
				}
				if (iY) {
					++iX;
					--iY;
					iMapOffSetX += nBottomScale * 2;
				}
				else {
					++iScan;
					iX = MAP_EDGE_MIN;
					iY = iScan;
					iMapOffSetX = iScreenOffSetX - nBottomScale * iScan;
				}
			} while (iScan < GAME_MAP_SIZE);

			// Bottom half
			iScan = MAP_EDGE_MIN + 1;
			iX = MAP_EDGE_MIN + 1;
			iY = MAP_EDGE_MAX;
			iMapOffSetX = iScreenOffSetX - 126 * nBottomScale;
			do {
				if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
					Game_DrawUnderTile(iX, iY);
				}
				if (iX == MAP_EDGE_MAX) {
					++iScan;
					iX = iScan;
					iY = MAP_EDGE_MAX;
					iMapOffSetX = nBottomScale * (iScan - MAP_EDGE_MAX) + iScreenOffSetX;
				}
				else {
					++iX;
					--iY;
					iMapOffSetX += nBottomScale * 2;
				}
			} while (iScan < GAME_MAP_SIZE);
		}
	}
}

extern "C" void __cdecl Hook_DrawUnderTile(__int16 iX, __int16 iY) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 nCoordsScale;
	__int16 nLandAltScale;
	__int16 nScale;
	__int16 nSpriteStart;
	__int16 iRight;
	__int16 iBottom;
	__int16 iTop;
	WORD iAltTop;
	__int16 iUndTrnSpr;
	WORD iTunnelLvl;
	__int16 iSprBottom;
	__int16 iSprPwrIndRight;
	__int16 iSprite;
	BOOL bIsFlipped;
	BYTE iTerrainTile;
	BYTE iUnderTile;
	BYTE iTile;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			nCoordsScale = COORDSCALE_VAL(pSCView->wSCVZoomLevel);
			nLandAltScale = LANDALTSCALE_VAL(pSCView->wSCVZoomLevel);
			nScale = SCALE_VAL(pSCView->wSCVZoomLevel);
			nSpriteStart = SPRITE_BOUNDARY_MULTIPLIER * pSCView->wSCVZoomLevel;

			iTerrainTile = GetTerrainTileID(iX, iY);
			iRight = iScreenOffSetX + nScale * (iX - iY);
			iBottom = iScreenOffSetY + nCoordsScale * (iX + iY) - nLandAltScale * ALTMReturnLandAltitude(iX, iY);

			// Note: There used to be an if check here for:
			// (iBottom + nScale >= scDst.top && rcDst.bottom >= iBottom)
			//
			// It used to contain all of the following code.
			//
			// It was removed in-order to account for certain sprite
			// despawning cases.

			// ------ Added to reconcile the sign height difference between above/underground layers.
			if (iTerrainTile < SUBMERGED_00)
				iAltTop = ALTMReturnLandAltitude(iX, iY);
			else
				iAltTop = ALTMReturnWaterLevel(iX, iY);
			iTop = iScreenOffSetY + nCoordsScale * (iX + iY) - nLandAltScale * iAltTop;
			// ^ ------ Added to reconcile the sign height difference between above/underground layers.

			iUndTrnSpr = wXTERToXUNDSpriteIDMap[iTerrainTile];
			iTunnelLvl = ALTMReturnTunnelLevels(iX, iY);
			iSprBottom = iBottom - pArrSpriteHeaders[nSpriteStart + iUndTrnSpr].wHeight;
			if (iTunnelLvl) {
				if (iTunnelLvl == 1) {
					iSprite = iTerrainTile + nSpriteStart + WATERFALL;
					Game_DrawProcessObject(iSprite, iRight, iBottom - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
				else {
					iSprite = nSpriteStart + SPRITE_SMALL_MISSILESILO;
					Game_DrawProcessObject(iSprite, iRight, iBottom + nLandAltScale * (iTunnelLvl - 1) - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			iUnderTile = GetUndergroundTileID(iX, iY);
			if (iUnderTile >= UNDER_TILE_PIPES_LR && iUnderTile < UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
				iUnderTile == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
				iUnderTile == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB) {
				if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE &&
					XBITReturnIsPiped(iX, iY) && XBITReturnIsWatered(iX, iY))
					iUnderTile += WATEREDPIPES_SPRITE_OFFSET;
				iSprite = iUnderTile + nSpriteStart + SPRITE_SMALL_BEDROCK_OUTLINE;
				Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
			}
			else if (iUnderTile) {
				iSprite = iUnderTile + nSpriteStart + SPRITE_SMALL_BEDROCK_OUTLINE;
				Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
				if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE) {
					if (XBITReturnIsPiped(iX, iY)) {
						if (XBITReturnIsWatered(iX, iY))
							iSprite = nSpriteStart + SPRITE_SMALL_BUILDINGPIPEWATERED_TRBL;
						else
							iSprite = nSpriteStart + SPRITE_SMALL_BUILDINGPIPE;
						Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
					}
				}
			}
			else {
				iSprite = iUndTrnSpr + nSpriteStart;
				if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE) {
					if (XBITReturnIsPiped(iX, iY)) {
						if (XBITReturnIsWatered(iX, iY))
							iSprite = nSpriteStart + SPRITE_SMALL_BUILDINGPIPEWATERED_TRBL;
						else
							iSprite = nSpriteStart + SPRITE_SMALL_BUILDINGPIPE;
					}
				}
				Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
			}
			iTile = GetTileID(iX, iY);
			if (iTile) {
				if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
					if (DisplayLayer[LAYER_INFRANATURE]) {
						if (iTile >= TILE_HIGHWAY_HTB &&
							iTile < TILE_SUBTORAIL_T) {
							if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
								iSprite = nSpriteStart + iTile;
								if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iY))
									iBottom += nLandAltScale * (ALTMReturnLandAltitude(iX, iY) - ALTMReturnWaterLevel(iX, iY));
								iSprBottom = iBottom + nCoordsScale - pArrSpriteHeaders[iSprite].wHeight;
								if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
									bIsFlipped = FALSE;
								else
									bIsFlipped = XBITReturnIsFlipped(iX, iY);
								Game_DrawProcessObject(iSprite, iRight, iSprBottom, bIsFlipped, 0);
							}
						}
						else if (iTile >= TILE_POWERLINES_LR) {
							iSprite = nSpriteStart + iTile;
							if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iY))
								iBottom += nLandAltScale * (ALTMReturnLandAltitude(iX, iY) - ALTMReturnWaterLevel(iX, iY));
							else if (iUndTrnSpr == SPRITE_SMALL_BEDROCK_OUTLINE)
								iBottom -= nLandAltScale;
							iSprBottom = iBottom - pArrSpriteHeaders[iSprite].wHeight;
							if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
								bIsFlipped = FALSE;
							else
								bIsFlipped = XBITReturnIsFlipped(iX, iY);
							Game_DrawProcessObject(iSprite, iRight, iSprBottom, bIsFlipped, 0);
							if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE &&
								XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
								iSprPwrIndRight = iRight + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nScale;
								if ((iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iY) && iTile == TILE_ELEVATED_POWERLINES) ||
									(iTerrainTile >= TERRAIN_01 && iTerrainTile < TERRAIN_13))
									iBottom -= nLandAltScale;
								iSprBottom = iBottom - nScale;
								Game_DrawProcessObject(nSpriteStart + SPRITE_SMALL_POWEROUTAGEINDICATOR, iSprPwrIndRight, iSprBottom, 0, 0);
							}
						}
					}
				}
				else {
					if (DisplayLayer[LAYER_BUILDINGS]) {
						iSprite = BuiltUpZones[XZONReturnZone(iX, iY)] + nSpriteStart + SPRITE_SMALL_GREENOUTLINE;
						Game_DrawProcessObject(iSprite, iRight, iSprBottom, 0, 0);
						if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							iSprPwrIndRight = iRight + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nScale;
							iSprBottom = iBottom - nScale;
							Game_DrawProcessObject(nSpriteStart + SPRITE_SMALL_POWEROUTAGEINDICATOR, iSprPwrIndRight, iSprBottom, 0, 0);
						}
					}
				}
			}

			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabelsAndObjects(iX, iY, iRight, iTop);

		}
	}
}

extern "C" void __cdecl Hook_DrawColorTile(__int16 iX, __int16 iY) {
	WORD iAltTop;
	__int16 iTop;
	__int16 iBottom;
	__int16 iBaseSprite;
	__int16 iSprite;
	__int16 iSpriteInd;
	BYTE iTerrainTile;
	BYTE bBlock;

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = g_iColorMapOffSetY - g_wColorLandAltScale * iAltTop;
	iBaseSprite = nXTERTileIDs[iTerrainTile];
	iSprite = g_wColorSpriteStart + iBaseSprite;
	if (rcDst.top <= iTop && rcDst.bottom >= iTop) {
		iBottom = iTop - pArrSpriteHeaders[iSprite].wHeight;
		if (iBaseSprite == SPRITE_SMALL_TERRAIN) {
			bBlock = 0;
			iSpriteInd = 0;
			switch (EditData) {
				case EDIT_DATA_TRAFFIC:
					bBlock = GetXTRFByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_POPDENSITY:
					bBlock = GetXPOPByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_RATEOFGROWTH1:
					bBlock = GetXROGByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_CRIMERATE:
					bBlock = GetXCRMByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_POLICEPWR:
					bBlock = GetXPLCByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_POLLUTION:
					bBlock = GetXPLTByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_LANDVALUE:
					bBlock = GetXVALByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_FIREPWR:
					bBlock = GetXFIRByteDataWithNormalCoordinates(iX, iY);
					break;
				case EDIT_DATA_POWERED:
					if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE) {
						if (XBITReturnIsPowerable(iX, iY)) {
							if (XBITReturnIsPowered(iX, iY))
								iSpriteInd = SPRITE_SMALL_GREENTILE;
							else
								iSpriteInd = SPRITE_SMALL_REDTILE;
						}
					}
					break;
				case EDIT_DATA_WATERED:
					if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE) {
						if (XBITReturnIsPiped(iX, iY)) {
							if (XBITReturnIsWatered(iX, iY))
								iSpriteInd = SPRITE_SMALL_GREENTILE;
							else
								iSpriteInd = SPRITE_SMALL_REDTILE;
						}
					}
					break;
				case EDIT_DATA_RATEOFGROWTH2:
					bBlock = GetXROGByteDataWithNormalCoordinates(iX, iY);
					if (bBlock < 131) {
						if (bBlock <= 124)
							iSpriteInd = SPRITE_SMALL_REDNEGATIVE;
					}
					else
						iSpriteInd = SPRITE_SMALL_GREENPLUS;
					break;
			}
			if (EditData <= EDIT_DATA_FIREPWR) {
				if (bBlock >= 16)
					iSprite = g_wColorSpriteStart + (bBlock >> 5) + SPRITE_SMALL_DENSITYOVERLAY1;
			}
			else {
				if (iSpriteInd)
					iSprite = g_wColorSpriteStart + iSpriteInd;
			}
		}
		// ----- Enabled the underground layer.
		if (DisplayLayer[LAYER_UNDERGROUND]) {
			if (iSprite - g_wColorSpriteStart == iBaseSprite) {
				iSprite = g_wColorSpriteStart + wXTERToXUNDSpriteIDMap[iTerrainTile];
				iBottom = g_iColorMapOffSetY - g_wColorLandAltScale * ALTMReturnLandAltitude(iX, iY) - pArrSpriteHeaders[iSprite].wHeight;
			}
		}
		// ^ ----- Enabled the underground layer.
		Game_DrawProcessObject(iSprite, g_iColorMapOffSetX, iBottom, 0, 0);
	}
}

extern "C" __int16 __cdecl Hook_PointToTile(__int16 x, __int16 y) {
	CSimcityAppPrimary* pSCApp = &pCSimcityAppThis;
	CSimcityView* pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	__int16 ptX = x;
	__int16 ptY = y;
	__int16 retval = 0;

	if (pSCView->dwSCVIsZoomed) {
		// TODO: multiple scales of zoomage
		ptX >>= 1;
		ptY >>= 1;
	}

	__int16 iOffsetAdjustmentCenter = (ptX - iScreenOffSetX - 6) >> 1;
	__int16 iOffsetAdjustmentX = ptY - iOffsetAdjustmentCenter - iScreenOffSetY - 1;
	__int16 iOffsetAdjustmentY = ptY - iScreenOffSetY + iOffsetAdjustmentCenter - 3;

	if (pSCView->wSCVZoomLevel) {
		if (pSCView->wSCVZoomLevel == 1)
			retval = Game_CalcTileHit8(iOffsetAdjustmentY + 12, iOffsetAdjustmentX + 12);
		else if (pSCView->wSCVZoomLevel == 2)
			retval = Game_CalcTileHit16(iOffsetAdjustmentY + 16, iOffsetAdjustmentX + 24);
		else
			retval = 0;
	} else
		retval = Game_CalcTileHit4(iOffsetAdjustmentY + 8, iOffsetAdjustmentX + 6);
	
	if (HIBYTE(retval) < MAP_EDGE_MIN || LOBYTE(retval) > MAP_EDGE_MAX)
		return -1;
	return retval;
}

void L_CheckTileHighlight_SC2K1996(CSimcityView *pSCView) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	if (wTileHighlightActive) {
		if (pSCApp->dwSCACursorGameHit)
			Game_SimcityView_MaintainCursor(pSCView);
		else
			Game_SimcityView_TileHighlightRemove(pSCView);
	}
}

void L_DrawHouse_SC2K1996(CSimcityView *pSCView, BOOL bLeaveTileHighlightActive) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	COLORREF cr;

	if (pSCView->SCVGraphics && pSCView->pSCVGraphicLockDIBRes || Game_SimcityView_CheckOrLoadGraphic(pSCView)) {
		curLockedDIBBits = Game_Graphics_LockDIBBits(pSCView->SCVGraphics);
		Game_Graphics_Width(pSCView->SCVGraphics);
		Game_Graphics_Height(pSCView->SCVGraphics);
		Game_BeginProcessObjects(pSCView, curLockedDIBBits, pSCView->dwSCVGraphicWidth, (__int16)pSCView->dwSCVGraphicHeight, &pSCView->SCVAreaView);
		rcDst = pSCView->SCVAreaView;
		theSCVDC = Game_Graphics_GetDC(pSCView->SCVGraphics);
		if (DisplayLayer[LAYER_UNDERGROUND])
			cr = colGameBackgndUnder;
		else
			cr = colGameBackgndAbove;
		SetBkColor(theSCVDC->m_hDC, cr);
		ExtTextOutA(theSCVDC->m_hDC, 0, 0, ETO_OPAQUE, &rcDst, 0, 0, 0);
		Game_Graphics_ReleaseDC(pSCView->SCVGraphics, theSCVDC);
		wCurrentPositionAngle = wPositionAngle[wViewRotation];
		if (!IsIconic(pSCView->m_hWnd) && showColor && EditData)
			Game_DrawAllColor();
		else if (DisplayLayer[LAYER_UNDERGROUND])
			Game_DrawAllUnder();
		else {
			if (pSCView->wSCVZoomLevel) {
				if (pSCView->wSCVZoomLevel == 1)
					Game_DrawAllSmall();
				else
					Game_DrawAllLarge();
			}
			else
				Game_DrawAllTiny();
		}
		if (!bLeaveTileHighlightActive)
			wTileHighlightActive = 0;
		else {
			if (wTileHighlightActive) {
				if (pSCApp->dwSCACursorGameHit)
					Game_SimcityView_DrawSquareHighlight(pSCView, wHighlightedTileX1, wHighlightedTileY1, wHighlightedTileX2, wHighlightedTileY2);
				else
					wTileHighlightActive = 0;
			}
		}
		Game_FinishProcessObjects();
		Game_SimcityView_MainWindowUpdate(pSCView, 0, TRUE);
		Game_UpdateCityMap();
		Game_Graphics_UnlockDIBBits(pSCView->SCVGraphics);
		curLockedDIBBits = 0;
	}
}

extern "C" void __stdcall Hook_SimcityView_DrawHouse() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	L_DrawHouse_SC2K1996(pThis, FALSE);
}

extern __int16 nFastCyclePos;
extern __int16 nMidCyclePos;
extern __int16 nSlowCyclePos;

int cycleIndices[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4, 
	 5,  6,  7,  0,  1,  2,  3,  4,  5,  6,  7,  0,  1,  2,  3,  4,
	 5,  6,  7,  3,  2,  1,  0, -1,  7,  6,  5,  4,  3,  2,  1,  0,
	 0,  1,  2,  3,  7,  6,  5,  4,  3,  2,  1,  0, -1, -1, -1, -1,
	 0,  1,  0,  1,  0,  1,  0,  1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

BYTE fastCycleOne[]   = { 0xC6, 0xC5, 0xC4, 0xC3 };
BYTE fastCycleTwo[]   = { 0xD0, 0xD1, 0xD2, 0xD3 };
BYTE midCycleOne[]    = { 0xE0, 0xE1 };
BYTE midCycleTwo[]    = { 0xE2, 0xE3 };
BYTE midCycleThree[]  = { 0xE4, 0xE5 };
BYTE midCycleFour[]   = { 0xE6, 0xE7 };
BYTE slowCycleOne[]   = { 0xCF, 0xCE, 0xCD, 0xCC, 0xCB, 0xCA, 0xC9, 0xC8 };
BYTE slowCycleTwo[]   = { 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2 };
BYTE slowCycleThree[] = { 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA };
BYTE slowCycleFour[]  = { 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2 };
BYTE slowCycleFive[]  = { 0xDB, 0xDA, 0xD9, 0xD8, 0xD7, 0xD6, 0xD5, 0xD4 };

BYTE GetCycleColIndex(BYTE col, BYTE *pRange, int nCount, int nTarg) {
	BYTE newCol = col;
	if (nTarg) {
		int nIdx = cycleIndices[col];
		if (nIdx >= 0 && pRange[nIdx] == col) {
			nIdx = (nIdx + nTarg) % nCount;
			if (nIdx < 0)
				nIdx = -nIdx;
			newCol = pRange[nIdx];
		}
	}
	return newCol;
}

static BYTE ProcessCyclingIndex(BYTE colIdx) {
	BYTE newIdx = colIdx;
	newIdx = GetCycleColIndex(newIdx, fastCycleOne, sizeof(fastCycleOne), (nFastCyclePos % 4));
	newIdx = GetCycleColIndex(newIdx, fastCycleTwo, sizeof(fastCycleTwo), (nFastCyclePos % 4));
	newIdx = GetCycleColIndex(newIdx, midCycleOne, sizeof(midCycleOne), (nMidCyclePos % 2));
	newIdx = GetCycleColIndex(newIdx, midCycleTwo, sizeof(midCycleTwo), (nMidCyclePos % 2));
	newIdx = GetCycleColIndex(newIdx, midCycleThree, sizeof(midCycleThree), (nMidCyclePos % 2));
	newIdx = GetCycleColIndex(newIdx, midCycleFour, sizeof(midCycleFour), (nMidCyclePos % 2));
	newIdx = GetCycleColIndex(newIdx, slowCycleOne, sizeof(slowCycleOne), (nSlowCyclePos % 8));
	newIdx = GetCycleColIndex(newIdx, slowCycleTwo, sizeof(slowCycleTwo), (nSlowCyclePos % 8));
	newIdx = GetCycleColIndex(newIdx, slowCycleThree, sizeof(slowCycleThree), (nSlowCyclePos % 8));
	newIdx = GetCycleColIndex(newIdx, slowCycleFour, sizeof(slowCycleFour), (nSlowCyclePos % 8));
	newIdx = GetCycleColIndex(newIdx, slowCycleFive, sizeof(slowCycleFive), (nSlowCyclePos % 8));
	return newIdx;
}

static BYTE ProcessWeatherIndex(BYTE colIdx) {
	BYTE newIdx = colIdx;
	// Snow effect - for ground or water tiles.
	if (newIdx == 0x73 || newIdx == 0x79 || newIdx == 0x7F || newIdx == 0x80) // Ground tiles here
		newIdx = 0x9A;
	else if (newIdx == 0x74 || newIdx == 0x7A || newIdx == 0x81)
		newIdx = 0x9B;
	else if (newIdx == 0x75 || newIdx == 0x7B || newIdx == 0x82)
		newIdx = 0x9C;
	else if (newIdx == 0x76 || newIdx == 0x7C || newIdx == 0x85)
		newIdx = 0x9D;
	else if (newIdx == 0x77 || newIdx == 0x7D)
		newIdx = 0x9E;
	else if (newIdx == 0x78 || newIdx == 0x7E)
		newIdx = 0x9F;
	else if (newIdx == 0xC8 || newIdx == 0xCC || newIdx == 0xD0) // Water tiles here
		newIdx = 0x53;
	else if (newIdx == 0xC9 || newIdx == 0xCD || newIdx == 0xD1)
		newIdx = 0x54;
	else if (newIdx == 0xCA || newIdx == 0xCE || newIdx == 0xD2)
		newIdx = 0x55;
	else if (newIdx == 0xCB || newIdx == 0xCF || newIdx == 0xD3)
		newIdx = 0x56;
	return newIdx;
}

static BYTE ProcessTreeSnowEffect(BYTE colIdx) {
	BYTE newIdx = colIdx;
	if (newIdx == 0x3B || newIdx == 0x40 || newIdx == 0x46 || newIdx == 0x50)
		newIdx = 0x9A;
	else if (newIdx == 0x3C || newIdx == 0x41 || newIdx == 0x47 || newIdx == 0x51)
		newIdx = 0x9B;
	else if (newIdx == 0x3D || newIdx == 0x42 || newIdx == 0x48 || newIdx == 0x52)
		newIdx = 0x9C;
	else if (newIdx == 0x3E || newIdx == 0x43 || newIdx == 0x49)
		newIdx = 0x9D;
	else if (newIdx == 0x3F || newIdx == 0x44 || newIdx == 0x4A)
		newIdx = 0x9E;
	else if (newIdx == 0x45)
		newIdx = 0x9F;
	return newIdx;
}

static BYTE ProcessTreeAutumnEffect(BYTE colIdx) {
	BYTE newIdx = colIdx;
	if (newIdx == 0x3B || newIdx == 0x40 || newIdx == 0x46 || newIdx == 0x50)
		newIdx = 0x7D;
	else if (newIdx == 0x3C || newIdx == 0x41 || newIdx == 0x47 || newIdx == 0x51)
		newIdx = 0x7E;
	else if (newIdx == 0x3D || newIdx == 0x42 || newIdx == 0x48 || newIdx == 0x52)
		newIdx = 0x7F;
	else if (newIdx == 0x3E || newIdx == 0x43 || newIdx == 0x49)
		newIdx = 0x28;
	else if (newIdx == 0x3F || newIdx == 0x44 || newIdx == 0x4A)
		newIdx = 0x29;
	else if (newIdx == 0x45)
		newIdx = 0x2A;
	return newIdx;
}

static BYTE ProcessSeasonIndex(BYTE colIdx, BOOL bIgnore = FALSE) {
	int iCityMonth = dwCityDays / 25 % 12;

	BYTE newIdx = colIdx;
	if (bWeatherTrend == 6 ||
		bWeatherTrend == 9) {
		if (!bIgnore)
			newIdx = ProcessTreeSnowEffect(newIdx);
	}
	if ((iCityMonth >= 0 && iCityMonth <= 2) ||
		(iCityMonth >= 9 && iCityMonth <= 11)) {
		newIdx = ProcessTreeAutumnEffect(newIdx);
	}
	return newIdx;
}

static BYTE ProcessSpritePaletteIndex(__int16 nSpriteID, BYTE colIdx, WORD nRemHeight, int nPos) {
	BYTE palIdx = colIdx;
	// Only enable this if the "Frequent Updates" setting is enabled.
	if (bFrequentUpdates) {
		// Proof-of-concept weather experiment.
		if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_TREES1, SPRITE_SMALL_TREES7)) {
			if ((nPos % 4) == 0 || (nPos % 4) == 2 || (nPos % 4) == 3) {
				BOOL bIgnore = FALSE;
				if ((nPos % 4) == 2)
					bIgnore = TRUE;
				palIdx = ProcessSeasonIndex(palIdx, bIgnore);
			}
		}
		else if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_TERRAIN, SPRITE_SMALL_WATER_R_TERRAIN_TBL)) {
			if (bWeatherTrend == 6 || bWeatherTrend == 9) {
				// This if is for partial drawing based on row.
				/*if (nRemHeight <= (shapeCurrent[nSpriteID].wHeight / 2))*/
				if ((nPos % 4) == 3 || (nPos % 4) == 1)
					palIdx = ProcessWeatherIndex(palIdx);
			}
		}
		palIdx = ProcessCyclingIndex(palIdx);
	}
	return palIdx;
}

static BYTE AdjustInversion(__int16 nSpriteID, BYTE palIdx) {
	BYTE newIdx = ~palIdx;
	// In the DOS and Macintosh version the tile inversion
	// colouration was slightly different, this was a result
	// of different palette indices. To compensate all that's
	// required is to increment by 0x20 to get it back into
	// range.
	BOOL bPalOffset = TRUE;
	if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_UNDERGROUND_TERRAIN, SPRITE_SMALL_SUBWAYENTRANCE)) {
		if (bDarkUnderground)
			bPalOffset = FALSE;
	}

	if (bPalOffset)
		newIdx += 0x20;
	return newIdx;
}

static BYTE CheckInversion(__int16 nSpriteID, BYTE palIdx) {
	BYTE newIdx = palIdx;
	
	BOOL bPalOffset = TRUE;
	if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_UNDERGROUND_TERRAIN, SPRITE_SMALL_SUBWAYENTRANCE)) {
		if (bDarkUnderground)
			bPalOffset = FALSE;
	}

	if (bPalOffset)
		newIdx -= 0x20;
	return newIdx;
}

static BYTE CheckWeatherInversion(__int16 nSpriteID, BYTE palIdx, int nPos) {
	BYTE newIdx = palIdx;
	if (!GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_UNDERGROUND_TERRAIN, SPRITE_SMALL_SUBWAYENTRANCE)) {
		if (bWeatherTrend == 6 || bWeatherTrend == 9) {
			if ((nPos % 4) == 3 || (nPos % 4) == 1)
				newIdx = ProcessWeatherIndex(newIdx);
		}
	}
	return newIdx;
}

static void L_drawShape_Invert_MainArea(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom) {
	BYTE *pShapeBitsLine, *spritePtr, *pShapeBits;
	BYTE nCount;
	BYTE nChunkMode;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	spritePtr = shapePtr;
	pShapeBits = pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			break;
		case MIF_CM_SKIPPIXELS:
			pShapeBits += nCount;
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				if (CheckWeatherInversion(nSpriteID, *pShapeBits, nPos) == CheckWeatherInversion(nSpriteID, *spritePtr, nPos))
					*pShapeBits = AdjustInversion(nSpriteID, *spritePtr);
				else if ((char)(CheckInversion(nSpriteID, *spritePtr) ^ *pShapeBits) == -1)
					*pShapeBits = CheckWeatherInversion(nSpriteID, *spritePtr, nPos);
				++pShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			break;
		default:
			return;
		}
	}
}

static void L_drawShape_Invert_OutOfContext(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *spritePtr;
	WORD nRemHeight;
	int leftShapeBits, rightShapeBits;
	BYTE *pShapeBits, nCount, nChunkMode;
	bool bReachedBottom;

	leftEdge = shapeLeft - right;
	rightEdge = shapeRight - right;
	topEdge = shapeTop - bottom;
	bottomEdge = shapeBottom - bottom;
	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	nRemHeight = shapeCurrent[nSpriteID].wHeight;
	spritePtr = shapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			--topEdge;
		} while (topEdge);
	}
	leftShapeBits = (int)pShapeBitsLine;
	pShapeBits = pShapeBitsLine;
	rightShapeBits = (int)pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			leftShapeBits = leftEdge;
			rightShapeBits = rightEdge;
			bReachedBottom = --bottomEdge < 0;
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			--nRemHeight;
			if (!bReachedBottom)
				continue;
			break;
		case MIF_CM_SKIPPIXELS:
			leftShapeBits -= nCount;
			rightShapeBits -= nCount;
			pShapeBits += nCount;
			continue;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				if (leftShapeBits <= 0 && rightShapeBits > 0) {
					if (CheckWeatherInversion(nSpriteID, *pShapeBits, nPos) == CheckWeatherInversion(nSpriteID, *spritePtr, nPos))
						*pShapeBits = AdjustInversion(nSpriteID, *spritePtr);
					else if ((char)(CheckInversion(nSpriteID, *spritePtr) ^ *pShapeBits) == -1)
						*pShapeBits = CheckWeatherInversion(nSpriteID, *spritePtr, nPos);
				}
				--leftShapeBits;
				++pShapeBits;
				--rightShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			continue;
		default:
			return;
		}
		break;
	}
}

static void L_drawShape_MainArea(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped) {
	BYTE *pShapeBitsLine, *spritePtr, *pShapeBits;
	BYTE nCount;
	BYTE nChunkMode;
	WORD nRemHeight;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	nRemHeight = shapeCurrent[nSpriteID].wHeight;
	spritePtr = shapePtr;
	pShapeBits = pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			--nRemHeight;
			break;
		case MIF_CM_SKIPPIXELS:
			if (isFlipped)
				pShapeBits -= nCount;
			else
				pShapeBits += nCount;
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				BOOL bProcessBit = (isRoadMask) ? FALSE : TRUE;
				if (isRoadMask) {
					if (*pShapeBits == 0xA1)
						bProcessBit = TRUE;
				}
				if (bProcessBit)
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			break;
		default:
			return;
		}
	}
}

static void L_drawShape_OutOfContext(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *spritePtr;
	WORD nRemHeight;
	int leftShapeBits, rightShapeBits;
	BYTE *pShapeBits, nCount, nChunkMode;
	bool bReachedBottom;

	if (isFlipped) {
		leftEdge = right - shapeLeft;
		rightEdge = right - shapeRight;
	}
	else {
		leftEdge = shapeLeft - right;
		rightEdge = shapeRight - right;
	}
	topEdge = shapeTop - bottom;
	bottomEdge = shapeBottom - bottom;
	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	nRemHeight = shapeCurrent[nSpriteID].wHeight;
	spritePtr = shapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			--topEdge;
		} while (topEdge);
	}
	leftShapeBits = (int)pShapeBitsLine;
	pShapeBits = pShapeBitsLine;
	rightShapeBits = (int)pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			leftShapeBits = leftEdge;
			rightShapeBits = rightEdge;
			bReachedBottom = --bottomEdge < 0;
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			--nRemHeight;
			if (!bReachedBottom)
				continue;
			break;
		case MIF_CM_SKIPPIXELS:
			leftShapeBits -= nCount;
			rightShapeBits -= nCount;
			if (isFlipped)
				pShapeBits -= nCount;
			else
				pShapeBits += nCount;
			continue;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				BOOL bProcessBit = FALSE;
				if (isFlipped) {
					if (rightShapeBits <= 0 && leftShapeBits > 0)
						bProcessBit = (isRoadMask && *pShapeBits != 0xA1) ? FALSE : TRUE;
				}
				else {
					if (leftShapeBits <= 0 && rightShapeBits > 0)
						bProcessBit = (isRoadMask && *pShapeBits != 0xA1) ? FALSE : TRUE;
				}
				if (bProcessBit)
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
				--leftShapeBits;
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				--rightShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			continue;
		default:
			return;
		}
		break;
	}
}

extern "C" void __cdecl Hook_drawShape(__int16 nSpriteID, __int16 right, __int16 bottom, __int16 isFlipped, __int16 doInvert) {
	sprite_header_t *shapePtr;
	BYTE *shapeData;
	int nShapeBottom, nShapeRight;

	shapePtr = &shapeCurrent[nSpriteID];
	if (shapePtr) {
		shapeData = shapePtr->sprOffset.sprPtr;
		if (shapeData) {
			nShapeBottom = bottom + shapePtr->wHeight;
			nShapeRight = right + shapePtr->wWidth;
			if (shapeRight > right && shapeLeft < nShapeRight && shapeBottom > bottom && shapeTop < nShapeBottom) {
				int nRight = (isFlipped) ? nShapeRight : right;
				// The 'doInvert' flag is used from the InvertShape and InvertTerrain calls.
				// It's the "Placement Preview".
				if (doInvert) {
					if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight)
						L_drawShape_Invert_OutOfContext(shapeData, nSpriteID, right, bottom);
					else
						L_drawShape_Invert_MainArea(shapeData, nSpriteID, right, bottom);
				}
				else if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight)
					L_drawShape_OutOfContext(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
				else
					L_drawShape_MainArea(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
			}
		}
	}
}

static BYTE ProcessWeatherSolidShadow(BYTE palIdx, BYTE currIdx) {
	if (palIdx != currIdx) {
		// The original intent here was to allow
		// for shadow projection onto the tiles
		// that had their indices adjusted by
		// the snow effect, however a side-effect
		// was that other objects were touched that
		// fell within range - but the effect looked
		// rather good, so here it will remain for
		// all seasons.
		if (currIdx >= 0x9A && currIdx <= 0x9F)
			return currIdx;
	}
	return palIdx;
}

static void L_drawShadowShape_MainArea(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isFlipped) {
	BYTE *pShapeBitsLine, *spritePtr, *pShapeBits;
	BYTE nCount;
	BYTE nChunkMode;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	spritePtr = shapePtr;
	pShapeBits = pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			break;
		case MIF_CM_SKIPPIXELS:
			if (isFlipped)
				pShapeBits -= nCount;
			else
				pShapeBits += nCount;
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				if (*pShapeBits == 0x5F)
					*pShapeBits = 0x64;
				else if (*pShapeBits >= ProcessWeatherSolidShadow(0x74, *pShapeBits) && *pShapeBits <= ProcessWeatherSolidShadow(0x7E, *pShapeBits))
					*pShapeBits = 0x7E;
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			break;
		default:
			return;
		}
	}
}

static void L_drawShadowShape_OutOfContext(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isFlipped) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *spritePtr;
	int leftShapeBits, rightShapeBits;
	BYTE *pShapeBits, nCount, nChunkMode;
	bool bReachedBottom;

	if (isFlipped) {
		leftEdge = right - shapeLeft;
		rightEdge = right - shapeRight;
	}
	else {
		leftEdge = shapeLeft - right;
		rightEdge = shapeRight - right;
	}
	topEdge = shapeTop - bottom;
	bottomEdge = shapeBottom - bottom;
	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	spritePtr = shapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			--topEdge;
		} while (topEdge);
	}
	leftShapeBits = (int)pShapeBitsLine;
	pShapeBits = pShapeBitsLine;
	rightShapeBits = (int)pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			leftShapeBits = leftEdge;
			rightShapeBits = rightEdge;
			bReachedBottom = --bottomEdge < 0;
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			if (!bReachedBottom)
				continue;
			break;
		case MIF_CM_SKIPPIXELS:
			leftShapeBits -= nCount;
			rightShapeBits -= nCount;
			if (isFlipped)
				pShapeBits -= nCount;
			else
				pShapeBits += nCount;
			continue;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				BOOL bProcessBit = FALSE;
				if (isFlipped) {
					if (rightShapeBits <= 0 && leftShapeBits > 0)
						bProcessBit = TRUE;
				}
				else {
					if (leftShapeBits <= 0 && rightShapeBits > 0)
						bProcessBit = TRUE;
				}
				if (bProcessBit) {
					if (*pShapeBits == 0x5F)
						*pShapeBits = 0x64;
					else if (*pShapeBits >= ProcessWeatherSolidShadow(0x74, *pShapeBits) && *pShapeBits <= ProcessWeatherSolidShadow(0x7E, *pShapeBits))
						*pShapeBits = 0x7E;
				}
				--leftShapeBits;
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				--rightShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			continue;
		default:
			return;
		}
		break;
	}
}

extern "C" void __cdecl Hook_drawMaskShape(__int16 nSpriteID, __int16 left, __int16 top, __int16 isFlipped) {
	sprite_header_t *shapePtr;
	BYTE *shapeData;
	int nShapeTop, nShapeLeft;

	shapePtr = &shapeCurrent[nSpriteID];
	if (shapePtr) {
		shapeData = shapePtr->sprOffset.sprPtr;
		if (shapeData) {
			nShapeTop = top + shapePtr->wHeight;
			nShapeLeft = left + shapePtr->wWidth;
			if (shapeRight > left && nShapeLeft > shapeLeft && shapeBottom > top && nShapeTop > shapeTop) {
				int nLeft = (isFlipped) ? nShapeLeft : left;
				if (shapeTop >= top || shapeLeft >= left || nShapeTop >= shapeBottom || nShapeLeft >= shapeRight)
					L_drawShape_OutOfContext(shapeData, nSpriteID, nLeft, top, TRUE, isFlipped);
				else
					L_drawShape_MainArea(shapeData, nSpriteID, nLeft, top, TRUE, isFlipped);
			}
		}
	}
}

void L_drawShadowShape_SC2K1996(__int16 nSpriteID, __int16 right, __int16 bottom, __int16 isFlipped) {
	sprite_header_t *shapePtr;
	BYTE *shapeData;
	int nShapeBottom, nShapeRight;

	shapePtr = &shapeCurrent[nSpriteID];
	if (shapePtr) {
		shapeData = shapePtr->sprOffset.sprPtr;
		if (shapeData) {
			nShapeBottom = bottom + shapePtr->wHeight;
			nShapeRight = right + shapePtr->wWidth;
			if (shapeRight > right && shapeLeft < nShapeRight && shapeBottom > bottom && shapeTop < nShapeBottom) {
				int nRight = (isFlipped) ? nShapeRight : right;
				if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight)
					L_drawShadowShape_OutOfContext(shapeData, nSpriteID, nRight, bottom, isFlipped);
				else
					L_drawShadowShape_MainArea(shapeData, nSpriteID, nRight, bottom, isFlipped);
			}
		}
	}
}

extern "C" void __cdecl Hook_drawShadowShape(__int16 nSpriteID, __int16 right, __int16 bottom, __int16 isFlipped) {
	L_drawShadowShape_SC2K1996(nSpriteID, right, bottom, isFlipped);
}

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	SafeVirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawLargeTile
	SafeVirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);

	// Hook for DrawSmallTile
	SafeVirtualProtect((LPVOID)0x401E79, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E79, Hook_DrawSmallTile);

	// Hook for DrawTinyTile
	SafeVirtualProtect((LPVOID)0x4022D9, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4022D9, Hook_DrawTinyTile);

	// Hook for DrawAllUnder
	SafeVirtualProtect((LPVOID)0x40251D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40251D, Hook_DrawAllUnder);

	// Hook for DrawUnderTile
	SafeVirtualProtect((LPVOID)0x402D9C, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402D9C, Hook_DrawUnderTile);

	// Hook for DrawColorTile
	SafeVirtualProtect((LPVOID)0x402F6D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402F6D, Hook_DrawColorTile);

	// Hook for PointToTile
	SafeVirtualProtect((LPVOID)0x401D16, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401D16, Hook_PointToTile);

	// Hook for CSimcityView::DrawHouse
	SafeVirtualProtect((LPVOID)0x402810, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402810, Hook_SimcityView_DrawHouse);

	// Hook for drawShape
	SafeVirtualProtect((LPVOID)0x401393, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401393, Hook_drawShape);

	// Hook for drawMaskShape
	SafeVirtualProtect((LPVOID)0x4023AB, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4023AB, Hook_drawMaskShape);

	// Hook for drawShadowShape
	SafeVirtualProtect((LPVOID)0x401357, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401357, Hook_drawShadowShape);

	UpdateDrawingHooks_SC2K1996();
}

void UpdateDrawingHooks_SC2K1996(void) {
	COLORREF undgrndBkgnd;

	undgrndBkgnd = PALETTERGB(192, 192, 192); // Default
	if (bDarkUnderground)
		undgrndBkgnd = PALETTERGB(60, 60, 60);    // Dark Grey

	// Set via InitializeDataColorsFonts() first (on program load).
	SafeVirtualProtect((LPVOID)0x42C008, 4, PAGE_EXECUTE_READWRITE);
	*(COLORREF *)0x42C008 = undgrndBkgnd;

	// Set to the actual variable second (during runtime).
	colGameBackgndUnder = undgrndBkgnd;

	if (bMapWireFrame) {
		// Set via InitializeDataColorsFonts() first (on program load).
		SafeVirtualProtect((LPVOID)0x42BFFE, 4, PAGE_EXECUTE_READWRITE);
		*(COLORREF *)0x42BFFE = undgrndBkgnd;

		// Set to the actual variable second (during runtime).
		colGameBackgndAbove = undgrndBkgnd;
	}
}
