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

void InstallGraphsScanningStatsHandlingHooks_SC2K1996(void) {
	// Hook RecalculateCityValue
	SafeVirtualProtect((LPVOID)0x401F50, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401F50, Hook_RecalculateCityValue);
}
