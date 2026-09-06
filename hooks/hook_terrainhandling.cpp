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

static void L_Demolish_YieldAndUpdHouse(CSimcityView *pSCView, mapcoord_t nX, mapcoord_t nY, int16_t nArea) {
	Game_YieldToWindows(100);
	L_Demolish_UpdHouse(pSCView, nX, nY, nArea);
}

static void L_Demolish_PlaySoundYieldAndUpdHouse(CSimcityView *pSCView, mapcoord_t nX, mapcoord_t nY, int16_t nArea) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;

	Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
	L_Demolish_YieldAndUpdHouse(pSCView, nX, nY, nArea);
}

static void L_Demolish_UpdatePlaySoundYieldAndUpdHouse(CSimcityView *pSCView, mapcoord_t nX, mapcoord_t nY, int16_t nArea) {
	UpdateWindow(pSCView->m_hWnd);
	L_Demolish_PlaySoundYieldAndUpdHouse(pSCView, nX, nY, nArea);
}

// XXX (araxestroy): This function needs some serious comment work.
// Note: There's an original game bug when it comes to demoliting marinas
//       whereas when they're in a certain position the dust cloud will
//       be incorrectly offset (investigate eventually).
extern "C" void __stdcall Hook_SimcityView_Demolish(mapcoord_t x, mapcoord_t y, BOOL bExplosion) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	BYTE *pLockedBits = NULL;
	BYTE *pLockedBaseBits = NULL;
	bool bDoYield;
	mapcoord_t nX, nY;
	int16_t nTileID, nLoopTileID;
	mapcoord_t nCornerX, nCornerY;
	int16_t nArea;
	int16_t nCoordScale, nLandAltScale, nScaleVal;
	int16_t nHighwayTile;
	int16_t nHorzMult, nVertMult;
	int16_t nStoredHorzMult, nStoredVertMult;
	int16_t nSpriteBase, nSpriteID;
	mapcoord_t nExplodeX, nExplodeY, nAltitude;
	mapcoord_t nAreaExplodeX, nAreaExplodeY;
	mapcoord_t nAreaCornerX, nAreaCornerY;
	mapcoord_t nOffsetX, nOffsetY;
	int16_t nRubbleTile;
	WORD nLandAlt;
	BYTE bIsFlipped;
	BYTE bTextOverlay;
	CMFC3XPoint pt;
	coords_w_t tileCoords;

#if 0
	// Debugging and testing.
	if (GetAsyncKeyState(VK_MENU) < 0) {
		GameMain_SimcityView_Demolish(pThis, x, y, bExplosion);
		return;
	}
#endif

	pSCApp = &pCSimcityAppThis;
	pLockedBits = Game_Graphics_LockDIBBits(pThis->SCVGraphics);
	pLockedBaseBits = Game_Graphics_LockDIBBits(pBaseGraphics);
	bDoYield = false;
	nX = x;
	nY = y;
	nTileID = GetTileID(nX, nY);
	if (nTileID >= TILE_TREES1) {
		nCornerX = nX;
		nCornerY = nY;
		nArea = Game_FindCorner(&nCornerX, &nCornerY, nTileID);
		nCoordScale = COORDSCALE_VAL(pThis->wSCVZoomLevel);
		nLandAltScale = LANDALTSCALE_VAL(pThis->wSCVZoomLevel);
		nScaleVal = SCALE_VAL(pThis->wSCVZoomLevel);
		nSpriteBase = SPRITE_BOUNDARY_MULTIPLIER * pThis->wSCVZoomLevel;
		Game_DirtyThing(wDisasterObject);
		if (nArea == 1 && (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ELEVATED_POWERLINES) ||
			GET_TILE_RANGE(nTileID, TILE_REINFORCED_BRIDGE_PYLON, TILE_REINFORCED_BRIDGE))) {
			// Originally this one may have been undefined
			// until it got further down the chain.
			nHighwayTile = -1;
		HighwayChk:
			if (nArea == 2)
				--nCornerY;
			if (nArea == 1 &&
				nCornerX < GAME_MAP_SIZE &&
				nCornerY < GAME_MAP_SIZE &&
				XBITReturnIsFlipped(nCornerX, nCornerY) ||
				nArea == 2 &&
				(nHighwayTile & 1) == 0) {
				nHorzMult = 1;
				nVertMult = 0;
			}
			else {
				nHorzMult = 0;
				nVertMult = 1;
			}
			nStoredHorzMult = nHorzMult * nArea;
			nStoredVertMult = nVertMult * nArea;
			while (TRUE) {
				nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY);
				if (nArea != 2 || nHighwayTile < 13) {
					if (nArea != 1)
						goto AreaChkOne;
					nLoopTileID = GetTileID(nCornerX, nCornerY);
					if ((nLoopTileID < TILE_SUSPENSION_BRIDGE_START_B || nLoopTileID > TILE_ELEVATED_POWERLINES) &&
						nLoopTileID != TILE_REINFORCED_BRIDGE_PYLON &&
						nLoopTileID != TILE_REINFORCED_BRIDGE)
						break;
				}
				nCornerX -= nStoredHorzMult;
				nCornerY -= nStoredVertMult;
			}
			if (nCornerX >= GAME_MAP_SIZE ||
				nCornerY >= GAME_MAP_SIZE ||
				!XBITReturnIsWater(nCornerX, nCornerY)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						nLandAlt = ALTMReturnLandAltitude(nCornerX, nCornerY) - 1;
						ALTMSetLandAltitude(nCornerX, nCornerY, nLandAlt);
						XBITSetBits(nCornerX, nCornerY, XBIT_WATER);
					}
				}
				Game_SetTerrainTile(nCornerX, nCornerY);
				if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE)
					XBITClearBits(nCornerX, nCornerY, XBIT_FLIPPED);
			}
		AreaChkOne:
			if (nArea == 2 && nTileID != TILE_HIGHWAY_LR && nTileID != TILE_HIGHWAY_TB) {
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
				bDoYield = true;
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			}
			nExplodeX = -1;
			nExplodeY = -1;
			while (true) {
				nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY);
				if (nArea != 2 || nHighwayTile < 13) {
					if (nArea != 1)
						goto AreaChkTwo;
					nLoopTileID = GetTileID(nCornerX, nCornerY);
					if ((nLoopTileID < TILE_SUSPENSION_BRIDGE_START_B || nLoopTileID > TILE_ELEVATED_POWERLINES) &&
						nLoopTileID != TILE_REINFORCED_BRIDGE_PYLON &&
						nLoopTileID != TILE_REINFORCED_BRIDGE)
						break;
				}
				Game_DirtyTile(nCornerX, nCornerY);
				if (nArea == 2 && nHighwayTile < 15) {
					Game_DirtyTile(nCornerX, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY);
				}
				if (bExplosion) {
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
					nExplodeY = iScreenOffSetY + nCoordScale * (nCornerX + nCornerY) -
						nLandAltScale * ALTMReturnWaterLevel(nCornerX, nCornerY) -
						pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
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
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nAreaExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
						nAreaExplodeX = nExplodeX + 2 * nScaleVal;
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nExplodeY);
						nAreaExplodeY = nExplodeY + nCoordScale;
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nAreaExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
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
			if (nCornerX >= GAME_MAP_SIZE ||
				nCornerY >= GAME_MAP_SIZE ||
				!XBITReturnIsWater(nCornerX, nCornerY)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						nLandAlt = ALTMReturnLandAltitude(nCornerX, nCornerY) - 1;
						ALTMSetLandAltitude(nCornerX, nCornerY, nLandAlt);
						XBITSetBits(nCornerX, nCornerY, XBIT_WATER);
					}
				}
				Game_SetTerrainTile(nCornerX, nCornerY);
			}
		AreaChkTwo:
			if (nArea == 2 && nTileID != TILE_HIGHWAY_LR && nTileID != TILE_HIGHWAY_TB) {
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
			if (!bExplosion) {
				L_Demolish_UpdHouse(pThis, nX, nY, nArea);
				return;
			}
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd)
				Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
			else
				Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
			UpdateWindow(pThis->m_hWnd);
			if (bDoYield) {
				L_Demolish_YieldAndUpdHouse(pThis, nX, nY, nArea);
				return;
			}
			L_Demolish_PlaySoundYieldAndUpdHouse(pThis, nX, nY, nArea);
			return;
		}
		if (nArea == 2) {
			nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY - 1);
			if (nHighwayTile >= 13)
				goto HighwayChk;
		}
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_PIER, TILE_INFRASTRUCTURE_CRANE)) {
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
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE && XBITReturnIsWater(tileCoords.x, tileCoords.y))
						nAltitude = ALTMReturnWaterLevel(tileCoords.x, tileCoords.y);
					else
						nAltitude = ALTMReturnLandAltitude(tileCoords.x, tileCoords.y);
					nExplodeY = iScreenOffSetY + (nCoordScale * (tileCoords.x + tileCoords.y)) - nLandAltScale * nAltitude - pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
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
			if (!bExplosion) {
				L_Demolish_UpdHouse(pThis, nX, nY, nArea);
				return;
			}
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd) {
			FullRdrw:
				Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
				L_Demolish_UpdatePlaySoundYieldAndUpdHouse(pThis, nX, nY, nArea);
				return;
			}
		}
		else {
			if (nTileID != TILE_INFRASTRUCTURE_RUNWAY && nTileID != TILE_INFRASTRUCTURE_RUNWAYCROSS) {
				if (nTileID < TILE_TUNNEL_T || nTileID > TILE_TUNNEL_L) {
					if (bExplosion) {
						nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
						if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE && XBITReturnIsWater(nX, nY))
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
												nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
												nAreaExplodeY = nCurrHorzPos + nExplodeY - pArrSpriteHeaders[nSpriteID].wHeight - nVertPos;
												bIsFlipped = rand() & 1;
												Game_DrawProcessObject(nSpriteID, nAreaExplodeIntX, nAreaExplodeY, bIsFlipped, 0);
												Game_DirtyCloud(nSpriteID, nAreaExplodeIntX, nAreaExplodeY);
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
								if (!bDoYield) {
									Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
									bDoYield = true;
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
									XBITClearBits(nCurrX, nCurrY, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
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
					L_Demolish_UpdHouse(pThis, nX, nY, nArea);
					return;
				}
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
				nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				bIsFlipped = rand() & 1;
				Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
				Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
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
				nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				bIsFlipped = rand() & 1;
				Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
				Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				Game_FinishProcessObjects();
				if (pThis == (CSimcityView *)&pSomeWnd)
					Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
				else
					Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
				UpdateWindow(pThis->m_hWnd);
				if (!bExplosion) {
					L_Demolish_UpdHouse(pThis, nX, nY, nArea);
					return;
				}
				L_Demolish_PlaySoundYieldAndUpdHouse(pThis, nX, nY, nArea);
				return;
			}
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
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					nExplodeY = iScreenOffSetY + nCoordScale * (tileCoords.x + tileCoords.y) - nLandAltScale * ALTMReturnLandAltitude(tileCoords.x, tileCoords.y) - pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
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
			if (!bExplosion) {
				L_Demolish_UpdHouse(pThis, nX, nY, nArea);
				return;
			}
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd)
				goto FullRdrw;
		}
		Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
		L_Demolish_UpdatePlaySoundYieldAndUpdHouse(pThis, nX, nY, nArea);
		return;
	}
}

void InstallTerrainHandlingHooks_SC2K1996(void) {
	// Hook for CSimcityView::Demolish()
	SafeVirtualProtect((LPVOID)0x402211, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402211, Hook_SimcityView_Demolish);
}
