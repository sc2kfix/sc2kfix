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

#define COPYBLOCKTO(D, S, P, SZ, MLT) memcpy(D[P], &S[P * (SZ * MLT)], SZ * MLT)

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
	bOptionsAutoBudget = jsonMISC["options"]["auto_budget"].ToInt32();
	bOptionsAutoGoto = jsonMISC["options"]["auto_goto"].ToInt32();
	pSCApp->dwSCAGameSound = jsonMISC["options"]["sound"].ToInt32();
	pSCApp->dwSCAGameMusic = jsonMISC["options"]["music"].ToInt32();
	if (!pSCApp->dwSCAGameMusic)
		Game_Sound_MusicStop(pSCApp->SCASNDLayer);

	bNoDisasters = jsonMISC["options"]["no_disasters"].ToInt32();
	bNewspaperSubscription = jsonMISC["city"]["newspaper_subscription"].ToInt32();
	bNewspaperExtra = jsonMISC["city"]["newspaper_extra"].ToInt32();
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

bool Save_SaveCitySC2X(CSimcityAppPrimary* pSCApp, FILE* fOut) {
	mz_zip_archive* pZip = NULL;
	std::string strJSONDumpTemp;
	json::JSON jsonSaveMETA = {};
	json::JSON jsonSaveMISC = {};
	void* pArchiveBuf;
	size_t uArchiveSize;

	if (!fOut)
		WRITE_BAILOUT("!fOut");

	if (fseek(fOut, 0, SEEK_SET))
		WRITE_BAILOUT("fseek");

	pZip = (mz_zip_archive*)malloc(sizeof(mz_zip_archive));
	if (!pZip)
		WRITE_BAILOUT("!pZip");

	memset(pZip, 0, sizeof(mz_zip_archive));

	if (!mz_zip_writer_init_heap(pZip, 0, 0))
		WRITE_BAILOUT("!mz_zip_writer_init_heap");

	Game_GetOccupiedTileCount();

	// Write the META "chunk" as a JSON file
	jsonSaveMETA["sc2x"]["magic"] = SC2X_MAGIC;
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
	if (!mz_zip_writer_add_mem(pZip, "current/XTHG", (const void*)dwMapXTHG[0], THING_ALLOC_SIZE, MZ_DEFAULT_COMPRESSION))
		WRITE_BAILOUT("XTHG");
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
	fwrite(pArchiveBuf, 1, uArchiveSize, fOut);
	mz_zip_writer_end(pZip);

	free(pZip);
	return true;

ABORTWRITE:
	if (pZip)
		free(pZip);
	return false;
}

// Magic happens here.
json::JSON jsonSaveMETA;
json::JSON jsonSaveMISC;

// Attempts to load an SC2X file into the engine. Returns false if nothing went wrong or true
// on an error.
bool Save_LoadCitySC2X(CSimcityAppPrimary* pSCApp, FILE* pFile, const char* lpFileName) {
	mz_zip_archive* pZip = NULL;
	bool ret = false;
	bool bGotLabel = false;
	size_t nFileSize = 0;
	uint8_t* pDump;
	std::string strJSONDumpTemp;
	char szTempCityName[255 + 1], szCityName[255 + 1];

	// Clear out the META/MISC JSON structures
	jsonSaveMETA = {};
	jsonSaveMISC = {};
	
	// Open a ZIP stream on pFile from the beginning of the file
	fseek(pFile, 0, 0);

	pZip = (mz_zip_archive*)malloc(sizeof(mz_zip_archive));
	if (!pZip)
		goto FAIL;

	memset(pZip, 0, sizeof(mz_zip_archive));

	if (mz_zip_reader_init_cfile(pZip, pFile, 0, 0)) {
		GameMain_String_Empty(&pszCityName);
		
		// META JSON chunk
		// XXX (araxestroy): This is just a quick test to make sure things are okay and should
		// probably check and report more.
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "META.json", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found META.json, size %d, addr: 0x%08X.\n", nFileSize, pDump);

			char* szLoadedDump = (char*)malloc(nFileSize + 1);
			if (!szLoadedDump) {
				ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": szLoadedDump malloc returned null.\n");
				ret = false;
				goto FAIL;
			}

			memset(szLoadedDump, 0, nFileSize + 1);
			memcpy(szLoadedDump, pDump, nFileSize);

			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": META.json dump:\n%s\n", szLoadedDump);

			jsonSaveMETA = json::Load(szLoadedDump);

			if (jsonSaveMETA["sc2x"]["magic"].ToString() != SC2X_MAGIC) {
				ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": magic number fail; expected \"%s\", got \"%s\".\n", SC2X_MAGIC, jsonSaveMETA["sc2x"]["magic"].ToString());
				ret = false;
				goto FAIL;
			}

			if (jsonSaveMETA["game"]["city_name"].ToString() != "")
				GameMain_String_OperatorSet(&pszCityName, (char*)jsonSaveMETA["game"]["city_name"].ToString().c_str());
			else
				Save_MakeCityNameFromFileName(lpFileName);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find META.json.\n");
			ret = false;
			goto FAIL;
		}
		ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": pMicrosimArr = 0x%08X\n", pMicrosimArr);
		
		// MISC.json -> MiscInfo
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/MISC.json", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/MISC.json, size %d, addr: 0x%08X.\n", nFileSize, pDump);

			char* szLoadedDump = (char*)malloc(nFileSize + 1);
			memset(szLoadedDump, 0, nFileSize + 1);
			memcpy(szLoadedDump, pDump, nFileSize);
			jsonSaveMISC = json::Load(szLoadedDump);

			Save_LoadMiscInfoFromJSON(pSCApp, jsonSaveMISC);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/MISC.json.\n");
			ret = false;
			goto FAIL;
		}

		// ALTM - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/ALTM", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/ALTM.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapALTM, pDump, nPos, sizeof(map_ALTM_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/ALTM.\n");
			ret = false;
			goto FAIL;
		}

		// XTER - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XTER", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XTER.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXTER, pDump, nPos, sizeof(map_XTER_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XTER.\n");
			ret = false;
			goto FAIL;
		}

		// XBLD - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XBLD", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XBLD.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXBLD, pDump, nPos, sizeof(map_XBLD_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XBLD.\n");
			ret = false;
			goto FAIL;
		}

		// XZON - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XZON", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XZON.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXZON, pDump, nPos, sizeof(map_XZON_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XZON.\n");
			ret = false;
			goto FAIL;
		}

		// XUND - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XUND", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XUND.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXUND, pDump, nPos, sizeof(map_XUND_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XUND.\n");
			ret = false;
			goto FAIL;
		}

		// XTXT - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XTXT", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XTXT.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXTXT, pDump, nPos, sizeof(map_XTXT_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XTXT.\n");
			ret = false;
			goto FAIL;
		}

		// XLAB
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XLAB", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XLAB.\n");

			memcpy(dwMapXLAB[0], pDump, LABEL_ALLOC_SIZE);
			bGotLabel = true;

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XLAB.\n");
			ret = false;
			goto FAIL;
		}
		ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": pMicrosimArr = 0x%08X\n", pMicrosimArr);

		// XMIC
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XMIC", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XMIC.\n");

			memcpy(&pMicrosimArr[0], pDump, MICROSIM_ALLOC_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XMIC.\n");
			ret = false;
			goto FAIL;
		}

		// XTHG - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XTHG", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XTHG.\n");

			for (int nPos = 0; nPos < MAX_THING_COUNT; ++nPos)
				COPYBLOCKTO(&dwMapXTHG[0], pDump, nPos, sizeof(map_XTHG_t), 1);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XTHG.\n");
			ret = false;
			goto FAIL;
		}

		// XBIT - fullmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XBIT", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XBIT.\n");

			for (int nPos = 0; nPos < GAME_MAP_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXBIT, pDump, nPos, sizeof(map_XBIT_t), GAME_MAP_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XBIT.\n");
			ret = false;
			goto FAIL;
		}

		// XTRF - halfmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XTRF", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XTRF.\n");

			for (int nPos = 0; nPos < MAP_MINI_HALF_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXTRF, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_HALF_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XTRF.\n");
			ret = false;
			goto FAIL;
		}

		// XPLT - halfmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XPLT", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XPLT.\n");

			for (int nPos = 0; nPos < MAP_MINI_HALF_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXPLT, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_HALF_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XPLT.\n");
			ret = false;
			goto FAIL;
		}

		// XVAL - halfmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XVAL", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XVAL.\n");

			for (int nPos = 0; nPos < MAP_MINI_HALF_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXVAL, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_HALF_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XVAL.\n");
			ret = false;
			goto FAIL;
		}

		// XCRM - halfmap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XCRM", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XCRM.\n");

			for (int nPos = 0; nPos < MAP_MINI_HALF_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXCRM, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_HALF_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XCRM.\n");
			ret = false;
			goto FAIL;
		}

		// XPLC - quartermap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XPLC", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XPLC.\n");

			for (int nPos = 0; nPos < MAP_MINI_QUARTER_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXPLC, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_QUARTER_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XPLC.\n");
			ret = false;
			goto FAIL;
		}

		// XFIR - quartermap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XFIR", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XFIR.\n");

			for (int nPos = 0; nPos < MAP_MINI_QUARTER_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXFIR, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_QUARTER_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XFIR.\n");
			ret = false;
			goto FAIL;
		}

		// XPOP - quartermap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XPOP", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XPOP.\n");

			for (int nPos = 0; nPos < MAP_MINI_QUARTER_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXPOP, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_QUARTER_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XPOP.\n");
			ret = false;
			goto FAIL;
		}

		// XROG - quartermap
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XROG", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XROG.\n");

			for (int nPos = 0; nPos < MAP_MINI_QUARTER_SIZE; ++nPos)
				COPYBLOCKTO(dwMapXROG, pDump, nPos, sizeof(map_mini_half_t), MAP_MINI_QUARTER_SIZE);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XROG.\n");
			ret = false;
			goto FAIL;
		}

		// XGRP
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XGRP", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XGRP.\n");

			for (int nPos = 0; nPos < MAX_GRAPHS; ++nPos)
				COPYBLOCKTO(dwMapXGRP, pDump, nPos, sizeof(DWORD), MAX_GRAPH_ENTRIES);

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XGRP.\n");
			ret = false;
			goto FAIL;
		}

		// XFIX.json -> jsonXFIX
		pDump = (uint8_t*)mz_zip_reader_extract_file_to_heap(pZip, "current/XFIX.json", &nFileSize, 0);
		if (pDump) {
			if (save_debug & SAVE_DEBUG_JSON_LOAD)
				ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Found current/XFIX.json.\n");

			char* szLoadedDump = (char*)malloc(nFileSize + 1);
			memset(szLoadedDump, 0, nFileSize + 1);
			memcpy(szLoadedDump, pDump, nFileSize);

			if (save_debug & SAVE_DEBUG_XFIX)
				ConsoleLog(LOG_DEBUG, "LOAD: Read XFIX chunk, contents:\n%s\n", szLoadedDump);
			jsonXFIX.merge(json::Load(szLoadedDump));
			UpdateXFIXSettings();

			free(pDump);
		} else {
			ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed to find current/XFIX.json.\n");
			ret = false;
			goto FAIL;
		}

		if (save_debug & SAVE_DEBUG_JSON_LOAD)
			ConsoleLog(LOG_DEBUG, "SAVE: " __FUNCTION__ ": Finalizing load.\n");

		// Finalize city load
		L_InitializeCityData();
		Game_GetOccupiedTileCount();
		Game_GraphKludge();
		if (bGotLabel)
			GameMain_ResetLabelStringState();
		else
			Game_ClearLabels();
		ret = true;
	} else
		ConsoleLog(LOG_ERROR, "SAVE: " __FUNCTION__ ": Failed at startup - miniz error %d.\n", pZip->m_last_error);
FAIL:
	return ret;
}