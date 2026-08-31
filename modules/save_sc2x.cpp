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

#define WRITE_BAILOUT(str) \
	{ \
		if (pZip) \
			ConsoleLog(LOG_ERROR, "SC2X: " __FUNCTION__ " called BAILOUT, reason: %s -- miniz error %d\n", str, pZip->m_last_error); \
		else \
			ConsoleLog(LOG_ERROR, "SC2X: " __FUNCTION__ " called BAILOUT, reason: %s\n", str); \
		goto ABORTWRITE;\
	}

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

static inline void Save_LoadNeighborName(int i) {
	__int16 nIdx = wNeighborNameIdx[i];
	if (nIdx)
		Game_LoadNamedEntryFromRsrcOffset(&szNeighborCities[MAX_NEIGH_BUF_SIZE * i], 1000, nIdx);
	else
		strcpy_s(&szNeighborCities[MAX_NEIGH_BUF_SIZE * i], MAX_NEIGH_BUF_SIZE, "Ocean");
}

static void Save_LoadMiscInfoFromJSON(CSimcityAppPrimary* pSCApp, json::JSON& jsonMISC) {
	CSimcityView* pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);

	wCityMode = jsonMISC["city"]["mode"].ToInt16();
	wViewRotation = jsonMISC["city"]["view_rotation"].ToInt16();
	wCityStartYear = jsonMISC["city"]["start_year"].ToInt16();
	dwCityDays = jsonMISC["city"]["days"].ToInt32();
	dwCityFunds = jsonMISC["city"]["funds"].ToInt32();
	dwCityBonds = jsonMISC["city"]["bonds"].ToInt32();
	wCityDifficulty = jsonMISC["city"]["difficulty"].ToInt16();
	wCityProgression = jsonMISC["city"]["progression"].ToInt16();
	dwCityValue = jsonMISC["city"]["value"].ToInt32();
	dwCityLandValue = jsonMISC["city"]["land_value"].ToInt32();
	dwCityCrime = jsonMISC["city"]["crime"].ToInt32();
	dwCityTrafficCount = jsonMISC["city"]["traffic_count"].ToInt32();
	dwCityPollution = jsonMISC["city"]["pollution"].ToInt32();
	dwCityFame = jsonMISC["city"]["fame"].ToInt32();
	dwCityAdvertising = jsonMISC["city"]["advertising"].ToInt32();
	dwCityGarbage = jsonMISC["city"]["garbage"].ToUint32();
	dwCityWorkforcePercent = jsonMISC["city"]["workforce_percent"].ToInt32();
	dwCityWorkforceLE = jsonMISC["city"]["workforce_le"].ToInt32();
	dwCityWorkforceEQ = jsonMISC["city"]["workforce_eq"].ToInt32();
	dwNationalPopulation = jsonMISC["nation"]["population"].ToInt32();
	dwNationalValue = jsonMISC["nation"]["value"].ToInt32();
	wNationalFedRate = jsonMISC["nation"]["fed_rate"].ToInt16();
	wNationalEconomyTrend = jsonMISC["nation"]["economy_trend"].ToInt16();
	bWeatherHeat = jsonMISC["city"]["weather_heat"].ToInt8();
	bWeatherWind = jsonMISC["city"]["weather_wind"].ToInt8();
	bWeatherRain = jsonMISC["city"]["weather_rain"].ToInt8();
	bWeatherTrend = jsonMISC["city"]["weather_trend"].ToInt8();
	wSetTriggerDisasterType = jsonMISC["city"]["disaster_type"].ToInt16();
	dwCityOldResPopulation = jsonMISC["city"]["old_res_pop"].ToInt32();

	// XXX (araxestroy): WTF?
	int16_t wGrantedRewards = jsonMISC["city"]["granted_rewards"].ToInt16();
	if (!dwGrantedItems[CITYTOOL_GROUP_REWARDS] || wGrantedRewards) {
		if (dwGrantedItems[CITYTOOL_GROUP_REWARDS] || !wGrantedRewards)
			Game_MainFrame_DisableCityToolBarButton((CMainFrame*)pSCApp->m_pMainWnd, CITYTOOL_BUTTON_REWARDS);
	}
	dwGrantedItems[CITYTOOL_GROUP_REWARDS] = wGrantedRewards;

	DecodeUint32Array((uint32_t*)pRawPopRatioTable, jsonMISC["city"]["pop_ratio_table"], 20);
	DecodeUint32Array((uint32_t*)pEQRatioTable, jsonMISC["city"]["eq_ratio_table"], 20);
	DecodeUint32Array((uint32_t*)pLERatioTable, jsonMISC["city"]["le_ratio_table"], 20);

	DecodeUint16Array((uint16_t*)wTileCount, jsonMISC["city"]["tile_count"], 256);
	DecodeUint32Array((uint32_t*)pZonePops, jsonMISC["city"]["zone_pops"], 8);
	DecodeUint16Array((uint16_t*)wArrBondData, jsonMISC["city"]["bond_data"], 50);

	wNeighborNameIdx[0] = jsonMISC["neighbors"]["north"]["name"].ToInt16();
	Save_LoadNeighborName(0);
	dwNeighborPopulation[0] = jsonMISC["neighbors"]["north"]["population"].ToInt32();
	dwNeighborValue[0] = jsonMISC["neighbors"]["north"]["value"].ToInt32();
	dwNeighborFame[0] = jsonMISC["neighbors"]["north"]["fame"].ToInt32();

	wNeighborNameIdx[1] = jsonMISC["neighbors"]["east"]["name"].ToInt16();
	Save_LoadNeighborName(1);
	dwNeighborPopulation[1] = jsonMISC["neighbors"]["east"]["population"].ToInt32();
	dwNeighborValue[1] = jsonMISC["neighbors"]["east"]["value"].ToInt32();
	dwNeighborFame[1] = jsonMISC["neighbors"]["east"]["fame"].ToInt32();

	wNeighborNameIdx[2] = jsonMISC["neighbors"]["south"]["name"].ToInt16();
	Save_LoadNeighborName(2);
	dwNeighborPopulation[2] = jsonMISC["neighbors"]["south"]["population"].ToInt32();
	dwNeighborValue[2] = jsonMISC["neighbors"]["south"]["value"].ToInt32();
	dwNeighborFame[2] = jsonMISC["neighbors"]["south"]["fame"].ToInt32();

	wNeighborNameIdx[3] = jsonMISC["neighbors"]["west"]["name"].ToInt16();
	Save_LoadNeighborName(3);
	dwNeighborPopulation[3] = jsonMISC["neighbors"]["west"]["population"].ToInt32();
	dwNeighborValue[3] = jsonMISC["neighbors"]["west"]["value"].ToInt32();
	dwNeighborFame[3] = jsonMISC["neighbors"]["west"]["fame"].ToInt32();

	DecodeInt16Array(wCityDemand, jsonMISC["city"]["demands"], 8);
	DecodeInt16Array(wCityInventionYears, jsonMISC["city"]["invention_years"], 17);

	for (int i = 0; i < 16; i++)
		DecodeBudgetArray(&pBudgetArr[i], jsonMISC["city"]["budget"][mapEnumBudgetTypeToJSONName[i]]);

	bYearEndFlag = jsonMISC["city"]["year_end_flag"].ToInt8();
	wWaterLevel = jsonMISC["city"]["water_level"].ToInt16();
	bCityHasOcean = jsonMISC["city"]["has_ocean"].ToInt8();
	bCityHasRiver = jsonMISC["city"]["has_river"].ToInt8();
	bMilitaryBaseType = jsonMISC["city"]["military_base_type"].ToInt8();

	DecodeByteArray((uint8_t*)pPaperArr, jsonMISC["city"]["newspaper_papers_array"], 30);
	DecodeByteArray((uint8_t*)pNewsArr, jsonMISC["city"]["newspaper_news_array"], 72);

	dwCityOrdinances = jsonMISC["city"]["ordinances"].ToUint32();
	dwCityUnemployment = jsonMISC["city"]["unemployment"].ToInt32();

	DecodeUint16Array((uint16_t*)wMilitaryTiles, jsonMISC["city"]["military_tile_count"], 16);

	wSubwayXUNDCount = jsonMISC["city"]["xund_count"].ToUint16();
	pSCApp->wSCAGameSpeedLOW = jsonMISC["options"]["speed"].ToInt16();
	bOptionsAutoBudget = jsonMISC["options"]["auto_budget"].ToBool();
	bOptionsAutoGoto = jsonMISC["options"]["auto_goto"].ToBool();
	pSCApp->dwSCAGameSound = jsonMISC["options"]["sound"].ToBool();
	pSCApp->dwSCAGameMusic = jsonMISC["options"]["music"].ToBool();
	if (!pSCApp->dwSCAGameMusic)
		Game_Sound_MusicStop(pSCApp->SCASNDLayer);

	bNoDisasters = jsonMISC["options"]["no_disasters"].ToBool();
	bNewspaperSubscription = jsonMISC["city"]["newspaper_subscription"].ToBool();
	bNewspaperExtra = jsonMISC["city"]["newspaper_extra"].ToBool();
	wNewspaperChoice = jsonMISC["city"]["newspaper_choice"].ToInt16();

	int nTilePos = jsonMISC["city"]["screen_point"].ToInt32();
	if (nTilePos == -1) {
		wViewInitialCoordX = 64;
		wViewInitialCoordY = 128;
	}
	else {
		wViewInitialCoordX = nTilePos & 0x7F;
		wViewInitialCoordY = nTilePos >> 8;
	}

	pSCView->wSCVZoomLevel = jsonMISC["city"]["screen_zoom"].ToUint16();
	wCityCenterX = jsonMISC["city"]["center_x"].ToInt16();
	wCityCenterY = jsonMISC["city"]["center_y"].ToInt16();
	dwArcologyPopulation = jsonMISC["city"]["arcology_population"].ToInt32();
	wConnectTiles = jsonMISC["city"]["connection_tiles"].ToInt16();
	wStadiumSportsTeams = jsonMISC["city"]["sports_teams"].ToInt16();
	dwCityPopulation = jsonMISC["city"]["population"].ToUint32();
	wIndustrialMixBonus = jsonMISC["city"]["industrial_mix_bonus"].ToInt16();
	wIndustrialMixPollutionBonus = jsonMISC["city"]["industrial_mix_pollution_bonus"].ToInt16();
	wOldArrests = jsonMISC["city"]["old_arrests"].ToInt16();
	wPrisonBonus = jsonMISC["city"]["prison_bonus"].ToInt16();
	wDisasterObject = jsonMISC["city"]["disaster_object"].ToInt16();
	wCurrentDisasterType = jsonMISC["city"]["disaster_type"].ToInt16();
	dwDisasterActive = jsonMISC["city"]["disaster_active"].ToBool();
	wSewerBonus = jsonMISC["city"]["sewer_bonus"].ToInt16();
}

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

bool Save_WriteTestSC2XFile(CSimcityAppPrimary* pSCApp, const char* szFilename) {
	mz_zip_archive* pZip = NULL;
	std::string strJSONDumpTemp;
	json::JSON jsonSaveMETA = {};
	json::JSON jsonSaveMISC = {};
	FILE* fOut;
	void* pArchiveBuf;
	size_t uArchiveSize;

	if (!szFilename)
		WRITE_BAILOUT("!szFilename");

	pZip = (mz_zip_archive*)malloc(sizeof(mz_zip_archive));
	if (!pZip)
		WRITE_BAILOUT("!pZip");

	memset(pZip, 0, sizeof(mz_zip_archive));

	if (!mz_zip_writer_init_heap(pZip, 0, 0))
		WRITE_BAILOUT("!mz_zip_writer_init_heap");

	Game_GetOccupiedTileCount();

	// Write the META "chunk" as a JSON file
	jsonSaveMETA["sc2x"]["creator"] = "sc2kfix " SC2KFIX_VERSION;
	jsonSaveMETA["sc2x"]["timestamp"] = time(NULL);
	jsonSaveMETA["sc2x"]["version"] = 1;
	jsonSaveMETA["game"]["dimensions"] = 128;
	jsonSaveMETA["game"]["city_name"] = pszCityName.m_pchData;
	strJSONDumpTemp = jsonSaveMETA.dump();
	if (!mz_zip_writer_add_mem(pZip, "META.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("ALTM");

	// Write the MISC chunk as a JSON file
	Save_CreateJSONFromMiscInfo(pSCApp, jsonSaveMISC);
	strJSONDumpTemp = jsonSaveMISC.dump();
	if (!mz_zip_writer_add_mem(pZip, "current/MISC.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("MISC");

	// Write the binary blob chunks
	if (!mz_zip_writer_add_mem(pZip, "current/ALTM", (const void*)dwMapALTM[0], ALTM_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("ALTM");
	if (!mz_zip_writer_add_mem(pZip, "current/XTER", (const void*)dwMapXTER[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XTER");
	if (!mz_zip_writer_add_mem(pZip, "current/XBLD", (const void*)dwMapXBLD[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XBLD");
	if (!mz_zip_writer_add_mem(pZip, "current/XZON", (const void*)dwMapXZON[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XZON");
	if (!mz_zip_writer_add_mem(pZip, "current/XUND", (const void*)dwMapXUND[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XUND");
	if (!mz_zip_writer_add_mem(pZip, "current/XTXT", (const void*)dwMapXTXT[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XTXT");
	if (!mz_zip_writer_add_mem(pZip, "current/XLAB", (const void*)dwMapXLAB[0], LABEL_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XLAB");
	if (!mz_zip_writer_add_mem(pZip, "current/XMIC", (const void*)pMicrosimArr, MICROSIM_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XMIC");
	if (!mz_zip_writer_add_mem(pZip, "current/XBIT", (const void*)dwMapXBIT[0], FULLMAP_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XBIT");
	if (!mz_zip_writer_add_mem(pZip, "current/XTRF", (const void*)dwMapXTRF[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XTRF");
	if (!mz_zip_writer_add_mem(pZip, "current/XPLT", (const void*)dwMapXPLT[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XPLT");
	if (!mz_zip_writer_add_mem(pZip, "current/XVAL", (const void*)dwMapXVAL[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XVAL");
	if (!mz_zip_writer_add_mem(pZip, "current/XCRM", (const void*)dwMapXCRM[0], MINIMAP64_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XCRM");
	if (!mz_zip_writer_add_mem(pZip, "current/XPLC", (const void*)dwMapXPLC[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XPLC");
	if (!mz_zip_writer_add_mem(pZip, "current/XFIR", (const void*)dwMapXFIR[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XFIR");
	if (!mz_zip_writer_add_mem(pZip, "current/XPOP", (const void*)dwMapXPOP[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XPOP");
	if (!mz_zip_writer_add_mem(pZip, "current/XROG", (const void*)dwMapXROG[0], MINIMAP32_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XROG");
	if (!mz_zip_writer_add_mem(pZip, "current/XGRP", (const void*)dwMapXGRP[0], GRAPH_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XGRP");

	// Write the XFIX chunk as a JSON file
	if (!bNoXFIX) {
		strJSONDumpTemp = jsonXFIX.dump();
		if (!mz_zip_writer_add_mem(pZip, "current/XFIX.json", strJSONDumpTemp.c_str(), strJSONDumpTemp.size() + 1, MZ_DEFAULT_COMPRESSION))
			WRITE_BAILOUT("XFIX");
	}

	// Finalize the archive in memory and write it out to the file
	if (!mz_zip_writer_finalize_heap_archive(pZip, &pArchiveBuf, &uArchiveSize))
		WRITE_BAILOUT("!mz_zip_writer_finalize_heap_archive");
	if (fopen_s(&fOut, szFilename, "wb"))
		WRITE_BAILOUT("!fopen_s");
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