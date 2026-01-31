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

#define DECR_LARGE 12
#define DECR_SMALL (DECR_LARGE / 2)
#define DECR_TINY (DECR_SMALL / 2)

#define MDRAWING_DEBUG_OTHER 1

#define MDRAWING_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MDRAWING_DEBUG
#define MDRAWING_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mdrawing_debug = MDRAWING_DEBUG;

static DWORD dwDummy;

static int DoWaterfallEdge(__int16 shpWidth, int iX, int iY, __int16 iBottom, __int16 iWaterFallSpriteID, __int16 iDecr) {
	__int16 iTopog;

	if (iX < GAME_MAP_SIZE &&
		iY < GAME_MAP_SIZE &&
		XBITReturnIsWater(iX, iY)) {
		iTopog = ALTMReturnWaterLevel(iX, iY) - ALTMReturnLandAltitude(iX, iY);
		if (iTopog > 0) {
			while (rcDst.top <= iBottom) {
				if (rcDst.bottom > iBottom)
					Game_DrawProcessObject(iWaterFallSpriteID, shpWidth, iBottom, 0, 0);
				iBottom -= iDecr;
				if (--iTopog <= 0)
					return 2;
			}
			return 0;
		}
	}
	return 1;
}

static int DoMapEdge(__int16 shpWidth, int iX, int iY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iWaterFallSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, shpWidth, iBottom, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				return DoWaterfallEdge(shpWidth, iX, iY, iBottom, iWaterFallSpriteID, iDecr);
		}
	}
	return 1;
}

static void DoBedrockEdge(__int16 shpWidth, __int16 iOffSetX, __int16 iOffSetY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, shpWidth + iOffSetX, iBottom + iOffSetY, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				break;
		}
	}
}

extern "C" void __stdcall Hook_DrawAllLarge() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iShpWidth, iShpHeight;
	__int16 iScan, iX, iY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		rcDst.left -= GAME_MAP_SIZE;
		rcDst.bottom += 220;
		rcDst.top -= 32;

		// Top
	iScan = MAP_EDGE_MIN;
		iX = MAP_EDGE_MIN;
		iY = MAP_EDGE_MIN;
		iShpWidth = iScreenOffSetX;
		iShpHeight = iScreenOffSetY;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth) {
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
			}
			if (iY) {
				++iX;
				--iY;
				iShpWidth += 32;
			}
			else {
				++iScan;
				iX = MAP_EDGE_MIN;
				iY = iScan;
				iShpWidth = iScreenOffSetX - 16 * iScan;
				iShpHeight += 8;
			}
		} while (iScan < GAME_MAP_SIZE);

		// Bottom
		iScan = MAP_EDGE_MIN + 1;
		iX = MAP_EDGE_MIN + 1;
		iY = MAP_EDGE_MAX;
		iShpWidth = iScreenOffSetX - 2016;
		iShpHeight = iScreenOffSetY + 1024;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth) {
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
			}
			if (iX == MAP_EDGE_MAX) {
				++iScan;
				iX = iScan;
				iY = MAP_EDGE_MAX;
				iShpWidth = 16 * (iScan - MAP_EDGE_MAX) + iScreenOffSetX;
				iShpHeight += 8;
			}
			else {
				++iX;
				--iY;
				iShpWidth += 32;
			}
		} while (iScan < GAME_MAP_SIZE);
	}
}

extern "C" void __cdecl Hook_DrawLargeTile(__int16 shpWidth, __int16 shpHeight, int iX, int iY) {
	__int16 iBottom;
	__int16 iTop;
	__int16 iSprTop;
	__int16 iAltTop;
	__int16 iLandAlt;
	__int16 iSprite;
	__int16 iTrafficSprite;
	__int16 iThing;
	BOOL bIsFlipped;
	BYTE iTerrainTile;
	BYTE iTile;
	BYTE iZone;
	BYTE iOff;
	BYTE iTraffic, iLowTrfTheshold, iHeavyTrfThreshold;
	BYTE iTrafficTile;

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = shpHeight - pArrSpriteHeaders[SPRITE_LARGE_BEDROCK].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(shpWidth, iX, iY, iBottom, SPRITE_LARGE_WATERFALL, DECR_LARGE))
				return;
		}
	}

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = shpHeight - 12 * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
		else
			iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (XTXTGetTextOverlayID(iX, iY))
			Game_DrawLabel(iX, iY, shpWidth, iTop);
		return;
	}
	if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						iOff = GetTileCoverage(iTile);
						iSprite = iTile + SPRITE_LARGE_START;
						if (iOff >= 0) {
							iBottom = shpHeight - pArrSpriteHeaders[SPRITE_LARGE_BEDROCK].wHeight;
							iLandAlt = ALTMReturnLandAltitude(iX, iY);
							if (iOff == 0) {
								if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
									DoBedrockEdge(shpWidth,  0,  0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
							}
							else if (iOff == 1) {
								if (iX + 1 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 32,  0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 16,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 0,   0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 16,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
							else if (iOff == 2) {
								if (iX + 2 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 64,  0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 48,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 32, 16, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 0,   0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 16,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 32, 16, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
							else if (iOff == 3) {
								if (iX + 3 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 96,  0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 80,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 64, 16, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 48, 24, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, 0,   0, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 16,  8, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 32, 16, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, 48, 24, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
						}
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						Game_DrawProcessObject(iSprite, shpWidth, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - 8, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - 16, iTop - 16, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
					iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + SPRITE_LARGE_GREENTILE;
				else
					iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
			if (!DisplayLayer[LAYER_INFRANATURE]) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if (XTXTGetTextOverlayID(iX, iY))
					Game_DrawLabel(iX, iY, shpWidth, iTop);
				return;
			}
			if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
				iSprTop = shpHeight - 12 * iAltTop;
				if (iTerrainTile == TERRAIN_13)
					iSprTop = iTop - 12;
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if(iTile == TILE_RAISING_BRIDGE_LOWERED) {
					if (wActiveShips) {
						for (iThing = MIN_THING_IDX; iThing <= MAX_THING_IDX; ++iThing) {
							if (XTHGGetType(iThing) == XTHG_CARGO_SHIP)
								break;
						}
						if (iThing != MAX_THING_IDX + 1) {
							map_XTHG_t *pThing = GetXTHG(iThing);

							if (Game_ShipApproachingRaisingBridge(iX, iY, pThing->iX, pThing->iY) < 8)
								iTile = TILE_RAISING_BRIDGE_RAISED;
						}
					}
				}
				iSprite = iTile + SPRITE_LARGE_START;
				iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				if (iSprite < SPRITE_LARGE_HIGHWAY_LR || iSprite >= SPRITE_LARGE_SUSPENSION_BRIDGE_START_B)
					iLowTrfTheshold = 85;
				else
					iLowTrfTheshold = 28;
				iHeavyTrfThreshold = iLowTrfTheshold * 2;
				if (iTraffic > iLowTrfTheshold) {
					iTrafficTile = trafficSpriteOverlay[iSprite];
					if (iTrafficTile) {
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (iTrafficTile == 11) {
							if (!IsEven(iX))
								iTrafficTile = 12;
						}
						else if (iTrafficTile == 12) {
							bIsFlipped = TRUE;
							if (!IsEven(iY))
								iTrafficTile = 11;
						}
						if (iTraffic > iHeavyTrfThreshold)
							iTrafficTile = trafficSpriteOverlayLevels[iTrafficTile];
						iTrafficSprite = iTrafficTile + SPRITE_LARGE_FIRE4;
						Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
					}
				}
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
					Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - 16, iTop - 16, 0, 0);
				}
			}
			else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + 16, iTop - pArrSpriteHeaders[iSprite].wHeight - 8, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + 32, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + 16, iTop - pArrSpriteHeaders[iSprite].wHeight + 8, 0, 0);
				iSprite = iTile + SPRITE_LARGE_START;
				iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + 8;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				iLowTrfTheshold = 28;
				iHeavyTrfThreshold = iLowTrfTheshold * 2;
				if (iTraffic > iLowTrfTheshold) {
					iTrafficTile = trafficSpriteOverlay[iSprite];
					if (iTraffic > iHeavyTrfThreshold)
						iTrafficTile = trafficSpriteOverlayLevels[iTrafficTile];
					if (iTrafficTile)
						iTrafficSprite = iTrafficTile + SPRITE_LARGE_FIRE4;
					Game_DrawProcessObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0, 0);
				}
			}
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
		iSprTop = shpHeight - 12 * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - 12;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
		else
			iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (!DisplayLayer[LAYER_INFRANATURE]) {
			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabel(iX, iY, shpWidth, iTop);
			return;
		}
		iSprite = iTile + SPRITE_LARGE_START;
		Game_DrawProcessObject(iSprite, shpWidth, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
			Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - 16, iTop - 16, 0, 0);
			return;
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, shpWidth, iTop);
}

extern "C" void __cdecl Hook_DrawTinyTile(__int16 shpWidth, __int16 shpHeight, int iX, int iY) {

}

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	VirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawLargeTile
	VirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);

	// Hook for DrawTinyTile
	//VirtualProtect((LPVOID)0x4022D9, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJMP((LPVOID)0x4022D9, Hook_DrawTinyTile);

	// This disables the edge-checker for >= 2x2 buildings.
	// *** REMEMBER TO REMOVE AT THE END ***
	VirtualProtect((LPVOID)0x44056E, 166, PAGE_EXECUTE_READWRITE, &dwDummy);
	memset((LPVOID)0x44056E, 0x90, 166);
}
