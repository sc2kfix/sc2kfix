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

#define MDRAWING_DEBUG_OTHER 1

#define MDRAWING_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MDRAWING_DEBUG
#define MDRAWING_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mdrawing_debug = MDRAWING_DEBUG;

static DWORD dwDummy;

extern "C" void __stdcall Hook_DrawAllLarge() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iShpWidth;
	__int16 iShpHeight;
	__int16 iScan, iX, iY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		rcDst.left -= GAME_MAP_SIZE;
		rcDst.bottom += 220;
		rcDst.top -= 32;
		iScan = MAP_EDGE_MIN;
		iX = MAP_EDGE_MIN;
		iY = MAP_EDGE_MIN;
		iShpWidth = iScreenOffSetX;
		iShpHeight = iScreenOffSetY;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth)
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
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
		iScan = MAP_EDGE_MIN + 1;
		iX = MAP_EDGE_MIN + 1;
		iY = MAP_EDGE_MAX;
		iShpWidth = iScreenOffSetX - 2016;
		iShpHeight = iScreenOffSetY + 1024;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth)
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
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
	__int16 iTopog;
	__int16 iSprite;
	BOOL bIsFlipped;
	BYTE iTerrainTile;
	BYTE iTile;
	BYTE iZone;

	if (iX != MAP_EDGE_MAX && iY != MAP_EDGE_MAX)
		goto PROCEEDNEXT;
	iBottom = shpHeight - pArrSpriteHeaders[SPRITE_LARGE_BEDROCK].wHeight;
	iLandAlt = ALTMReturnLandAltitude(iX, iY);
	if (!iLandAlt) {
		PROCEEDBASE:
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsWater(iX, iY)) {
			iTopog = ALTMReturnWaterLevel(iX, iY) - ALTMReturnLandAltitude(iX, iY);
			if (iTopog > 0) {
				while (rcDst.top <= iBottom) {
					if (rcDst.bottom > iBottom)
						Game_DrawProcessObject(SPRITE_LARGE_WATERFALL, shpWidth, iBottom, 0, 0);
					iBottom -= 12;
					if (--iTopog <= 0) {
						goto PROCEEDNEXT;
					}
				}
				return;
			}
		}
PROCEEDNEXT:
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
							iSprite = iTile + SPRITE_LARGE_START;
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
						iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
						Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					}
				}
				else {
					if (DisplayLayer[LAYER_ZONES] || !iZone)
						iSprite = BuiltUpZones[iZone] + SPRITE_LARGE_GREENTILE;
					else
						iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (!DisplayLayer[LAYER_INFRANATURE]) {
					iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					if (XTXTGetTextOverlayID(iX, iY))
						Game_DrawLabel(iX, iY, shpWidth, iTop);
					return;
				}
				if (iTile < TILE_HIGHWAY_HTB ||
					iTile >= TILE_SUBTORAIL_T) {

				}
				else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {

				}
			}
		}
		else {
			iSprTop = shpHeight - 12 * iAltTop;
			if (iTile == TILE_SMALLPARK)
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
		return;
	}
	while (rcDst.top <= iBottom) {
		if (rcDst.bottom > iBottom)
			Game_DrawProcessObject(SPRITE_LARGE_BEDROCK, shpWidth, iBottom, 0, 0);
		iBottom -= 12;
		if (--iLandAlt <= 0)
			goto PROCEEDBASE;
	}
}

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	VirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawLargeTile
	//VirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);
}
