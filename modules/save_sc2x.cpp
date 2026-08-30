// sc2kfix modules/save_sc2x.cpp: implementation of the SC2X extended save format
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>

#include <sc2kfix.h>
#include "../thirdparty/miniz/miniz.h"

// NOTE: Debug variable is in save_vanilla.cpp

// NOTE: Keep this in sync with save_vanilla.cpp
#define MISCINF_ALLOC_SIZE   0x12C0 // Pending demystification
#define FULLMAP_ALLOC_SIZE   (GAME_MAP_SIZE * GAME_MAP_SIZE)
#define ALTM_ALLOC_SIZE      (FULLMAP_ALLOC_SIZE * 2)
#define MINIMAP64_ALLOC_SIZE (MAP_MINI_HALF_SIZE * MAP_MINI_HALF_SIZE)
#define MINIMAP32_ALLOC_SIZE (MAP_MINI_QUARTER_SIZE * MAP_MINI_QUARTER_SIZE)
#define LABEL_ALLOC_SIZE     (MAX_LABEL_COUNT * sizeof(map_XLAB_t))
#define MICROSIM_ALLOC_SIZE  (MAX_MICROSIM_COUNT * sizeof(microsim_t))
#define THING_ALLOC_SIZE     (MAX_THING_COUNT * sizeof(map_XTHG_t))
#define GRAPH_ALLOC_SIZE     (MAX_GRAPHS * MAX_GRAPH_ENTRIES * sizeof(DWORD))

static std::map<int, std::string> mapEnumBudgetTypeToJSONName = {
	{ BUDGET_RESFUND, "residential" },
	{ BUDGET_COMFUND, "commercial" },
	{ BUDGET_INDFUND, "industrial" },
	{ BUDGET_ORDINANCE, "ordinances" },
	{ BUDGET_BOND, "bonds" },
	{ BUDGET_POLICE, "police" },
	{ BUDGET_FIRE, "fire" },
	{ BUDGET_HEALTH, "health" },
	{ BUDGET_SCHOOL, "schools" },
	{ BUDGET_COLLEGE, "colleges" },
	{ BUDGET_ROAD, "raods" },
	{ BUDGET_HIGHWAY, "highways" },
	{ BUDGET_BRIDGE, "bridges" },
	{ BUDGET_RAIL, "rail" },
	{ BUDGET_SUBWAY, "subways" },
	{ BUDGET_TUNNEL, "tunnels" }
};

static void Save_CreateJSONFromMiscInfo(CSimcityAppPrimary* pSCApp, json::JSON& jsonMISC) {
	CSimcityView* pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);

	jsonMISC["city"]["mode"] = wCityMode;
	jsonMISC["city"]["view_rotation"] = wViewRotation;
	jsonMISC["city"]["start_year"] = wCityStartYear;
	jsonMISC["city"]["days"] = dwCityDays;
	jsonMISC["city"]["funds"] = dwCityFunds;
	jsonMISC["city"]["bonds"] = dwCityBonds;
	jsonMISC["city"]["difficulty"] = wCityDifficulty;
	jsonMISC["city"]["progression"] = wCityProgression;
	jsonMISC["city"]["value"] = dwCityValue;
	jsonMISC["city"]["land_value"] = dwCityLandValue;
	jsonMISC["city"]["crime"] = dwCityCrime;
	jsonMISC["city"]["traffic_count"] = dwCityTrafficCount;
	jsonMISC["city"]["pollution"] = dwCityPollution;
	jsonMISC["city"]["fame"] = dwCityFame;
	jsonMISC["city"]["advertising"] = dwCityAdvertising;
	jsonMISC["city"]["garbage"] = dwCityGarbage;
	jsonMISC["city"]["workforce_percent"] = dwCityWorkforcePercent;
	jsonMISC["city"]["workforce_le"] = dwCityWorkforceLE;
	jsonMISC["city"]["workforce_eq"] = dwCityWorkforceEQ;
	jsonMISC["nation"]["population"] = dwNationalPopulation;
	jsonMISC["nation"]["value"] = dwNationalValue;
	jsonMISC["nation"]["fed_rate"] = wNationalFedRate;
	jsonMISC["nation"]["economy_trend"] = wNationalEconomyTrend;
	jsonMISC["city"]["weather_heat"] = bWeatherHeat;
	jsonMISC["city"]["weather_wind"] = bWeatherWind;
	jsonMISC["city"]["weather_rain"] = bWeatherRain;
	jsonMISC["city"]["weather_trend"] = bWeatherTrend;
	jsonMISC["city"]["disaster_type"] = wSetTriggerDisasterType;
	jsonMISC["city"]["old_res_pop"] = dwMapXGRP[1][1];
	jsonMISC["city"]["granted_rewards"] = dwGrantedItems[CITYTOOL_GROUP_REWARDS];

	jsonMISC["city"]["pop_ratio_table"] = EncodeUint32Array((uint32_t*)pRawPopRatioTable, 20);
	jsonMISC["city"]["eq_ratio_table"] = EncodeUint32Array((uint32_t*)pEQRatioTable, 20);
	jsonMISC["city"]["le_ratio_table"] = EncodeUint32Array((uint32_t*)pLERatioTable, 20);

	jsonMISC["city"]["tile_count"] = EncodeUint16Array((uint16_t*)wTileCount, 256);
	jsonMISC["city"]["zone_pops"] = EncodeUint32Array((uint32_t*)pZonePops, 8);
	jsonMISC["city"]["bond_data"] = EncodeUint16Array((uint16_t*)wArrBondData, 50);

	jsonMISC["neighbors"]["north"]["name"] = wNeighborNameIdx[0];
	jsonMISC["neighbors"]["north"]["population"] = dwNeighborPopulation[0];
	jsonMISC["neighbors"]["north"]["value"] = dwNeighborValue[0];
	jsonMISC["neighbors"]["north"]["fame"] = dwNeighborFame[0];
	jsonMISC["neighbors"]["east"]["name"] = wNeighborNameIdx[1];
	jsonMISC["neighbors"]["east"]["population"] = dwNeighborPopulation[1];
	jsonMISC["neighbors"]["east"]["value"] = dwNeighborValue[1];
	jsonMISC["neighbors"]["east"]["fame"] = dwNeighborFame[1];
	jsonMISC["neighbors"]["south"]["name"] = wNeighborNameIdx[2];
	jsonMISC["neighbors"]["south"]["population"] = dwNeighborPopulation[2];
	jsonMISC["neighbors"]["south"]["value"] = dwNeighborValue[2];
	jsonMISC["neighbors"]["south"]["fame"] = dwNeighborFame[2];
	jsonMISC["neighbors"]["west"]["name"] = wNeighborNameIdx[3];
	jsonMISC["neighbors"]["west"]["population"] = dwNeighborPopulation[3];
	jsonMISC["neighbors"]["west"]["value"] = dwNeighborValue[3];
	jsonMISC["neighbors"]["west"]["fame"] = dwNeighborFame[3];

	jsonMISC["city"]["demands"] = EncodeInt16Array((int16_t*)wCityDemand, 8);
	jsonMISC["city"]["invention_years"] = EncodeInt16Array((int16_t*)wCityInventionYears, 17);

	for (int i = 0; i < 16; i++)
		jsonMISC["city"]["budget"][mapEnumBudgetTypeToJSONName[i]] = EncodeBudgetArray(&pBudgetArr[i]);

	jsonMISC["city"]["year_end_flag"] = bYearEndFlag;
	jsonMISC["city"]["water_level"] = wWaterLevel;
	jsonMISC["city"]["has_ocean"] = bCityHasOcean;
	jsonMISC["city"]["has_river"] = bCityHasRiver;
	jsonMISC["city"]["military_base_type"] = bMilitaryBaseType;

	jsonMISC["city"]["newspaper_papers_array"] = EncodeByteArray((uint8_t*)pPaperArr, 30);
	jsonMISC["city"]["newspaper_news_array"] = EncodeByteArray((uint8_t*)pNewsArr, 72);

	jsonMISC["city"]["ordinances"] = dwCityOrdinances;
	jsonMISC["city"]["unemployment"] = dwCityUnemployment;

	jsonMISC["city"]["military_tile_count"] = EncodeUint16Array((uint16_t*)wMilitaryTiles, 16);

	jsonMISC["city"]["xund_count"] = wSubwayXUNDCount;
	jsonMISC["options"]["speed"] = pSCApp->wSCAGameSpeedLOW;
	jsonMISC["options"]["auto_budget"] = bOptionsAutoBudget;
	jsonMISC["options"]["auto_goto"] = bOptionsAutoGoto;
	jsonMISC["options"]["sound"] = pSCApp->dwSCAGameSound;
	jsonMISC["options"]["music"] = pSCApp->dwSCAGameMusic;
	jsonMISC["options"]["no_disasters"] = bNoDisasters;
	jsonMISC["city"]["newspaper_subscription"] = bNewspaperSubscription;
	jsonMISC["city"]["newspaper_extra"] = bNewspaperExtra;
	jsonMISC["city"]["newspaper_choice"] = wNewspaperChoice;
	jsonMISC["city"]["screen_point"] = (pSCView) ? Game_PointToTile(iScreenPointX, iScreenPointY) : 128;	// XXX (araxestroy): better name?
	jsonMISC["city"]["screen_zoom"] = (pSCView) ? pSCView->wSCVZoomLevel : ZOOM_LEVEL_SMALL;
	jsonMISC["city"]["center_x"] = wCityCenterX;
	jsonMISC["city"]["center_y"] = wCityCenterY;
	jsonMISC["city"]["arcology_population"] = dwArcologyPopulation;
	jsonMISC["city"]["connection_tiles"] = wConnectTiles;
	jsonMISC["city"]["sports_teams"] = wStadiumSportsTeams;
	jsonMISC["city"]["population"] = dwCityPopulation;
	jsonMISC["city"]["industrial_mix_bonus"] = wIndustrialMixBonus;
	jsonMISC["city"]["industrial_mix_pollution_bonus"] = wIndustrialMixPollutionBonus;
	jsonMISC["city"]["old_arrests"] = wOldArrests;
	jsonMISC["city"]["prison_bonus"] = wPrisonBonus;
	jsonMISC["city"]["disaster_object"] = wDisasterObject;
	jsonMISC["city"]["disaster_type"] = wCurrentDisasterType;
	jsonMISC["city"]["disaster_active"] = dwDisasterActive;
	jsonMISC["city"]["sewer_bonus"] = wSewerBonus;
}

#define BAILOUT(str) \
	{ \
		ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ " called BAILOUT, reason: " str " -- miniz error %d\n", pZip->m_last_error);\
		goto ABORTWRITE;\
	}

#define BAILOUTNOMZ(str) \
	{ \
		ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ " called BAILOUT, reason: " str "\n");\
		goto ABORTWRITE;\
	}

bool Save_WriteTestSC2XFile(CSimcityAppPrimary* pSCApp, const char* szFilename) {
	mz_zip_archive* pZip = NULL;
	std::string strJSONDumpTemp;
	json::JSON jsonSaveMETA = {};
	json::JSON jsonSaveMISC = {};
	FILE* fOut;
	void* pArchiveBuf;
	size_t uArchiveSize;

	if (!szFilename)
		BAILOUTNOMZ("!szFilename");

	pZip = (mz_zip_archive*)malloc(sizeof(mz_zip_archive));
	if (!pZip)
		BAILOUTNOMZ("!pZip");

	memset(pZip, 0, sizeof(mz_zip_archive));

	if (!mz_zip_writer_init_heap(pZip, 0, 0))
		BAILOUT("!mz_zip_writer_init_heap");

	Game_GetOccupiedTileCount();

	// Write the META "chunk" as a JSON file
	jsonSaveMETA["sc2x"]["creator"] = "sc2kfix " SC2KFIX_VERSION;
	jsonSaveMETA["sc2x"]["timestamp"] = time(NULL);
	jsonSaveMETA["sc2x"]["version"] = 1;
	jsonSaveMETA["game"]["dimensions"] = 128;
	jsonSaveMETA["game"]["city_name"] = pszCityName.m_pchData;
	strJSONDumpTemp = jsonSaveMETA.dump();
	if (!mz_zip_writer_add_mem(pZip, "META.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
		BAILOUT("ALTM");

	// Write the MISC chunk as a JSON file
	Save_CreateJSONFromMiscInfo(pSCApp, jsonSaveMISC);
	strJSONDumpTemp = jsonSaveMISC.dump();
	if (!mz_zip_writer_add_mem(pZip, "MISC.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
		BAILOUT("MISC");

	// Write the binary blob chunks
	if (!mz_zip_writer_add_mem(pZip, "current/ALTM", (const void*)dwMapALTM[0], ALTM_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("ALTM");
	if (!mz_zip_writer_add_mem(pZip, "current/XTER", (const void*)dwMapXTER[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XTER");
	if (!mz_zip_writer_add_mem(pZip, "current/XBLD", (const void*)dwMapXBLD[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XBLD");
	if (!mz_zip_writer_add_mem(pZip, "current/XZON", (const void*)dwMapXZON[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XZON");
	if (!mz_zip_writer_add_mem(pZip, "current/XUND", (const void*)dwMapXUND[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XUND");
	if (!mz_zip_writer_add_mem(pZip, "current/XTXT", (const void*)dwMapXTXT[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XTXT");
	if (!mz_zip_writer_add_mem(pZip, "current/XLAB", (const void*)dwMapXLAB[0], LABEL_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XLAB");
	if (!mz_zip_writer_add_mem(pZip, "current/XMIC", (const void*)pMicrosimArr, MICROSIM_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XMIC");
	if (!mz_zip_writer_add_mem(pZip, "current/XBIT", (const void*)dwMapXBIT[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XBIT");
	if (!mz_zip_writer_add_mem(pZip, "current/XTRF", (const void*)dwMapXTRF[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XTRF");
	if (!mz_zip_writer_add_mem(pZip, "current/XPLT", (const void*)dwMapXPLT[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XPLT");
	if (!mz_zip_writer_add_mem(pZip, "current/XVAL", (const void*)dwMapXVAL[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XVAL");
	if (!mz_zip_writer_add_mem(pZip, "current/XCRM", (const void*)dwMapXCRM[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XCRM");
	if (!mz_zip_writer_add_mem(pZip, "current/XPLC", (const void*)dwMapXPLC[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XPLC");
	if (!mz_zip_writer_add_mem(pZip, "current/XFIR", (const void*)dwMapXFIR[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XFIR");
	if (!mz_zip_writer_add_mem(pZip, "current/XPOP", (const void*)dwMapXPOP[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XPOP");
	if (!mz_zip_writer_add_mem(pZip, "current/XROG", (const void*)dwMapXROG[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XROG");
	if (!mz_zip_writer_add_mem(pZip, "current/XGRP", (const void*)dwMapXGRP[0], GRAPH_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		BAILOUT("XGRP");

	// Write the XFIX chunk as a JSON file
	if (!bNoXFIX) {
		strJSONDumpTemp = jsonXFIX.dump();
		if (!mz_zip_writer_add_mem(pZip, "current/XFIX.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
			BAILOUT("XFIX");
	}

	// Finalize the archive in memory and write it out to the file
	if (!mz_zip_writer_finalize_heap_archive(pZip, &pArchiveBuf, &uArchiveSize))
		BAILOUT("!mz_zip_writer_finalize_heap_archive");
	if (fopen_s(&fOut, szFilename, "wb"))
		BAILOUT("!fopen_s");
	fwrite(pArchiveBuf, 1, uArchiveSize, fOut);
	fclose(fOut);
	mz_zip_writer_end(pZip);

	free(pZip);
	return true;

ABORTWRITE:
	if (pZip)
		free(pZip);
	return false;
}