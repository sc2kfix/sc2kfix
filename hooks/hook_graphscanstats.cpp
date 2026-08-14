// sc2kfix hooks/hook_graphscanstats.cpp: hooks to do with graphs, scans and stats
// calls
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Need more terms in filename.

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

extern "C" void __stdcall Hook_RecalculateCityValue(void) {
	int dwNewCityValue, nVal, nTool, nSubTool, nTileArea;
	BYTE nTileID;
	BOOL bValid;

	// Start with subway tiles to initialize the value.
	dwNewCityValue = costFromSubTool[CITY_MENUTOOL_POS(RAILS_SUBWAY, CITYTOOL_GROUP_RAIL)] * wSubwayXUNDCount;
	// Infrastructure tiles.
	for (nTileID = TILE_POWERLINES_LR; nTileID <= TILE_SUBTORAIL_L; ++nTileID) {
		bValid = FALSE;
		nVal = 0;

		if (GET_TILE_RANGE(nTileID, TILE_POWERLINES_LR, TILE_POWERLINES_LTBR)) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(POWER_WIRES, CITYTOOL_GROUP_POWER)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_ROAD_LR, TILE_ROAD_LTBR)) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(ROADS_ROAD, CITYTOOL_GROUP_ROADS)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_RAIL_LR, TILE_RAIL_HHLR)) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(RAILS_RAIL, CITYTOOL_GROUP_RAIL)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_TUNNEL_T, TILE_CROSSOVER_HIGHWAYTB_POWERLR)) {
			nVal = 15 * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ONRAMP_BR)) {
			nVal = 100 * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_HIGHWAY_HTB, TILE_REINFORCED_BRIDGE)) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(ROADS_HIGHWAY, CITYTOOL_GROUP_ROADS)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_SUBTORAIL_T, TILE_SUBTORAIL_L)) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(RAILS_SUBTORAIL, CITYTOOL_GROUP_RAIL)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		if (bValid)
			dwNewCityValue += nVal;
	}

	// Now for buildings objects.
	for (nTileID = TILE_POWERPLANT_HYDRO1; nTileID <= TILE_ARCOLOGY_LAUNCH; ++nTileID) {
		bValid = FALSE;
		nVal = 0;

		if (GET_TILE_RANGE(nTileID, TILE_POWERPLANT_HYDRO1, TILE_POWERPLANT_WIND)) {
			if (nTileID == TILE_POWERPLANT_WIND)
				nSubTool = POWER_PLANTS_WIND;
			else
				nSubTool = POWER_PLANTS_HYDRO;
			nVal = costFromSubTool[CITY_MENUTOOL_POS(nSubTool, CITYTOOL_GROUP_POWER)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_POWERPLANT_GAS, TILE_POWERPLANT_COAL)) {
			if (nTileID == TILE_POWERPLANT_GAS)
				nSubTool = POWER_PLANTS_GAS;
			else if (nTileID == TILE_POWERPLANT_OIL)
				nSubTool = POWER_PLANTS_OIL;
			else if (nTileID == TILE_POWERPLANT_NUCLEAR)
				nSubTool = POWER_PLANTS_NUCLEAR;
			else if (nTileID == TILE_POWERPLANT_SOLAR)
				nSubTool = POWER_PLANTS_SOLAR;
			else if (nTileID == TILE_POWERPLANT_MICROWAVE)
				nSubTool = POWER_PLANTS_MICROWAVE;
			else if (nTileID == TILE_POWERPLANT_FUSION)
				nSubTool = POWER_PLANTS_FUSION;
			else
				nSubTool = POWER_PLANTS_COAL;
			nVal = costFromSubTool[CITY_MENUTOOL_POS(nSubTool, CITYTOOL_GROUP_POWER)] * (wTileCount[nTileID] / 16);
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_SERVICES_HOSPITAL, TILE_INFRASTRUCTURE_WATERPUMP)) {
			// Statues aren't factored into the calculation anywhere within this loop.
			if (nTileID == TILE_SERVICES_STATUE)
				continue;
			if (nTileID == TILE_SERVICES_SCHOOL || nTileID == TILE_SERVICES_COLLEGE || nTileID == TILE_SERVICES_MUSEUM) {
				nTileArea = (nTileID == TILE_SERVICES_COLLEGE) ? 16 : 9;
				nTool = CITYTOOL_GROUP_EDUCATION;
				if (nTileID == TILE_SERVICES_SCHOOL)
					nSubTool = EDUCATION_SCHOOL;
				else if (nTileID == TILE_SERVICES_COLLEGE)
					nSubTool = EDUCATION_COLLEGE;
				else
					nSubTool = EDUCATION_MUSEUM;
			}
			else if (nTileID == TILE_SERVICES_POLICE || nTileID == TILE_SERVICES_FIRE || nTileID == TILE_SERVICES_HOSPITAL || nTileID == TILE_SERVICES_PRISON) {
				nTileArea = (nTileID == TILE_SERVICES_PRISON) ? 16 : 9;
				nTool = CITYTOOL_GROUP_SERVICES;
				if (nTileID == TILE_SERVICES_POLICE)
					nSubTool = SERVICES_POLICE;
				else if (nTileID == TILE_SERVICES_FIRE)
					nSubTool = SERVICES_FIRESTATION;
				else if (nTileID == TILE_SERVICES_HOSPITAL)
					nSubTool = SERVICES_HOSPITAL;
				else
					nSubTool = SERVICES_PRISON;
			}
			else if (nTileID == TILE_SERVICES_BIGPARK || nTileID == TILE_SERVICES_ZOO || nTileID == TILE_SERVICES_STADIUM) {
				nTileArea = (nTileID == TILE_SERVICES_BIGPARK) ? 9 : 16;
				nTool = CITYTOOL_GROUP_PARKS;
				if (nTileID == TILE_SERVICES_BIGPARK)
					nSubTool = PARKS_BIGPARK;
				else if (nTileID == TILE_SERVICES_ZOO)
					nSubTool = PARKS_ZOO;
				else
					nSubTool = PARKS_STADIUM;
			}
			else {
				nTileArea = 1;
				nTool = CITYTOOL_GROUP_WATER;
				nSubTool = WATER_PUMP;
			}
			nVal = costFromSubTool[CITY_MENUTOOL_POS(nSubTool, nTool)] * (wTileCount[nTileID] / nTileArea);
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_RUNWAY, TILE_INFRASTRUCTURE_RUNWAYCROSS) ||
			nTileID == TILE_INFRASTRUCTURE_CONTROLTOWER_CIV ||
			GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_BUILDING1, TILE_MILITARY_TARMAC) ||
			nTileID == TILE_MILITARY_RADAR || nTileID == TILE_INFRASTRUCTURE_PARKINGLOT || nTileID == TILE_INFRASTRUCTURE_HANGAR2) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(PORTS_AIRPORT, CITYTOOL_GROUP_PORTS)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_PIER || nTileID == TILE_INFRASTRUCTURE_CRANE || nTileID == TILE_MILITARY_WAREHOUSE ||
			nTileID == TILE_INFRASTRUCTURE_LOADINGBAY || nTileID == TILE_INFRASTRUCTURE_CARGOYARD) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(PORTS_SEAPORT, CITYTOOL_GROUP_PORTS)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_SUBWAYSTATION) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(RAILS_SUBSTATION, CITYTOOL_GROUP_RAIL)] * wTileCount[nTileID];
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_WATERTOWER) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(WATER_TOWER, CITYTOOL_GROUP_WATER)] * (wTileCount[nTileID] / 4);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_BUSDEPOT) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(ROADS_BUSSTATION, CITYTOOL_GROUP_ROADS)] * (wTileCount[nTileID] / 4);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_RAILSTATION) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(RAILS_DEPOT, CITYTOOL_GROUP_RAIL)] * (wTileCount[nTileID] / 4);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_WATERTREATMENT) {
			// The divisor here was previously 9, however since
			// the object only occupies 4 tiles it has been
			// adjusted accordingly. (Marking this just in case
			// this was NOT a bug)
			nVal = costFromSubTool[CITY_MENUTOOL_POS(WATER_TREATMENT, CITYTOOL_GROUP_WATER)] * (wTileCount[nTileID] / 4);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_LIBRARY) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(EDUCATION_LIBRARY, CITYTOOL_GROUP_EDUCATION)] * (wTileCount[nTileID] / 4);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_MARINA) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(PARKS_MARINA, CITYTOOL_GROUP_PARKS)] * (wTileCount[nTileID] / 9);
			bValid = TRUE;
		}
		else if (nTileID == TILE_INFRASTRUCTURE_DESALINIZATIONPLANT) {
			nVal = costFromSubTool[CITY_MENUTOOL_POS(WATER_DESALINIZATION, CITYTOOL_GROUP_WATER)] * (wTileCount[nTileID] / 9);
			bValid = TRUE;
		}
		else if (GET_TILE_RANGE(nTileID, TILE_ARCOLOGY_PLYMOUTH, TILE_ARCOLOGY_LAUNCH)) {
			if (nTileID == TILE_ARCOLOGY_PLYMOUTH)
				nSubTool = REWARDS_ARCOLOGIES_PLYMOUTH;
			else if (nTileID == TILE_ARCOLOGY_FOREST)
				nSubTool = REWARDS_ARCOLOGIES_FOREST;
			else if (nTileID == TILE_ARCOLOGY_DARCO)
				nSubTool = REWARDS_ARCOLOGIES_DARCO;
			else
				nSubTool = REWARDS_ARCOLOGIES_LAUNCH;
			nVal = costFromSubTool[CITY_MENUTOOL_POS(nSubTool, CITYTOOL_GROUP_REWARDS)] * (wTileCount[nTileID] / 16);
			bValid = TRUE;
		}
		if (bValid)
			dwNewCityValue += nVal;
	}
	dwCityValue = dwNewCityValue;
}

// This occurs in two places:
// - InitializeCityData()
// - SimulationPollutionTerrainLandValueScan()

static void L_PrepareLandValueCalculations() {
	__int16 iX, iY, iXHalf, iYHalf, iXQuarter, iYQuarter;
	__int16 *pTempMapResCom, *pTempMapInd;
	__int16 nBaseResComValue, nBaseIndValue;
	BYTE iTileID, iTerrainTileID;

	wCityDevelopedTiles = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		iXHalf = iX / 2;
		iXQuarter = iXHalf / 2;
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			iYHalf = iY / 2;
			iYQuarter = iYHalf / 2;
			pTempMapResCom = GetTMap(iXQuarter, iYQuarter);
			pTempMapInd = GetTMap(iXQuarter + MINI_MAP_32, iYQuarter);
			nBaseResComValue = *pTempMapResCom;
			nBaseIndValue = *pTempMapInd;
			iTileID = GetTileID(iX, iY);
			if (iTileID) {
				if (iTileID == TILE_SERVICES_BIGPARK)
					nBaseResComValue += 40;
				else if (iTileID < TILE_TREES1 || iTileID > TILE_SMALLPARK) {
					if (iTileID <= TILE_RADIOACTIVITY)
						nBaseResComValue -= 20;
				}
				else
					nBaseResComValue += 20;
			}
			else if (XBITReturnIsWater(iX, iY)) {
				nBaseResComValue += 12;
				nBaseIndValue += 12;
			}
			else
				nBaseResComValue += 4;
			if (iTileID >= TILE_ROAD_LR || XZONReturnZone(iX, iY) != ZONE_NONE) {
				// Maximum bound here changed from GAME_SIZE_MAP to
				// MINI_MAP_64 due to it using the half-coordinate vars.
				// In this context even if the coordinate values may not
				// have a bearing on actual placement on the full-size map
				// it seems that the intent is a temporary corresponding
				// value that can be used in-conjunction with the temp map(s)
				// and any referenced mini-maps.
				if (iXHalf < MINI_MAP_64 && iYHalf < MINI_MAP_64)
					XBITSetBits(iXHalf, iYHalf, XBIT_MARK);
				++wCityDevelopedTiles;
			}
			if (XBITReturnIsWatered(iX, iY)) {
				nBaseResComValue += 4;
				nBaseIndValue += 4;
			}
			iTerrainTileID = GetTerrainTileID(iX, iY);
			if (iTerrainTileID) {
				if (iTerrainTileID < SUBMERGED_00)
					nBaseResComValue += 12;
			}
			*pTempMapResCom = nBaseResComValue;
			*pTempMapInd = nBaseIndValue;
		}
	}
}

void L_InitializeCityData() {
	__int16 iX, iY;
	BYTE iTileID;
	DWORD dwCurrBonds;
	WORD wCurrBond;

	wCommerceConnect = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			if (XTXTGetTextOverlayID(iX, iY) == NGHBR_CONNECTION_TEXT_ENTRY) {
				iTileID = GetTileID(iX, iY);
				if (GET_TILE_RANGE(iTileID, TILE_ROAD_LR, TILE_ROAD_LTBR) ||
					GET_TILE_RANGE(iTileID, TILE_TUNNEL_T, TILE_TUNNEL_L) ||
					GET_TILE_RANGE(iTileID, TILE_CROSSOVER_HIGHWAYLR_ROADTB, TILE_CROSSOVER_HIGHWAYTB_ROADLR) ||
					GET_TILE_RANGE(iTileID, TILE_ONRAMP_TL, TILE_ONRAMP_BR))
					++wCommerceConnect;
			}
		}
	}

	if (save_debug & SAVE_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SAVE: Loaded %d $1000 neighbor connections (Commerce Connect).\n", wCommerceConnect);

	// The 'wIndustryConnect' block was previously missing
	// and fixed via a separate detour, however it has now been
	// formalized in this re-constructed call.
	wIndustryConnect = 0;
	for (iX = 0; iX < GAME_MAP_SIZE; ++iX) {
		for (iY = 0; iY < GAME_MAP_SIZE; ++iY) {
			if (XTXTGetTextOverlayID(iX, iY) == NGHBR_CONNECTION_TEXT_ENTRY) {
				iTileID = GetTileID(iX, iY);
				if (GET_TILE_RANGE(iTileID, TILE_RAIL_LR, TILE_RAIL_HHLR) ||
					GET_TILE_RANGE(iTileID, TILE_CROSSOVER_ROADLR_RAILTB, TILE_CROSSOVER_HIGHWAYTB_POWERLR) ||
					GET_TILE_RANGE(iTileID, TILE_HIGHWAY_HTB, TILE_HIGHWAY_LTBR))
					++wIndustryConnect;
			}
		}
	}

	if (save_debug & SAVE_DEBUG_LOAD)
		ConsoleLog(LOG_DEBUG, "SAVE: Loaded %d $1500 neighbor connections (Industry Connect).\n", wIndustryConnect);

	dwBusPassengers = 0;
	dwRailPassengers = 0;
	dwSubwayPassengers = 0;

	Game_SimulationUpdatePowerConsumption();
	Game_SimulationUpdateWaterConsumption();

	L_PrepareLandValueCalculations();

	wDisasterWindy = 0;
	wDisasterFloodArea = 0;

	wCurrBond = 0;
	dwCurrBonds = dwCityBonds;
	dwInterestRateSum = 0;
	if (dwCityBonds > 0) {
		do {
			dwInterestRateSum += wArrBondData[wCurrBond];
			++wCurrBond;
			--dwCurrBonds;
		} while (dwCurrBonds);
	}
}

void InstallGraphsScanningStatsHandlingHooks_SC2K1996(void) {
	// Hook RecalculateCityValue
	SafeVirtualProtect((LPVOID)0x401F50, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401F50, Hook_RecalculateCityValue);
}
