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
			ConsoleLog(LOG_INFO, "DumpMapThings:\n");
			for (__int16 i = MIN_THING_IDX; i <= MAX_THING_IDX; i++) {
				map_XTHG_t *pXTHG = GetXTHG(i);
				if (pXTHG) {
					ConsoleLog(LOG_INFO, "Thing(%d): Label(0x%06X)(%u), Thing[%s](%u), Direction[%s](%u), DirCoord(%u, %u), Goal(%u), State(%u), Coord(%u, %u, %u), PCoord(%u, %u)\n", i, 
						((pXTHG->bLabel) ? pXTHG->bLabel : 0), ((pXTHG->bLabel) ? pXTHG->bLabel : 0),
						szThingNames[pXTHG->iType], pXTHG->iType,
						szThingDirectionNames[pXTHG->iDirection], pXTHG->iDirection,
						pXTHG->iDX, pXTHG->iDY,
						pXTHG->iGoal, pXTHG->iState,
						pXTHG->iX, pXTHG->iY, pXTHG->iZ,
						pXTHG->iPX, pXTHG->iPY);
				}
			}
			ConsoleLog(LOG_INFO, "wActiveMaxisMan: %u\n", wActiveMaxisMan);
			ConsoleLog(LOG_INFO, "wActiveTrans: %u\n", wActiveTrains);
			ConsoleLog(LOG_INFO, "wSailingBoats: %u\n", wSailingBoats);
			ConsoleLog(LOG_INFO, "wActiveShips: %u\n", wActiveShips);
			ConsoleLog(LOG_INFO, "wMonsterSpawned: %u\n", wMonsterSpawned);
			ConsoleLog(LOG_INFO, "wActiveHelicopters: %u\n", wActiveHelicopters);
			ConsoleLog(LOG_INFO, "wActivePlanes: %u\n", wActivePlanes);
			ConsoleLog(LOG_INFO, "wActiveTornadoes: %u\n", wActiveTornadoes);
			ConsoleLog(LOG_INFO, "wDisasterObject: %d\n", wDisasterObject);
			ConsoleLog(LOG_INFO, "wPoliceUnitsDispatched: %u, wPoliceAvailDispatch: %u, dwPlacePoliceThingFail: %u\n", wPoliceUnitsDispatched, wPoliceAvailDispatch, dwPlacePoliceThingFail);
			ConsoleLog(LOG_INFO, "wFireUnitsDispatched: %u, wFireAvailDispatch: %u, dwPlaceFireThingFail: %u\n", wFireUnitsDispatched, wFireAvailDispatch, dwPlaceFireThingFail);
			ConsoleLog(LOG_INFO, "wMilitaryUnitsDispatched: %u, wMilitaryAvailDispatch: %u, dwPlaceMilitaryThingFail: %u\n", wMilitaryUnitsDispatched, wMilitaryAvailDispatch, dwPlaceMilitaryThingFail);
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
			for (__int16 i = MIN_THING_IDX; i <= MAX_THING_IDX; i++) {
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
				ConsoleLog(LOG_INFO, "wActiveMaxisMan: %d -> %u\n", wOldMaxisManCnt, wActiveMaxisMan);
				ConsoleLog(LOG_INFO, "wActiveTrans: %d -> %u\n", wOldTrainCnt, wActiveTrains);
				ConsoleLog(LOG_INFO, "wSailingBoats: %d -> %u\n", wOldSailingBoatCnt, wSailingBoats);
				ConsoleLog(LOG_INFO, "wActiveShips: %d -> %u\n", wOldShipCnt, wActiveShips);
				ConsoleLog(LOG_INFO, "wMonsterSpawned: %d -> %u\n", wOldMonsterCnt, wMonsterSpawned);
				ConsoleLog(LOG_INFO, "wActiveHelicopters: %d -> %u\n", wOldCopterCnt, wActiveHelicopters);
				ConsoleLog(LOG_INFO, "wActivePlanes: %d -> %u\n", wOldPlaneCnt, wActivePlanes);
				ConsoleLog(LOG_INFO, "wActiveTornadoes: %d -> %u\n", wOldTornadoCnt, wActiveTornadoes);
				ConsoleLog(LOG_INFO, "wDisasterObject: %d\n", wDisasterObject);
				ConsoleLog(LOG_INFO, "wPoliceUnitsDispatched: %d -> %u, wPoliceAvailDispatch: %u, dwPlacePoliceThingFail: %u\n", wOldPoliceUnitCnt, wPoliceUnitsDispatched, wPoliceAvailDispatch, dwPlacePoliceThingFail);
				ConsoleLog(LOG_INFO, "wFireUnitsDispatched: %d -> %u, wFireAvailDispatch: %u, dwPlaceFireThingFail: %u\n", wOldFireUnitCnt, wFireUnitsDispatched, wFireAvailDispatch, dwPlaceFireThingFail);
				ConsoleLog(LOG_INFO, "wMilitaryUnitsDispatched: %d -> %u, wMilitaryAvailDispatch: %u, dwPlaceMilitaryThingFail: %u\n", wOldMilitaryUnitCnt, wMilitaryUnitsDispatched, wMilitaryAvailDispatch, dwPlaceMilitaryThingFail);
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
					ConsoleLog(LOG_INFO, "CheckAndFixDeployThingCounter(): wActiveMaxisMan(%u), wPoliceUnitsDispatched(%u), wFireUnitsDispatched(%u), wMilitaryUnitsDispatched(%u)\n",
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
				ConsoleLog(LOG_DEBUG, "DeleteMapThingByIdx(%d):\n", nIdx);

			int nCnt = 0;
			for (__int16 i = MIN_THING_IDX; i <= MAX_THING_IDX; i++) {
				if (i == nIdx || nIdx == -1) {
					map_XTHG_t *pXTHG = GetXTHG(i);
					if (pXTHG) {
						ConsoleLog(LOG_INFO, "DeleteMapThingByIdx(%d): (%d) [%s](%u)\n", nIdx, i, szThingNames[pXTHG->iType], pXTHG->iType);
						if ((pXTHG->iX >= 0 && pXTHG->iX < GAME_MAP_SIZE) &&
							(pXTHG->iY >= 0 && pXTHG->iY < GAME_MAP_SIZE)) {
							ConsoleLog(LOG_INFO, "DeleteMapThingByIdx(%d): (%d) [%s](%u) (%d, %d) [%u] (XTXT entry cleared).\n", nIdx, i, szThingNames[pXTHG->iType], pXTHG->iType, 
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
				ConsoleLog(LOG_INFO, "DeleteMapThingByIdx(%d): %d things deleted\n", nIdx, nCnt);
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
				ConsoleLog(LOG_DEBUG, "DeleteMapThingsByType(%u):\n", nType);
	
			for (__int16 i = MIN_THING_IDX; i <= MAX_THING_IDX; i++) {
				map_XTHG_t *pXTHG = GetXTHG(i);
				if (pXTHG && pXTHG->iType == nType) {
					if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
						ConsoleLog(LOG_INFO, "DeleteMapThingsByType(%u): (%d) [%s](%u)\n", nType, i, szThingNames[pXTHG->iType], pXTHG->iType);
					if ((pXTHG->iX >= 0 && pXTHG->iX < GAME_MAP_SIZE) &&
						(pXTHG->iY >= 0 && pXTHG->iY < GAME_MAP_SIZE)) {
						if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
							ConsoleLog(LOG_INFO, "DeleteMapThingsByType(%u): (%d) [%s](%u) (%d, %d) [%u] (XTXT entry cleared).\n", nType, i, szThingNames[pXTHG->iType], pXTHG->iType, 
								pXTHG->iX, pXTHG->iY, XTXTGetTextOverlayID(pXTHG->iX, pXTHG->iY));
						XTXTSetTextOverlayID(pXTHG->iX, pXTHG->iY, 0);
					}
					memset(pXTHG, 0, sizeof(*pXTHG));
					nCnt++;
				}
			}
			if (nCnt > 0) {
				if (bVerbose || things_debug & THINGS_DEBUG_VERBOSE)
					ConsoleLog(LOG_INFO, "DeleteMapThingsByType(%u): %d things deleted\n", nType, nCnt);
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
		ConsoleLog(LOG_DEBUG, "ResetThingCleanupState()\n");
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
