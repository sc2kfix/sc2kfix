// sc2kfix hooks/hook_terrainhandling.cpp: hooks to do with terrain handling calls.
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>
#include <stack>

#include <sc2kfix.h>
#include "../resource.h"

#define SOUND_YIELD_TICS 60

#define VALID_BRIDGETYPEAREA_NOTSINGLE -1
#define VALID_BRIDGETYPEAREA_NO        0
#define VALID_BRIDGETYPEAREA_YES       1

static void L_Demolish_DirtyAndSetTerrainTile(mapcoord_t x, mapcoord_t y) {
	Game_DirtyTile(x, y);
	Game_SetTerrainTile(x, y);
}

// Dust-cloud during explosions.
static int16_t L_Demolish_GetDustCloudSprite(int16_t nSpriteBase) {
	return (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
}

static void L_Demolish_DoDustCloud(int16_t nSpriteID, mapcoord_t explodeX, mapcoord_t explodeY) {
	Game_DrawProcessObject(nSpriteID, explodeX, explodeY, rand() & 1, 0);
	Game_DirtyCloud(nSpriteID, explodeX, explodeY);
}

static bool L_Demolish_IsValidSingleBridgeTypeTile(mapcoord_t cornerX, mapcoord_t cornerY) {
	// Only tile-types that are specified within the range are valid.
	// The reinforced bridge tiles aren't valid and aren't included.
	uint8_t nTileID = GetTileID(cornerX, cornerY);
	return (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ELEVATED_POWERLINES)) ? true : false;
}

static int L_Demolish_IsValidBridgeTypeArea(mapcoord_t cornerX, mapcoord_t cornerY, int16_t nArea, int16_t *nOutHighwayRet) {
	int16_t nHighwayRet = Game_GetHighwayTilePlacementType(cornerX, cornerY);
	*nOutHighwayRet = nHighwayRet;
	if (nArea != 2 || nHighwayRet < HIGHWAY_BRIDGE_LR) {
		if (nArea != 1) {
			return VALID_BRIDGETYPEAREA_NOTSINGLE;
		}
		if (!L_Demolish_IsValidSingleBridgeTypeTile(cornerX, cornerY))
			return VALID_BRIDGETYPEAREA_NO;
	}
	return VALID_BRIDGETYPEAREA_YES;
}

// This checks to see whether the highway tile is constructed of
// a distinct 1x1 object (of the non-crossover variety). I suspect
// this is likely down to the 1x1 crossover highway-type tiles
// not being present on a non-flat surface, so it wouldn't then go
// "out-of-spec" with what's expected during terrain reversion.
static bool L_Demolish_HighwayTileUnitCheck(uint8_t nTileID) {
	return (GET_TILE_RANGE(nTileID, TILE_HIGHWAY_LR, TILE_HIGHWAY_TB));
}

// This function is hit twice during demolition, but only during bridge-type object clearing
// and only if their nArea is 1 (for bSingleObject).
// It is for removing the entry/exit points and reverting the altitude and terrain back to the
// water type.
// The bClearFlipped boolean is only set to true on the first call (likely to account for the
// entry or exit point being a flipped tile).
static void L_Demolish_ClearEntryExitAndSetTerrain(mapcoord_t cornerX, mapcoord_t cornerY, int16_t nArea, bool bClearFlipped) {
	if (cornerX >= GAME_MAP_SIZE || cornerY >= GAME_MAP_SIZE || !XBITReturnIsWater(cornerX, cornerY)) {
		Game_DirtyTile(cornerX, cornerY);
		Game_PlaceTile(cornerX, cornerY, TILE_CLEAR);
		if (cornerX >= MAP_EDGE_MIN) {
			if (cornerX < GAME_MAP_SIZE && cornerY < GAME_MAP_SIZE) {
				WORD nLandAlt = ALTMReturnLandAltitude(cornerX, cornerY) - 1;
				ALTMSetLandAltitude(cornerX, cornerY, nLandAlt);
				XBITSetBits(cornerX, cornerY, XBIT_WATER);
			}
		}
		Game_SetTerrainTile(cornerX, cornerY);
		uint8_t xbitMask = 0;
		if (bClearFlipped)
			xbitMask = XBIT_FLIPPED;
		if (nArea == 1) {
			// Added this here so you have the ability to remove
			// the relevant power bits (powerline entry/exit bug case).
			if (GetAsyncKeyState(VK_MENU) < 0)
				xbitMask |= (XBIT_POWERED | XBIT_POWERABLE);
		}
		if (cornerX < GAME_MAP_SIZE && cornerY < GAME_MAP_SIZE)
			XBITClearBits(cornerX, cornerY, xbitMask);
	}
}

static void L_PierCheckStackPush(mapcoord_t x, mapcoord_t y) {
	uint32_t nTileID = GetTileID(x, y);
	if (nTileID) {
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_PIER, TILE_INFRASTRUCTURE_CRANE))
			Game_StackPush(x, y);
	}
}

static void L_RunwayCheckStackPush(mapcoord_t x, mapcoord_t y) {
	uint32_t nTileID = GetTileID(x, y);
	if (nTileID) {
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_RUNWAY, TILE_INFRASTRUCTURE_RUNWAYCROSS))
			Game_StackPush(x, y);
	}
}

static void L_Demolish_UpdateMainWindow(CSimcityView *pSCView) {
	if (pSCView == (CSimcityView *)&pSomeWnd)
		Game_SimcityView_MainWindowUpdate(pSCView, NULL, TRUE);
	else
		Game_SimcityView_MainWindowUpdate(pSCView, &dirtyRect, TRUE);
	UpdateWindow(pSCView->m_hWnd);
	Game_YieldToWindows(SOUND_YIELD_TICS);
}

static void L_Demolish_PlayExplosionSound(CSimcityAppPrimary *pSCApp) {
	if (pSCApp->dwSCAGameSound) {
		Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
		Game_YieldToWindows(SOUND_YIELD_TICS);
	}
}

static void L_Demolish_UpdHouse(CSimcityView *pSCView, mapcoord_t nX, mapcoord_t nY, int16_t nArea) {
	int16_t nAreaX = (nX + nArea);
	if (nAreaX > nX) {
		for (mapcoord_t nCurrX = nX; nCurrX < nAreaX; ++nCurrX) {
			Game_DirtyTile(nCurrX - 1, nY - 1);
			Game_DirtyTile(nCurrX - 2, nY - 2);
			Game_DirtyTile(nCurrX - 3, nY - 3);
			Game_DirtyTile(nCurrX - 4, nY - 4);
		}
	}
	int16_t nAreaY = (nY + nArea);
	if (nAreaY > nY) {
		for (mapcoord_t nCurrY = nY; nCurrY < nAreaY; ++nCurrY) {
			Game_DirtyTile(nX - 1, nCurrY - 1);
			Game_DirtyTile(nX - 2, nCurrY - 2);
			Game_DirtyTile(nX - 3, nCurrY - 3);
			Game_DirtyTile(nX - 4, nCurrY - 4);
		}
	}
	if (nAreaX > nX) {
		for (mapcoord_t nCurrX = nX; nCurrX < nAreaX; ++nCurrX) {
			for (mapcoord_t nCurrY = nY; nCurrY < nAreaY; ++nCurrY)
				Game_DirtyTile(nCurrX, nCurrY);
		}
	}
	Game_SimcityView_UpdateHouse(pSCView);
}

// XXX (araxestroy): This function needs some serious comment work.
extern "C" void __stdcall Hook_SimcityView_Demolish(mapcoord_t x, mapcoord_t y, BOOL bExplosion) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	uint8_t *pLockedBits = NULL;
	uint8_t *pLockedBaseBits = NULL;
	bool bSingleTile;
	bool bGeneralUpdate;
	int nValidBridgeType;
	mapcoord_t nX, nY;
	uint8_t nTileID, nIntermediateTile;
	mapcoord_t nCornerX, nCornerY;
	int16_t nArea;
	int16_t nCoordScale, nLandAltScale, nScaleVal;
	int16_t nHighwayRet;
	int16_t nMoveX, nMoveY;
	int16_t nSpriteBase, nSpriteID;
	mapcoord_t nExplodeX, nExplodeY, nAltitude;
	mapcoord_t nAreaExplodeX, nAreaExplodeY;
	mapcoord_t nAreaCornerX, nAreaCornerY;
	int16_t nRubbleTile;
	uint8_t bTextOverlay;
	CMFC3XPoint pt;
	coords_w_t tileCoords;

#if 0
	// Debugging and testing.
	if (!bWeatherEffects) {
		GameMain_SimcityView_Demolish(pThis, x, y, bExplosion);
		return;
	}
#endif

	pSCApp = &pCSimcityAppThis;
	pLockedBits = Game_Graphics_LockDIBBits(pThis->SCVGraphics);
	pLockedBaseBits = Game_Graphics_LockDIBBits(pBaseGraphics);
	bGeneralUpdate = true;
	nX = x;
	nY = y;
	nTileID = GetTileID(nX, nY);
	if (nTileID > TILE_RADIOACTIVITY) {
		nCornerX = nX;
		nCornerY = nY;
		nArea = Game_FindCorner(&nCornerX, &nCornerY, nTileID);
		nHighwayRet = (nArea == 2) ? Game_GetHighwayTilePlacementType(nCornerX, nCornerY - 1) : HIGHWAY_INVALID;
		nCoordScale = COORDSCALE_VAL(pThis->wSCVZoomLevel);
		nLandAltScale = LANDALTSCALE_VAL(pThis->wSCVZoomLevel);
		nScaleVal = SCALE_VAL(pThis->wSCVZoomLevel);
		nSpriteBase = SPRITE_BOUNDARY_MULTIPLIER * pThis->wSCVZoomLevel;
		Game_DirtyThing(wDisasterObject);
		if (nArea == 1 && (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ELEVATED_POWERLINES) || GET_TILE_RANGE(nTileID, TILE_REINFORCED_BRIDGE_PYLON, TILE_REINFORCED_BRIDGE)) ||
			nArea == 2 && nHighwayRet > HIGHWAY_LTBR) {
			if (nArea == 2)
				--nCornerY;
			// This block here determines the X/Y bridge tile direction as it advances from
			// entry to exit points.
			nMoveX = 0;
			nMoveY = 1;
			if (nArea == 1 && nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE && XBITReturnIsFlipped(nCornerX, nCornerY) ||
				nArea == 2 && (nHighwayRet & 1) == 0) {
				nMoveX = 1;
				nMoveY = 0;
			}
			nMoveX *= nArea;
			nMoveY *= nArea;
			// Move back until the entry point is hit.
			bSingleTile = true;
			while (true) {
				nValidBridgeType = L_Demolish_IsValidBridgeTypeArea(nCornerX, nCornerY, nArea, &nHighwayRet);
				if (nValidBridgeType <= VALID_BRIDGETYPEAREA_NO) {
					if (nValidBridgeType == VALID_BRIDGETYPEAREA_NOTSINGLE)
						bSingleTile = false;
					break;
				}
				nCornerX -= nMoveX;
				nCornerY -= nMoveY;
			}
			if (bSingleTile) {
				// Entry point.
				// bClearFlipped set to true to unset the 'flip' XBIT attribute
				// from either the entry or exit point.
				L_Demolish_ClearEntryExitAndSetTerrain(nCornerX, nCornerY, nArea, true);
			}
			if (nArea == 2 && !L_Demolish_HighwayTileUnitCheck(nTileID)) {
				L_Demolish_DirtyAndSetTerrainTile(nCornerX, nCornerY);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX + 1, nCornerY);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX + 1, nCornerY + 1);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX, nCornerY + 1);
			}
			// Advance in order to hit the relevant bridge tile.
			nCornerX += nMoveX;
			nCornerY += nMoveY;
			if (bExplosion)
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			nExplodeX = -1;
			nExplodeY = -1;
			// Move forward until the exit point is hit.
			bSingleTile = true;
			while (true) {
				nValidBridgeType = L_Demolish_IsValidBridgeTypeArea(nCornerX, nCornerY, nArea, &nHighwayRet);
				if (nValidBridgeType <= VALID_BRIDGETYPEAREA_NO) {
					if (nValidBridgeType == VALID_BRIDGETYPEAREA_NOTSINGLE)
						bSingleTile = false;
					break;
				}
				Game_DirtyTile(nCornerX, nCornerY);
				if (nArea == 2 && nHighwayRet < HIGHWAY_BRIDGE_REINFORCED) {
					Game_DirtyTile(nCornerX, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY);
				}
				if (bExplosion) {
					nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
					nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE && XBITReturnIsWater(nCornerX, nCornerY))
						nAltitude = ALTMReturnWaterLevel(nCornerX, nCornerY);
					else
						nAltitude = ALTMReturnLandAltitude(nCornerX, nCornerY);
					nExplodeY = iScreenOffSetY + nCoordScale * (nCornerX + nCornerY) - nLandAltScale * nAltitude - pArrSpriteHeaders[nSpriteID].wHeight;
					L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nCornerX, nCornerY);
						uint8_t xbitMask = XBIT_FLIPPED;
						if (nArea == 1) {
							// Added this here so you have the ability to remove
							// the relevant power bits (powerline entry/exit bug case).
							if (GetAsyncKeyState(VK_MENU) < 0)
								xbitMask |= (XBIT_POWERED | XBIT_POWERABLE);
						}
						XBITClearBits(nCornerX, nCornerY, xbitMask);
					}
				}
				if (nArea == 2) {
					if (bExplosion) {
						nAreaExplodeX = nExplodeX + nScaleVal;
						nAreaExplodeY = nExplodeY - nCoordScale;
						L_Demolish_DoDustCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
						nAreaExplodeX = nExplodeX + 2 * nScaleVal;
						L_Demolish_DoDustCloud(nSpriteID, nAreaExplodeX, nExplodeY);
						nAreaExplodeY = nExplodeY + nCoordScale;
						L_Demolish_DoDustCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
					}
					nAreaCornerY = nCornerY + 1;
					Game_PlaceTile(nCornerX, nAreaCornerY, TILE_CLEAR);
					if (nCornerX < GAME_MAP_SIZE && nAreaCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nCornerX, nAreaCornerY);
						XBITClearBits(nCornerX, nAreaCornerY, XBIT_FLIPPED);
					}
					nAreaCornerX = nCornerX + 1;
					Game_PlaceTile(nAreaCornerX, nAreaCornerY, TILE_CLEAR);
					if (nCornerX >= MAP_EDGE_MIN && nAreaCornerX < GAME_MAP_SIZE && nAreaCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nAreaCornerX, nAreaCornerY);
						XBITClearBits(nAreaCornerX, nAreaCornerY, XBIT_FLIPPED);
					}
					Game_PlaceTile(nAreaCornerX, nCornerY, TILE_CLEAR);
					if (nCornerX >= MAP_EDGE_MIN && nAreaCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nAreaCornerX, nCornerY);
						XBITClearBits(nAreaCornerX, nCornerY, XBIT_FLIPPED);
					}
				}
				nCornerX += nMoveX;
				nCornerY += nMoveY;
			}
			if (bSingleTile) {
				// Exit point.
				L_Demolish_ClearEntryExitAndSetTerrain(nCornerX, nCornerY, nArea, false);
			}
			if (nArea == 2 && !L_Demolish_HighwayTileUnitCheck(nTileID)) {
				L_Demolish_DirtyAndSetTerrainTile(nCornerX, nCornerY);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX + 1, nCornerY);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX + 1, nCornerY + 1);
				L_Demolish_DirtyAndSetTerrainTile(nCornerX, nCornerY + 1);
			}
			if (bExplosion)
				Game_FinishProcessObjects();
		}
		else if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_PIER, TILE_INFRASTRUCTURE_CRANE)) {
			if (bExplosion)
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			Game_InitStack(nX, nY);
			while (Game_StackSize()) {
				Game_StackPop(&pt);
				tileCoords.x = (mapcoord_t)pt.x;
				tileCoords.y = (mapcoord_t)pt.y;
				Game_PlaceTile(tileCoords.x, tileCoords.y, TILE_CLEAR);
				if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
					XZONClearCorners(tileCoords.x, tileCoords.y);
					XBITClearBits(tileCoords.x, tileCoords.y, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
				}
				if (bExplosion) {
					nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE && XBITReturnIsWater(tileCoords.x, tileCoords.y))
						nAltitude = ALTMReturnWaterLevel(tileCoords.x, tileCoords.y);
					else
						nAltitude = ALTMReturnLandAltitude(tileCoords.x, tileCoords.y);
					nExplodeY = iScreenOffSetY + (nCoordScale * (tileCoords.x + tileCoords.y)) - nLandAltScale * nAltitude - pArrSpriteHeaders[nSpriteID].wHeight;
					L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				if (tileCoords.x > MAP_EDGE_MIN)
					L_PierCheckStackPush(tileCoords.x - 1, tileCoords.y);
				if (tileCoords.x < MAP_EDGE_MAX)
					L_PierCheckStackPush(tileCoords.x + 1, tileCoords.y);
				if (tileCoords.y > MAP_EDGE_MIN)
					L_PierCheckStackPush(tileCoords.x, tileCoords.y - 1);
				// This one here was also tileCoords.x.
				// Most likely a bug, commenting just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_PierCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (bExplosion)
				Game_FinishProcessObjects();
		}
		else if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_RUNWAY, TILE_INFRASTRUCTURE_RUNWAYCROSS)) {
			if (bExplosion)
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			Game_InitStack(nX, nY);
			while (Game_StackSize()) {
				Game_StackPop(&pt);
				tileCoords.x = (mapcoord_t)pt.x;
				tileCoords.y = (mapcoord_t)pt.y;
				nRubbleTile = (rand() & 3) + 1;
				Game_PlaceTile(tileCoords.x, tileCoords.y, nRubbleTile);
				if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
					XZONClearCorners(tileCoords.x, tileCoords.y);
					XBITClearBits(tileCoords.x, tileCoords.y, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
				}
				if (bExplosion) {
					nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					nExplodeY = iScreenOffSetY + nCoordScale * (tileCoords.x + tileCoords.y) - nLandAltScale * ALTMReturnLandAltitude(tileCoords.x, tileCoords.y) - pArrSpriteHeaders[nSpriteID].wHeight;
					L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				if (tileCoords.x > MAP_EDGE_MIN)
					L_RunwayCheckStackPush(tileCoords.x - 1, tileCoords.y);
				if (tileCoords.x < MAP_EDGE_MAX)
					L_RunwayCheckStackPush(tileCoords.x + 1, tileCoords.y);
				if (tileCoords.y > MAP_EDGE_MIN)
					L_RunwayCheckStackPush(tileCoords.x, tileCoords.y - 1);
				// This one here was also tileCoords.x.
				// Most likely a bug, commenting just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_RunwayCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (bExplosion)
				Game_FinishProcessObjects();
		}
		else if (GET_TILE_RANGE(nTileID, TILE_TUNNEL_T, TILE_TUNNEL_L)) {
			// This block determines the X/Y direction based on the specific
			// Tunnel tile-type that's being demolished.
			nMoveX = 0;
			nMoveY = 0;
			switch (nTileID) {
				case TILE_TUNNEL_T:
					nMoveX = -1;
					nMoveY = 0;
					break;
				case TILE_TUNNEL_R:
					nMoveX = 0;
					nMoveY = -1;
					break;
				case TILE_TUNNEL_B:
					nMoveX = 1;
					nMoveY = 0;
					break;
				case TILE_TUNNEL_L:
					nMoveX = 0;
					nMoveY = 1;
					break;
				default:
					break;
			}
			Game_DirtyTile(nX, nY);
			// Addition: rubble is now placed at the entry point.
			nRubbleTile = (rand() & 3) + 1;
			Game_PlaceTile(nX, nY, nRubbleTile);
			if (nX >= MAP_EDGE_MIN) {
				if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE) {
					XZONClearCorners(nX, nY);
					ALTMSetTunnelLevels(nX, nY, 0);
				}
			}
			if (bExplosion) {
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
				nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
			}
			// This loop deals with the unsetting of the tunnel levels from entry to exit points.
			nIntermediateTile = GetTileID(nX, nY);
			while (!GET_TILE_RANGE(nIntermediateTile, TILE_TUNNEL_T, TILE_TUNNEL_L)) {
				nX += nMoveX;
				nY += nMoveY;
				if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
					ALTMSetTunnelLevels(nX, nY, 0);
				nIntermediateTile = GetTileID(nX, nY);
			}
			Game_DirtyTile(nX, nY);
			// Addition: rubble is now placed at the exit point.
			nRubbleTile = (rand() & 3) + 1;
			Game_PlaceTile(nX, nY, nRubbleTile);
			if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
				XZONClearCorners(nX, nY);
			if (bExplosion) {
				nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
				Game_FinishProcessObjects();
			}
		}
		else {
			if (bExplosion) {
				nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
				// There was a very old bug here in the game-side function whereas
				// instead of it using nCornerX/nCornerY for the 'if' it used nX and nY
				// which then resulted in an erroneous vertical offset of the explosive
				// effect depending on where you clicked on certain objects (the Marina
				// being a prime example).
				if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE && XBITReturnIsWater(nCornerX, nCornerY))
					nAltitude = ALTMReturnWaterLevel(nCornerX, nCornerY);
				else
					nAltitude = ALTMReturnLandAltitude(nCornerX, nCornerY);
				nExplodeY = iScreenOffSetY + (nCoordScale * (nCornerX + nCornerY)) - nLandAltScale * nAltitude;
				// For the Arcologies and Braun Llama Dome set dirtyRect.top to 0 to temporarily retain
				// the object image until the explosion has completed.
				if (nTileID >= TILE_ARCOLOGY_PLYMOUTH)
					dirtyRect.top = 0;
				int16_t nCurrPosHeightLimit = 0; // The current height limit for the explosion sprite
				for (int16_t nAreaPos = nArea; nAreaPos > 0; --nAreaPos) {
					L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
					int16_t nStartAreaExplodeX = nExplodeX; // The starting X tile position
					int16_t nStartPosY = 0;                 // The Y tile starting/reset position
					for (int16_t nHorzAreaPos = nArea; nHorzAreaPos > 0; --nHorzAreaPos) {
						nAreaExplodeX = nStartAreaExplodeX; // Get the (new) X position
						int16_t nCurrPosY = nStartPosY;     // Get the (new) Y position
						for (int16_t nVertAreaPos = nArea; nVertAreaPos > 0; --nVertAreaPos) {
							nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
							nAreaExplodeY = nCurrPosY + nExplodeY - pArrSpriteHeaders[nSpriteID].wHeight - nCurrPosHeightLimit;
							L_Demolish_DoDustCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
							nAreaExplodeX += nScaleVal; // Advance the current X position (nScaleVal)
							nCurrPosY -= nCoordScale;   // Move back on the current Y position (nCoordScale)
						}
						// Advance the starting positions (nScaleVal and nCoordScale respectively)
						nStartAreaExplodeX += nScaleVal;
						nStartPosY += nCoordScale;
					}
					Game_FinishProcessObjects();
					bGeneralUpdate = false;
					// This needs to be here, otherwise the explosive effect is
					// truncated.
					L_Demolish_UpdateMainWindow(pThis);
					// Set bGeneralUpdate to false in-order to not
					// hit the general version of UpdateMainWindow
					// (DrawHouse will still be called at the end as normal).
					nCurrPosHeightLimit += nCoordScale;
				}
				L_Demolish_PlayExplosionSound(pSCApp);
			}
			// This block here handles the following:
			// 1) During the demolition of certain rewards it toggles the grant back to available.
			// 2) During the demolition of a Stadium (assuming bTextOverlay is in-range), find the
			//    related Microsim that matches the nTileID and adjust wStadiumSportsTeams.
			bTextOverlay = XTXTGetTextOverlayID(nCornerX, nCornerY);
			if (nTileID == TILE_INFRASTRUCTURE_MAYORSHOUSE)
				Game_SimulationToggleGrantReward(0, 1);
			if (nTileID == TILE_SERVICES_CITYHALL)
				Game_SimulationToggleGrantReward(1, 1);
			if (nTileID == TILE_SERVICES_STATUE)
				Game_SimulationToggleGrantReward(2, 1);
			if (nTileID == TILE_OTHER_BRAUNLLAMADOME)
				Game_SimulationToggleGrantReward(3, 1);
			if (nTileID == TILE_SERVICES_STADIUM && bTextOverlay >= MIN_SIM_TEXT_ENTRIES && bTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
				uint8_t bMicrosimEntry = MICROSIMID_ENTRY(bTextOverlay);
				if (GetMicroSimulatorTileID(bMicrosimEntry) == nTileID)
					wStadiumSportsTeams += -1 << GetMicroSimulatorStat2(bMicrosimEntry);
			}
			for (int16_t nPosX = 0; nArea > nPosX; ++nPosX) {
				for (int16_t nPosY = 0; nArea > nPosY; ++nPosY) {
					mapcoord_t nCurrX = nPosX + nCornerX;
					mapcoord_t nCurrY = nCornerY - nPosY;
					if (nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
						// On TERRAIN_00 set down rubble, on anything else keep it clear.
						nRubbleTile = (GetTerrainTileID(nCurrX, nCurrY)) ? TILE_CLEAR : (rand() & 3) + 1;
						Game_PlaceTile(nCurrX, nCurrY, nRubbleTile);
						if (nCurrX >= MAP_EDGE_MIN && nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
							XBITClearBits(nCurrX, nCurrY, XBIT_FLIPPED | XBIT_POWERED | XBIT_POWERABLE);
							XZONClearCorners(nCurrX, nCurrY);
						}
						// In this block:
						// 1) Label removal
						// 2) Commerce/Industry connection removal
						bTextOverlay = XTXTGetTextOverlayID(nCurrX, nCurrY);
						if (bTextOverlay) {
							if (bTextOverlay <= MAX_XTHG_TEXT_ENTRIES || bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
								if (bTextOverlay <= MAX_SIM_TEXT_ENTRIES || bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY)
									XTXTSetTextOverlayID(nCurrX, nCurrY, 0);
								Game_RemoveLabel(bTextOverlay);
								if (bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
									if (GET_TILE_RANGE(nTileID, TILE_ROAD_LR, TILE_ROAD_LTBR) ||
										GET_TILE_RANGE(nTileID, TILE_TUNNEL_T, TILE_CROSSOVER_ROADTB_RAILLR) ||
										GET_TILE_RANGE(nTileID, TILE_CROSSOVER_HIGHWAYLR_ROADTB, TILE_CROSSOVER_HIGHWAYTB_ROADLR) ||
										GET_TILE_RANGE(nTileID, TILE_ONRAMP_TL, TILE_ONRAMP_BR))
										--wCommerceConnect;
									else
										--wIndustryConnect;
								}
							}
						}
					}
				}
			}
			// It should be noted here that when it comes to the demolition of
			// certain highway tiles on inclines (partially or otherwise) it will
			// leave the terrain permanently altered (this is native-original behaviour).
			//
			// Another detail that stands out is the lack of rubble, it's possible that
			// any that was placed previously is subsequently removed during SetTerrainTile().
			if (GET_TILE_RANGE(nTileID, TILE_HIGHWAY_HTB, TILE_REINFORCED_BRIDGE) ||
				GET_TILE_RANGE(nTileID, TILE_HIGHWAY_LR, TILE_CROSSOVER_HIGHWAYTB_POWERLR)) {
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY - 1);
				Game_SetTerrainTile(nCornerX, nCornerY - 1);
			}
			// This case here is encountered while demolishing certain objects on hills.
			// It should be noted that this case is also hit when you demolish a specific
			// entry/exit point of a bridge - though it doesn't try to restore the original
			// terrain under that circumstance (restoration only occurs after the main bridge
			// portion is demolished).
			if (nArea == 1 && nTileID < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
				if (GetTerrainTileID(nCornerX, nCornerY))
					Game_SetTerrainTile(nCornerX, nCornerY);
			}
		}
		// Except for when building area explosion dust clouds are generated,
		// all other update and sound play situations occur here.
		if (bGeneralUpdate) {
			if (bExplosion) {
				L_Demolish_UpdateMainWindow(pThis);
				L_Demolish_PlayExplosionSound(pSCApp);
			}
		}
		L_Demolish_UpdHouse(pThis, nX, nY, nArea);
	}
}

void InstallTerrainHandlingHooks_SC2K1996(void) {
	// Hook for CSimcityView::Demolish()
	SafeVirtualProtect((LPVOID)0x402211, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402211, Hook_SimcityView_Demolish);
}
