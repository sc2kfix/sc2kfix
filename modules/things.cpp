// sc2kfix modules/things.cpp: thing handling
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define THINGS_DEBUG_OTHER 1
#define THINGS_DEBUG_VERBOSE 2

#define THINGS_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef THINGS_DEBUG
#define THINGS_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT things_debug = THINGS_DEBUG;

static int nWillRunCleanup = 2; // Have it run once by default.

void DumpMapThings_SC2K1996() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			ConsoleLog(LOG_INFO, "XTHG: DumpMapThings:\n");
			for (__int16 i = MIN_THING_IDX; i < MAX_THING_COUNT; i++) {
				map_XTHG_t *pXTHG = GetXTHG(i);
				if (pXTHG) {
					ConsoleLog(LOG_INFO, "XTHG:  - Thing(%d): Label(0x%06X)(%u), Thing[%s](%u), Direction[%s](%u), DirCoord(%u, %u), Goal(%u), State(%u), Coord(%u, %u, %u), PCoord(%u, %u)\n", i, 
						((pXTHG->bLabel) ? pXTHG->bLabel : 0), ((pXTHG->bLabel) ? pXTHG->bLabel : 0),
						szThingNames[pXTHG->iType], pXTHG->iType,
						szThingDirectionNames[pXTHG->iDirection], pXTHG->iDirection,
						pXTHG->iDX, pXTHG->iDY,
						pXTHG->iGoal, pXTHG->iState,
						pXTHG->iX, pXTHG->iY, pXTHG->iZ,
						pXTHG->iPX, pXTHG->iPY);
				}
			}
			ConsoleLog(LOG_INFO, "XTHG:  - wActiveMaxisMan: %u\n", wActiveMaxisMan);
			ConsoleLog(LOG_INFO, "XTHG:  - wActiveTrans: %u\n", wActiveTrains);
			ConsoleLog(LOG_INFO, "XTHG:  - wSailingBoats: %u\n", wSailingBoats);
			ConsoleLog(LOG_INFO, "XTHG:  - wActiveShips: %u\n", wActiveShips);
			ConsoleLog(LOG_INFO, "XTHG:  - wMonsterSpawned: %u\n", wMonsterSpawned);
			ConsoleLog(LOG_INFO, "XTHG:  - wActiveHelicopters: %u\n", wActiveHelicopters);
			ConsoleLog(LOG_INFO, "XTHG:  - wActivePlanes: %u\n", wActivePlanes);
			ConsoleLog(LOG_INFO, "XTHG:  - wActiveTornadoes: %u\n", wActiveTornadoes);
			ConsoleLog(LOG_INFO, "XTHG:  - wDisasterObject: %d\n", wDisasterObject);
			ConsoleLog(LOG_INFO, "XTHG:  - wPoliceUnitsDispatched: %u, wPoliceAvailDispatch: %u, dwPlacePoliceThingFail: %u\n", wPoliceUnitsDispatched, wPoliceAvailDispatch, dwPlacePoliceThingFail);
			ConsoleLog(LOG_INFO, "XTHG:  - wFireUnitsDispatched: %u, wFireAvailDispatch: %u, dwPlaceFireThingFail: %u\n", wFireUnitsDispatched, wFireAvailDispatch, dwPlaceFireThingFail);
			ConsoleLog(LOG_INFO, "XTHG:  - wMilitaryUnitsDispatched: %u, wMilitaryAvailDispatch: %u, dwPlaceMilitaryThingFail: %u\n", wMilitaryUnitsDispatched, wMilitaryAvailDispatch, dwPlaceMilitaryThingFail);
		}
	}
}

static void RecalculateThings_SC2K1996(BOOL bVerbose = TRUE) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 wOldMaxisManCnt = 0,
		wOldTrainCnt = 0,
		wOldSailingBoatCnt = 0,
		wOldShipCnt = 0,
		wOldCopterCnt = 0,
		wOldPlaneCnt = 0,
		wOldMonsterCnt = 0,
		wOldTornadoCnt = 0,
		wOldPoliceUnitCnt = 0,
		wOldFireUnitCnt = 0,
		wOldMilitaryUnitCnt = 0;
	__int16 wMaxisManCnt = 0,
		wTrainCnt = 0,
		wSailingBoatCnt = 0,
		wShipCnt = 0,
		wCopterCnt = 0,
		wPlaneCnt = 0,
		wMonsterCnt = 0,
		wTornadoCnt = 0,
		wPoliceUnitCnt = 0,
		wFireUnitCnt = 0,
		wMilitaryUnitCnt = 0;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			BYTE nType = XTHG_NONE;
			for (__int16 i = MIN_THING_IDX; i < MAX_THING_COUNT; i++) {
				map_XTHG_t *pXTHG = GetXTHG(i);
				if (pXTHG) {
					if (pXTHG->iType == XTHG_AIRPLANE)
						wPlaneCnt++;
					else if (pXTHG->iType == XTHG_HELICOPTER)
						wCopterCnt++;
					else if (pXTHG->iType == XTHG_CARGO_SHIP)
						wShipCnt++;
					else if (pXTHG->iType == XTHG_MONSTER)
						wMonsterCnt++;
					else if (pXTHG->iType == XTHG_DEPLOY_POLICE)
						wPoliceUnitCnt++;
					else if (pXTHG->iType == XTHG_DEPLOY_FIRE)
						wFireUnitCnt++;
					else if (pXTHG->iType == XTHG_SAILBOAT)
						wSailingBoatCnt++;
					else if (pXTHG->iType == XTHG_TRAIN_ENGINE || pXTHG->iType == XTHG_SUBWAY_TRAIN_ENGINE)
						wTrainCnt++;
					else if (pXTHG->iType == XTHG_DEPLOY_MILITARY)
						wMilitaryUnitCnt++;
					else if (pXTHG->iType == XTHG_TORNADO)
						wTornadoCnt++;
					else if (pXTHG->iType == XTHG_MAXIS_MAN)
						wMaxisManCnt++;
				}
			}

			wOldPlaneCnt = wActivePlanes;
			wActivePlanes = wPlaneCnt;
			wOldCopterCnt = wActiveHelicopters;
			wActiveHelicopters = wCopterCnt;
			wOldShipCnt = wActiveShips;
			wActiveShips = wShipCnt;
			wOldMonsterCnt = wMonsterSpawned;
			wMonsterSpawned = wMonsterCnt;
			wOldPoliceUnitCnt = wPoliceUnitsDispatched;
			wPoliceUnitsDispatched = wPoliceUnitCnt;
			wOldFireUnitCnt = wFireUnitsDispatched;
			wFireUnitsDispatched = wFireUnitCnt;
			wOldSailingBoatCnt = wSailingBoats;
			wSailingBoats = wSailingBoatCnt;
			wOldTrainCnt = wActiveTrains;
			wActiveTrains = wTrainCnt;
			wOldMilitaryUnitCnt = wMilitaryUnitsDispatched;
			wMilitaryUnitsDispatched = wMilitaryUnitCnt;
			wOldTornadoCnt = wActiveTornadoes;
			wActiveTornadoes = wTornadoCnt;
			wOldMaxisManCnt = wActiveMaxisMan;
			wActiveMaxisMan = wMaxisManCnt;

			if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE) {
				ConsoleLog(LOG_DEBUG, "XTHG: Thing counts recalculated:\n", wOldMaxisManCnt, wActiveMaxisMan);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActiveMaxisMan: %d -> %u\n", wOldMaxisManCnt, wActiveMaxisMan);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActiveTrans: %d -> %u\n", wOldTrainCnt, wActiveTrains);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wSailingBoats: %d -> %u\n", wOldSailingBoatCnt, wSailingBoats);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActiveShips: %d -> %u\n", wOldShipCnt, wActiveShips);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wMonsterSpawned: %d -> %u\n", wOldMonsterCnt, wMonsterSpawned);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActiveHelicopters: %d -> %u\n", wOldCopterCnt, wActiveHelicopters);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActivePlanes: %d -> %u\n", wOldPlaneCnt, wActivePlanes);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wActiveTornadoes: %d -> %u\n", wOldTornadoCnt, wActiveTornadoes);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wDisasterObject: %d\n", wDisasterObject);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wPoliceUnitsDispatched: %d -> %u, wPoliceAvailDispatch: %u, dwPlacePoliceThingFail: %u\n", wOldPoliceUnitCnt, wPoliceUnitsDispatched, wPoliceAvailDispatch, dwPlacePoliceThingFail);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wFireUnitsDispatched: %d -> %u, wFireAvailDispatch: %u, dwPlaceFireThingFail: %u\n", wOldFireUnitCnt, wFireUnitsDispatched, wFireAvailDispatch, dwPlaceFireThingFail);
				ConsoleLog(LOG_DEBUG, "XTHG:  - wMilitaryUnitsDispatched: %d -> %u, wMilitaryAvailDispatch: %u, dwPlaceMilitaryThingFail: %u\n", wOldMilitaryUnitCnt, wMilitaryUnitsDispatched, wMilitaryAvailDispatch, dwPlaceMilitaryThingFail);
			}
		}
	}
}

static void CheckAndFixDeployThingCounter_SC2K1996(BOOL bVerbose = TRUE) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			if (wActiveMaxisMan > 0 || wPoliceUnitsDispatched > 0 || wFireUnitsDispatched > 0 || wMilitaryUnitsDispatched > 0) {
				if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
					ConsoleLog(LOG_INFO, "XTHG: CheckAndFixDeployThingCounter(): wActiveMaxisMan(%u), wPoliceUnitsDispatched(%u), wFireUnitsDispatched(%u), wMilitaryUnitsDispatched(%u)\n",
						wActiveMaxisMan, wPoliceUnitsDispatched, wFireUnitsDispatched, wMilitaryUnitsDispatched);
				RecalculateThings_SC2K1996(bVerbose);
			}
		}
	}
}

void DeleteMapThingByIdx_SC2K1996(__int16 nIdx) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			if (things_debug & THINGS_DEBUG_VERBOSE)
				ConsoleLog(LOG_DEBUG, "XTHG: DeleteMapThingByIdx(%d):\n", nIdx);

			int nCnt = 0;
			for (__int16 i = MIN_THING_IDX; i < MAX_THING_COUNT; i++) {
				if (i == nIdx || nIdx == -1) {
					map_XTHG_t *pXTHG = GetXTHG(i);
					if (pXTHG) {
						ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingByIdx(%d): (%d) [%s](%u)\n", nIdx, i, szThingNames[pXTHG->iType], pXTHG->iType);
						if ((pXTHG->iX >= 0 && pXTHG->iX < GAME_MAP_SIZE) &&
							(pXTHG->iY >= 0 && pXTHG->iY < GAME_MAP_SIZE)) {
							ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingByIdx(%d): (%d) [%s](%u) (%d, %d) [%u] (XTXT entry cleared).\n", nIdx, i, szThingNames[pXTHG->iType], pXTHG->iType, 
								pXTHG->iX, pXTHG->iY, XTXTGetTextOverlayID(pXTHG->iX, pXTHG->iY));
							XTXTSetTextOverlayID(pXTHG->iX, pXTHG->iY, 0);
						}
						memset(pXTHG, 0, sizeof(*pXTHG));
						nCnt++;
						if (i == nIdx)
							break;
					}
				}
			}
			if (nCnt > 0) {
				ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingByIdx(%d): %d things deleted\n", nIdx, nCnt);
				RecalculateThings_SC2K1996();
			}
		}
	}
}

static int DeleteMapThingsByType_SC2K1996(BYTE nType, BOOL bVerbose = TRUE) {
	int nCnt = 0;
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			if (things_debug & THINGS_DEBUG_VERBOSE)
				ConsoleLog(LOG_DEBUG, "XTHG: DeleteMapThingsByType(%u):\n", nType);
	
			for (__int16 i = MIN_THING_IDX; i < MAX_THING_COUNT; i++) {
				map_XTHG_t *pXTHG = GetXTHG(i);
				if (pXTHG && pXTHG->iType == nType) {
					if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
						ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingsByType(%u): (%d) [%s](%u)\n", nType, i, szThingNames[pXTHG->iType], pXTHG->iType);
					if ((pXTHG->iX >= 0 && pXTHG->iX < GAME_MAP_SIZE) &&
						(pXTHG->iY >= 0 && pXTHG->iY < GAME_MAP_SIZE)) {
						if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
							ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingsByType(%u): (%d) [%s](%u) (%d, %d) [%u] (XTXT entry cleared).\n", nType, i, szThingNames[pXTHG->iType], pXTHG->iType, 
								pXTHG->iX, pXTHG->iY, XTXTGetTextOverlayID(pXTHG->iX, pXTHG->iY));
						XTXTSetTextOverlayID(pXTHG->iX, pXTHG->iY, 0);
					}
					memset(pXTHG, 0, sizeof(*pXTHG));
					nCnt++;
				}
			}
			if (nCnt > 0) {
				if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
					ConsoleLog(LOG_INFO, "XTHG: DeleteMapThingsByType(%u): %d things deleted\n", nType, nCnt);
			}
		}
	}

	return nCnt;
}

void DeleteAllPlanes_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_AIRPLANE, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllCopters_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_HELICOPTER, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllShips_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_CARGO_SHIP, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllSailboats_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_SAILBOAT, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllTrains_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_TRAIN_ENGINE, bVerbose);
	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_TRAIN_CAR, bVerbose);
	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_SUBWAY_TRAIN_ENGINE, bVerbose);
	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_SUBWAY_TRAIN_CAR, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllMaxisMen_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_MAXIS_MAN, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
	// Last ditch if there's any detritus left-over.
	CheckAndFixDeployThingCounter_SC2K1996(bVerbose);
}

void DeleteAllMonsters_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_MONSTER, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllTornadoes_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_TORNADO, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
}

void DeleteAllPoliceDeploys_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_POLICE, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
	// Last ditch if there's any detritus left-over.
	CheckAndFixDeployThingCounter_SC2K1996(bVerbose);
}

void DeleteAllFireDeploys_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_FIRE, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
	// Last ditch if there's any detritus left-over.
	CheckAndFixDeployThingCounter_SC2K1996(bVerbose);
}

void DeleteAllMilitaryDeploys_SC2K1996(BOOL bVerbose) {
	int nCnt = 0;

	nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_MILITARY, bVerbose);
	if (nCnt > 0)
		RecalculateThings_SC2K1996(bVerbose);
	// Last ditch if there's any detritus left-over.
	CheckAndFixDeployThingCounter_SC2K1996(bVerbose);
}

void DeleteAllDisasterDeploys_SC2K1996() {
	int nCnt = 0;

	if (bDisableAutoThingCleanup)
		return;

	if (!dwDisasterActive) {
		if (nWillRunCleanup == 2) {
			nCnt += DeleteMapThingsByType_SC2K1996(XTHG_MAXIS_MAN, FALSE);
			nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_POLICE, FALSE);
			nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_FIRE, FALSE);
			nCnt += DeleteMapThingsByType_SC2K1996(XTHG_DEPLOY_MILITARY, FALSE);
			if (nCnt > 0)
				RecalculateThings_SC2K1996(FALSE);
			// Last ditch if there's any detritus left-over.
			CheckAndFixDeployThingCounter_SC2K1996(FALSE);
			nWillRunCleanup = 1;
		}
		else
			nWillRunCleanup = 0;
	}
	else
		nWillRunCleanup = 2;
}

void ResetThingCleanupState_SC2K1996() {
	if (things_debug & THINGS_DEBUG_OTHER)
		ConsoleLog(LOG_DEBUG, "XTHG: ResetThingCleanupState()\n");
	nWillRunCleanup = 2;
}

static const char *szThingDef[THING_CLEAN_COUNT] = {
	"Plane",
	"Copter",
	"Ship",
	"Sailboat",
	"Train",
	"Maxis Man",
	"Monster",
	"Tornado",
	"Police Deploy",
	"Fire Deploy",
	"Military Deploy"
};

void DoThingClean_SC2K1996(int nThingDef) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	char szMsgStr[256 + 1];

	if (nThingDef < THING_CLEAN_PLANES || nThingDef >= THING_CLEAN_COUNT)
		return;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {
			if (wCityMode != GAME_MODE_CITY) {
				sprintf_s(szMsgStr, sizeof(szMsgStr) - 1, "You cannot clear %s 'things' at this time.", szThingDef[nThingDef]);
				L_MessageBoxA(GameGetRootWindowHandle(), szMsgStr, "Error", MB_OK|MB_ICONERROR);
				return;
			}
			sprintf_s(szMsgStr, sizeof(szMsgStr) - 1, "Are you sure that you want to clear all existing %s 'things'? This action cannot be undone.", szThingDef[nThingDef]);
			if (L_MessageBoxA(GameGetRootWindowHandle(), szMsgStr, "Warning", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				switch (nThingDef) {
					case THING_CLEAN_PLANES:
						DeleteAllPlanes_SC2K1996(FALSE);
						break;
					case THING_CLEAN_COPTERS:
						DeleteAllCopters_SC2K1996(FALSE);
						break;
					case THING_CLEAN_SHIPS:
						DeleteAllShips_SC2K1996(FALSE);
						break;
					case THING_CLEAN_SAILBOATS:
						DeleteAllSailboats_SC2K1996(FALSE);
						break;
					case THING_CLEAN_TRAINS:
						DeleteAllTrains_SC2K1996(FALSE);
						break;
					case THING_CLEAN_HERO:
						DeleteAllMaxisMen_SC2K1996(FALSE);
						break;
					case THING_CLEAN_MONSTER:
						DeleteAllMonsters_SC2K1996(FALSE);
						break;
					case THING_CLEAN_TORNADO:
						DeleteAllTornadoes_SC2K1996(FALSE);
						break;
					case THING_CLEAN_PLDEPLOY:
						DeleteAllPoliceDeploys_SC2K1996(FALSE);
						break;
					case THING_CLEAN_FRDEPLOY:
						DeleteAllFireDeploys_SC2K1996(FALSE);
						break;
					case THING_CLEAN_MLDEPLOY:
						DeleteAllMilitaryDeploys_SC2K1996(FALSE);
						break;
				}
			}
		}
	}
}

extern "C" void __stdcall Hook_SimcityView_DrawThingObjects(__int16 x, __int16 y, __int16 nThingID) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	map_XTHG_t *pTHG;
	int nFlip;
	__int16 nBaseSprite, nSpriteID;
	__int16 nScale, nElevation;
	sprite_header_t *pSprite;
	__int16 bottom, right;

	pTHG = GetXTHG(nThingID);
	if (wThingZoomVisibility[pTHG->iType] <= pThis->wSCVZoomLevel) {
		if ((pTHG->iX != x || pTHG->iY != y) && pTHG->iType != XTHG_TRAIN_ENGINE && pTHG->iType != XTHG_TRAIN_CAR) {
			XTXTSetTextOverlayID(x, y, 0);
			return;
		}
		switch (pTHG->iType) {
			case XTHG_AIRPLANE:
			case XTHG_HELICOPTER:
			case XTHG_CARGO_SHIP:
				if (DisplayLayer[LAYER_UNDERGROUND])
					return;
				nBaseSprite = nShipDirectionPos[pTHG->iDirection] + wThingSprites[pTHG->iType];
				nFlip = nShipDirectionFlip[pTHG->iDirection];
				break;
			case XTHG_BULLDOZER:
			case XTHG_SAILBOAT:
				if (DisplayLayer[LAYER_UNDERGROUND])
					return;
				if (pTHG->iType != XTHG_SAILBOAT || !pTHG->iState) {
					nBaseSprite = nThingDirectionPosition[pTHG->iDirection] + wThingSprites[pTHG->iType];
					nFlip = nThingDirectionFlip[pTHG->iDirection];
				}
				else {
					nBaseSprite = SPRITE_SMALL_NESSIE;
					nFlip = IsEven(nNessieFlip);
				}
				break;
			case XTHG_MONSTER:
				Game_SimcityView_DrawMonster(pThis, x, y, nThingID);
				return;
			case XTHG_EXPLOSION:
				nBaseSprite = pTHG->iDirection + wThingSprites[pTHG->iType];
				nFlip = Game_RandomWordLFSRMod(2);
				break;
			case XTHG_DEPLOY_POLICE:
			case XTHG_DEPLOY_FIRE:
			case XTHG_DEPLOY_MILITARY:
				Game_DrawThings(x, y, nThingID);
				return;
			case XTHG_TRAIN_ENGINE:
			case XTHG_TRAIN_CAR:
				Game_DrawTrain(x, y, nThingID);
				return;
			case XTHG_SUBWAY_TRAIN_ENGINE:
			case XTHG_SUBWAY_TRAIN_CAR:
				return;
			case XTHG_TORNADO:
				Game_SimcityView_DrawTornado(pThis, x, y, nThingID);
				return;
			case XTHG_MAXIS_MAN:
				nBaseSprite = wThingSprites[pTHG->iType];
				nFlip = pTHG->iDirection > XTHG_DIRECTION_SOUTH_EAST;
				break;
			default:
				return;
		}
		nScale = 2 << pThis->wSCVZoomLevel;
		nSpriteID = SPRITE_MEDIUM_START * pThis->wSCVZoomLevel + nBaseSprite;
		if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE && XBITReturnIsWater(x, y))
			nElevation = ALTMReturnWaterLevel(x, y);
		else
			nElevation = ALTMReturnLandAltitude(x, y);
		pSprite = &pArrSpriteHeaders[nSpriteID];
		bottom = iScreenOffSetY +
			nScale * (x + y) + (pTHG->iPY + pTHG->iPX) / nThingZoomYDivisor[pThis->wSCVZoomLevel] -
			pSprite->wHeight - ((3 << pThis->wSCVZoomLevel) * nElevation + pTHG->iZ * nScale);
		right = iScreenOffSetX +
			(pTHG->iPX - pTHG->iPY) / nThingZoomXDivisor[pThis->wSCVZoomLevel] +
			(4 << pThis->wSCVZoomLevel) * (x - y + 1) - (pSprite->wWidth >> 1);
		if ((pTHG->iType == XTHG_HELICOPTER || pTHG->iType == XTHG_AIRPLANE || pTHG->iType == XTHG_MAXIS_MAN) &&
			GetTileID(x, y) < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1)
			Game_DrawProcessShadowObject(nSpriteID, right, bottom + nScale * (pTHG->iZ - 2), nFlip);
		Game_DrawProcessObject(nSpriteID, right, bottom, nFlip, 0);
	}
}

typedef struct {
	__int16 x;
	__int16 y;
	__int16 xOffset;
	__int16 yOffset;
	BYTE bTextOverlay;
	RECT rFace;
	RECT rPole;
} signPositions_t;

std::vector<signPositions_t> signPos;

void L_ClearStoredSignPos() {
	signPos.clear();
}

void L_FindNearestSignPos(CSimcityView *pSCView, RECT *r) {
	int nZoomDiff = 0;
	RECT rBounds;

	if (!DisplayLayer[LAYER_SIGNS])
		return;

	switch (pSCView->wSCVZoomLevel) {
		case ZOOM_LEVEL_TINY:
			nZoomDiff = 2;
			break;
		case ZOOM_LEVEL_SMALL:
			nZoomDiff = 4;
			break;
		default:
			nZoomDiff = 8;
			break;
	}
	SetRect(&rBounds, r->left - nZoomDiff, r->top - nZoomDiff, r->right + nZoomDiff, r->bottom + nZoomDiff);
	for (std::vector<signPositions_t>::iterator sP = signPos.begin(); sP != signPos.end();) {
		if (&sP) {
			if (rBounds.left <= sP->rFace.right && 
				rBounds.right >= sP->rFace.left && 
				(rBounds.top <= sP->rPole.bottom || rBounds.top >= sP->rPole.top) && 
				rBounds.bottom >= sP->rFace.top) {
				//ConsoleLog(LOG_DEBUG, "L_FindNearestSignPos(): (%d, %d) (%d, %d) (%u) rFace(%d, %d, %d, %d) rPole(%d, %d, %d, %d) - (%d) rBounds(%d, %d, %d, %d)\n",
				//	sP->x, sP->y,
				//	sP->xOffset, sP->yOffset,
				//	sP->bTextOverlay,
				//	sP->rFace.left, sP->rFace.top, sP->rFace.right, sP->rFace.bottom,
				//	sP->rPole.left, sP->rPole.bottom, sP->rPole.right, sP->rPole.top,
				//	nZoomDiff,
				//	rBounds.left, rBounds.top, rBounds.right, rBounds.bottom);
				if (XTXTGetTextOverlayID(sP->x, sP->y))
					L_DrawLabelsAndObjects(sP->x, sP->y, sP->xOffset, sP->yOffset, true);
			}
		}
		++sP;
	}
}

static bool L_FindStoredSignPos(__int16 x, __int16 y, __int16 xOffset, __int16 yOffset, BYTE bTextOverlay, RECT *rFace, RECT *rPole) {
	for (std::vector<signPositions_t>::iterator sP = signPos.begin(); sP != signPos.end();) {
		if (sP->x == x && sP->y == y && 
			sP->xOffset == xOffset && sP->yOffset == yOffset && 
			sP->bTextOverlay == bTextOverlay && 
			&sP->rFace == rFace && 
			&sP->rPole == rPole)
			return true;
		++sP;
	}
	return false;
}

static void L_StoreSignPos(__int16 x, __int16 y, __int16 xOffset, __int16 yOffset, BYTE bTextOverlay, RECT *rFace, RECT *rPole) {
	signPositions_t sP;

	if (L_FindStoredSignPos(x, y, xOffset, yOffset, bTextOverlay, rFace, rPole))
		return;

	sP.x = x;
	sP.y = y;
	sP.xOffset = xOffset;
	sP.yOffset = yOffset;
	sP.bTextOverlay = bTextOverlay;
	CopyRect(&sP.rFace, rFace);
	CopyRect(&sP.rPole, rPole);
	signPos.push_back(sP);
}

static void L_DrawSignPart(HDC hDC, RECT *pRect, COLORREF cr) {
	HPEN hPenBase, hPenShine, hPenShade, hPenInitial;
	HBRUSH hBrush;
	POINT pt;

	hPenBase = CreatePen(PS_SOLID, 1, crSignBase);
	hPenShine = CreatePen(PS_SOLID, 2, crSignShine);
	hPenShade = CreatePen(PS_SOLID, 2, crSignShade); // There was previously also an unused hPenShade with a width of 1.
	hBrush = CreateSolidBrush(cr);
	FillRect(hDC, pRect, hBrush);
	hPenInitial = SelectPen(hDC, hPenShine);
	MoveToEx(hDC, pRect->right - 2, pRect->top, &pt);
	LineTo(hDC, pRect->left, pRect->top);
	LineTo(hDC, pRect->left, pRect->bottom - 2);
	SelectPen(hDC, hPenShade);
	MoveToEx(hDC, pRect->left + 1, pRect->bottom - 2, &pt);
	LineTo(hDC, pRect->right - 2, pRect->bottom - 2);
	LineTo(hDC, pRect->right - 2, pRect->top + 1);
	SelectPen(hDC, hPenBase);
	MoveToEx(hDC, pRect->left, pRect->bottom - 1, &pt);
	LineTo(hDC, pRect->left + 1, pRect->bottom - 2);
	MoveToEx(hDC, pRect->right - 1, pRect->top, &pt);
	MoveToEx(hDC, pRect->right - 2, pRect->top + 1, &pt);
	SelectPen(hDC, hPenInitial);
	DeleteBrush(hBrush);
	DeletePen(hPenShade);
	DeletePen(hPenShine);
	DeletePen(hPenBase);
}

void L_DrawLabelsAndObjects(__int16 x, __int16 y, __int16 inXOffset, __int16 inYOffset, bool bOnlySign) {
	CSimcityAppPrimary *pSCApp = &pCSimcityAppThis;
	CSimcityView *pSCView;
	CMFC3XDC *pDC;
	HFONT hFont;
	COLORREF crSignSurface, crSignPostEdge;
	__int16 xOffset, yOffset;
	BYTE bTextOverlay;
	__int16 nNeighCompassDir, nPosNum;
	char *pLabel;
	int nLen, nPos;
	int nLabelLen;
	SIZE txtSZ;
	RECT rFace;
	RECT rPole;

	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	pDC = Game_Graphics_GetDC(pSCView->SCVGraphics);
	crSignSurface = RGB(187, 187, 187);
	crSignPostEdge = RGB(159, 159, 159);
	xOffset = inXOffset;
	yOffset = inYOffset;
	bTextOverlay = XTXTGetTextOverlayID(x, y);
	if (bTextOverlay) {
		if (bTextOverlay < MIN_SIM_TEXT_ENTRIES || bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
			if (DisplayLayer[LAYER_SIGNS]) {
				pLabel = GetXLABEntry(bTextOverlay);
				if (pSCView->wSCVZoomLevel) {
					if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_SMALL) {
						xOffset += 8;
						yOffset -= 4;
					}
					else if (pSCView->wSCVZoomLevel == ZOOM_LEVEL_LARGE) {
						xOffset += 16;
						yOffset -= 8;
					}
				}
				else {
					xOffset += 4;
					yOffset -= 2;
				}
				hFont = SelectFont(pDC->m_hDC, MainFontsArl[pSCView->wSCVZoomLevel]->m_hObject);
				if (bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
					nNeighCompassDir = VIEWROTATION_NORTH;
					nPosNum = 0;
					if (y < MAP_EDGE_MIN + 2) {
						nNeighCompassDir = VIEWROTATION_WEST;
						nPosNum = x;
					}
					if (x > MAP_EDGE_MAX - 2) {
						nNeighCompassDir = VIEWROTATION_NORTH;
						nPosNum = y;
					}
					if (y > MAP_EDGE_MAX - 2) {
						nNeighCompassDir = VIEWROTATION_EAST;
						nPosNum = MAP_EDGE_MAX - x;
					}
					if (x < MAP_EDGE_MIN + 2) {
						nNeighCompassDir = VIEWROTATION_SOUTH;
						nPosNum = MAP_EDGE_MAX - y;
					}
					nLen = strlen(&stNeighborCities[MAX_NEIGH_BUF_SIZE * ((nNeighCompassDir + wViewRotation) & 3)]);
					if (nLen > MAX_CONNLABEL_LEN)
						nLen = MAX_CONNLABEL_LEN;
					memset(pLabel, 0, MAX_LABEL_LEN);
					for (nPos = 0; nLen > nPos; ++nPos)
						pLabel[nPos] = stNeighborCities[MAX_NEIGH_BUF_SIZE * ((nNeighCompassDir + wViewRotation) & 3) + nPos];
					pLabel[nLen] = ' ';
					pLabel[nLen + 1] = nPosNum % 5 + '5';
					pLabel[nLen + 2] = 0;
				}
				nLabelLen = strlen(pLabel);
				GetTextExtentPointA(pDC->m_hAttribDC, pLabel, nLabelLen, &txtSZ);
				rFace.bottom = yOffset - 15 * pSCView->wSCVZoomLevel - 20;
				rFace.top = rFace.bottom - wFontHeightsArl[pSCView->wSCVZoomLevel] - 5;
				rFace.left = xOffset - (txtSZ.cx / 2) - 5; // Was 8 - Sign Font Fix
				rFace.right = rFace.left + txtSZ.cx + 10; // Was 16 - Sign Font Fix
				rPole.top = rFace.bottom;
				rPole.bottom = yOffset;
				rPole.left = xOffset - 2;
				rPole.right = xOffset + 2;
				// Sign Pole
				L_DrawSignPart(pDC->m_hDC, &rPole, crSignPostEdge);
				// Sign face.
				rFace.bottom += 2; // Added this to avoid some bottom-extent text overlap cases.
				L_DrawSignPart(pDC->m_hDC, &rFace, crSignSurface);
				SetTextColor(pDC->m_hDC, crSignText);
				SetTextAlign(pDC->m_hDC, TA_LEFT);
				TextOutA(pDC->m_hDC, rFace.left + 4, rFace.top + 2, pLabel, nLabelLen);
				SelectFont(pDC->m_hDC, hFont);
				// Only store during general drawing, not for sign bound checking during tile inversion.
				if (!bOnlySign)
					L_StoreSignPos(x, y, inXOffset, inYOffset, bTextOverlay, &rFace, &rPole);
			}
		}
		else if (bTextOverlay < MIN_DISASTER_TEXT_ENTRIES) {
			if (!bOnlySign) {
				if (bTextOverlay <= MAX_XTHG_TEXT_ENTRIES && bTextOverlay >= MIN_XTHG_TEXT_ENTRIES)
					Game_SimcityView_DrawThingObjects(pSCView, x, y, XTHGID_ENTRY(bTextOverlay));
			}
		}
		else {
			if (!bOnlySign)
				Game_DrawDisasterObjects(x, y, bTextOverlay);
		}
	}
	Game_Graphics_ReleaseDC(pSCView->SCVGraphics, pDC);
}

void InstallThingHooks_SC2K1996(void) {
	// Hook for CSimcityView::DrawThingObjects
	SafeVirtualProtect((LPVOID)0x401334, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401334, Hook_SimcityView_DrawThingObjects);
}
