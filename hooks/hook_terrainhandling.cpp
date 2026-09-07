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

#define VALID_BRIDGETYPEAREA_NOTSINGLE -1
#define VALID_BRIDGETYPEAREA_NO        0
#define VALID_BRIDGETYPEAREA_YES       1

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

static int L_Demolish_IsValidBridgeTypeArea(mapcoord_t cornerX, mapcoord_t cornerY, __int16 nArea, __int16 *nOutHighwayRet) {
	__int16 nHighwayRet = Game_ValidateHighwayTilePlacementType(cornerX, cornerY);
	*nOutHighwayRet = nHighwayRet;
	if (nArea != 2 || nHighwayRet < 13) {
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
static void L_Demolish_ClearEntryExitAndSetTerrain(mapcoord_t cornerX, mapcoord_t cornerY, bool bClearFlipped) {
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
		if (bClearFlipped) {
			if (cornerX < GAME_MAP_SIZE && cornerY < GAME_MAP_SIZE)
				XBITClearBits(cornerX, cornerY, XBIT_FLIPPED);
		}
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

static void L_Demolish_UpdHouse(CSimcityView *pSCView, mapcoord_t nX, mapcoord_t nY, int16_t nArea) {
	mapcoord_t nFirstPosX = nX;
	mapcoord_t nSecondPosX = nX;
	mapcoord_t nAreaPosX = (nX + nArea);
	if (nAreaPosX > nX) {
		do {
			mapcoord_t nCurrPosX = nSecondPosX++;
			Game_DirtyTile(nCurrPosX - 1, nY - 1);
			Game_DirtyTile(nCurrPosX - 2, nY - 2);
			Game_DirtyTile(nCurrPosX - 3, nY - 3);
			Game_DirtyTile(nCurrPosX - 4, nY - 4);
		} while (nSecondPosX < nAreaPosX);
	}
	mapcoord_t nFirstPosY = nY;
	mapcoord_t nSecondPosY = nY;
	mapcoord_t nAreaPosY = (nY + nArea);
	if (nAreaPosY > nY) {
		do {
			mapcoord_t nCurrPosY = nSecondPosY++;
			Game_DirtyTile(nX - 1, nCurrPosY - 1);
			Game_DirtyTile(nX - 2, nCurrPosY - 2);
			Game_DirtyTile(nX - 3, nCurrPosY - 3);
			Game_DirtyTile(nX - 4, nCurrPosY - 4);
		} while (nSecondPosY < nAreaPosY);
	}
	if (nAreaPosX > nX) {
		do {
			for (mapcoord_t nCurrPosY = nFirstPosY; nCurrPosY < nAreaPosY; ++nCurrPosY)
				Game_DirtyTile(nFirstPosX, nCurrPosY);
			++nFirstPosX;
		} while (nFirstPosX < nAreaPosX);
	}
	Game_SimcityView_UpdateHouse(pSCView);
}

// XXX (araxestroy): This function needs some serious comment work.
extern "C" void __stdcall Hook_SimcityView_Demolish(mapcoord_t x, mapcoord_t y, BOOL bExplosion) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	BYTE *pLockedBits = NULL;
	BYTE *pLockedBaseBits = NULL;
	bool bSingleTile;
	int nValidBridgeType;
	bool bExplosionSoundPlayed;
	bool bOnlyUpdateHouse;
	mapcoord_t nX, nY;
	uint8_t nTileID;
	mapcoord_t nCornerX, nCornerY;
	int16_t nArea;
	int16_t nCoordScale, nLandAltScale, nScaleVal;
	int16_t nHighwayRet;
	int16_t nHorzMult, nVertMult;
	int16_t nStoredHorzMult, nStoredVertMult;
	int16_t nSpriteBase, nSpriteID;
	mapcoord_t nExplodeX, nExplodeY, nAltitude;
	mapcoord_t nAreaExplodeX, nAreaExplodeY;
	mapcoord_t nAreaCornerX, nAreaCornerY;
	mapcoord_t nOffsetX, nOffsetY;
	int16_t nRubbleTile;
	BYTE bTextOverlay;
	CMFC3XPoint pt;
	coords_w_t tileCoords;

#if 1
	// Debugging and testing.
	if (GetAsyncKeyState(VK_MENU) < 0) {
		GameMain_SimcityView_Demolish(pThis, x, y, bExplosion);
		return;
	}
#endif

	pSCApp = &pCSimcityAppThis;
	pLockedBits = Game_Graphics_LockDIBBits(pThis->SCVGraphics);
	pLockedBaseBits = Game_Graphics_LockDIBBits(pBaseGraphics);
	bExplosionSoundPlayed = false;
	bOnlyUpdateHouse = false;
	nX = x;
	nY = y;
	nTileID = GetTileID(nX, nY);
	if (nTileID > TILE_RADIOACTIVITY) {
		nCornerX = nX;
		nCornerY = nY;
		nArea = Game_FindCorner(&nCornerX, &nCornerY, nTileID);
		nHighwayRet = (nArea == 2) ? Game_ValidateHighwayTilePlacementType(nCornerX, nCornerY - 1) : -1;
		nCoordScale = COORDSCALE_VAL(pThis->wSCVZoomLevel);
		nLandAltScale = LANDALTSCALE_VAL(pThis->wSCVZoomLevel);
		nScaleVal = SCALE_VAL(pThis->wSCVZoomLevel);
		nSpriteBase = SPRITE_BOUNDARY_MULTIPLIER * pThis->wSCVZoomLevel;
		Game_DirtyThing(wDisasterObject);
		ConsoleLog(LOG_DEBUG, "coord(%d, %d) cornercoord(%d, %d) nArea(%d) [%s]\n", nX, nY, nCornerX, nCornerY, nArea, szTileNames[nTileID]);
		if (nArea == 1 && (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ELEVATED_POWERLINES) || GET_TILE_RANGE(nTileID, TILE_REINFORCED_BRIDGE_PYLON, TILE_REINFORCED_BRIDGE)) ||
			nArea == 2 && nHighwayRet >= 13) {
			ConsoleLog(LOG_DEBUG, "in 'if': coord(%d, %d) cornercoord(%d, %d) nArea(%d) nHighwayRet(%d) [%s]\n", nX, nY, nCornerX, nCornerY, nArea, nHighwayRet, szTileNames[nTileID]);
			if (nArea == 2)
				--nCornerY;
			if (nArea == 1 && nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE && XBITReturnIsFlipped(nCornerX, nCornerY) ||
				nArea == 2 && (nHighwayRet & 1) == 0) {
				nHorzMult = 1;
				nVertMult = 0;
			}
			else {
				nHorzMult = 0;
				nVertMult = 1;
			}
			nStoredHorzMult = nHorzMult * nArea;
			nStoredVertMult = nVertMult * nArea;
			bSingleTile = true;
			while (TRUE) {
				nValidBridgeType = L_Demolish_IsValidBridgeTypeArea(nCornerX, nCornerY, nArea, &nHighwayRet);
				if (nValidBridgeType <= VALID_BRIDGETYPEAREA_NO) {
					if (nValidBridgeType == VALID_BRIDGETYPEAREA_NOTSINGLE)
						bSingleTile = false;
					break;
				}
				nCornerX -= nStoredHorzMult;
				nCornerY -= nStoredVertMult;
			}
			if (bSingleTile) {
				// Entry point.
				// bClearFlipped set to true to unset the 'flip' XBIT attribute
				// from either the entry or exit point.
				L_Demolish_ClearEntryExitAndSetTerrain(nCornerX, nCornerY, true);
			}
			ConsoleLog(LOG_DEBUG, "bSingleTile check one: (%d, %d) (%d, %d) nArea(%d) nHighwayRet(%d) bSingleTile(%c) [%s]\n", nX, nY, nCornerX, nCornerY, nArea, nHighwayRet, (bSingleTile ? 'Y' : 'N'), szTileNames[nTileID]);
			if (nArea == 2 && !L_Demolish_HighwayTileUnitCheck(nTileID)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_DirtyTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY + 1);
				Game_DirtyTile(nCornerX + 1, nCornerY + 1);
				Game_SetTerrainTile(nCornerX, nCornerY + 1);
				Game_DirtyTile(nCornerX, nCornerY + 1);
			}
			nCornerX += nStoredHorzMult;
			nCornerY += nStoredVertMult;
			if (bExplosion) {
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
				bExplosionSoundPlayed = true;
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			}
			nExplodeX = -1;
			nExplodeY = -1;
			bSingleTile = true;
			while (true) {
				nValidBridgeType = L_Demolish_IsValidBridgeTypeArea(nCornerX, nCornerY, nArea, &nHighwayRet);
				if (nValidBridgeType <= VALID_BRIDGETYPEAREA_NO) {
					if (nValidBridgeType == VALID_BRIDGETYPEAREA_NOTSINGLE)
						bSingleTile = false;
					break;
				}
				Game_DirtyTile(nCornerX, nCornerY);
				if (nArea == 2 && nHighwayRet < 15) {
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
					nExplodeY = iScreenOffSetY + nCoordScale * (nCornerX + nCornerY) -
						nLandAltScale * nAltitude -
						pArrSpriteHeaders[nSpriteID].wHeight;
					L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nCornerX, nCornerY);
						XBITClearBits(nCornerX, nCornerY, XBIT_FLIPPED);
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
				nCornerX += nStoredHorzMult;
				nCornerY += nStoredVertMult;
			}
			if (bSingleTile) {
				// Exit point.
				L_Demolish_ClearEntryExitAndSetTerrain(nCornerX, nCornerY, false);
			}
			ConsoleLog(LOG_DEBUG, "bSingleTile check two: (%d, %d) (%d, %d) nArea(%d) nHighwayRet(%d) bSingleTile(%c) [%s]\n", nX, nY, nCornerX, nCornerY, nArea, nHighwayRet, (bSingleTile ? 'Y' : 'N'), szTileNames[nTileID]);
			if (nArea == 2 && !L_Demolish_HighwayTileUnitCheck(nTileID)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_DirtyTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY + 1);
				Game_DirtyTile(nCornerX + 1, nCornerY + 1);
				Game_SetTerrainTile(nCornerX, nCornerY + 1);
				Game_DirtyTile(nCornerX, nCornerY + 1);
			}
			if (bExplosion) {
				Game_FinishProcessObjects();
				if (pThis == (CSimcityView *)&pSomeWnd)
					Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
				else
					Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
				UpdateWindow(pThis->m_hWnd);
				if (!bExplosionSoundPlayed)
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
				Game_YieldToWindows(100);
			}
			bOnlyUpdateHouse = true;
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
				// Most likely a bug, commenting
				// just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_PierCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (bExplosion) {
				Game_FinishProcessObjects();
				if (pThis == (CSimcityView *)&pSomeWnd) {
					Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
					UpdateWindow(pThis->m_hWnd);
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
					Game_YieldToWindows(100);
					bOnlyUpdateHouse = true;
				}
			}
			ConsoleLog(LOG_DEBUG, "else if (pier): (%d, %d) (%d) [%s]\n", nX, nY, nArea, szTileNames[nTileID]);
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
				// Most likely a bug, commenting
				// just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_RunwayCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (bExplosion) {
				Game_FinishProcessObjects();
				if (pThis == (CSimcityView *)&pSomeWnd) {
					Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
					UpdateWindow(pThis->m_hWnd);
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
					Game_YieldToWindows(100);
					bOnlyUpdateHouse = true;
				}
			}
			ConsoleLog(LOG_DEBUG, "else if (runway): (%d, %d) (%d) [%s]\n", nX, nY, nArea, szTileNames[nTileID]);
		}
		else if (GET_TILE_RANGE(nTileID, TILE_TUNNEL_T, TILE_TUNNEL_L)) {
			nOffsetX = 0;
			nOffsetY = 0;
			switch (nTileID) {
			case TILE_TUNNEL_T:
				nOffsetX = -1;
				nOffsetY = 0;
				break;
			case TILE_TUNNEL_R:
				nOffsetX = 0;
				nOffsetY = -1;
				break;
			case TILE_TUNNEL_B:
				nOffsetX = 1;
				nOffsetY = 0;
				break;
			case TILE_TUNNEL_L:
				nOffsetX = 0;
				nOffsetY = 1;
				break;
			default:
				break;
			}
			Game_DirtyTile(nX, nY);
			nTileID = TILE_CLEAR;
			nRubbleTile = (rand() & 3) + 1;
			Game_PlaceTile(nX, nY, nRubbleTile);
			if (nX >= MAP_EDGE_MIN) {
				if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE) {
					XZONClearCorners(nX, nY);
					ALTMSetTunnelLevels(nX, nY, 0);
				}
			}
			L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
			nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
			nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
			L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
			while (nTileID < TILE_TUNNEL_T || nTileID > TILE_TUNNEL_L) {
				nX += nOffsetX;
				nY += nOffsetY;
				if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
					ALTMSetTunnelLevels(nX, nY, 0);
				nTileID = GetTileID(nX, nY);
			}
			Game_DirtyTile(nX, nY);
			nRubbleTile = (rand() & 3) + 1;
			Game_PlaceTile(nX, nY, nRubbleTile);
			if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
				XZONClearCorners(nX, nY);
			nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
			nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
			nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
			L_Demolish_DoDustCloud(nSpriteID, nExplodeX, nExplodeY);
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd)
				Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
			else
				Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
			UpdateWindow(pThis->m_hWnd);
			// For this case it is the sound.
			if (bExplosion) {
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
				Game_YieldToWindows(100);
			}
			bOnlyUpdateHouse = true;
			ConsoleLog(LOG_DEBUG, "else if (tunnel): (%d, %d) (%d) bExplosionSoundPlayed(%c) bExplosion(%c) (pThis == (CSimcityView *)&pSomeWnd)(%c) [%s]\n", nX, nY, nArea, (bExplosionSoundPlayed ? 'Y' : 'N'), (bExplosion ? 'Y' : 'N'), ((pThis == (CSimcityView *)&pSomeWnd) ? 'Y' : 'N'), szTileNames[nTileID]);
		}
		else {
			if (bExplosion) {
				nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
				if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE && XBITReturnIsWater(nCornerX, nCornerY))
					nAltitude = ALTMReturnWaterLevel(nCornerX, nCornerY);
				else
					nAltitude = ALTMReturnLandAltitude(nCornerX, nCornerY);
				nExplodeY = iScreenOffSetY + (nCoordScale * (nCornerX + nCornerY)) - nLandAltScale * nAltitude;
				if (nTileID >= TILE_ARCOLOGY_PLYMOUTH)
					dirtyRect.top = 0;
				if (nArea > 0) {
					int16_t nVertPos = 0;
					int16_t nAreaPos = nArea;
					do {
						L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
						if (nArea > 0) {
							int16_t nHorzPos = 0;
							int16_t nHorzAreaPos = nArea;
							nAreaExplodeX = nExplodeX;
							do {
								if (nArea > 0) {
									int16_t nAreaExplodeIntX = nAreaExplodeX;
									int16_t nCurrHorzPos = nHorzPos;
									int16_t nVertAreaPos = nArea;
									do {
										nSpriteID = L_Demolish_GetDustCloudSprite(nSpriteBase);
										nAreaExplodeY = nCurrHorzPos + nExplodeY - pArrSpriteHeaders[nSpriteID].wHeight - nVertPos;
										L_Demolish_DoDustCloud(nSpriteID, nAreaExplodeIntX, nAreaExplodeY);
										nCurrHorzPos -= nCoordScale;
										nAreaExplodeIntX += nScaleVal;
										--nVertAreaPos;
									} while (nVertAreaPos);
								}
								nHorzPos += nCoordScale;
								nAreaExplodeX += nScaleVal;
								--nHorzAreaPos;
							} while (nHorzAreaPos);
						}
						Game_FinishProcessObjects();
						if (pThis == (CSimcityView *)&pSomeWnd)
							Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
						else
							Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
						UpdateWindow(pThis->m_hWnd);
						if (!bExplosionSoundPlayed) {
							Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
							bExplosionSoundPlayed = true;
						}
						Game_YieldToWindows(100);
						nVertPos += nCoordScale;
						--nAreaPos;
					} while (nAreaPos);
				}
			}
			bTextOverlay = XTXTGetTextOverlayID(nCornerX, nCornerY);
			if (nTileID == TILE_INFRASTRUCTURE_MAYORSHOUSE)
				Game_SimulationToggleGrantReward(0, 1);
			if (nTileID == TILE_SERVICES_CITYHALL)
				Game_SimulationToggleGrantReward(1, 1);
			if (nTileID == TILE_SERVICES_STATUE)
				Game_SimulationToggleGrantReward(2, 1);
			if (nTileID == TILE_OTHER_BRAUNLLAMADOME)
				Game_SimulationToggleGrantReward(3, 1);
			if (nTileID == TILE_SERVICES_STADIUM &&
				bTextOverlay >= MIN_SIM_TEXT_ENTRIES &&
				bTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
				BYTE bMicrosimEntry = MICROSIMID_ENTRY(bTextOverlay);
				if (GetMicroSimulatorTileID(bMicrosimEntry) == nTileID)
					wStadiumSportsTeams += -1 << GetMicroSimulatorStat2(bMicrosimEntry);
			}
			for (__int16 nPosX = 0; nArea > nPosX; ++nPosX) {
				for (__int16 nPosY = 0; nArea > nPosY; ++nPosY) {
					__int16 nCurrX = nPosX + nCornerX;
					__int16 nCurrY = nCornerY - nPosY;
					if (nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
						nRubbleTile = (GetTerrainTileID(nCurrX, nCurrY)) ? TILE_CLEAR : (rand() & 3) + 1;
						Game_PlaceTile(nCurrX, nCurrY, nRubbleTile);
						if (nCurrX >= MAP_EDGE_MIN && nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
							XBITClearBits(nCurrX, nCurrY, XBIT_FLIPPED | XBIT_POWERED | XBIT_POWERABLE);
							XZONClearCorners(nCurrX, nCurrY);
						}
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
			if (GET_TILE_RANGE(nTileID, TILE_HIGHWAY_HTB, TILE_REINFORCED_BRIDGE) ||
				GET_TILE_RANGE(nTileID, TILE_HIGHWAY_LR, TILE_CROSSOVER_HIGHWAYTB_POWERLR)) {
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY - 1);
				Game_SetTerrainTile(nCornerX, nCornerY - 1);
			}
			if (nArea == 1 && nTileID <= TILE_SUBTORAIL_L) {
				if (GetTerrainTileID(nCornerX, nCornerY))
					Game_SetTerrainTile(nCornerX, nCornerY);
			}
			bOnlyUpdateHouse = true;
			ConsoleLog(LOG_DEBUG, "else (everything else): (%d, %d) (%d) bExplosionSoundPlayed(%c) bExplosion(%c) (pThis == (CSimcityView *)&pSomeWnd)(%c) [%s]\n", nX, nY, nArea, (bExplosionSoundPlayed ? 'Y' : 'N'), (bExplosion ? 'Y' : 'N'), ((pThis == (CSimcityView *)&pSomeWnd) ? 'Y' : 'N'), szTileNames[nTileID]);
		}
		ConsoleLog(LOG_DEBUG, "Demolish(): (%d, %d) (%d) bExplosionSoundPlayed(%c) bExplosion(%c) bOnlyUpdateHouse(%c) (pThis == (CSimcityView *)&pSomeWnd)(%c)\n", nX, nY, nArea, (bExplosionSoundPlayed ? 'Y' : 'N'), (bExplosion ? 'Y' : 'N'), (bOnlyUpdateHouse ? 'Y' : 'N'), ((pThis == (CSimcityView *)&pSomeWnd) ? 'Y' : 'N'));
		if (!bOnlyUpdateHouse) {
			Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
			UpdateWindow(pThis->m_hWnd);
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
			Game_YieldToWindows(100);
		}
		L_Demolish_UpdHouse(pThis, nX, nY, nArea);
		return;
	}
}

void InstallTerrainHandlingHooks_SC2K1996(void) {
	// Hook for CSimcityView::Demolish()
	SafeVirtualProtect((LPVOID)0x402211, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402211, Hook_SimcityView_Demolish);
}
