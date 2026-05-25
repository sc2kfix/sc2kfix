// sc2kfix hooks/hook_spritesandtilesets.cpp: sprite and tileset handling
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

#define SPRITE_DEBUG_OTHER 1
#define SPRITE_DEBUG_SPRITES 2
#define SPRITE_DEBUG_TILESETS 4
#define SPRITE_DEBUG_CACHING 8

#define SPRITE_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef SPRITE_DEBUG
#define SPRITE_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT sprite_debug = SPRITE_DEBUG;

static DWORD dwDummy; 

#define PALCACHE_TYPE_NONE                    -1
#define PALCACHE_TYPE_CYCLE                    0
#define PALCACHE_TYPE_TREES_SEASON_AUTUMN      1
#define PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW  2
#define PALCACHE_TYPE_TREES_SEASON_SNOW        3
#define PALCACHE_TYPE_TERRAIN_SNOW             4
#define PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD    5
#define PALCACHE_TYPE_WATER_ICE                6
#define PALCACHE_TYPE_WATER_ICE_BLIZZARD       7
#define PALCACHE_GRASS_SNOW                    8

#define CACHED_FRAMES 16

std::vector<spriteCache_t> spriteCache;

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
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainSnowFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_SNOW);
	Delete_SpriteFrame_Cache(pSpriteCache->sprTerrainBlizzardFrame, pSpriteCache->nID, PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD);
	Delete_SpriteFrame_Cache(pSpriteCache->sprDeepWaterIceFrame, pSpriteCache->nID, PALCACHE_TYPE_WATER_ICE);
	Delete_SpriteFrame_Cache(pSpriteCache->sprDeepWaterBlizzardFrame, pSpriteCache->nID, PALCACHE_TYPE_WATER_ICE_BLIZZARD);
	Delete_SpriteFrame_Cache(pSpriteCache->sprGrassSnowFrame, pSpriteCache->nID, PALCACHE_GRASS_SNOW);
}

void Clear_SpriteCache() {
	for (unsigned i = 0; i < spriteCache.size(); ++i) {
		Delete_Sprite_Cache(&spriteCache[i]);
	}

	spriteCache.clear();
}

static void Init_SpriteCache(bool bReload) {
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
				if ((*spritePtr >= 0xAB && *spritePtr <= 0xC6) || (*spritePtr >= 0xC8 && *spritePtr <= 0xDB) || (*spritePtr >= 0xE0 && *spritePtr <= 0xE7))
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

static BYTE fastCacheCycleOne[]   = { 0xC3, 0xC4, 0xC5, 0xC6 };
static BYTE fastCacheCycleTwo[]   = { 0xD0, 0xD1, 0xD2, 0xD3 };
static BYTE midCacheCycleOne[]    = { 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1 };
static BYTE midCacheCycleTwo[]    = { 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE2, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3 };
static BYTE midCacheCycleThree[]  = { 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE4, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5, 0xE5 };
static BYTE midCacheCycleFour[]   = { 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 };
static BYTE slowCacheCycleOne[]   = { 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF };
static BYTE slowCacheCycleTwo[]   = { 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2 };
static BYTE slowCacheCycleThree[] = { 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA };
static BYTE slowCacheCycleFour[]  = { 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2 };
static BYTE slowCacheCycleFive[]  = { 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB };

extern __int16 nCycleIdx;

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

static BYTE CyclePaletteIdx(BYTE colIdx, int cIdx) {
	BYTE newIdx = colIdx;
	newIdx = GetCycleColIdx(newIdx, fastCacheCycleOne, sizeof(fastCacheCycleOne), (cIdx % 4), false);
	newIdx = GetCycleColIdx(newIdx, fastCacheCycleTwo, sizeof(fastCacheCycleTwo), (cIdx % 4), false);
	newIdx = GetCycleColIdx(newIdx, midCacheCycleOne, sizeof(midCacheCycleOne), (cIdx % 16), false);
	newIdx = GetCycleColIdx(newIdx, midCacheCycleTwo, sizeof(midCacheCycleTwo), (cIdx % 16), false);
	newIdx = GetCycleColIdx(newIdx, midCacheCycleThree, sizeof(midCacheCycleThree), (cIdx % 16), false);
	newIdx = GetCycleColIdx(newIdx, midCacheCycleFour, sizeof(midCacheCycleFour), (cIdx % 16), false);
	newIdx = GetCycleColIdx(newIdx, slowCacheCycleOne, sizeof(slowCacheCycleOne), (cIdx % 8), true);
	newIdx = GetCycleColIdx(newIdx, slowCacheCycleTwo, sizeof(slowCacheCycleTwo), (cIdx % 8), false);
	newIdx = GetCycleColIdx(newIdx, slowCacheCycleThree, sizeof(slowCacheCycleThree), (cIdx % 8), false);
	newIdx = GetCycleColIdx(newIdx, slowCacheCycleFour, sizeof(slowCacheCycleFour), (cIdx % 8), false);
	newIdx = GetCycleColIdx(newIdx, slowCacheCycleFive, sizeof(slowCacheCycleFive), (cIdx % 8), false);
	return newIdx;
}

extern BYTE ProcessBuildingSnowIndex(BYTE colIdx);
extern BYTE ProcessTerrainSnowIndex(BYTE colIdx, bool bBlizzard);
extern BYTE ProcessTreeSnowEffect(BYTE colIdx);
extern BYTE ProcessTreeAutumnEffect(BYTE colIdx);

static void Adjust_SpritePalette(BYTE *shapePtr, int cIdx, int nType) {
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
				BYTE palIdx = *spritePtr;
				if (nType == PALCACHE_TYPE_CYCLE)
					palIdx = CyclePaletteIdx(palIdx, cIdx);
				else if (nType >= PALCACHE_TYPE_TREES_SEASON_AUTUMN && nType <= PALCACHE_TYPE_TREES_SEASON_SNOW) {
					if ((nPos % 4) == 0 || (nPos % 4) == 2 || (nPos % 4) == 3) {
						if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
							palIdx = ProcessTreeAutumnEffect(palIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW) {
							palIdx = ProcessTreeAutumnEffect(palIdx);
							if ((nPos % 4) != 2)
								palIdx = ProcessTreeSnowEffect(*spritePtr);
						}
						else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW)
							palIdx = ProcessTreeSnowEffect(palIdx);
					}
				}
				else if (nType >= PALCACHE_TYPE_TERRAIN_SNOW && nType <= PALCACHE_TYPE_WATER_ICE_BLIZZARD) {
					if (nType == PALCACHE_TYPE_TERRAIN_SNOW)
						palIdx = ProcessTerrainSnowIndex(palIdx, false);
					else if (nType == PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD)
						palIdx = ProcessTerrainSnowIndex(palIdx, true);
					else if (nType == PALCACHE_TYPE_WATER_ICE) {
						if ((nPos % 4) == 2)
							palIdx = ProcessTerrainSnowIndex(palIdx, false);
					}
					else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD) {
						if ((nPos % 4) == 1 || (nPos % 4) == 2)
							palIdx = ProcessTerrainSnowIndex(palIdx, true);
					}
				}
				else if (nType == PALCACHE_GRASS_SNOW)
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

	sprFrame.nFrID = nFrm;
	sprFrame.wHeight = wHeight;
	sprFrame.wWidth = wWidth;
	sprFrame.nSize = nSize;
	sprFrame.pBuf = (BYTE *)malloc(nSize);
	memcpy(sprFrame.pBuf, pSpriteBuf, nSize);
	if (nType > PALCACHE_TYPE_NONE)
		Adjust_SpritePalette(sprFrame.pBuf, cIdx, nType);
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

static void Snow_SpritePalette_Terrain(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache - ground should be mostly covered in regular snow or absolutely blanketed in a blizzard.
	Create_SpriteFrame(spriteCache[nID].sprTerrainSnowFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_SNOW);
	Create_SpriteFrame(spriteCache[nID].sprTerrainBlizzardFrame, pSpriteFrame, nFrmID, PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD);
}

static void Snow_SpritePalette_Grass(DWORD nID, spriteFrame_t *pSpriteFrame, int nFrmID) {
	// Cache accumulated snow on grass for various objects
	Create_SpriteFrame(spriteCache[nID].sprGrassSnowFrame, pSpriteFrame, nFrmID, PALCACHE_GRASS_SNOW);
}

static void Cache_Sprite(DWORD nID, BYTE *pSpriteBuf, int nSize, WORD wHeight, WORD wWidth) {
	Delete_Sprite_Cache(&spriteCache[nID]);

	bool bCycling = (!bLoColor) ? Scan_Sprite(pSpriteBuf) : false;

	for (int nFrm = 0; nFrm < CACHED_FRAMES; ++nFrm) {
		if (!bCycling && nFrm > 0)
			break;
		// Cycling (or only frame for non-cycling cases)
		Create_SpriteNew(spriteCache[nID].sprFrame, pSpriteBuf, nSize, wHeight, wWidth, nFrm, ((bCycling) ? PALCACHE_TYPE_CYCLE : PALCACHE_TYPE_NONE));
		if (!bLoColor && !bOnTheFlyPalIdx) {
			if (GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_TREES1, SPRITE_SMALL_TREES7))
				Season_SpritePalette_Trees(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			else if (GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_TERRAIN, SPRITE_SMALL_SEAPORTZONE)) {
				if (GET_OVERALL_SPRITE(nID, SPRITE_SMALL_WATER_TRBL))
					Snow_SpritePalette_DeepWater(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
				else
					Snow_SpritePalette_Terrain(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			}
			else if ((GET_OVERALL_SPRITE_RANGE(nID, SPRITE_SMALL_RESIDENTIAL_1X1_LOWERCLASSHOMES1, SPRITE_SMALL_SERVICES_STATUE) ||
				GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_MAYORSHOUSE) || GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_LIBRARY) ||
				GET_OVERALL_SPRITE(nID, SPRITE_SMALL_SMALLPARK) || GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_WATERPUMP) ||
				GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INFRASTRUCTURE_CHURCH)) &&
				!GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INDUSTRIAL_2X2_FACTORY2) && !GET_OVERALL_SPRITE(nID, SPRITE_SMALL_INDUSTRIAL_3X3_THINGAMAJIG)) {
				Snow_SpritePalette_Grass(nID, &spriteCache[nID].sprFrame[nFrm], nFrm);
			}
		}
	}
}

extern int iForcedSeason;

BYTE *Get_SpriteFrame_Buffer(std::vector<spriteFrame_t> &frameCache, BYTE *pSpriteBuf, DWORD nFrmIdx) {
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

BYTE *Get_SpriteCache_BaseBuffer(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	BYTE *pSpriteBuf;

	if (pShapePtr->wHeight > 1) {
		if (bFrequentUpdates) {
			pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, 0);
			if (pSpriteBuf)
				return pSpriteBuf;
		}
		return pShapePtr->sprOffset.sprPtr;
	}
	return NULL;
}

BYTE *Get_SpriteCache_Buffer(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	int nType = PALCACHE_TYPE_NONE;
	int iCityMonth = dwCityDays / 25 % 12;
	BYTE *pSpriteBuf;

	if (pShapePtr->wHeight > 1) {
		if (bFrequentUpdates) {
			int nFrmIdx = nCycleIdx % CACHED_FRAMES;
			if (nFrmIdx < 0)
				nFrmIdx = -nFrmIdx;
			pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, nFrmIdx);
			if (pSpriteBuf) {
				if (bWeatherEffects && !bLoColor && !bOnTheFlyPalIdx) {
					if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_TREES1, SPRITE_SMALL_TREES7)) {
						if (bWeatherTrend == WEATHER_TREND_BLIZZARD ||
							bWeatherTrend == WEATHER_TREND_SNOW ||
							iForcedSeason == FORCED_SEASON_SNOW || iForcedSeason == FORCED_SEASON_BLIZZARD)
							nType = PALCACHE_TYPE_TREES_SEASON_SNOW;

						if ((iCityMonth >= 0 && iCityMonth <= 2) ||
							(iCityMonth >= 9 && iCityMonth <= 11) ||
							iForcedSeason == FORCED_SEASON_AUTUMN || iForcedSeason == FORCED_SEASON_WINTER)
							nType = (nType == PALCACHE_TYPE_TREES_SEASON_SNOW) ? PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW : PALCACHE_TYPE_TREES_SEASON_AUTUMN;

						if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMN)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_AUTUMNSNOW)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonAutumnSnowFrame, pSpriteBuf, nFrmIdx);
						else if (nType == PALCACHE_TYPE_TREES_SEASON_SNOW)
							return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprSeasonSnowFrame, pSpriteBuf, nFrmIdx);
					}
					else if (GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_TERRAIN, SPRITE_SMALL_SEAPORTZONE)) {
						if (bWeatherTrend == WEATHER_TREND_SNOW || bWeatherTrend == WEATHER_TREND_BLIZZARD ||
							iForcedSeason == FORCED_SEASON_SNOW || iForcedSeason == FORCED_SEASON_BLIZZARD) {
							if (GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_WATER_TRBL)) {
								nType = (bWeatherTrend == WEATHER_TREND_BLIZZARD || iForcedSeason == FORCED_SEASON_BLIZZARD) ? PALCACHE_TYPE_WATER_ICE_BLIZZARD : PALCACHE_TYPE_WATER_ICE;
								if (nType == PALCACHE_TYPE_WATER_ICE)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterIceFrame, pSpriteBuf, nFrmIdx);
								else if (nType == PALCACHE_TYPE_WATER_ICE_BLIZZARD)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprDeepWaterBlizzardFrame, pSpriteBuf, nFrmIdx);
							}
							else {
								nType = (bWeatherTrend == WEATHER_TREND_BLIZZARD || iForcedSeason == FORCED_SEASON_BLIZZARD) ? PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD : PALCACHE_TYPE_TERRAIN_SNOW;
								if (nType == PALCACHE_TYPE_TERRAIN_SNOW)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainSnowFrame, pSpriteBuf, nFrmIdx);
								else if (nType == PALCACHE_TYPE_TERRAIN_SNOW_BLIZZARD)
									return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprTerrainBlizzardFrame, pSpriteBuf, nFrmIdx);
							}
						}
					}
					else if ((GET_OVERALL_SPRITE_RANGE(nSpriteID, SPRITE_SMALL_RESIDENTIAL_1X1_LOWERCLASSHOMES1, SPRITE_SMALL_SERVICES_STATUE) ||
						GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INFRASTRUCTURE_MAYORSHOUSE) || GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INFRASTRUCTURE_LIBRARY) ||
						GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_SMALLPARK) || GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INFRASTRUCTURE_WATERPUMP) ||
						GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INFRASTRUCTURE_CHURCH)) &&
						!GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INDUSTRIAL_2X2_FACTORY2) && !GET_OVERALL_SPRITE(nSpriteID, SPRITE_SMALL_INDUSTRIAL_3X3_THINGAMAJIG)) {
						if (bWeatherTrend == WEATHER_TREND_SNOW || bWeatherTrend == WEATHER_TREND_BLIZZARD ||
							iForcedSeason == FORCED_SEASON_SNOW || iForcedSeason == FORCED_SEASON_BLIZZARD) {
							nType = PALCACHE_GRASS_SNOW; // Yes I know.. this is the only option here currently.
							if (nType == PALCACHE_GRASS_SNOW)
								return Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprGrassSnowFrame, pSpriteBuf, nFrmIdx);
						}
					}
				}
				return pSpriteBuf;
			}
		}
		return pShapePtr->sprOffset.sprPtr;
	}
	return NULL;
}

WORD Get_SpriteCache_Height(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	BYTE *pSpriteBuf;

	if (pShapePtr->wHeight > 1) {
		if (bFrequentUpdates) {
			pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, 0);
			if (pSpriteBuf)
				return spriteCache[nSpriteID].sprFrame[0].wHeight;
		}
		return pShapePtr->wHeight;
	}
	return 0;
}

WORD Get_SpriteCache_Width(sprite_header_t *pShapePtr, __int16 nSpriteID) {
	BYTE *pSpriteBuf;

	if (pShapePtr->wHeight > 1) {
		if (bFrequentUpdates) {
			pSpriteBuf = Get_SpriteFrame_Buffer(spriteCache[nSpriteID].sprFrame, NULL, 0);
			if (pSpriteBuf)
				return spriteCache[nSpriteID].sprFrame[0].wWidth;
		}
		return pShapePtr->wWidth;
	}
	return 0;
}

std::vector<sprite_ids_t> spriteIDs;

static BOOL CheckForExistingID(WORD nID) {
	WORD nSkipHit;
	sprite_ids_t *pSprEnt;

	nSkipHit = 0;
	for (int i = 0; i < (int)spriteIDs.size(); i++) {
		pSprEnt = &spriteIDs[i];
		if (pSprEnt && pSprEnt->nID == nID) {
			if (sprite_debug & SPRITE_DEBUG_SPRITES)
				ConsoleLog(LOG_DEBUG, "CheckForExistingID(%u): (%u, %u, 0x%06X, %d) ID already exists.\n", nID, pSprEnt->nArcID, pSprEnt->nID, pSprEnt->sprOffset, pSprEnt->nSize);
			pSprEnt->bMultiple = TRUE;
			nSkipHit = ++pSprEnt->nSkipHit;
		}
	}
	return (nSkipHit) ? TRUE : FALSE;
}

static void AllocateAndLoadSprites1996(FILE *pFile, sprite_archive_t *lpBuf, WORD nSpriteSet) {
	WORD nPos, nID;
	sprite_ids_t *pSprEnt;
	BYTE *pSpriteData;

	fseek(pFile, lpBuf->pData[0].sprHeader.sprOffset.sprLong, SEEK_SET);
	for (nPos = 0; nPos < spriteIDs.size(); ++nPos) {
		pSprEnt = &spriteIDs[nPos];
		if (pSprEnt && pSprEnt->nArcID == nSpriteSet) {
			nID = pSprEnt->nID;
			if (pSprEnt->bMultiple)
				if (sprite_debug & SPRITE_DEBUG_SPRITES)
					ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): Multiple sprites with the same ID Detected (%u, 0x%06X, %d) (%u)\n", nSpriteSet, nID, pSprEnt->sprOffset, pSprEnt->nSize, pSprEnt->nSkipHit);
			if (pSprEnt->nSize > 0) {
				pSpriteData = (BYTE *)Game_AllocateDataEntry(pSprEnt->nSize);
				if (pSpriteData) {
					if (fread(pSpriteData, 1, pSprEnt->nSize, pFile) == pSprEnt->nSize) {
						if (pSprEnt->bMultiple && pSprEnt->nSkipHit > 0) {
							if (sprite_debug & SPRITE_DEBUG_SPRITES)
								ConsoleLog(LOG_DEBUG, "AllocateAndLoadSprites(%u): discarding skipped sprite with ID (%u, 0x%06X, %d).\n", nSpriteSet, nID, pSprEnt->sprOffset, pSprEnt->nSize);
							Game_FreeDataEntry(pSpriteData);
							continue;
						}
						if (pArrSpriteHeaders[nID].sprOffset.sprPtr) {
							Game_FreeDataEntry(pArrSpriteHeaders[nID].sprOffset.sprPtr);
							pArrSpriteHeaders[nID].sprOffset.sprPtr = 0;
						}
						pArrSpriteHeaders[nID].sprOffset.sprPtr = pSpriteData;
						pArrSpriteHeaders[nID].wHeight = pSprEnt->wHeight;
						pArrSpriteHeaders[nID].wWidth = pSprEnt->wWidth;

						Cache_Sprite(nID, pSpriteData, pSprEnt->nSize, pSprEnt->wHeight, pSprEnt->wWidth);
					}
				}
			}
		}
	}
}

extern "C" void __cdecl Hook_LoadSpriteDataArchive1996(WORD nSpriteSet) {
	FILE *f;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nPos, nNextPos;
	__int16 nSpriteCnt;
	WORD nID, nNextID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;
	int32_t sprNextOffset;
	sprite_ids_t spriteEnt;
	int nSize;

	GameMain_String_Cons(&retString);
	GameMain_String_Cons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> LoadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	Game_SimcityApp_GetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	GameMain_String_Format(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	f = old_fopen(retStrPath.m_pchData, "rb");
	if (f) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		fseek(f, 0, SEEK_END);
		nFlen = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(&nSpriteCnt, 2, 1, f);
		nSpriteCnt = _byteswap_ushort(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (fread(&lpBuf->pData, 1, nBufSize, f) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = _byteswap_ushort(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;

					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = _byteswap_ulong(pSprtHead->sprOffset.sprLong);

					pArrSpriteHeaders[nID] = *pSprtHead;
					pArrSpriteHeaders[nID].sprOffset.sprPtr = 0;
					pArrSpriteHeaders[nID].wHeight = _byteswap_ushort(pArrSpriteHeaders[nID].wHeight);
					pArrSpriteHeaders[nID].wWidth = _byteswap_ushort(pArrSpriteHeaders[nID].wWidth);

					nNextPos = (nPos >= nSpriteCnt - 1) ? -1 : nPos + 1;
					nNextID = (nNextPos >=0) ? nID : -1;

					if (nNextPos >= 0) {
						// The next position hasn't yet been processed, do so here so
						// we can get the file size.
						sprNextOffset = _byteswap_ulong(lpBuf->pData[nNextPos].sprHeader.sprOffset.sprLong);
						nSize = (sprNextOffset - pSprtHead->sprOffset.sprLong);
					}
					else
						nSize = (nFlen - pSprtHead->sprOffset.sprLong);

					if (nSize > 0) {
						spriteEnt.nArcID = nSpriteSet;
						spriteEnt.nID = nID;
						spriteEnt.sprOffset = pSprtHead->sprOffset.sprLong;
						spriteEnt.nSize = nSize;
						spriteEnt.wHeight = pArrSpriteHeaders[nID].wHeight;
						spriteEnt.wWidth = pArrSpriteHeaders[nID].wWidth;
						spriteEnt.nSkipHit = 0;
						spriteEnt.bMultiple = (CheckForExistingID(nID)) ? TRUE : FALSE;
						spriteIDs.push_back(spriteEnt);
					}
				}

				AllocateAndLoadSprites1996(f, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		fclose(f);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
}

static void ResetCustomTileNames() {
	for (int i = 0; i < SPRITE_MEDIUM_START; ++i) {
		if (pTileNames[i])
			Game_FreeDataEntry(pTileNames[i]);
		pTileNames[i] = 0;
	}
}

static void ReloadSpriteDataArchive1996(WORD nSpriteSet) {
	FILE *f;
	CMFC3XString retString;
	CMFC3XString retStrPath;
	CMFC3XString *pString;
	UINT uFailMsg;
	int nFlen;
	__int16 nSpriteCnt;
	WORD nPos;
	WORD nID;
	int nBufSize;
	sprite_archive_t *lpBuf;
	sprite_archive_stored_t *lpMainBuf;
	sprite_header_t *pSprtHead;

	GameMain_String_Cons(&retString);
	GameMain_String_Cons(&retStrPath);

	uFailMsg = 0;

	if (dwBaseSpriteLoading[nSpriteSet].pData) {
		if (sprite_debug & SPRITE_DEBUG_SPRITES)
			ConsoleLog(LOG_DEBUG, "SPRT: 0x%06X -> ReloadSpriteDataArchive(%u): Already loading (0x%06X)\n", _ReturnAddress(), nSpriteSet, dwBaseSpriteLoading[nSpriteSet].pData);
		goto GETOUT;
	}

	Game_SimcityApp_GetValueStringA(&pCSimcityAppThis, &retString, aPaths, aData);

	pString = &cStrDataArchiveNames[nSpriteSet];
	if (!pString)
		goto GETOUT;

	GameMain_String_Format(&retStrPath, "%s%s%s", retString.m_pchData, cBackslash, pString->m_pchData);

	f = old_fopen(retStrPath.m_pchData, "rb");
	if (f) {
		// Set the uFailMsg by default here - then unset it once
		// the main read begins.
		uFailMsg = 48;
		fseek(f, 0, SEEK_END);
		nFlen = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(&nSpriteCnt, 2, 1, f);
		nSpriteCnt = _byteswap_ushort(nSpriteCnt);
		lpMainBuf = &dwBaseSpriteLoading[nSpriteSet];
		nBufSize = 10 * nSpriteCnt;
		lpBuf = (sprite_archive_t *)malloc(nBufSize + 2);
		lpMainBuf->pData = lpBuf;
		lpBuf->nSprites = nSpriteCnt;
		if (lpBuf) {
			if (fread(&lpBuf->pData, 1, nBufSize, f) == nBufSize) {
				uFailMsg = 0;
				for (nPos = 0; nPos < nSpriteCnt; ++nPos) {
					nID = _byteswap_ushort(lpBuf->pData[nPos].nSprNum);
					lpBuf->pData[nPos].nSprNum = nID;
					pSprtHead = &lpBuf->pData[nPos].sprHeader;
					pSprtHead->sprOffset.sprLong = _byteswap_ulong(pSprtHead->sprOffset.sprLong);
					pSprtHead->wHeight = _byteswap_ushort(pSprtHead->wHeight);
					pSprtHead->wWidth = _byteswap_ushort(pSprtHead->wWidth);
				}

				AllocateAndLoadSprites1996(f, lpBuf, nSpriteSet);
				free(lpMainBuf->pData);
				lpMainBuf->pData = 0;
			}
		}
		fclose(f);
	}
	else {
		uFailMsg = 47;
	}

	if (uFailMsg)
		Game_FailRadio(uFailMsg);

GETOUT:
	GameMain_String_Dest(&retStrPath);
	GameMain_String_Dest(&retString);
}

static void L_LoadFixedLargeSpritesRsrc_SC2K1996() {
	HRSRC hTileSetHandle;
	HGLOBAL hTileSetGlobal;
	DWORD dwTileDatSz;
	DWORD dwOffset;
	WORD nChunk;
	char *szHead[4];
	DWORD dwSize;
	__int16 nSpriteID;
	WORD nWidth, nHeight;
	DWORD dwSize_Shap;
	WORD nTileNameID, nNameLength;
	BOOL bGotShap, bGotName, bResize;
	char *pRsrcDat;
	char *pTileDat;
	char *pBuf;
	tilesetMainHeader_t *pTileHeader;
	tilesetHeadInfo_t *pTileInfo;
	tilesetHeadInfo_t *pTileTiles;
	tilesetMem_t *pTileMem;
	tileMem_t *pTileContents;
	tileShap_t *pTileShap;
	tileName_t *pTileName;
	int iReplacementsLoaded = 0;

	dwOffset = 0;
	hTileSetHandle = FindResourceA(hSC2KFixModule, MAKEINTRESOURCE(IDR_TSET_FIXED), "TSET");
	if (hTileSetHandle) {
		hTileSetGlobal = LoadResource(hSC2KFixModule, hTileSetHandle);
		dwTileDatSz = SizeofResource(hSC2KFixModule, hTileSetHandle);
		pRsrcDat = (char *)LockResource(hTileSetGlobal);
		if (pRsrcDat) {
			pTileDat = (char *)malloc(dwTileDatSz);
			if (pTileDat) {
				memcpy(pTileDat, pRsrcDat, dwTileDatSz);
				pTileHeader = (tilesetMainHeader_t *)pTileDat;
				if (memcmp(pTileHeader->szTypeHead, "MIFF", 4) == 0 &&
					memcmp(pTileHeader->szSC2KHead, "SC2K", 4) == 0) {
					dwSize = _byteswap_ulong(pTileHeader->dwSize);
					dwOffset += sizeof(tilesetMainHeader_t);
					pTileInfo = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
					if (pTileInfo && memcmp(pTileInfo->szHead, "INFO", 4) == 0) {
						dwSize = _byteswap_ulong(pTileInfo->dwSize);
						dwOffset += sizeof(tilesetHeadInfo_t) + dwSize;
						pTileTiles = (tilesetHeadInfo_t *)(pTileDat + dwOffset);
						if (pTileTiles && memcmp(pTileTiles->szHead, "TILE", 4) == 0) {
							dwSize = _byteswap_ulong(pTileTiles->dwSize);
							dwOffset += sizeof(tilesetHeadInfo_t);
							pTileMem = (tilesetMem_t *)(pTileDat + dwOffset);
							if (pTileMem) {
								pTileMem->nMaxChunks = _byteswap_ushort(pTileMem->nMaxChunks);
								pTileContents = &pTileMem->tileMem;
								if (pTileContents) {
									for (nChunk = 0; pTileMem->nMaxChunks > nChunk; ++nChunk) {
										memcpy(szHead, pTileContents->szHead, 4);
										dwSize = _byteswap_ulong(pTileContents->dwSize);
										pBuf = &pTileContents->pBuf;

										bGotShap = bGotName = bResize = FALSE;
										if (memcmp(szHead, "SHAP", 4) == 0) {
											pTileShap = (tileShap_t *)pBuf;
											nSpriteID = _byteswap_ushort(pTileShap->nSpriteID);
											nWidth = _byteswap_ushort(pTileShap->nWidth);
											nHeight = _byteswap_ushort(pTileShap->nHeight);
											dwSize_Shap = _byteswap_ulong(pTileShap->dwSize);
											// In this case we ONLY want to load the large sprites.
											if (nSpriteID < SPRITE_LARGE_START)
												bGotShap = TRUE;
											else
												bGotShap = (nHeight > 1) ? Game_ChangeTileSpriteEntry(nSpriteID, nWidth, nHeight, dwSize_Shap, &pTileShap->pBuf) : TRUE;

											if (bGotShap && nHeight > 1 && nSpriteID >= SPRITE_LARGE_START) {
												iReplacementsLoaded++;
												if (sprite_debug & SPRITE_DEBUG_TILESETS)
													ConsoleLog(LOG_DEBUG, "TILE: Loaded replacement large sprite for: %s\n", szSpriteNames[nSpriteID - SPRITE_LARGE_START]);
											}
										}
										else if (memcmp(szHead, "NAME", 4) == 0) {
											pTileName = (tileName_t *)pBuf;
											nTileNameID = _byteswap_ushort(pTileName->nTileNameID);
											nNameLength = _byteswap_ushort(pTileName->nNameLength);
											// Although we process the above we leave the
											// names alone here.
											bGotName = TRUE;
										}

										// Added. If this is set to true it stands to reason
										// you'd then want to break out of the loop.
										if (bTilesetLoadOutOfMemory)
											break;

										if (bGotShap || bGotName || bResize) {
											bResize = FALSE;
											pTileContents = (tileMem_t *)&pBuf[dwSize];
											continue;
										}

										bResize = TRUE;
										pTileContents = (tileMem_t *)Game_ReallocateDataEntry((char *)pTileMem, pBuf);
									}
								}
							}
						}
					}
				}
				free(pTileDat);
			}
		}
		FreeResource(hTileSetGlobal);
	}

	if (iReplacementsLoaded && sprite_debug & SPRITE_DEBUG_TILESETS)
		ConsoleLog(LOG_DEBUG, "TILE: Loaded %i replacement default large sprite resources.\n", iReplacementsLoaded);
}

void ReloadDefaultTileSet_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;

	if (L_MessageBoxA(GameGetRootWindowHandle(), "Are you sure that you want to reload the base game tile set?", gamePrimaryKey, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) != IDYES)
		return;

	GameMain_CmdTarget_BeginWaitCursor(pSCApp);
	Init_SpriteCache(true);

	ResetCustomTileNames();
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SPECIAL);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_LARGE);
	ReloadSpriteDataArchive1996(TILEDAT_DEFS_SMALLMED);
	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GameMain_CmdTarget_EndWaitCursor(pSCApp);

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __declspec(naked) __stdcall Hook_LoadSpriteArchives1996() {
	Init_SpriteCache(false);

	Game_LoadDataArchive(TILEDAT_DEFS_SPECIAL);
	Game_LoadDataArchive(TILEDAT_DEFS_LARGE);
	Game_LoadDataArchive(TILEDAT_DEFS_SMALLMED);

	if (!bDisableFixedTiles)
		L_LoadFixedLargeSpritesRsrc_SC2K1996();
	GAMEJMP(0x42C332)
}

extern "C" void __stdcall Hook_SimcityApp_LoadTileset1996() {
	CSimcityAppPrimary *pThis;

	__asm mov[pThis], ecx

	CMFC3XString strFileTypes;
	CMFC3XString strCaption;
	CMFC3XFileDialog fileDialog;
	CMFC3XString strFilePath;
	int nPathLen, nFileLen, nNewLen;
	char szPath[MAX_PATH + 1];
	OPENFILENAMEA* pOfn;

	memset(szPath, 0, sizeof(szPath));

	GameMain_String_Cons(&strFileTypes);
	GameMain_String_Cons(&strCaption);

	GameMain_String_LoadStringA(&strFileTypes, 4004);
	GameMain_String_LoadStringA(&strCaption, 4005);

	GameMain_FileDialog_Cons(&fileDialog, (void *)1, "mif", "*.mif", OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, strFileTypes.m_pchData, 0);

	Game_SimcityApp_GetValueStringA(pThis, &strFilePath, aPaths, aTilesets);

	old_strcpy(fileDialog.m_szFileName, strFilePath.m_pchData);
	pOfn = &fileDialog.m_ofn;
	pOfn->lpstrInitialDir = fileDialog.m_szFileName;
	pOfn->lpstrTitle = strCaption.m_pchData;

	if (GameMain_FileDialog_DoModal(&fileDialog) == 1) {
		GameMain_CmdTarget_BeginWaitCursor(pThis);
		Game_ReadTilesetFile(pOfn->lpstrFile);

		nNewLen = 0;
		nPathLen = strlen(pOfn->lpstrFile);
		nFileLen = strlen(pOfn->lpstrFileTitle);
		if (nPathLen > 0 && nFileLen > 0) {
			nNewLen = nPathLen - nFileLen;
			if (nNewLen > 0) {
				strncpy_s(szPath, sizeof(szPath) - 1, pOfn->lpstrFile, nNewLen);
				if (L_IsPathValid(szPath))
					jsonSettingsCore[C_SC2KFIX][S_FIX_PATHS][I_FIX_PATHS_TILESETS] = szPath;
			}
		}

		GameMain_CmdTarget_EndWaitCursor(pThis);
	}

	GameMain_String_Dest(&strFilePath);

	Game_GameFileDialog_Dest(&fileDialog);

	GameMain_String_Dest(&strCaption);
	GameMain_String_Dest(&strFileTypes);
}

extern "C" void __cdecl Hook_ReadTilesetFile1996(char *pFilePath) {
	CSimcityAppPrimary *pSCApp;
	FILE *f;
	DWORD nLen;
	char currChar;
	const char *pFileName;
	char *pBuf;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	bTilesetLoadOutOfMemory = FALSE;
	f = old_fopen(pFilePath, "rb");
	if (f) {
		if (Game_CheckTilesetFileHeader(f)) {
			// There was a prior null function prior to
			// actual main tileset loading; it most likely
			// could have been to debug the main body of
			// the file prior to actual loading.
			Game_VerifyAndLoadNewTiles(f);
			if (bTilesetLoadOutOfMemory) {
				GameMain_String_LoadStringA(&reqCaption, 4007);
				GameMain_String_LoadStringA(&reqText, 4008);
				nLen = strlen(pFilePath);
				do
					currChar = pFilePath[--nLen];
				while (currChar != '\\' && nLen > 0);
				pFileName = &pFilePath[nLen + 1];
				pBuf = (char *)Game_AllocateDataEntry(reqText.m_nDataLength + 2 * (strlen(pFileName) + 1) - 2);
				if (pBuf) {
					wsprintfA(pBuf, reqText.m_pchData, pFileName, pFileName);
					GameMain_CmdTarget_EndWaitCursor(pSCApp);
					L_MessageBoxA(0, pBuf, reqCaption.m_pchData, MB_OK);
					pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
					if (pSCView)
						UpdateWindow(pSCView->m_hWnd);
					Game_FreeDataEntry(pBuf);
				}
				GameMain_String_ReleaseBuffer(&reqCaption, 0);
				GameMain_String_ReleaseBuffer(&reqText, 0);
			}
		}
		fclose(f);
	}
}

extern "C" BOOL __cdecl Hook_CheckTilesetFileHeader1996(FILE *f) {
	tilesetMainHeader_t tileHead;

	if (!f)
		return FALSE;

	// The original program-native calls are required otherwise
	// the crash is spectacular.
	fseek(f, 0, SEEK_SET);
	fread(&tileHead, sizeof(tilesetMainHeader_t), 1, f);

	return memcmp(tileHead.szTypeHead, "MIFF", 4) == 0 && memcmp(tileHead.szSC2KHead, "SC2K", 4) == 0;
}

extern "C" void __cdecl Hook_VerifyAndLoadNewTiles1996(FILE *f) {
	char *pBuf;
	tilesetHeadInfo_t tilesetInfo;
	tilesetChunkHeader_t tilesetChunkHeader;
	CSimcityView *pSCView;

	// Seek and process 'Info' portion.
	fseek(f, sizeof(tilesetMainHeader_t), SEEK_SET);
	fread(tilesetInfo.szHead, 4, 1, f);
	if (memcmp(tilesetInfo.szHead, "INFO", 4) != 0)
		return;

	fread(&tilesetInfo.dwSize, 4, 1, f);
	tilesetInfo.dwSize = _byteswap_ulong(tilesetInfo.dwSize);
	fseek(f, tilesetInfo.dwSize, SEEK_CUR);

	// Process 'Tile' portion.
	memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
	fread(tilesetChunkHeader.szHead, 4, 1, f);
	if (memcmp(tilesetChunkHeader.szHead, "TILE", 4) != 0)
		return;

	fread(&tilesetChunkHeader.dwSize, 4, 1, f);
	tilesetChunkHeader.dwSize = _byteswap_ulong(tilesetChunkHeader.dwSize);
	
	pBuf = (char *)malloc(tilesetChunkHeader.dwSize);
	if (!pBuf)
		Game_LoadTilesFromFile(f);
	else {
		Game_GetAndLoadNextTileFileChunkToMemory(f, pBuf, tilesetChunkHeader.dwSize);
		Game_LoadTilesFromMemory(pBuf);
		free(pBuf);
	}

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(&pCSimcityAppThis);
	if (pSCView) {
		Game_SimcityView_DrawHouse(pSCView);
		UpdateWindow(pSCView->m_hWnd);
	}
}

extern "C" void __stdcall Hook_GetAndLoadNextTileFileChunkToMemory1996(FILE *f, char *pBuf, DWORD dwSize) {
	DWORD dwRemainingSize, dwChunkSize, dwFetchedSize;

	dwRemainingSize = dwSize;
	dwChunkSize = 0x8000;
	dwFetchedSize = 0;
	while (dwRemainingSize > 0) {
		if (dwChunkSize > dwRemainingSize)
			dwChunkSize = dwRemainingSize;
		dwFetchedSize = fread(pBuf, 1, dwChunkSize, f);
		if (dwFetchedSize == 0)
			break;
		dwRemainingSize -= dwFetchedSize;
		pBuf += dwFetchedSize;
	}
}

extern "C" void *__cdecl Hook_ReallocateDataEntry1996(char *pDest, char *pSrc) {
	DWORD dwCurr;
	DWORD dwDiff;
	char *pDestPtr;
	DWORD dwSize;
	DWORD dwPos;
	void *pNew;

	dwCurr = GlobalSize(pDest);
	dwDiff = 0;
	if (pSrc != pDest) {
		do
			pDestPtr = &pDest[++dwDiff];
		while (pDestPtr != pSrc);
	}
	dwSize = dwCurr - dwDiff;
	for (dwPos = 0; dwSize > dwPos; ++dwPos)
		pDest[dwPos] = pSrc[dwPos];
	pNew = realloc(pDest, dwSize);
	return (pNew) ? pNew : pDest;
}

extern "C" void __cdecl Hook_LoadTilesFromMemory1996(tilesetMem_t *pTileMem) {
	WORD nMaxChunks, nChunk;
	tileMem_t *pTileMemEntry;
	tilesetChunkHeader_t tilesetChunkHeader;
	char *pBuf;
	BOOL bGotShap, bGotName, bResize;

	nMaxChunks = _byteswap_ushort(pTileMem->nMaxChunks);
	pTileMemEntry = &pTileMem->tileMem;
	for (nChunk = 0; nMaxChunks > nChunk; ++nChunk) {
		memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
		memcpy(tilesetChunkHeader.szHead, pTileMemEntry->szHead, 4);
		tilesetChunkHeader.dwSize = _byteswap_ulong(pTileMemEntry->dwSize);
		pBuf = &pTileMemEntry->pBuf;

		bGotShap = bGotName = FALSE;
		if (memcmp(tilesetChunkHeader.szHead, "SHAP", 4) == 0) {
			bGotShap = Game_ReadTileShapInformation((tileShap_t *)pBuf);
			if (!bGotShap) {
				if (!bTilesetLoadOutOfMemory && bResize) {
					bTilesetLoadOutOfMemory = TRUE;
					bResize = FALSE;
				}
			}
		}
		else if (memcmp(tilesetChunkHeader.szHead, "NAME", 4) == 0)
			bGotName = Game_ReadTileNameInformation((tileName_t *)pBuf);

		// Added. If this is set to true it stands to reason
		// you'd then want to break out of the loop.
		if (bTilesetLoadOutOfMemory)
			break;

		if (bGotShap || bGotName || bResize) {
			bResize = FALSE;
			pTileMemEntry = (tileMem_t *)&pBuf[tilesetChunkHeader.dwSize];
			continue;
		}

		bResize = TRUE;
		pTileMemEntry = (tileMem_t *)Game_ReallocateDataEntry((char *)pTileMem, pBuf);
	}
}

extern "C" void __cdecl Hook_LoadTilesFromFile1996(FILE *f) {
	char *pBuf;
	WORD nMaxChunks, nChunk;
	DWORD dwSize;
	tilesetChunkHeader_t tilesetChunkHeader;
	BOOL bSomeBool;

	bTilesetLoadOutOfMemory = FALSE;
	pBuf = (char *)malloc(0x10000);
	if (pBuf) {
		fread(&nMaxChunks, 2, 1, f);
		nMaxChunks = _byteswap_ushort(nMaxChunks);
		for (nChunk = 0; nMaxChunks > nChunk; ++nChunk) {
			memset(&tilesetChunkHeader, 0, sizeof(tilesetChunkHeader_t));
			fread(tilesetChunkHeader.szHead, 1, 4, f);
			if (feof(f))
				break;
			fread(&dwSize, 4, 1, f);
			bSomeBool = (dwSize & 0x1000000) == 0;
			tilesetChunkHeader.dwSize = _byteswap_ulong(dwSize);
			if (!bSomeBool)
				tilesetChunkHeader.dwSize += 1;
			fread(pBuf, 1, tilesetChunkHeader.dwSize, f);
			if (memcmp(tilesetChunkHeader.szHead, "SHAP", 4) == 0) {
				if (!Game_ReadTileShapInformation((tileShap_t *)pBuf) && !bTilesetLoadOutOfMemory)
					bTilesetLoadOutOfMemory = TRUE;
			}
			else if (memcmp(tilesetChunkHeader.szHead, "NAME", 4) == 0)
				Game_ReadTileNameInformation((tileName_t *)pBuf);

			// Added. If this is set to true it stands to reason
			// you'd then want to break out of the loop.
			if (bTilesetLoadOutOfMemory)
				break;
		}
		free(pBuf);
	}
	else
		bTilesetLoadOutOfMemory = TRUE;
}

extern "C" BOOL __cdecl Hook_ReadTileShapInformation1996(tileShap_t *pTileShap) {
	__int16 nSpriteID;
	WORD nWidth, nHeight;
	DWORD dwSize;

	// if 'nHeight' > 1 then change the tile sprite entry, otherwise just return TRUE.
	// This check avoids zero'ing a building that may not be contained within the
	// tileset that's being loaded.

	nSpriteID = _byteswap_ushort(pTileShap->nSpriteID);
	nWidth = _byteswap_ushort(pTileShap->nWidth);
	nHeight = _byteswap_ushort(pTileShap->nHeight);
	dwSize = _byteswap_ulong(pTileShap->dwSize);
	return (nHeight > 1) ? Game_ChangeTileSpriteEntry(nSpriteID, nWidth, nHeight, dwSize, &pTileShap->pBuf) : TRUE;
}

extern "C" BOOL __cdecl Hook_ReadTileNameInformation1996(tileName_t *pTileName) {
	WORD nTileNameID, nNameLength;
	char *pNewTileName;

	nTileNameID = _byteswap_ushort(pTileName->nTileNameID);
	nNameLength = _byteswap_ushort(pTileName->nNameLength);
	pNewTileName = (char *)Game_AllocateDataEntry(nNameLength + 1);
	if (pNewTileName) {
		memcpy(pNewTileName, &pTileName->pBuf, nNameLength);
		pNewTileName[nNameLength] = 0;
		if (pTileNames[nTileNameID]) {
			Game_FreeDataEntry(pTileNames[nTileNameID]);
			pTileNames[nTileNameID] = 0;
		}
		pTileNames[nTileNameID] = pNewTileName;
	}
	return TRUE;
}

extern "C" BOOL __cdecl Hook_ChangeTileSpriteEntry1996(int nSpriteID, WORD nWidth, WORD nHeight, DWORD dwSize, void *pBuf) {
	BYTE *pDst;

	pDst = (BYTE *)Game_AllocateDataEntry(dwSize);
	if (pDst) {
		if (pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr) {
			Game_FreeDataEntry(pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr);
			pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = 0;
		}
		memcpy(pDst, pBuf, dwSize);
		pArrSpriteHeaders[nSpriteID].sprOffset.sprPtr = pDst;
		pArrSpriteHeaders[nSpriteID].wWidth = nWidth;
		pArrSpriteHeaders[nSpriteID].wHeight = nHeight;
		Cache_Sprite(nSpriteID, pDst, dwSize, nHeight, nWidth);
	}
	return (pDst) ? TRUE : FALSE;
}

void InstallSpriteAndTileSetHooks_SC2K1996(void) {
	// Hook LoadSpriteDataArchive
	SafeVirtualProtect((LPVOID)0x4029B4, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4029B4, Hook_LoadSpriteDataArchive1996);

	// Hook into InitializeDataColorsFonts - move actual sprite loading into external call.
	SafeVirtualProtect((LPVOID)0x42C314, 30, PAGE_EXECUTE_READWRITE);
	memset((LPVOID)0x42C314, 0x90, 30);
	NEWJMP((LPVOID)0x42C314, Hook_LoadSpriteArchives1996);

	// Hook CSimcityApp:LoadTileset
	SafeVirtualProtect((LPVOID)0x401E29, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401E29, Hook_SimcityApp_LoadTileset1996);

	// Hook ReadTilesetFile
	SafeVirtualProtect((LPVOID)0x4021F8, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4021F8, Hook_ReadTilesetFile1996);

	// Hook CheckTilesetFileHeader
	SafeVirtualProtect((LPVOID)0x401280, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401280, Hook_CheckTilesetFileHeader1996);

	// Hook VerifyAndLoadNewTiles
	SafeVirtualProtect((LPVOID)0x4019F6, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4019F6, Hook_VerifyAndLoadNewTiles1996);

	// Hook GetAndLoadNextTileFileChunkToMemory
	SafeVirtualProtect((LPVOID)0x402739, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402739, Hook_GetAndLoadNextTileFileChunkToMemory1996);

	// Hook ReallocateDataEntry
	SafeVirtualProtect((LPVOID)0x40264E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40264E, Hook_ReallocateDataEntry1996);

	// Hook LoadTilesFromMemory
	SafeVirtualProtect((LPVOID)0x401654, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401654, Hook_LoadTilesFromMemory1996);

	// Hook LoadTilesFromFile
	SafeVirtualProtect((LPVOID)0x401C35, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401C35, Hook_LoadTilesFromFile1996);

	// Hook ReadTileShapInformation
	SafeVirtualProtect((LPVOID)0x403044, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x403044, Hook_ReadTileShapInformation1996);

	// Hook ReadTileNameInformation
	SafeVirtualProtect((LPVOID)0x40260D, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x40260D, Hook_ReadTileNameInformation1996);

	// Hook ChangeTileSpriteEntry
	SafeVirtualProtect((LPVOID)0x4013E3, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4013E3, Hook_ChangeTileSpriteEntry1996);
}
