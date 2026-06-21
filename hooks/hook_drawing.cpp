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
			if (pSCView->wSCVZoomLevel && pSCView->wSCVZoomLevel != ZOOM_LEVEL_SMALL) {	// XXX (araxestroy): should this just be `if (pSCView->wSCVZoomLevel == 2)`?
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
		if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_SMALL)
			retval = Game_CalcTileHit8(iOffsetAdjustmentY + 12, iOffsetAdjustmentX + 12);
		else if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_LARGE)
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

extern CGraphics *pBaseGraphics;
extern LONG nBaseGraphicWidth;
extern LONG nBaseGraphicHeight;
extern BYTE *pBaseGraphicLockDIBRes;

void *curBaseLockedDIBBits = NULL;
CMFC3XDC *pBaseSCVDC = NULL;

BYTE *shapeBaseBits = NULL;

static void __cdecl L_SetSpriteForDrawing(BYTE *pBaseBits, BYTE *pModdedBits, sprite_header_t *pCurrentSprite, int x, int y, RECT *p_rc) {
	shapeBaseBits = pBaseBits;
	shapeBits = pModdedBits;
	shapeCurrent = pCurrentSprite;
	shapeY = y;
	shapeX = x;
	shapeLeft = p_rc->left;
	shapeRight = p_rc->right;
	shapeTop = p_rc->top;
	shapeBottom = p_rc->bottom;
}

static void __cdecl L_BeginProcessObjects(HWND hWnd, void *pBaseBits, void *pModdedBits, int x, int y, RECT *r) {
	CMFC3XRect clRect;

	GetClientRect(hWnd, &clRect);
	if (IsRectEmpty(r))
		currWndClientRect = clRect;
	else if (!IntersectRect(&currWndClientRect, r, &clRect))
		return;
	if (currWndClientRect.top > 1)
		--currWndClientRect.top;
	L_SetSpriteForDrawing((BYTE *)pBaseBits, (BYTE *)pModdedBits, pArrSpriteHeaders, x, y, &currWndClientRect);
}

// CSimcityView::DrawHouse, as named in the SCURK and Win3.1 Demo debugging data, is actually the
// functon that's called to draw the whole view.
// For more detalied information, see https://sc2kfix.net/images/drawhouse.png
void L_DrawHouse_SC2K1996(CSimcityView *pSCView, BOOL bLeaveTileHighlightActive) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	COLORREF cr;

	if (pSCView->SCVGraphics && pSCView->pSCVGraphicLockDIBRes || Game_SimcityView_CheckOrLoadGraphic(pSCView)) {
		if (pBaseGraphics && pBaseGraphicLockDIBRes) {
			// Lock what we need to and set up the drawing process
			curBaseLockedDIBBits = Game_Graphics_LockDIBBits(pBaseGraphics);
			curLockedDIBBits = Game_Graphics_LockDIBBits(pSCView->SCVGraphics);
			Game_Graphics_Width(pSCView->SCVGraphics);		// XXX (araxestroy): needed? return value discarded, no side effects
			Game_Graphics_Height(pSCView->SCVGraphics);		// XXX (araxestroy): needed? return value discarded, no side effects
			L_BeginProcessObjects(pSCView->m_hWnd, curBaseLockedDIBBits, curLockedDIBBits, pSCView->dwSCVGraphicWidth, pSCView->dwSCVGraphicHeight, &pSCView->SCVAreaView);
			rcDst = pSCView->SCVAreaView;
			pBaseSCVDC = pBaseGraphics->GetDC_SC2K1996();
			theSCVDC = Game_Graphics_GetDC(pSCView->SCVGraphics);

			// Set the background colour based on the display layer and draw it
			if (DisplayLayer[LAYER_UNDERGROUND])
				cr = colGameBackgndUnder;
			else
				cr = colGameBackgndAbove;
			SetBkColor(pBaseSCVDC->m_hDC, cr);
			SetBkColor(theSCVDC->m_hDC, cr);
			ExtTextOutA(pBaseSCVDC->m_hDC, 0, 0, ETO_OPAQUE, &rcDst, 0, 0, 0);
			ExtTextOutA(theSCVDC->m_hDC, 0, 0, ETO_OPAQUE, &rcDst, 0, 0, 0);
			pBaseGraphics->ReleaseDC_SC2K1996(pBaseSCVDC);
			Game_Graphics_ReleaseDC(pSCView->SCVGraphics, theSCVDC);
			wCurrentPositionAngle = wPositionAngle[wViewRotation];

			// Draw colour data for map overlays if we're in one of those modes
			if (!IsIconic(pSCView->m_hWnd) && showColor && EditData)
				Game_DrawAllColor();

			// Otherwise check to see if the active display layer is the underground view
			else if (DisplayLayer[LAYER_UNDERGROUND])
				Game_DrawAllUnder();

			// If neither of those, draw the above ground view
			else {
				switch (pSCView->wSCVZoomLevel) {
				case 0:
					Game_DrawAllTiny();
					break;
				case 1:
					Game_DrawAllSmall();
					break;
				case 2:
					Game_DrawAllLarge();
					break;
				default:
					ConsoleLog(LOG_WARNING, "DRAW: CSimcityView::DrawHouse got bad ::wSCVZoomLevel = %d, assuming 2.\n", pSCView->wSCVZoomLevel);
					Game_DrawAllLarge();
					break;
				}
			}

			// Draw the highlight if needed, and turn it off if requested
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

			// Clean up the drawing process and redraw the window (and any subdialogs)
			Game_FinishProcessObjects();
			Game_SimcityView_MainWindowUpdate(pSCView, 0, TRUE);
			Game_UpdateCityMap();
			Game_Graphics_UnlockDIBBits(pSCView->SCVGraphics);
			curLockedDIBBits = 0;
			Game_Graphics_UnlockDIBBits(pBaseGraphics);
			curBaseLockedDIBBits = 0;
		}
	}
}

extern "C" void __stdcall Hook_SimcityView_DrawHouse() {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	L_DrawHouse_SC2K1996(pThis, FALSE);
}

// Cycling and palette swap tables/maps - related vars

#define SHORT_CYCLE 4
#define MID_CYCLE   8
#define LONG_CYCLE  16

extern __int16 nCycleIdx;

static int cycleCacheIndices[256] = {
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
	 5,  6,  7,  0,  1,  2,  3, -1,  0,  1,  2,  3,  4,  5,  6,  7,
	 0,  1,  2,  3,  0,  1,  2,  3,  4,  5,  6,  7, -1, -1, -1, -1,
	 0,  8,  0,  8,  0,  8,  0,  8, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static BYTE shortCycleLightsRedYellow[] = { 0xC3, 0xC4, 0xC5, 0xC6 };
static BYTE shortCycleBlueShimmer[]     = { 0xD0, 0xD1, 0xD2, 0xD3 };
static BYTE longCycleRedBlinkLight[]    = { 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1 };
static BYTE longCycleYellowBlinkLight[] = { 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3 };
static BYTE longCycleGreenBlinkLight[]  = { 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5 };
static BYTE longCycleBlueBlinkLight[]   = { 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };
static BYTE midCycleBlueShimmer[]       = { 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF };
static BYTE midCycleBlueGreyLong[]      = { 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2 };
static BYTE midCycleBlackGreyShort[]    = { 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA };
static BYTE midCycleBlueGreyShort[]     = { 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2 };
static BYTE midCycleEarthAndGreyTones[] = { 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB };

bool CycleIndexCheck(BYTE palIdx) {
	if ((palIdx >= 0xAB && palIdx <= 0xC6) || (palIdx >= 0xC8 && palIdx <= 0xDB) || (palIdx >= 0xE0 && palIdx <= 0xE7))
		return true;
	return false;
}

static BYTE GetCycleColIdx(BYTE col, BYTE *pRange, int nCount, int nTarg, bool bRev) {
	BYTE newCol = col;
	if (nTarg) {
		int nIdx = cycleCacheIndices[col];
		if (nIdx >= 0) {
			if (pRange[nIdx] == col) {
				if (bRev)
					nIdx = (nIdx - nTarg) & nCount - 1;
				else
					nIdx = (nIdx + nTarg) & nCount - 1;
				if (nIdx < 0)
					nIdx = -nIdx;
				newCol = pRange[nIdx];
			}
		}
	}
	return newCol;
}

BYTE CyclePaletteIdx(BYTE colIdx, int cIdx) {
	BYTE newIdx = colIdx;
	newIdx = GetCycleColIdx(newIdx, shortCycleLightsRedYellow, sizeof(shortCycleLightsRedYellow), (cIdx % SHORT_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, shortCycleBlueShimmer, sizeof(shortCycleBlueShimmer), (cIdx % SHORT_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, longCycleRedBlinkLight, sizeof(longCycleRedBlinkLight), (cIdx % LONG_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, longCycleYellowBlinkLight, sizeof(longCycleYellowBlinkLight), (cIdx % LONG_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, longCycleGreenBlinkLight, sizeof(longCycleGreenBlinkLight), (cIdx % LONG_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, longCycleBlueBlinkLight, sizeof(longCycleBlueBlinkLight), (cIdx % LONG_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, midCycleBlueShimmer, sizeof(midCycleBlueShimmer), (cIdx % MID_CYCLE), true);
	newIdx = GetCycleColIdx(newIdx, midCycleBlueGreyLong, sizeof(midCycleBlueGreyLong), (cIdx % MID_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, midCycleBlackGreyShort, sizeof(midCycleBlackGreyShort), (cIdx % MID_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, midCycleBlueGreyShort, sizeof(midCycleBlueGreyShort), (cIdx % MID_CYCLE), false);
	newIdx = GetCycleColIdx(newIdx, midCycleEarthAndGreyTones, sizeof(midCycleEarthAndGreyTones), (cIdx % MID_CYCLE), true);
	return newIdx;
}

static int GetTerrainCosmetic() {
	if (iTerrainCosmeticMode == TERRAIN_COSMETIC_GREY)
		return PALCACHE_TYPE_TERRAIN_GEN_GREY;
	else if (iTerrainCosmeticMode == TERRAIN_COSMETIC_GREEN)
		return PALCACHE_TYPE_TERRAIN_GEN_GREEN;
	else if (iTerrainCosmeticMode == TERRAIN_COSMETIC_COLD)
		return PALCACHE_TYPE_TERRAIN_GEN_COLD;
	else if (iTerrainCosmeticMode == TERRAIN_COSMETIC_HOT)
		return PALCACHE_TYPE_TERRAIN_GEN_HOT;
	return 0;
}

std::map<BYTE, BYTE> mapTerrainGenGreyIndexMap = {
	// Ground tiles
	{0x73, 0x9E}, { 0x79, 0xA3 }, { 0x7F, 0xA5 }, { 0x80, 0xA7 },
	{0x74, 0x9F}, { 0x7A, 0xA4 }, { 0x81, 0xA6 },
	{0x75, 0xA0}, { 0x7B, 0xA4 }, { 0x82, 0xA6 },
	{0x76, 0xA2}, { 0x7C, 0xA4 }, { 0x85, 0xA7 },
	{0x77, 0xA2}, { 0x7D, 0xA5 },
	{0x78, 0xA3}, { 0x7E, 0xA5 },
};

static BYTE ProcessTerrainGenGreyIndex(BYTE colIdx) {
	auto iter = mapTerrainGenGreyIndexMap.find(colIdx);
	if (iter != mapTerrainGenGreyIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTerrainGenGreenIndexMap = {
	// Ground tiles
	{0x73, 0x34}, { 0x79, 0x47 }, { 0x7F, 0x49 }, { 0x80, 0x4A },
	{0x74, 0x3D}, { 0x7A, 0x3F }, { 0x81, 0x49 },
	{0x75, 0x45}, { 0x7B, 0x36 }, { 0x82, 0x49 },
	{0x76, 0x3E}, { 0x7C, 0x48 }, { 0x85, 0x4A },
	{0x77, 0x35}, { 0x7D, 0x48 },
	{0x78, 0x46}, { 0x7E, 0x48 },
};

static BYTE ProcessTerrainGenGreenIndex(BYTE colIdx) {
	auto iter = mapTerrainGenGreenIndexMap.find(colIdx);
	if (iter != mapTerrainGenGreenIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTerrainGenColdIndexMap = {
	// Ground tiles
	{0x73, 0x9A}, { 0x79, 0x9A }, { 0x7F, 0x9A }, { 0x80, 0x9A },
	{0x74, 0x9B}, { 0x7A, 0x9B }, { 0x81, 0x9B },
	{0x75, 0x9C}, { 0x7B, 0x9C }, { 0x82, 0x9C },
	{0x76, 0x9D}, { 0x7C, 0x9D }, { 0x85, 0x9D },
	{0x77, 0x9E}, { 0x7D, 0x9E },
	{0x78, 0x9F}, { 0x7E, 0x9F },
};

static BYTE ProcessTerrainGenColdIndex(BYTE colIdx) {
	auto iter = mapTerrainGenColdIndexMap.find(colIdx);
	if (iter != mapTerrainGenColdIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTerrainGenColdSnowIndexMap = {
	// Ground tiles
	{0x73, 0x9A}, { 0x79, 0x9A }, { 0x7F, 0x9A }, { 0x80, 0xA6 },
	{0x74, 0x9B}, { 0x7A, 0x9B }, { 0x81, 0xA3 },
	{0x75, 0x9C}, { 0x7B, 0x9C }, { 0x82, 0xA4 },
	{0x76, 0x9D}, { 0x7C, 0x9D }, { 0x85, 0xA5 },
	{0x77, 0x9E}, { 0x7D, 0x9E },
	{0x78, 0x9F}, { 0x7E, 0x9F },

	// Water tiles
	{0xC8, 0x53}, { 0xCC, 0x53 }, { 0xD0, 0x53 },
	{0xC9, 0x54}, { 0xCD, 0x54 }, { 0xD1, 0x54 },
	{0xCA, 0x55}, { 0xCE, 0x55 }, { 0xD2, 0x55 },
	{0xCB, 0x56}, { 0xCF, 0x56 }, { 0xD3, 0x56 },
};

static BYTE ProcessTerrainGenColdSnowIndex(BYTE colIdx) {
	auto iter = mapTerrainGenColdSnowIndexMap.find(colIdx);
	if (iter != mapTerrainGenColdSnowIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTerrainGenHotIndexMap = {
	// Ground tiles
	{0x73, 0x24}, { 0x79, 0x26 }, { 0x7F, 0x28 }, { 0x80, 0x2A },
	{0x74, 0x24}, { 0x7A, 0x26 }, { 0x81, 0x21 },
	{0x75, 0x24}, { 0x7B, 0x26 }, { 0x82, 0x29 },
	{0x76, 0x25}, { 0x7C, 0x20 }, { 0x85, 0x22 },
	{0x77, 0x25}, { 0x7D, 0x27 },
	{0x78, 0x25}, { 0x7E, 0x20 },
};

static BYTE ProcessTerrainGenHotIndex(BYTE colIdx) {
	auto iter = mapTerrainGenHotIndexMap.find(colIdx);
	if (iter != mapTerrainGenHotIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTerrainSnowIndexMap = {
	// Ground tiles
	{0x73, 0x9A}, { 0x79, 0x9A }, { 0x7F, 0x9A }, { 0x80, 0x9A },
	{0x74, 0x9B}, { 0x7A, 0x9B }, { 0x81, 0x9B },
	{0x75, 0x9C}, { 0x7B, 0x9C }, { 0x82, 0x9C },
	{0x76, 0x9D}, { 0x7C, 0x9D }, { 0x85, 0x9D },
	{0x77, 0x9E}, { 0x7D, 0x9E },
	{0x78, 0x9F}, { 0x7E, 0x9F },

	// Water tiles
	{0xC8, 0x53}, { 0xCC, 0x53 }, { 0xD0, 0x53 },
	{0xC9, 0x54}, { 0xCD, 0x54 }, { 0xD1, 0x54 },
	{0xCA, 0x55}, { 0xCE, 0x55 }, { 0xD2, 0x55 },
	{0xCB, 0x56}, { 0xCF, 0x56 }, { 0xD3, 0x56 },
};

std::map<BYTE, BYTE> mapTerrainBlizzardIndexMap = {
	// Ground tiles
	{0x73, 0x10}, { 0x79, 0x10 }, { 0x7F, 0x10 }, { 0x80, 0x10 },
	{0x74, 0x9A}, { 0x7A, 0x9A }, { 0x81, 0x9A },
	{0x75, 0x9B}, { 0x7B, 0x9B }, { 0x82, 0x9B },
	{0x76, 0x9C}, { 0x7C, 0x9C }, { 0x85, 0x9C },
	{0x77, 0x9D}, { 0x7D, 0x9D },
	{0x78, 0x9E}, { 0x7E, 0x9E },

	// Water tiles
	{0xC8, 0x53}, { 0xCC, 0x53 }, { 0xD0, 0x53 },
	{0xC9, 0x54}, { 0xCD, 0x54 }, { 0xD1, 0x54 },
	{0xCA, 0x55}, { 0xCE, 0x55 }, { 0xD2, 0x55 },
	{0xCB, 0x56}, { 0xCF, 0x56 }, { 0xD3, 0x56 },
};

static BYTE ProcessTerrainSnowIndex(BYTE colIdx) {
	auto iter = mapTerrainSnowIndexMap.find(colIdx);
	if (iter != mapTerrainSnowIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

static BYTE ProcessTerrainBlizzardIndex(BYTE colIdx) {
	auto iter = mapTerrainBlizzardIndexMap.find(colIdx);
	if (iter != mapTerrainBlizzardIndexMap.end())
		return iter->second;
	else
		return colIdx;
}

// Experimental. Looks pretty good on most default buildings but a few have had to be manually
// flagged as exempt, and some separate adjustments might need to be done specifically for the
// the Resort Hotel and College sprites.
std::map<BYTE, BYTE> mapBuildingSnowIndexMap = {
	{0x43, 0x10},
	{0x44, 0x10},
	{0x45, 0x9A},
	{0x46, 0x9B},
	{0x47, 0x9C},
	{0x48, 0x9D},
	{0x49, 0x9E},
	{0x4A, 0x9E},
};

static BYTE ProcessBuildingSnowIndex(BYTE colIdx) {
	auto iter = mapBuildingSnowIndexMap.find(colIdx);
	if (iter != mapBuildingSnowIndexMap.end())
		return iter->second;
	else
		return colIdx;
	return colIdx;
}

std::map<BYTE, BYTE> mapTreeSnowEffectMap = {
	{0x3B, 0x9A}, { 0x40, 0x9A }, { 0x46, 0x9A }, { 0x50, 0x9A },
	{0x3C, 0x9B}, { 0x41, 0x9B }, { 0x47, 0x9B }, { 0x51, 0x9B },
	{0x3D, 0x9C}, { 0x42, 0x9C }, { 0x48, 0x9C }, { 0x52, 0x9C },
	{0x3E, 0x9D}, { 0x43, 0x9D }, { 0x49, 0x9D },
	{0x3F, 0x9E}, { 0x44, 0x9E }, { 0x4A, 0x9E },
	{0x45, 0x9F},
};

static BYTE ProcessTreeSnowEffect(BYTE colIdx) {
	auto iter = mapTreeSnowEffectMap.find(colIdx);
	if (iter != mapTreeSnowEffectMap.end())
		return iter->second;
	else
		return colIdx;
}

std::map<BYTE, BYTE> mapTreeAutumnEffectMap = {
	{0x3B, 0x7D}, { 0x40, 0x7D }, { 0x46, 0x7D }, { 0x50, 0x7D },
	{0x3C, 0x7E}, { 0x41, 0x7E }, { 0x47, 0x7E }, { 0x51, 0x7E },
	{0x3D, 0x7F}, { 0x42, 0x7F }, { 0x48, 0x7F }, { 0x52, 0x7F },
	{0x3E, 0x28}, { 0x43, 0x28 }, { 0x49, 0x28 },
	{0x3F, 0x29}, { 0x44, 0x29 }, { 0x4A, 0x29 },
	{0x45, 0x2A},
};

static BYTE ProcessTreeAutumnEffect(BYTE colIdx) {
	auto iter = mapTreeAutumnEffectMap.find(colIdx);
	if (iter != mapTreeAutumnEffectMap.end())
		return iter->second;
	else
		return colIdx;
}

// Sprite, season and weather checks

int iForcedSeason = FORCED_SEASON_NONE;

bool UndergroundSpritesCheck(DWORD nID) {
	return GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_UNDERGROUND_TERRAIN, SPRITE_SMALL_SUBWAYENTRANCE) ? true : false;
}

bool TreeSpritesCheck(DWORD nID) {
	return GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_TREES1, SPRITE_SMALL_TREES7) ? true : false;
}

bool TerrainSpritesCheck(DWORD nID) {
	return GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_TERRAIN, SPRITE_SMALL_SEAPORTZONE) ? true : false;
}

bool DeepWaterSpriteCheck(DWORD nID) {
	return GET_OVERALL_SPRITE(nID, SPRITE_SMALL_WATER_TRBL) ? true : false;
}

bool ObjectGrassSpritesCheck(DWORD nID) {
	return ((GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_RESIDENTIAL_1X1_LOWERCLASSHOMES1, SPRITE_SMALL_SERVICES_STATUE) ||
		GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_MAYORSHOUSE) || GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_LIBRARY) ||
		GET_OVERALL_SPRITE(nID, SPRITE_SMALL_SMALLPARK) || GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_WATERPUMP) ||
		GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_WATERTOWER) || GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_CHURCH)) &&
		!GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INDUSTRIAL_2X2_FACTORY2) && !GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INDUSTRIAL_3X3_THINGAMAJIG)) ? true : false;
}

static bool SnowCheck() {
	return (bWeatherTrend == WEATHER_TREND_SNOW || iForcedSeason == FORCED_SEASON_SNOW) ? true : false;
}

static bool BlizzardCheck() {
	return (bWeatherTrend == WEATHER_TREND_BLIZZARD || iForcedSeason == FORCED_SEASON_BLIZZARD) ? true : false;
}

static bool ColdWeatherCheck() {
	return (SnowCheck() || BlizzardCheck()) ? true : false;
}

static bool HeatwaveCheck() {
	return ((bWeatherTrend == WEATHER_TREND_HOT && bWeatherHeat >= 185 && bWeatherHeat < 200) || iForcedSeason == FORCED_SEASON_HEATWAVE) ? true : false;
}

static bool DroughtCheck() {
	return ((bWeatherTrend == WEATHER_TREND_HOT && bWeatherHeat >= 200) || iForcedSeason == FORCED_SEASON_DROUGHT) ? true : false;
}

static bool HotWeatherCheck() {
	return (HeatwaveCheck() || DroughtCheck()) ? true : false;
}

static bool TreeBrowningCheck() {
	return (GetGameSeason() == GAME_SEASON_WINTER || GetGameSeason() == GAME_SEASON_AUTUMN ||
		iForcedSeason == FORCED_SEASON_AUTUMN || iForcedSeason == FORCED_SEASON_WINTER ||
		HotWeatherCheck()) ? true : false;
}

// Sprite Caching

// This is for grouping sequences of pixels
// during encoded processing.
#define SEQ_MODULUS 4

static std::vector<spriteCache_t> spriteCache;

static void Delete_SpriteFrame_Cache(std::vector<spriteFrame_t> &sprFrame, DWORD nID, int nType) {
	for (std::vector<spriteFrame_t>::iterator itFr = sprFrame.begin(); itFr != sprFrame.end();) {
		if (itFr->pBuf) {
			free(itFr->pBuf);
			itFr->pBuf = 0;
		}
		itFr = sprFrame.erase(itFr);
	}

	sprFrame.clear();
}

static void Delete_Sprite_Cache(spriteCache_t *pSpriteCache) {
	Delete_SpriteFrame_Cache(pSpriteCache->sprFrame, pSpriteCache->nID, PALCACHE_TYPE_CYCLE);
	Delete_SpriteFrame_Cache(pSpriteCache->sprSeasonAutumnFrame, pSpriteCache->nID, PALCACHE_TYPE_TREES_SEASON_AUTUMN);
	Delete_SpriteFrame_Cache(pSpriteCache->sprSeasonAutumnSnowFrame, pSpriteCache->nID, PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW);
	Delete_SpriteFrame_Cache(pSpriteCache->sprSeasonSnowFrame, pSpriteCache->nID, PALCACHE_TYPE_TREES_SEASON_SNOW);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainGenGreyFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_GEN_GREY);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainGenGreenFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_GEN_GREEN);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainGenColdFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_GEN_COLD);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainGenHotFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_GEN_HOT);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainSnowFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_SNOW);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainBlizzardFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD);
	Delete_SpriteFrame_Cache(pSpriteCache->sprDeepWaterIceFrame, pSpriteCache->nID, PALCACHE_TYPE_WATER_ICE);
	Delete_SpriteFrame_Cache(pSpriteCache->sprDeepWaterBlizzardFrame, pSpriteCache->nID, PALCACHE_TYPE_WATER_ICE_BLIZZARD);
	Delete_SpriteFrame_Cache(pSpriteCache->sprGrassSnowFrame, pSpriteCache->nID, PALCACHE_TYPE_GRASS_SNOW);
	Delete_SpriteFrame_Cache(pSpriteCache->sprSeasonHeatwaveFrame, pSpriteCache->nID, PALCACHE_TYPE_TREES_SEASON_HEAT);
	Delete_SpriteFrame_Cache(pSpriteCache->sprGrassHeatwaveFrame, pSpriteCache->nID, PALCACHE_TYPE_GRASS_HEAT);
	Delete_SpriteFrame_Cache(pSpriteCache->sprGrassDroughtFrame, pSpriteCache->nID, PALCACHE_TYPE_GRASS_DROUGHT);
}

void Clear_SpriteCache() {
	for (unsigned i = 0; i < spriteCache.size(); ++i) {
		Delete_Sprite_Cache(&spriteCache[i]);
	}

	spriteCache.clear();
}

void Init_SpriteCache(bool bReload) {
	if (bReload)
		Clear_SpriteCache();

	spriteCache_t sprCacheEnt;
	for (int i = 0; i < SPRITE_COUNT; ++i) {
		sprCacheEnt.nID = i;
		spriteCache.push_back(sprCacheEnt);
	}
}

static bool Scan_Sprite(BYTE *shapePtr) {
	BYTE *spritePtr;
	BYTE nCount;
	BYTE nChunkMode;

	spritePtr = shapePtr;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			break;
		case MIF_CM_SKIPPIXELS:
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				// Scan for the presence of any cycling palette index.
				if (CycleIndexCheck(*spritePtr))
					return true;
				--nPos;
			}
			if ((nCount & 1) != 0)
				++spritePtr;
			break;
		default:
			return false;
		}
	}

	return false;
}

static void Adjust_SpritePalette(BYTE *shapePtr, WORD wHeight, int cIdx, int nType) {
	BYTE *spritePtr;
	BYTE nCount;
	BYTE nChunkMode;
	WORD nRemHeight;

	nRemHeight = wHeight;
	spritePtr = shapePtr;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			--nRemHeight;
			break;
		case MIF_CM_SKIPPIXELS:
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr) {
				BYTE palIdx = *spritePtr;
				if (nType == PALCACHE_TYPE_CYCLE)
					palIdx = CyclePaletteIdx(palIdx, cIdx);
				else if (nType >= PALCACHE_TYPE_TREES_SEASON_AUTUMN && nType <= PALCACHE_TYPE_TREES_SEASON_SNOW) {
					if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3) {
						if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
							palIdx = ProcessTreeAutumnEffect(palIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW) {
							palIdx = ProcessTreeAutumnEffect(palIdx);
							if ((nPos % SEQ_MODULUS) != 2)
								palIdx = ProcessTreeSnowEffect(*spritePtr);
						}
						else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW)
							palIdx = ProcessTreeSnowEffect(palIdx);
					}
				}
				else if (nType >= PALCACHE_TYPE_TERRAIN_GEN_GREY && nType <= PALCACHE_TYPE_TERRAIN_GEN_HOT) {
					BOOL bProcess = FALSE;
					if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
						if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3)
							bProcess = TRUE;
					}
					else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
						if ((nPos % SEQ_MODULUS) == 1 || (nPos % SEQ_MODULUS) == 3)
							bProcess = TRUE;
					}
					else {
						if (((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) && ((nPos % 2) == 1))
							bProcess = TRUE;
					}
					if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
						if (bProcess)
							palIdx = ProcessTerrainGenGreyIndex(palIdx);
					}
					else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
						palIdx = (bProcess) ? ProcessTerrainGenGreenIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
					}
					else if (nType == PALCACHE_TYPE_TERRAIN_GEN_COLD) {
						if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2)
							palIdx = (bProcess) ? ProcessTerrainGenColdSnowIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
						else if ((nPos % 3) == 1)
							palIdx = ProcessTerrainGenColdIndex(palIdx);
					}
					else if (nType == PALCACHE_TYPE_TERRAIN_GEN_HOT) {
						if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) {
							if (!bProcess)
								palIdx = ProcessTerrainGenHotIndex(palIdx);
						}
						else
							palIdx = ProcessTerrainGenHotIndex(palIdx);
					}
				}
				else if (nType >= PALCACHE_TYPE_TERRAIN_SNOW && nType <= PALCACHE_TYPE_WATER_ICE_BLIZZARD) {
					if (nType == PALCACHE_TYPE_TERRAIN_SNOW)
						palIdx = ProcessTerrainSnowIndex(palIdx);
					else if (nType == PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD)
						palIdx = ProcessTerrainBlizzardIndex(palIdx);
					else if (nType == PALCACHE_TYPE_WATER_ICE) {
						if ((nPos % SEQ_MODULUS) == 2)
							palIdx = ProcessTerrainSnowIndex(palIdx);
					}
					else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD) {
						if ((nPos % SEQ_MODULUS) == 1 || (nPos % SEQ_MODULUS) == 2)
							palIdx = ProcessTerrainBlizzardIndex(palIdx);
					}
				}
				else if (nType == PALCACHE_TYPE_GRASS_SNOW)
					palIdx = ProcessBuildingSnowIndex(palIdx);
				*spritePtr = palIdx;
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

static void Create_SpriteNew(std::vector<spriteFrame_t> &frameCache, BYTE *pSpriteBuf, int nSize, WORD wHeight, WORD wWidth, int nFrm, int nType) {
	spriteFrame_t sprFrame;
	int cIdx = (nType == PALCACHE_TYPE_CYCLE) ? nFrm : 0;

	memset(&sprFrame, 0, sizeof(sprFrame));
	sprFrame.nFrID = nFrm;
	sprFrame.wHeight = wHeight;
	sprFrame.wWidth = wWidth;
	sprFrame.nSize = nSize;
	sprFrame.pBuf = (BYTE *)malloc(nSize);
	if (sprFrame.pBuf) {
		memcpy(sprFrame.pBuf, pSpriteBuf, nSize);
		if (nType > PALCACHE_TYPE_NONE)
			Adjust_SpritePalette(sprFrame.pBuf, sprFrame.wHeight, cIdx, nType);
	}
	else {
		ConsoleLog(LOG_ERROR, "Create_SpriteNew(%d, %u, %u, %d, %d): Allocation failed for sprite frame.\n", nSize, wHeight, wWidth, nFrm, nType);
		sprFrame.pBuf = 0;
	}
	// Push even if allocation fails.
	frameCache.push_back(sprFrame);
}

static void Create_SpriteFrame(std::vector<spriteFrame_t> &frameCache, spriteFrame_t *pSpriteFrame, int cIdx, int nType) {
	Create_SpriteNew(frameCache, pSpriteFrame->pBuf, pSpriteFrame->nSize, pSpriteFrame->wHeight, pSpriteFrame->wWidth, cIdx, nType);
}

static void Season_SpritePalette_Trees(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache accumulated snow or fading/browning on trees
	Create_SpriteFrame(spriteCache[nID].sprSeasonAutumnFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TREES_SEASON_AUTUMN);
	Create_SpriteFrame(spriteCache[nID].sprSeasonAutumnSnowFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW);
	Create_SpriteFrame(spriteCache[nID].sprSeasonSnowFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TREES_SEASON_SNOW);
}

static void Snow_SpritePalette_DeepWater(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache "deep water" snow/blizzard frames
	Create_SpriteFrame(spriteCache[nID].sprDeepWaterIceFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_WATER_ICE);
	Create_SpriteFrame(spriteCache[nID].sprDeepWaterBlizzardFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_WATER_ICE_BLIZZARD);
}

static void Effect_SpritePalette_Terrain(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache - overall selectable terrain effects.
	Create_SpriteFrame(spriteCache[nID].sprTerrainGenGreyFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_GEN_GREY);
	Create_SpriteFrame(spriteCache[nID].sprTerrainGenGreenFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_GEN_GREEN);
	Create_SpriteFrame(spriteCache[nID].sprTerrainGenColdFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_GEN_COLD);
	Create_SpriteFrame(spriteCache[nID].sprTerrainGenHotFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_GEN_HOT);
	// Cache - ground should be mostly covered in regular snow or absolutely blanketed in a blizzard.
	Create_SpriteFrame(spriteCache[nID].sprTerrainSnowFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_SNOW);
	Create_SpriteFrame(spriteCache[nID].sprTerrainBlizzardFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD);
}

static void Snow_SpritePalette_Grass(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache accumulated snow on grass for various objects
	Create_SpriteFrame(spriteCache[nID].sprGrassSnowFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_GRASS_SNOW);
}

void Cache_Sprite(DWORD nID, BYTE *pSpriteBuf, int nSize, WORD wHeight, WORD wWidth) {
	Delete_Sprite_Cache(&spriteCache[nID]);

	bool bCycling = (!bLoColor) ? Scan_Sprite(pSpriteBuf) : false;

	for (int nFrm = 0; nFrm < CACHED_FRAMES; ++nFrm) {
		if (!bCycling && nFrm > 0)
			break;
		// Cycling (or only frame for non-cycling cases)
		Create_SpriteNew(spriteCache[nID].sprFrame, pSpriteBuf, nSize, wHeight, wWidth, nFrm, ((bCycling) ? PALCACHE_TYPE_CYCLE : PALCACHE_TYPE_NONE));
		if (!bLoColor && !bOnTheFlyPalIdx) {
			if (TreeSpritesCheck(nID))
				Season_SpritePalette_Trees(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			else if (TerrainSpritesCheck(nID)) {
				if (DeepWaterSpriteCheck(nID))
					Snow_SpritePalette_DeepWater(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
				else
					Effect_SpritePalette_Terrain(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			}
			else if (ObjectGrassSpritesCheck(nID)) {
				Snow_SpritePalette_Grass(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			}
		}
	}
}

static BYTE *Get_SpriteFrame_Buffer(std::vector<spriteFrame_t> &frameCache, BYTE *pSpriteBuf, DWORD nFrmIdx) {
	if (nFrmIdx >= 0 && frameCache.size() > 0) {
		for (std::vector<spriteFrame_t>::reverse_iterator itFr = frameCache.rbegin(); itFr != frameCache.rend();) {
			if (itFr->nFrID <= nFrmIdx) {
				if (itFr->pBuf)
					return itFr->pBuf;
			}
			++itFr;
		}
	}
	return pSpriteBuf;
}

static BYTE *Get_SpriteCache_BaseBuffer(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	if (pShapePtr->wHeight > 1) {
		int nFrmIdx = nCycleIdx % CACHED_FRAMES;
		if (nFrmIdx < 0)
			nFrmIdx = -nFrmIdx;
		BYTE *pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, nFrmIdx);
		if (pSpriteBuf)
			return pSpriteBuf;
		return pShapePtr->sprOffset.sprPtr;
	}
	return NULL;
}

static BYTE *Get_SpriteCache_Buffer(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	int nType = PALCACHE_TYPE_NONE;

	if (pShapePtr->wHeight > 1) {
		int nFrmIdx = nCycleIdx % CACHED_FRAMES;
		if (nFrmIdx < 0)
			nFrmIdx = -nFrmIdx;
		BYTE *pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, nFrmIdx);
		if (pSpriteBuf) {
			if (!bLoColor && !bOnTheFlyPalIdx) {
				if (bWeatherEffects) {
					if (TreeSpritesCheck(nSpriteID)) {
						if (ColdWeatherCheck())
							nType = PALCACHE_TYPE_TREES_SEASON_SNOW;

						if (TreeBrowningCheck())
							nType = (nType == PALCACHE_TYPE_TREES_SEASON_SNOW) ? PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW : PALCACHE_TYPE_TREES_SEASON_AUTUMN;

						if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnSnowFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonSnowFrame, pSpriteBuf, nFrmIdx);
					}
					else if (TerrainSpritesCheck(nSpriteID)) {
						if (ColdWeatherCheck()) {
							if (DeepWaterSpriteCheck(nSpriteID)) {
								nType = (BlizzardCheck()) ? PALCACHE_TYPE_WATER_ICE_BLIZZARD : PALCACHE_TYPE_WATER_ICE;
								if (nType == PALCACHE_TYPE_WATER_ICE)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterIceFrame, pSpriteBuf, nFrmIdx);
								else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterBlizzardFrame, pSpriteBuf, nFrmIdx);
							}
							else {
								nType = (BlizzardCheck()) ? PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD : PALCACHE_TYPE_TERRAIN_SNOW;
								if (nType == PALCACHE_TYPE_TERRAIN_SNOW)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainSnowFrame, pSpriteBuf, nFrmIdx);
								else if (nType == PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainBlizzardFrame, pSpriteBuf, nFrmIdx);
							}
						}
					}
					else if (ObjectGrassSpritesCheck(nSpriteID)) {
						if (ColdWeatherCheck()) {
							nType = PALCACHE_TYPE_GRASS_SNOW; // Yes I know.. this is the only option here currently.
							if (nType == PALCACHE_TYPE_GRASS_SNOW)
								return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprGrassSnowFrame, pSpriteBuf, nFrmIdx);
						}
					}
				}
				if (TerrainSpritesCheck(nSpriteID) && !DeepWaterSpriteCheck(nSpriteID)) {
					nType = GetTerrainCosmetic();
					if (nType >= PALCACHE_TYPE_TERRAIN_GEN_GREY && nType <= PALCACHE_TYPE_TERRAIN_GEN_HOT) {
						if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenGreyFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenGreenFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TERRAIN_GEN_COLD)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenColdFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TERRAIN_GEN_HOT)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenHotFrame, pSpriteBuf, nFrmIdx);
					}
				}
			}
			return pSpriteBuf;
		}
		return pShapePtr->sprOffset.sprPtr;
	}
	return NULL;
}

static BYTE *Get_SpecificSpriteCache_Buffer(sprite_header_t *pShapePtr, __int16 nSpriteID, int nType) {
	if (pShapePtr->wHeight > 1) {
		int nFrmIdx = nCycleIdx % CACHED_FRAMES;
		if (nFrmIdx < 0)
			nFrmIdx = -nFrmIdx;
		BYTE *pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, nFrmIdx);
		if (pSpriteBuf) {
			if (nType > PALCACHE_TYPE_CYCLE && !bLoColor && !bOnTheFlyPalIdx) {
				if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnSnowFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonSnowFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenGreyFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenGreenFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_COLD)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenColdFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_HOT)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainGenHotFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_SNOW)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainSnowFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainBlizzardFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_WATER_ICE)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterIceFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterBlizzardFrame, pSpriteBuf, nFrmIdx);
				else if (nType == PALCACHE_TYPE_GRASS_SNOW)
					return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprGrassSnowFrame, pSpriteBuf, nFrmIdx);
			}
			return pSpriteBuf;
		}
		return pShapePtr->sprOffset.sprPtr;
	}
	return NULL;
}

static WORD Get_SpriteCache_Height(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	if (pShapePtr->wHeight > 1) {
		BYTE *pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, 0);
		if (pSpriteBuf)
			return spriteCache[nSpriteID].sprFrame[0].wHeight;
		return pShapePtr->wHeight;
	}
	return 0;
}

static WORD Get_SpriteCache_Width(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	if (pShapePtr->wHeight > 1) {
		BYTE *pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, 0);
		if (pSpriteBuf)
			return spriteCache[nSpriteID].sprFrame[0].wWidth;
		return pShapePtr->wWidth;
	}
	return 0;
}

// On-the-fly palette index swapping

static BYTE ProcessSeasonIndex(BYTE colIdx, BOOL bIgnore = FALSE) {
	BYTE newIdx = colIdx;
	if (ColdWeatherCheck()) {
		if (!bIgnore || BlizzardCheck())
			newIdx = ProcessTreeSnowEffect(newIdx);
	}
	if (TreeBrowningCheck()) {
		newIdx = ProcessTreeAutumnEffect(newIdx);
	}
	return newIdx;
}

static BYTE ProcessSpriteSpecificPaletteIndex(__int16 nSpriteID, BYTE colIdx, WORD nRemHeight, int nPos, int nType) {
	BYTE palIdx = colIdx;

	if (nType > PALCACHE_TYPE_CYCLE && !bLoColor && bOnTheFlyPalIdx) {
		// Accumulate snow or fading on trees
		if (TreeSpritesCheck(nSpriteID)) {
			if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3) {
				BOOL bIgnore = FALSE;
				if ((nPos % SEQ_MODULUS) == 2)
					bIgnore = TRUE;
				if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
					palIdx = ProcessTreeAutumnEffect(palIdx);
				else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW) {
					if (!bIgnore)
						palIdx = ProcessTreeSnowEffect(palIdx);
					palIdx = ProcessTreeAutumnEffect(palIdx);
				}
				else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW) {
					if (!bIgnore)
						palIdx = ProcessTreeSnowEffect(palIdx);
				}
			}
		}

		// Handle snow-related tile stuff
		// Accumulate snow on ground
		if (TerrainSpritesCheck(nSpriteID)) {
			// Handle deep water differently.
			if (DeepWaterSpriteCheck(nSpriteID)) {
				if (((nPos % SEQ_MODULUS) == 1 && nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD) || (nPos % SEQ_MODULUS) == 2) {
					if (nType == PALCACHE_TYPE_WATER_ICE)
						palIdx = ProcessTerrainSnowIndex(palIdx);
					else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD)
						palIdx = ProcessTerrainBlizzardIndex(palIdx);
				}
			}
			// Ground should be mostly covered in regular snow or absolutely blanketed in a blizzard.
			else {
				BOOL bProcess = FALSE;
				if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
					if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3)
						bProcess = TRUE;
				}
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
					if ((nPos % SEQ_MODULUS) == 1 || (nPos % SEQ_MODULUS) == 3)
						bProcess = TRUE;
				}
				else {
					if (((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) && ((nPos % 2) == 1))
						bProcess = TRUE;
				}
				if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
					if (bProcess)
						palIdx = ProcessTerrainGenGreyIndex(palIdx);
				}
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
					palIdx = (bProcess) ? ProcessTerrainGenGreenIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
				}
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_COLD) {
					if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2)
						palIdx = (bProcess) ? ProcessTerrainGenColdSnowIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
					else if ((nPos % 3) == 1)
						palIdx = ProcessTerrainGenColdIndex(palIdx);
				}
				else if (nType == PALCACHE_TYPE_TERRAIN_GEN_HOT) {
					if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) {
						if (!bProcess)
							palIdx = ProcessTerrainGenHotIndex(palIdx);
					}
					else
						palIdx = ProcessTerrainGenHotIndex(palIdx);
				}
			}
		}

		// Accumulate snow on grass
		else if (ObjectGrassSpritesCheck(nSpriteID)) {
			if (nType == PALCACHE_TYPE_GRASS_SNOW)
				palIdx = ProcessBuildingSnowIndex(palIdx);
		}

		// IDEA: high temperatures + WEATHER_TREND_HOT = patchy, dried out grass?
		// IDEA: detect drought conditions and affect the edges of water?
	}
	return palIdx;
}

static BYTE ProcessSpritePaletteIndex(__int16 nSpriteID, BYTE colIdx, WORD nRemHeight, int nPos, int nType = PALCACHE_TYPE_NONE) {
	BYTE palIdx = colIdx;

	// Goes directly to the specific call - sprite browser mode.
	if (nType >= PALCACHE_TYPE_CYCLE)
		return ProcessSpriteSpecificPaletteIndex(nSpriteID, colIdx, nRemHeight, nPos, nType);

	if (bWeatherEffects && !bLoColor && bOnTheFlyPalIdx) {
		// Accumulate snow or fading on trees
		if (TreeSpritesCheck(nSpriteID)) {
			if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3) {
				BOOL bIgnore = FALSE;
				if ((nPos % SEQ_MODULUS) == 2)
					bIgnore = TRUE;
				palIdx = ProcessSeasonIndex(palIdx, bIgnore);
			}
		}

		// Handle snow-related tile stuff
		// Accumulate snow on ground
		if (TerrainSpritesCheck(nSpriteID)) {
			if (ColdWeatherCheck()) {
				// Handle deep water differently.
				if (DeepWaterSpriteCheck(nSpriteID)) {
					if (((nPos % SEQ_MODULUS) == 1 && BlizzardCheck()) || (nPos % SEQ_MODULUS) == 2)
						palIdx = (BlizzardCheck() ? ProcessTerrainBlizzardIndex(palIdx) : ProcessTerrainSnowIndex(palIdx));
				}

				// Ground should be mostly covered in regular snow or absolutely blanketed in a blizzard.
				else 
					palIdx = (BlizzardCheck() ? ProcessTerrainBlizzardIndex(palIdx) : ProcessTerrainSnowIndex(palIdx));
			}
			else {
				if (!DeepWaterSpriteCheck(nSpriteID)) {
					int iTerrainCosmetic = GetTerrainCosmetic();
					if (iTerrainCosmetic >= PALCACHE_TYPE_TERRAIN_GEN_GREY && iTerrainCosmetic <= PALCACHE_TYPE_TERRAIN_GEN_HOT) {
						BOOL bProcess = FALSE;
						if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
							if ((nPos % SEQ_MODULUS) == 0 || (nPos % SEQ_MODULUS) == 2 || (nPos % SEQ_MODULUS) == 3)
								bProcess = TRUE;
						}
						else if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
							if ((nPos % SEQ_MODULUS) == 1 || (nPos % SEQ_MODULUS) == 3)
								bProcess = TRUE;
						}
						else {
							if (((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) && ((nPos % 2) == 1))
								bProcess = TRUE;
						}
						if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_GREY) {
							if (bProcess)
								palIdx = ProcessTerrainGenGreyIndex(palIdx);
						}
						else if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_GREEN) {
							palIdx = (bProcess) ? ProcessTerrainGenGreenIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
						}
						else if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_COLD) {
							if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2)
								palIdx = (bProcess) ? ProcessTerrainGenColdSnowIndex(palIdx) : ProcessTerrainGenGreyIndex(palIdx);
							else if ((nPos % 3) == 1)
								palIdx = ProcessTerrainGenColdIndex(palIdx);
						}
						else if (iTerrainCosmetic == PALCACHE_TYPE_TERRAIN_GEN_HOT) {
							if ((nRemHeight % 3) == 0 || (nRemHeight % 3) == 2) {
								if (!bProcess)
									palIdx = ProcessTerrainGenHotIndex(palIdx);
							}
							else
								palIdx = ProcessTerrainGenHotIndex(palIdx);
						}
					}
				}
			}
		}

		// Accumulate snow on grass
		else if (ObjectGrassSpritesCheck(nSpriteID)) {
			if (ColdWeatherCheck())
				palIdx = ProcessBuildingSnowIndex(palIdx);
		}

		// IDEA: high temperatures + WEATHER_TREND_HOT = patchy, dried out grass?
		// IDEA: detect drought conditions and affect the edges of water?
	}
	return palIdx;
}

static BYTE AdjustInversion(__int16 nSpriteID, BYTE palIdx, BOOL bInvUnder = FALSE) {
	BYTE newIdx = ~palIdx;
	// In the DOS and Macintosh version the tile inversion
	// colouration was slightly different, this was a result
	// of different palette indices. To compensate all that's
	// required is to increment by 0x20 to get it back into
	// range.
	BOOL bPalOffset = TRUE;
	if (UndergroundSpritesCheck(nSpriteID)) {
		if (bDarkUnderground && !bInvUnder)
			bPalOffset = FALSE;
	}

	if (bPalOffset)
		newIdx += 0x20;
	return newIdx;
}

static BYTE CheckInversion(__int16 nSpriteID, BYTE palIdx, BOOL bInvUnder = FALSE) {
	BYTE newIdx = palIdx;
	
	BOOL bPalOffset = TRUE;
	if (UndergroundSpritesCheck(nSpriteID)) {
		if (bDarkUnderground && !bInvUnder)
			bPalOffset = FALSE;
	}

	if (bPalOffset)
		newIdx -= 0x20;
	return newIdx;
}

static void L_drawShape_Invert_MainArea(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom) {
	BYTE *pShapeBitsLine, *spritePtr, *baseSpritePtr, *pShapeBits;
	BYTE nCount;
	BYTE nChunkMode;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	spritePtr = shapePtr;
	baseSpritePtr = baseShapePtr;
	pShapeBits = pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
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
				if (*pShapeBits == *spritePtr)
					*pShapeBits = AdjustInversion(nSpriteID, *baseSpritePtr);
				else if ((char)(CheckInversion(nSpriteID, *baseSpritePtr) ^ *pShapeBits) == -1)
					*pShapeBits = *spritePtr;
				++pShapeBits;
				++baseSpritePtr;
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++baseSpritePtr;;
				++spritePtr;
			}
			break;
		default:
			return;
		}
	}
}

static void L_drawShape_Invert_OutOfContext(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *spritePtr, *baseSpritePtr;
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
	baseSpritePtr = baseShapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			baseSpritePtr += SPRITEDATA(baseSpritePtr)->nCount + 2;
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
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
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
					if (*pShapeBits == *spritePtr)
						*pShapeBits = AdjustInversion(nSpriteID, *baseSpritePtr);
					else if ((char)(CheckInversion(nSpriteID, *baseSpritePtr) ^ *pShapeBits) == -1)
						*pShapeBits = *spritePtr;
				}
				--leftShapeBits;
				++pShapeBits;
				++baseSpritePtr;
				--rightShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++baseSpritePtr;
				++spritePtr;
			}
			continue;
		default:
			return;
		}
		break;
	}
}

static void L_drawShapeSpecific_Invert_MainArea(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isFlipped) {
	BYTE *pShapeBitsLine, *spritePtr, *baseSpritePtr, *pShapeBits;
	BYTE nCount;
	BYTE nChunkMode;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	spritePtr = shapePtr;
	baseSpritePtr = baseShapePtr;
	pShapeBits = pShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
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
				if ((char)(CheckInversion(nSpriteID, *baseSpritePtr, TRUE) ^ *baseSpritePtr) > -1)
					*pShapeBits = AdjustInversion(nSpriteID, *baseSpritePtr, TRUE);
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				++baseSpritePtr;
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++baseSpritePtr;;
				++spritePtr;
			}
			break;
		default:
			return;
		}
	}
}

static void L_drawShapeSpecific_Invert_OutOfContext(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isFlipped) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *spritePtr, *baseSpritePtr;
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
	baseSpritePtr = baseShapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			baseSpritePtr += SPRITEDATA(baseSpritePtr)->nCount + 2;
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
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
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
						bProcessBit = TRUE;
				}
				else {
					if (leftShapeBits <= 0 && rightShapeBits > 0)
						bProcessBit = TRUE;
				}
				if (bProcessBit) {
					if ((char)(CheckInversion(nSpriteID, *baseSpritePtr, TRUE) ^ *baseSpritePtr) > -1)
						*pShapeBits = AdjustInversion(nSpriteID, *baseSpritePtr, TRUE);
				}
				--leftShapeBits;
				if (isFlipped)
					--pShapeBits;
				else
					++pShapeBits;
				++baseSpritePtr;
				--rightShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++baseSpritePtr;
				++spritePtr;
			}
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
				if (bProcessBit) {
					if (bOnTheFlyPalIdx)
						*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
					else
						*pShapeBits = *spritePtr;
				}
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
				if (bProcessBit) {
					if (bOnTheFlyPalIdx)
						*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
					else
						*pShapeBits = *spritePtr;
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

static void L_drawShape_WithBase_MainArea(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped) {
	BYTE *pShapeBitsLine, *pBaseShapeBitsLine, *spritePtr, *baseSpritePtr, *pShapeBits, *pBaseShapeBits;
	BYTE nCount;
	BYTE nChunkMode;
	WORD nRemHeight;

	pShapeBitsLine = &shapeBits[right + shapeX * bottom];
	pBaseShapeBitsLine = &shapeBaseBits[right + shapeX * bottom];
	nRemHeight = shapeCurrent[nSpriteID].wHeight;
	spritePtr = shapePtr;
	baseSpritePtr = baseShapePtr;
	pShapeBits = pShapeBitsLine;
	pBaseShapeBits = pBaseShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			pBaseShapeBits = &pBaseShapeBitsLine[shapeX];
			pBaseShapeBitsLine += shapeX;
			--nRemHeight;
			break;
		case MIF_CM_SKIPPIXELS:
			if (isFlipped) {
				pShapeBits -= nCount;
				pBaseShapeBits -= nCount;
			}
			else {
				pShapeBits += nCount;
				pBaseShapeBits += nCount;
			}
			break;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr, ++baseSpritePtr) {
				BOOL bProcessBit = (isRoadMask) ? FALSE : TRUE;
				if (isRoadMask) {
					if (*pBaseShapeBits == 0xA1)
						bProcessBit = TRUE;
				}
				if (bProcessBit) {
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
					*pBaseShapeBits = *baseSpritePtr;
				}
				if (isFlipped) {
					--pShapeBits;
					--pBaseShapeBits;
				}
				else {
					++pShapeBits;
					++pBaseShapeBits;
				}
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++spritePtr;
				++baseSpritePtr;
			}
			break;
		default:
			return;
		}
	}
}

static void L_drawShape_WithBase_OutOfContext(BYTE *shapePtr, BYTE *baseShapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped) {
	__int16 leftEdge, topEdge, rightEdge, bottomEdge;
	BYTE *pShapeBitsLine, *pBaseShapeBitsLine, *spritePtr, *baseSpritePtr;
	WORD nRemHeight;
	int leftShapeBits, rightShapeBits, leftBaseShapeBits, rightBaseShapeBits;
	BYTE *pShapeBits, *pBaseShapeBits, nCount, nChunkMode;
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
	pBaseShapeBitsLine = &shapeBaseBits[right + shapeX * bottom];
	nRemHeight = shapeCurrent[nSpriteID].wHeight;
	spritePtr = shapePtr;
	baseSpritePtr = baseShapePtr;
	if (topEdge > 0) {
		bottomEdge -= topEdge;
		pShapeBitsLine += shapeX * topEdge;
		pBaseShapeBitsLine += shapeX * topEdge;
		do {
			spritePtr += SPRITEDATA(spritePtr)->nCount + 2;
			baseSpritePtr += SPRITEDATA(baseSpritePtr)->nCount + 2;
			--topEdge;
		} while (topEdge);
	}
	leftShapeBits = (int)pShapeBitsLine;
	pShapeBits = pShapeBitsLine;
	rightShapeBits = (int)pShapeBitsLine;
	leftBaseShapeBits = (int)pBaseShapeBitsLine;
	pBaseShapeBits = pBaseShapeBitsLine;
	rightBaseShapeBits = (int)pBaseShapeBitsLine;
	while (TRUE) {
		nCount = SPRITEDATA(spritePtr)->nCount;
		nChunkMode = SPRITEDATA(spritePtr)->nChunkMode;
		spritePtr = (BYTE *)&SPRITEDATA(spritePtr)->pBuf;
		baseSpritePtr = (BYTE *)&SPRITEDATA(baseSpritePtr)->pBuf;
		switch (nChunkMode) {
		case MIF_CM_EMPTY:
			continue;
		case MIF_CM_NEWROWSTART:
			leftShapeBits = leftEdge;
			rightShapeBits = rightEdge;
			leftBaseShapeBits = leftEdge;
			rightBaseShapeBits = rightEdge;
			bReachedBottom = --bottomEdge < 0;
			pShapeBits = &pShapeBitsLine[shapeX];
			pShapeBitsLine += shapeX;
			pBaseShapeBits = &pBaseShapeBitsLine[shapeX];
			pBaseShapeBitsLine += shapeX;
			--nRemHeight;
			if (!bReachedBottom)
				continue;
			break;
		case MIF_CM_SKIPPIXELS:
			leftShapeBits -= nCount;
			rightShapeBits -= nCount;
			leftBaseShapeBits -= nCount;
			rightBaseShapeBits -= nCount;
			if (isFlipped) {
				pShapeBits -= nCount;
				pBaseShapeBits -= nCount;
			}
			else {
				pShapeBits += nCount;
				pBaseShapeBits += nCount;
			}
			continue;
		case MIF_CM_PROCPIXELS:
			for (int nPos = nCount; nPos; ++spritePtr, ++baseSpritePtr) {
				BOOL bProcessBit = FALSE;
				if (isFlipped) {
					if (rightShapeBits <= 0 && leftShapeBits > 0)
						bProcessBit = (isRoadMask && *pBaseShapeBits != 0xA1) ? FALSE : TRUE;
				}
				else {
					if (leftShapeBits <= 0 && rightShapeBits > 0)
						bProcessBit = (isRoadMask && *pBaseShapeBits != 0xA1) ? FALSE : TRUE;
				}
				if (bProcessBit) {
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos);
					*pBaseShapeBits = *baseSpritePtr;
				}
				--leftShapeBits;
				--leftBaseShapeBits;
				if (isFlipped) {
					--pShapeBits;
					--pBaseShapeBits;
				}
				else {
					++pShapeBits;
					++pBaseShapeBits;
				}
				--rightShapeBits;
				--rightBaseShapeBits;
				--nPos;
			}
			if ((nCount & 1) != 0) {
				++spritePtr;
				++baseSpritePtr;
			}
			continue;
		default:
			return;
		}
		break;
	}
}

static void L_drawShapeSpecific_MainArea(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped, int nType) {
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
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos, nType);
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

static void L_drawShapeSpecific_OutOfContext(BYTE *shapePtr, __int16 nSpriteID, __int16 right, __int16 bottom, BOOL isRoadMask, BOOL isFlipped, int nType) {
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
					*pShapeBits = ProcessSpritePaletteIndex(nSpriteID, *spritePtr, nRemHeight, nPos, nType);
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
	BYTE *shapeData, *baseShapeData;
	int nShapeBottom, nShapeRight;

	shapePtr = &shapeCurrent[nSpriteID];
	if (shapePtr) {
		shapeData = Get_SpriteCache_Buffer(shapePtr, nSpriteID);
		if (shapeData) {
			nShapeBottom = bottom + Get_SpriteCache_Height(shapePtr, nSpriteID);
			nShapeRight = right + Get_SpriteCache_Width(shapePtr, nSpriteID);
			if (shapeRight > right && shapeLeft < nShapeRight && shapeBottom > bottom && shapeTop < nShapeBottom) {
				int nRight = (isFlipped) ? nShapeRight : right;
				// The 'doInvert' flag is used from the InvertShape and InvertTerrain calls.
				// It's the "Placement Preview".
				baseShapeData = Get_SpriteCache_BaseBuffer(shapePtr, nSpriteID);
				if (baseShapeData) {
					if (doInvert) {
						if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight) {
							if (shapeBaseBits) {

							}
							else
								L_drawShape_Invert_OutOfContext(shapeData, baseShapeData, nSpriteID, right, bottom);
						}
						else {
							if (shapeBaseBits) {

							}
							else
								L_drawShape_Invert_MainArea(shapeData, baseShapeData, nSpriteID, right, bottom);
						}
					}
					else if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight) {
						if (shapeBaseBits) {
							L_drawShape_WithBase_OutOfContext(shapeData, baseShapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
						}
						else
							L_drawShape_OutOfContext(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
					}
					else {
						if (shapeBaseBits) {
							L_drawShape_WithBase_MainArea(shapeData, baseShapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
						}
						else
							L_drawShape_MainArea(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped);
					}
				}
			}
		}
	}
}

void L_drawShapeSpecific_SC2K1996(__int16 nSpriteID, __int16 right, __int16 bottom, __int16 isFlipped, __int16 doInvert, int nType) {
	sprite_header_t *shapePtr;
	BYTE *shapeData, *baseShapeData;
	int nShapeBottom, nShapeRight;

	shapePtr = &shapeCurrent[nSpriteID];
	if (shapePtr) {
		shapeData = Get_SpecificSpriteCache_Buffer(shapePtr, nSpriteID, nType);
		if (shapeData) {
			nShapeBottom = bottom + Get_SpriteCache_Height(shapePtr, nSpriteID);
			nShapeRight = right + Get_SpriteCache_Width(shapePtr, nSpriteID);
			if (shapeRight > right && shapeLeft < nShapeRight && shapeBottom > bottom && shapeTop < nShapeBottom) {
				int nRight = (isFlipped) ? nShapeRight : right;
				// The 'doInvert' flag is used from the InvertShape and InvertTerrain calls.
				// It's the "Placement Preview".
				if (doInvert) {
					baseShapeData = Get_SpriteCache_BaseBuffer(shapePtr, nSpriteID);
					if (baseShapeData) {
						if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight)
							L_drawShapeSpecific_Invert_OutOfContext(shapeData, baseShapeData, nSpriteID, nRight, bottom, isFlipped);
						else
							L_drawShapeSpecific_Invert_MainArea(shapeData, baseShapeData, nSpriteID, nRight, bottom, isFlipped);
					}
				}
				else if (shapeTop >= bottom || shapeLeft >= right || shapeBottom <= nShapeBottom || shapeRight <= nShapeRight)
					L_drawShapeSpecific_OutOfContext(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped, nType);
				else
					L_drawShapeSpecific_MainArea(shapeData, nSpriteID, nRight, bottom, FALSE, isFlipped, nType);
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
		if (iTerrainCosmeticMode) {
			// Ordering:
			// Grey (don't include 0xA1 in the range to avoid traffic disruption)
			// Green
			// Hot
			//
			// It should be noted that as a result of the inclusion of these cases
			// when a cosmetic terrain mode is enabled, the shadow "can" intersect
			// with a non-terrain tile - this is more noticeable when a monster is
			// active (Revisit for future improvements).
			if (currIdx >= 0xA0 && currIdx <= 0xA7 && currIdx != 0xA1)
				return currIdx;
			else if ((currIdx >= 0x34 && currIdx <= 0x36) || (currIdx >= 0x3D && currIdx <= 0x3F) || (currIdx >= 0x45 && currIdx <= 0x4A))
				return currIdx;
			else if ((currIdx >= 0x20 && currIdx <= 0x22) || (currIdx >= 0x24 && currIdx <= 0x2A))
				return currIdx;
		}
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
		shapeData = Get_SpriteCache_Buffer(shapePtr, nSpriteID);
		if (shapeData) {
			nShapeTop = top + Get_SpriteCache_Height(shapePtr, nSpriteID);
			nShapeLeft = left + Get_SpriteCache_Width(shapePtr, nSpriteID);
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
		shapeData = Get_SpriteCache_Buffer(shapePtr, nSpriteID);
		if (shapeData) {
			nShapeBottom = bottom + Get_SpriteCache_Height(shapePtr, nSpriteID);
			nShapeRight = right + Get_SpriteCache_Width(shapePtr, nSpriteID);
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
