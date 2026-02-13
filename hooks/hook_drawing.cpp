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
					bBlock = GetXPOPByteDataWithNormalCoordinates(iX, iY);
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

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	VirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawLargeTile
	VirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);

	// Hook for DrawSmallTile
	VirtualProtect((LPVOID)0x401E79, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401E79, Hook_DrawSmallTile);

	// Hook for DrawTinyTile
	VirtualProtect((LPVOID)0x4022D9, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4022D9, Hook_DrawTinyTile);

	// Hook for DrawAllUnder
	VirtualProtect((LPVOID)0x40251D, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x40251D, Hook_DrawAllUnder);

	// Hook for DrawUnderTile
	VirtualProtect((LPVOID)0x402D9C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402D9C, Hook_DrawUnderTile);

	// Hook for DrawColorTile
	VirtualProtect((LPVOID)0x402F6D, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402F6D, Hook_DrawColorTile);

	UpdateDrawingHooks_SC2K1996();
}

void UpdateDrawingHooks_SC2K1996(void) {
	COLORREF undgrndBkgnd;

	undgrndBkgnd = PALETTERGB(192, 192, 192); // Default
	if (bSettingsDarkUndergroundBkgnd)
		undgrndBkgnd = PALETTERGB(60, 60, 60);    // Dark Grey

	// Set via InitializeDataColorsFonts() first (on program load).
	VirtualProtect((LPVOID)0x42C008, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(COLORREF *)0x42C008 = undgrndBkgnd;

	// Set to the actual variable second (during runtime).
	colGameBackgndUnder = undgrndBkgnd;

	if (bMapWireFrame) {
		// Set via InitializeDataColorsFonts() first (on program load).
		VirtualProtect((LPVOID)0x42BFFE, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
		*(COLORREF *)0x42BFFE = undgrndBkgnd;

		// Set to the actual variable second (during runtime).
		colGameBackgndAbove = undgrndBkgnd;
	}
}
