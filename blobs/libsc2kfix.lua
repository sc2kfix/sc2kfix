-- sc2kfix blobs/libsc2kfix.lua: the Lua side of the Lua modding library
-- (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

-- If libsc2kfix.lua exists in the same directory as SC2K (NOT the mods/ directory!), sc2kfix will
-- load it instead of the copy built into the plugin. This is meant for development only; if you
-- want to ensure your own Lua code runs before any mods and the REPL, create a file called
-- autoexec.lua in your SC2K folder.


------------------------------------------------------------------
-- Important functions required to build the rest of the tables --
------------------------------------------------------------------

-- "enumizes" a list of strings in arg e starting at value i and returns a table with the
-- appropriate indexing. Optionally a table t can be passed to destructively insert the
-- results into an existing table.
function enumize(e, i, t)
	t = t or {}
	i = i or 0
	e = e or error("No enum list passed to enumize()")
	for k,v in ipairs(e) do
		--io.write(string.format("%i: k = %s, v = %s\n", i, tostring(k), tostring(v)))
		t[v] = i
		i = i + 1
	end
	return t
end

-- Joins two tables together nondestructively and returns the result
function table.join(t1, t2)
	local t = {}
	for k,v in pairs(t1) do
		t[k] = v
	end
	for k,v in pairs(t2) do
		t[k] = v
	end
	return t
end

-- Merges two tables together destructively (t1 = t1 + t2) and returns the result
function table.merge(t1, t2)
	for k,v in pairs(t2) do
		t1[k] = v
	end
	return t1
end

debug.setmetatable({}, { __concat = table.join })


-------------------------------------------------
-- Core tables and sub-initialization routines --
-------------------------------------------------

-- Core sc2kfix table
sc2kfix = {
}

-- Table full of sc2k1996 stuff
sc2k = {
	cmps = enumize({
		"CMP_LESSTHAN",
		"CMP_GREATERTHAN",
		"CMP_GREATEROREQUAL",
		"CMP_EQUAL",
		"CMP_LESSOREQUAL"
	});

	build_states = enumize({
		"BUILD_START",
		"BUILD_THINK",
		"BUILD_ABANDON"
	});

	zones = enumize({
		"ZONE_NONE",
		"ZONE_LIGHT_RESIDENTIAL",
		"ZONE_DENSE_RESIDENTIAL",
		"ZONE_LIGHT_COMMERCIAL",
		"ZONE_DENSE_COMMERCIAL",
		"ZONE_LIGHT_INDUSTRIAL",
		"ZONE_DENSE_INDUSTRIAL",
		"ZONE_MILITARY",
		"ZONE_AIRPORT",
		"ZONE_SEAPORT"
	});

	growth_states = enumize({
		"GROWTH_START",
		"GROWTH_CONSIDERCHANGE",
		"GROWTH_CHANGE",
		"GROWTH_CONSIDERCONSTRUCTION",
		"GROWTH_COMPLETECONSTRUCTION",
		"GROWTH_CONSIDERABANDON",
		"GROWTH_ABANDON",
		"GROWTH_CONSIDERCOMMIT",
		"GROWTH_COMMIT"
	});

	population_levels = enumize({
		"TILEPOPLEVEL_NONE",
		"TILEPOPLEVEL_LOW",
		"TILEPOPLEVEL_MEDIUM",
		"TILEPOPLEVEL_HIGH",
		"TILEPOPLEVEL_VERYHIGH"
	});

	disasters = enumize({
		"DISASTER_NONE",
		"DISASTER_FIRE",
		"DISASTER_FLOOD",
		"DISASTER_RIOT",
		"DISASTER_TOXICSPILL",
		"DISASTER_AIRCRASH_MAYBE",
		"DISASTER_EARTHQUAKE",
		"DISASTER_TORNADO",
		"DISASTER_MONSTER",
		"DISASTER_MELTDOWN",
		"DISASTER_MICROWAVE",
		"DISASTER_VOLCANO",
		"DISASTER_FIRESTORM",
		"DISASTER_MASSRIOTS",
		"DISASTER_MASSFLOODS",
		"DISASTER_POLLUTION",
		"DISASTER_HURRICANE",
		"DISASTER_HELICOPTERCRASH",
		"DISASTER_PLANECRASH"
	});

	military_bases = enumize({
		"MILITARY_BASE_NONE",
		"MILITARY_BASE_DECLINED",
		"MILITARY_BASE_ARMY",
		"MILITARY_BASE_AIR_FORCE",
		"MILITARY_BASE_NAVY",
		"MILITARY_BASE_MISSILE_SILOS"
	});

	tiles = enumize({
		"TILE_CLEAR",
		"TILE_RUBBLE1",
		"TILE_RUBBLE2",
		"TILE_RUBBLE3",
		"TILE_RUBBLE4",
		"TILE_RADIOACTIVITY",
		"TILE_TREES1",
		"TILE_TREES2",
		"TILE_TREES3",
		"TILE_TREES4",
		"TILE_TREES5",
		"TILE_TREES6",
		"TILE_TREES7",
		"TILE_SMALLPARK",

		"TILE_POWERLINES_LR",
		"TILE_POWERLINES_TB",
		"TILE_POWERLINES_HTB",
		"TILE_POWERLINES_LHR",
		"TILE_POWERLINES_THB",
		"TILE_POWERLINES_HLR",
		"TILE_POWERLINES_BR",
		"TILE_POWERLINES_BL",
		"TILE_POWERLINES_TL",
		"TILE_POWERLINES_TR",
		"TILE_POWERLINES_RTB",
		"TILE_POWERLINES_LBR",
		"TILE_POWERLINES_TLB",
		"TILE_POWERLINES_LTR",
		"TILE_POWERLINES_LTBR",

		"TILE_ROAD_LR",
		"TILE_ROAD_TB",
		"TILE_ROAD_HTB",
		"TILE_ROAD_LHR",
		"TILE_ROAD_THB",
		"TILE_ROAD_HLR",
		"TILE_ROAD_BR",
		"TILE_ROAD_BL",
		"TILE_ROAD_TL",
		"TILE_ROAD_TR",
		"TILE_ROAD_RTB",
		"TILE_ROAD_LBR",
		"TILE_ROAD_TLB",
		"TILE_ROAD_LTR",
		"TILE_ROAD_LTBR",

		"TILE_RAIL_LR",
		"TILE_RAIL_TB",
		"TILE_RAIL_HTB",
		"TILE_RAIL_LHR",
		"TILE_RAIL_THB",
		"TILE_RAIL_HLR",
		"TILE_RAIL_BR",
		"TILE_RAIL_BL",
		"TILE_RAIL_TL",
		"TILE_RAIL_TR",
		"TILE_RAIL_RTB",
		"TILE_RAIL_LBR",
		"TILE_RAIL_TLB",
		"TILE_RAIL_LTR",
		"TILE_RAIL_LTBR",
		"TILE_RAIL_HHTB",
		"TILE_RAIL_LHHR",
		"TILE_RAIL_THHB",
		"TILE_RAIL_HHLR",

		"TILE_TUNNEL_T",
		"TILE_TUNNEL_R",
		"TILE_TUNNEL_B",
		"TILE_TUNNEL_L",

		"TILE_CROSSOVER_POWERTB_ROADLR",
		"TILE_CROSSOVER_POWERLR_ROADTB",
		"TILE_CROSSOVER_ROADLR_RAILTB",
		"TILE_CROSSOVER_ROADTB_RAILLR",
		"TILE_CROSSOVER_POWERTB_RAILLR",
		"TILE_CROSSOVER_POWERLR_RAILTB",

		"TILE_HIGHWAY_LR",
		"TILE_HIGHWAY_TB",

		"TILE_CROSSOVER_HIGHWAYLR_ROADTB",
		"TILE_CROSSOVER_HIGHWAYTB_ROADLR",
		"TILE_CROSSOVER_HIGHWAYLR_RAILTB",
		"TILE_CROSSOVER_HIGHWAYTB_RAILLR",
		"TILE_CROSSOVER_HIGHWAYLR_POWERTB",
		"TILE_CROSSOVER_HIGHWAYTB_POWERLR",

		"TILE_SUSPENSION_BRIDGE_START_B",
		"TILE_SUSPENSION_BRIDGE_MIDDLE_B",
		"TILE_SUSPENSION_BRIDGE_CENTER_B",
		"TILE_SUSPENSION_BRIDGE_MIDDLE_T",
		"TILE_SUSPENSION_BRIDGE_END_T",
		"TILE_RAISING_BRIDGE_TOWER",
		"TILE_CAUSEWAY_PYLON",
		"TILE_RAISING_BRIDGE_LOWERED",
		"TILE_RAISING_BRIDGE_RAISED",
		"TILE_RAIL_BRIDGE_PYLON",
		"TILE_RAIL_BRIDGE",
		"TILE_ELEVATED_POWERLINES",

		"TILE_ONRAMP_TL",
		"TILE_ONRAMP_TR",
		"TILE_ONRAMP_BL",
		"TILE_ONRAMP_BR",

		"TILE_HIGHWAY_HTB",
		"TILE_HIGHWAY_LHR",
		"TILE_HIGHWAY_THB",
		"TILE_HIGHWAY_HLR",
		"TILE_HIGHWAY_BR",
		"TILE_HIGHWAY_BL",
		"TILE_HIGHWAY_TL",
		"TILE_HIGHWAY_TR",
		"TILE_HIGHWAY_LTBR",

		"TILE_REINFORCED_BRIDGE_PYLON",
		"TILE_REINFORCED_BRIDGE",

		"TILE_SUBTORAIL_T",
		"TILE_SUBTORAIL_R",
		"TILE_SUBTORAIL_B",
		"TILE_SUBTORAIL_L",
	
		"TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1",
		"TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES2",
		"TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES3",
		"TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES4",
		"TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES1",
		"TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES2",
		"TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES3",
		"TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES4",
		"TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES1",
		"TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES2",
		"TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES3",
		"TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES4",

		"TILE_COMMERCIAL_1X1_GASSTATION1",
		"TILE_COMMERCIAL_1X1_BEDANDBREAKFAST",
		"TILE_COMMERCIAL_1X1_CONVENIENCESTORE",
		"TILE_COMMERCIAL_1X1_GASSTATION2",
		"TILE_COMMERCIAL_1X1_SMALLOFFICEBUILDING1",
		"TILE_COMMERCIAL_1X1_SMALLOFFICEBUILDING2",
		"TILE_COMMERCIAL_1X1_WAREHOUSE",
		"TILE_COMMERCIAL_1X1_TOYSTORE",

		"TILE_INDUSTRIAL_1X1_SMALLWAREHOUSE1",
		"TILE_INDUSTRIAL_1X1_CHEMICALSTORAGE",
		"TILE_INDUSTRIAL_1X1_SMALLWAREHOUSE2",
		"TILE_INDUSTRIAL_1X1_SUBSTATION",

		"TILE_MISC_1X1_CONSTRUCTION1",
		"TILE_MISC_1X1_CONSTRUCTION2",
		"TILE_MISC_1X1_ABANDONED1",
		"TILE_MISC_1X1_ABANDONED2",

		"TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS1",
		"TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS2",
		"TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS3",
		"TILE_RESIDENTIAL_2X2_MEDIUMAPARTMENTS1",
		"TILE_RESIDENTIAL_2X2_MEDIUMAPARTMENTS2",
		"TILE_RESIDENTIAL_2X2_MEDIUMCONDOS1",
		"TILE_RESIDENTIAL_2X2_MEDIUMCONDOS2",
		"TILE_RESIDENTIAL_2X2_MEDIUMCONDOS3",

		"TILE_COMMERCIAL_2X2_SHOPPINGCENTER",
		"TILE_COMMERCIAL_2X2_GROCERYSTORE",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE1",
		"TILE_COMMERCIAL_2X2_RESORTHOTEL",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE2",
		"TILE_COMMERCIAL_2X2_OFFICERETAIL",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE3",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE4",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE5",
		"TILE_COMMERCIAL_2X2_MEDIUMOFFICE6",

		"TILE_INDUSTRIAL_2X2_MEDIUMWAREHOUSE",
		"TILE_INDUSTRIAL_2X2_CHEMICALPROCESSING",
		"TILE_INDUSTRIAL_2X2_FACTORY1",
		"TILE_INDUSTRIAL_2X2_FACTORY2",
		"TILE_INDUSTRIAL_2X2_FACTORY3",
		"TILE_INDUSTRIAL_2X2_FACTORY4",
		"TILE_INDUSTRIAL_2X2_FACTORY5",
		"TILE_INDUSTRIAL_2X2_FACTORY6",

		"TILE_MISC_2X2_CONSTRUCTION1",
		"TILE_MISC_2X2_CONSTRUCTION2",
		"TILE_MISC_2X2_CONSTRUCTION3",
		"TILE_MISC_2X2_CONSTRUCTION4",
		"TILE_MISC_2X2_ABANDONED1",
		"TILE_MISC_2X2_ABANDONED2",
		"TILE_MISC_2X2_ABANDONED3",
		"TILE_MISC_2X2_ABANDONED4",

		"TILE_RESIDENTIAL_3X3_LARGEAPARTMENTS1",
		"TILE_RESIDENTIAL_3X3_LARGEAPARTMENTS2",
		"TILE_RESIDENTIAL_3X3_LARGECONDOS1",
		"TILE_RESIDENTIAL_3X3_LARGECONDOS2",

		"TILE_COMMERCIAL_3X3_OFFICEPARK",
		"TILE_COMMERCIAL_3X3_OFFICETOWER1",
		"TILE_COMMERCIAL_3X3_MINIMALL",
		"TILE_COMMERCIAL_3X3_THEATERSQUARE",
		"TILE_COMMERCIAL_3X3_DRIVEINTHEATER",
		"TILE_COMMERCIAL_3X3_OFFICETOWER2",
		"TILE_COMMERCIAL_3X3_OFFICETOWER3",
		"TILE_COMMERCIAL_3X3_PARKINGLOT",
		"TILE_COMMERCIAL_3X3_HISTORICOFFICE",
		"TILE_COMMERCIAL_3X3_CORPORATEHQ",

		"TILE_INDUSTRIAL_3X3_CHEMICALPROCESSING",
		"TILE_INDUSTRIAL_3X3_LARGEFACTORY",
		"TILE_INDUSTRIAL_3X3_THINGAMAJIG",
		"TILE_INDUSTRIAL_3X3_MEDIUMFACTORY",
		"TILE_INDUSTRIAL_3X3_LARGEWAREHOUSE1",
		"TILE_INDUSTRIAL_3X3_LARGEWAREHOUSE2",

		"TILE_MISC_3X3_CONSTRUCTION1",
		"TILE_MISC_3X3_CONSTRUCTION2",
		"TILE_MISC_3X3_ABANDONED1",
		"TILE_MISC_3X3_ABANDONED2",

		"TILE_POWERPLANT_HYDRO1",
		"TILE_POWERPLANT_HYDRO2",
		"TILE_POWERPLANT_WIND",
		"TILE_POWERPLANT_GAS",
		"TILE_POWERPLANT_OIL",
		"TILE_POWERPLANT_NUCLEAR",
		"TILE_POWERPLANT_SOLAR",
		"TILE_POWERPLANT_MICROWAVE",
		"TILE_POWERPLANT_FUSION",
		"TILE_POWERPLANT_COAL",

		"TILE_SERVICES_CITYHALL",
		"TILE_SERVICES_HOSPITAL",
		"TILE_SERVICES_POLICE",
		"TILE_SERVICES_FIRE",
		"TILE_SERVICES_MUSEUM",
		"TILE_SERVICES_BIGPARK",
		"TILE_SERVICES_SCHOOL",
		"TILE_SERVICES_STADIUM",
		"TILE_SERVICES_PRISON",
		"TILE_SERVICES_COLLEGE",
		"TILE_SERVICES_ZOO",
		"TILE_SERVICES_STATUE",
	
		"TILE_INFRASTRUCTURE_WATERPUMP",
		"TILE_INFRASTRUCTURE_RUNWAY",
		"TILE_INFRASTRUCTURE_RUNWAYCROSS",
		"TILE_INFRASTRUCTURE_PIER",
		"TILE_INFRASTRUCTURE_CRANE",
		"TILE_INFRASTRUCTURE_CONTROLTOWER_CIV",
		"TILE_MILITARY_CONTROLTOWER",
		"TILE_MILITARY_WAREHOUSE",
		"TILE_INFRASTRUCTURE_BUILDING1",
		"TILE_INFRASTRUCTURE_BUILDING2",
		"TILE_MILITARY_TARMAC",
		"TILE_MILITARY_F15B",
		"TILE_MILITARY_HANGAR1",
		"TILE_INFRASTRUCTURE_SUBWAYSTATION",
		"TILE_MILITARY_RADAR",
		"TILE_INFRASTRUCTURE_WATERTOWER",
		"TILE_INFRASTRUCTURE_BUSDEPOT",
		"TILE_INFRASTRUCTURE_RAILSTATION",
		"TILE_INFRASTRUCTURE_PARKINGLOT",
		"TILE_MILITARY_PARKINGLOT",
		"TILE_INFRASTRUCTURE_LOADINGBAY",
		"TILE_MILITARY_TOPSECRET",
		"TILE_INFRASTRUCTURE_CARGOYARD",
		"TILE_INFRASTRUCTURE_MAYORSHOUSE",
		"TILE_INFRASTRUCTURE_WATERTREATMENT",
		"TILE_INFRASTRUCTURE_LIBRARY",
		"TILE_INFRASTRUCTURE_HANGAR2",
		"TILE_INFRASTRUCTURE_CHURCH",
		"TILE_INFRASTRUCTURE_MARINA",
		"TILE_MILITARY_MISSILESILO",
		"TILE_INFRASTRUCUTRE_DESALINIZATIONPLANT",

		"TILE_ARCOLOGY_PLYMOUTH",
		"TILE_ARCOLOGY_FOREST",
		"TILE_ARCOLOGY_DARCO",
		"TILE_ARCOLOGY_LAUNCH",
	
		"TILE_OTHER_BRAUNLLAMADOME"
	});

	xund = enumize({
		"UNDER_TILE_CLEAR",

		"UNDER_TILE_SUBWAY_LR",
		"UNDER_TILE_SUBWAY_TB",
		"UNDER_TILE_SUBWAY_HTB",
		"UNDER_TILE_SUBWAY_LHR",
		"UNDER_TILE_SUBWAY_THB",
		"UNDER_TILE_SUBWAY_HLR",
		"UNDER_TILE_SUBWAY_BR",
		"UNDER_TILE_SUBWAY_BL",
		"UNDER_TILE_SUBWAY_TL",
		"UNDER_TILE_SUBWAY_TR",
		"UNDER_TILE_SUBWAY_RTB",
		"UNDER_TILE_SUBWAY_LBR",
		"UNDER_TILE_SUBWAY_TLB",
		"UNDER_TILE_SUBWAY_LTR",
		"UNDER_TILE_SUBWAY_LTBR",

		"UNDER_TILE_PIPES_LR",
		"UNDER_TILE_PIPES_TB",
		"UNDER_TILE_PIPES_HTB",
		"UNDER_TILE_PIPES_LHR",
		"UNDER_TILE_PIPES_THB",
		"UNDER_TILE_PIPES_HLR",
		"UNDER_TILE_PIPES_BR",
		"UNDER_TILE_PIPES_BL",
		"UNDER_TILE_PIPES_TL",
		"UNDER_TILE_PIPES_TR",
		"UNDER_TILE_PIPES_RTB",
		"UNDER_TILE_PIPES_LBR",
		"UNDER_TILE_PIPES_TLB",
		"UNDER_TILE_PIPES_LTR",
		"UNDER_TILE_PIPES_LTBR",

		"UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR",
		"UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB",
		"UNDER_TILE_UNKNOWN",
		"UNDER_TILE_MISSILESILO",
		"UNDER_TILE_SUBWAYENTRANCE"
	});

	things = enumize({
		"XTHG_NONE",
		"XTHG_AIRPLANE",
		"XTHG_HELICOPTER",
		"XTHG_CARGO_SHIP",
		"XTHG_BULLDOZER",
		"XTHG_MONSTER",
		"XTHG_EXPLOSION",
		"XTHG_DEPLOY_POLICE",
		"XTHG_DEPLOY_FIRE",
		"XTHG_SAILBOAT",
		"XTHG_TRAIN_ENGINE",
		"XTHG_TRAIN_CAR",
		"XTHG_SUBWAY_TRAIN_ENGINE",					-- TRAIN_ENGINE switches to this state when it's in a subway tunnel
		"XTHG_SUBWAY_TRAIN_CAR",					-- Likewise for TRAIN_CAR
		"XTHG_DEPLOY_MILITARY",
		"XTHG_TORNADO",
		"XTHG_MAXIS_MAN",

		"XTHG_COUNT"
	});

	thing_directions = enumize({
		"XTHG_DIRECTION_NORTH",
		"XTHG_DIRECTION_NORTH_EAST",
		"XTHG_DIRECTION_EAST",
		"XTHG_DIRECTION_SOUTH_EAST",
		"XTHG_DIRECTION_SOUTH",
		"XTHG_DIRECTION_SOUTH_WEST",
		"XTHG_DIRECTION_WEST",
		"XTHG_DIRECTION_NORTH_WEST",

		"XTHG_DIRECTION_COUNT"
	});

	game_modes = enumize({
		"GAME_MODE_TERRAIN_EDIT",
		"GAME_MODE_CITY",
		"GAME_MODE_DISASTER"
	});

	difficulties = enumize({
		"GAME_DIFFICULTY_NONE",
		"GAME_DIFFICULTY_EASY",
		"GAME_DIFFICULTY_MEDIUM",
		"GAME_DIFFICULTY_HARD"
	});

	speeds = enumize({
		"GAME_SPEED_PAUSED",
		"GAME_SPEED_TURTLE",
		"GAME_SPEED_LLAMA",
		"GAME_SPEED_CHEETAH",
		"GAME_SPEED_AFRICAN_SWALLOW"
	}, 1);

	industry_types = enumize({
		"CITY_INDUSTRY_STEEL_OR_MINING",
		"CITY_INDUSTRY_TEXTILES",
		"CITY_INDUSTRY_PETROCHEMICAL",
		"CITY_INDUSTRY_FOOD",
		"CITY_INDUSTRY_CONSTRUCTION",
		"CITY_INDUSTRY_AUTOMOTIVE",
		"CITY_INDUSTRY_AEROSPACE",
		"CITY_INDUSTRY_FINANCE",
		"CITY_INDUSTRY_MEDIA",
		"CITY_INDUSTRY_ELECTRONICS",
		"CITY_INDUSTRY_TOURISM"
	});

	weather_trends = enumize({
		"WEATHER_TREND_COLD",
		"WEATHER_TREND_CLEAR",
		"WEATHER_TREND_HOT",
		"WEATHER_TREND_FOGGY",
		"WEATHER_TREND_CHILLY",
		"WEATHER_TREND_OVERCAST",
		"WEATHER_TREND_SNOW",
		"WEATHER_TREND_RAIN",
		"WEATHER_TREND_WINDY",
		"WEATHER_TREND_BLIZZARD",
		"WEATHER_TREND_HURRICANE",
		"WEATHER_TREND_TORNADO"
	});
	
	sounds = enumize({
		"SOUND_BUILD",
		"SOUND_ERROR",
		"SOUND_WIND",
		"SOUND_PLOP",
		"SOUND_EXPLODE",
		"SOUND_CLICK",
		"SOUND_POLICE",
		"SOUND_FIRE",
		"SOUND_BULLDOZER",
		"SOUND_FIRETRUCK",
		"SOUND_SIMCOPTER",
		"SOUND_FLOOD",
		"SOUND_BOOS",
		"SOUND_CHEERS",
		"SOUND_ZAP",
		"SOUND_MAYDAY",
		"SOUND_IMHIT",
		"SOUND_SHIP",
		"SOUND_TAKEOFF",
		"SOUND_LAND",
		"SOUND_SIREN",
		"SOUND_HORNS",
		"SOUND_PRISON",
		"SOUND_SCHOOL",
		"SOUND_TRAIN",
		"SOUND_MILITARY",
		"SOUND_ARCO",
		"SOUND_MONSTER",
		"SOUND_BULLDOZER2",					-- identical to SOUND_BULLDOZER
		"SOUND_RETICULATINGSPLINES",
		"SOUND_SILENT"
	}, 500);

	-- !!! HIC SUNT DRACONES !!!
	-- Generated from sc2k_1996.h using the following:
	--    sed -E "s/GAMEOFF.*\([_a-zA-Z0-8 *]+\,[\t ](\w+),[\t ]*(0x[0-9A-F]+).+/\1 = \2;/"
	-- Do NOT modify this manually unless you know what you are doing!
	-- Last updated: 2026-03-28 (@araxestroy)
	addr = {
		pCSimcityAppThis = 0x4C7010;
		wCurrentTileCoordinates = 0x4C7A98;
		wTileCoordinateX = 0x4C7AB0;
		wTileCoordinateY = 0x4C7AB4;
		gameViewPt = 0x4C7AC0;
		wGameScreenAreaX = 0x4C7AD8;
		wGameScreenAreaY = 0x4C7ADC;
		dwCityToolBarArcologyDialogCancel = 0x4C7B98;
		MainFontsArl = 0x4C7C88;
		wViewInitialCoordX = 0x4C7CB0;
		wViewInitialCoordY = 0x4C7CB4;
		wViewInitialZoom = 0x4C7CB8;
		currWndClientRect = 0x4C7CD0;
		wCurrentAngle = 0x4C7CF8;
		rcDst = 0x4C7D08;
		wCurrentPositionAngle = 0x4C7D18;
		g_wColorSpriteStart = 0x4C7D1C;
		g_wColorMapYOffs = 0x4C7D20;
		g_wColorLandAltScale = 0x4C7D24;
		g_wColorScale = 0x4C7D28;
		g_wColorMapXOffs = 0x4C7D2C;
		g_iColorMapOffSetX = 0x4C7D30;
		g_iColorMapOffSetY = 0x4C7D34;
		traceDir = 0x4C7D60;
		traceAction = 0x4C7D88;
		wMilitaryAvailDispatch = 0x4C7D98;
		wFireAvailDispatch = 0x4C7D9C;
		wPoliceAvailDispatch = 0x4C838C;
		wFireUnitsDispatched = 0x4C8390;
		dwBusPassengers = 0x4C85A0;
		dwRailPassengers = 0x4C85A4;
		wPoliceUnitsDispatched = 0x4C85A8;
		wMilitaryUnitsDispatched = 0x4C85F0;
		dwSubwayPassengers = 0x4C8600;
		pPalAnimMain = 0x4C87D8;
		pPalOnCycle = 0x4C8BD8;
		pPalOffCycle = 0x4C90A8;
		wDisasterFloodArea = 0x4C93A8;
		wCityDevelopedTiles = 0x4C93B4;
		wIndustrialMixPollutionBonus = 0x4C9428;
		wViewRotation = 0x4C942C;
		bCityHasOcean = 0x4C94C0;
		dwArcologyPopulation = 0x4C94C4;
		cityToolGroupStrings = 0x4C94C8;
		dwDisasterActive = 0x4C9EE8;
		wBondArr = 0x4C9EF0;
		cStrDataArchiveNames = 0x4CA160;
		dwCityResidentialPopulation = 0x4CA194;
		pszCityName = 0x4CA1A0;
		wNationalEconomyTrend = 0x4CA1BC;
		bArrNewspaperTable2 = 0x4CA1C0;
		wPrisonBonus = 0x4CA1DC;
		wCityTerrainSliderHills = 0x4CA1E0;
		wIndustrialMixBonus = 0x4CA1E8;
		wCurrentMapToolGroup = 0x4CA1EC;
		wCityNeighborConnections1500 = 0x4CA3F0;
		wArrIndustrialDemands = 0x4CA3F4;
		EditData = 0x4CA404;
		wSubwayXUNDCount = 0x4CA41C;
		wSetTriggerDisasterType = 0x4CA420;
		pZonePops = 0x4CA428;
		wCityMode = 0x4CA42C;
		wOldArrests = 0x4CA430;
		colGameBackgndAbove = 0x4CA43C;
		dwCityLandValue = 0x4CA440;
		dwCityFunds = 0x4CA444;
		dwTileCount = 0x4CA4C8;
		dwCityValue = 0x4CA4D0;
		bOptionsAutoGoto = 0x4CA5D8;
		dwCityGarbage = 0x4CA5F0;
		wCityStartYear = 0x4CA5F4;
		dwCityUnemployment = 0x4CA5F8;
		dwNeighborValue = 0x4CA804;
		wNewspaperChoice = 0x4CA808;
		wWaterLevel = 0x4CA818;
		wDisasterObject = 0x4CA81C;
		dwNationalPopulation = 0x4CA928;
		dwNeighborFame = 0x4CA92C;
		dwMilitaryTiles = 0x4CA934;
		wNationalTax = 0x4CA938;
		wCurrentDisasterID = 0x4CA93C;
		dwCityOrdinances = 0x4CAA40;
		MainBrushFace = 0x4CAA48;
		dwPowerUsedPercentage = 0x4CAA50;
		dwDisasterPoint = 0x4CAA58;
		dwCityPopulation = 0x4CAA74;
		wCityTerrainSliderWater = 0x4CAAF8;
		pSomeWnd = 0x4CAC18;
		dwNeighborPopulation = 0x4CAD10;
		bMainFrameInactive = 0x4CAD14;
		iScreenOffSetX = 0x4CAD18;
		iScreenOffSetY = 0x4CAD1C;
		colGameBackgndUnder = 0x4CAD20;
		bArrNewspaperTable1 = 0x4CAD24;
		dwCityFame = 0x4CAD28;
		bYearEndFlag = 0x4CAD2C;
		iScreenPointX = 0x4CAD30;
		iScreenPointY = 0x4CAD34;
		bInScenario = 0x4CAD44;
		szNeighborNameSouth = 0x4CAD58;
		szNeighborNameWest = 0x4CAD78;
		szNeighborNameNorth = 0x4CAD98;
		szNeighborNameEast = 0x4CADB8;
		wCityTerrainSliderTrees = 0x4CADD8;
		bWeatherHeat = 0x4CADE0;
		dirtyRect = 0x4CAD48;
		stNeighborCities = 0x4CAD58;
		dwCityDays = 0x4CAE04;
		bWeatherWind = 0x4CAE0C;
		wCityProgression = 0x4CB010;
		dwNationalValue = 0x4CB014;
		dwCityAdvertising = 0x4CB018;
		wCityCurrentMonth = 0x4CB01C;
		wCityElapsedYears = 0x4CB020;
		MainBrushBorder = 0x4CB1B0;
		pArrSpriteHeaders = 0x4CB1B8;
		bNewspaperSubscription = 0x4CB3D0;
		bWeatherHumidity = 0x4CB3D4;
		wSewerBonus = 0x4CB3DC;
		wArrIndustrialTaxRates = 0x4CB3E0;
		wCityCurrentSeason = 0x4CB3E8;
		pMicrosimArr = 0x4CB3EC;
		dwArrIndustrialPopulations = 0x4CB3F0;
		bCityHasRiver = 0x4CB3F8;
		colBtnFace = 0x4CB3FC;
		wCityDifficulty = 0x4CB404;
		bWeatherTrend = 0x4CB40C;
		dwCityWorkforceLE = 0x4CB410;
		wCityInventionYears = 0x4CB430;
		dwCityCrime = 0x4CB454;
		wCityCenterX = 0x4CB458;
		wCityCenterY = 0x4CB45C;
		dwCityWorkforcePercent = 0x4CB460;
		wCurrentCityToolGroup = 0x4CB464;
		bOptionsAutoBudget = 0x4CC4B0;
		dwCityWorkforceEQ = 0x4CC4B4;
		dwWaterUsedPercentage = 0x4CC4B8;
		bNewspaperExtra = 0x4CC4BC;
		pBudgetArr = 0x4CC4CC;
		bNoDisasters = 0x4CC4D4;
		wNeighborNameIdx = 0x4CC4DC;
		wCityNeighborConnections1000 = 0x4CC4D8;
		wSportsTeams = 0x4CC4E0;
		bMilitaryBaseType = 0x4CC4E4;
		dwCityBonds = 0x4CC4E8;
		dwCityTrafficUnknown = 0x4CC6F4;
		wCityDemand = 0x4CC8F8;
		dwCityPollution = 0x4CC910;
		wScenarioDisasterID = 0x4CC918;
		wScenarioTimeLimitMonths = 0x4CC91C;
		dwScenarioCitySize = 0x4CC91E;
		dwScenarioResPopulation = 0x4CC922;
		dwScenarioComPopulation = 0x4CC926;
		dwScenarioIndPopulation = 0x4CC92A;
		dwScenarioCashGoal = 0x4CC92E;
		dwScenarioLandValueGoal = 0x4CC932;
		wScenarioLEGoal = 0x4CC936;
		wScenarioEQGoal = 0x4CC938;
		dwScenarioPollutionLimit = 0x4CC93A;
		dwScenarioCrimeLimit = 0x4CC93E;
		dwScenarioTrafficLimit = 0x4CC942;
		bScenarioBuildingGoal1 = 0x4CC946;
		bScenarioBuildingGoal2 = 0x4CC947;
		wScenarioBuildingGoal1Count = 0x4CC948;
		wScenarioBuildingGoal2Count = 0x4CC94A;
		wSelectedSubtool = 0x4CC950;
		pCustomTileNamesFromSpriteID = 0x4CCEC8;
		wHighlightedTileX1 = 0x4CDB68;
		wHighlightedTileX2 = 0x4CDB6C;
		wHighlightedTileY1 = 0x4CDB70;
		wHighlightedTileY2 = 0x4CDB74;
		dwLFSRState = 0x4CDB7C;
		dwLCGState = 0x4CDB80;
		szSoundPath = 0x4CDB88;
		pTileNames = 0x4CDE68;
		hWndMovieCap = 0x4CE7E8;
		hWndMovie = 0x4CE7EC;
		game_AfxCoreState = 0x4CE8C0;
		hGameModule = 0x4CE8C8;
		areaFromSubTool = 0x4DC068;
		costFromSubTool = 0x4DC140;
		wPositionAngle = 0x4DC4C8;
		bCSAMainFrameDirectReleaseCapture = 0x4E6000;
		dwMovieClassRegistered = 0x4E6004;
		wIdleCount = 0x4E6008;
		dwDisasterStringIndex = 0x4E6010;
		bKeepPalette = 0x4E60B8;
		aTitlescrBmp = 0x4E6120;
		aPresentsBmp = 0x4E6130;
		aIntroBSmk = 0x4E6140;
		aIntroASmk = 0x4E6150;
		gameCurrDollar = 0x4E6168;
		gameCurrDM = 0x4E6180;
		gameLangGerman = 0x4E6198;
		gameCurrFF = 0x4E619C;
		gameLangFrench = 0x4E61B4;
		aPaths = 0x4E61D0;
		aMusic = 0x4E622C;
		cBackslash = 0x4E6278;
		bRedraw = 0x4E62B4;
		dwSimulationSubtickCounter = 0x4E63D8;
		iCheatEntry = 0x4E6520;
		iCheatExpectedCharPos = 0x4E6524;
		szNewItem = 0x4E66EC;
		pCSimcityDoc = 0x4E66F8;
		wPreviousTileCoordinateX = 0x4E6808;
		gameStrHyphen = 0x4E6804;
		wPreviousTileCoordinateY = 0x4E680C;
		pCSimcityView = 0x4E682C;
		dwCityProgressionRequirements = 0x4E6984;
		colRCI = 0x4E6F28;
		dwNextRefocusSongID = 0x4E6F8C;
		bInfraTile = 0x4E6FA0;
		dwZoneNameStringIDs = 0x4E7140;
		aData = 0x4E728C;
		dwBaseSpriteLoading = 0x4E7448;
		wBuildingPopLevel = 0x4E7458;
		bTileState = 0x4E7508;
		wBuildingPopulation = 0x4E75B0;
		nXTERTileIDs = 0x4E7628;
		wXTERToXUNDSpriteIDMap = 0x4E76B8;
		trafficSpriteOffsets = 0x4E772B;
		trafficSpriteOverlayLevels = 0x4E7798;
		BuiltUpZones = 0x4E77B8;
		dwPlacePoliceThingFail = 0x4E7FC4;
		dwPlaceFireThingFail = 0x4E7FC8;
		dwPlaceMilitaryThingFail = 0x4E7FCC;
		wDisasterWindy = 0x4E86B0;
		bCSimcityDocSC2InUse = 0x4E9744;
		bCSimcityDocSCNInUse = 0x4E9748;
		dwUnknownInitVarOne = 0x4E974C;
		dwCityNoticeStringIDs = 0x4E98B8;
		wActivePlanes = 0x4E99C0;
		wActiveHelicopters = 0x4E99C4;
		wActiveShips = 0x4E99C8;
		wActiveMaxisMan = 0x4E99CC;
		wSailingBoats = 0x4E99D0;
		wMonsterSpawned = 0x4E99D4;
		wActiveTrains = 0x4E99D8;
		wActiveTornadoes = 0x4E99DC;
		dwGrantedItems = 0x4E9A10;
		dwCityRewardsUnlocked = 0x4E9A24;
		DisplayLayer = 0x4E9E48;
		hDC_Global = 0x4EA03C;
		g_hBitmapOld = 0x4EA040;
		hLoColor = 0x4EA044;
		bHiColor = 0x4EA048;
		bLoColor = 0x4EA04C;
		bPaletteSet = 0x4EA050;
		rgbLoColor = 0x4EA058;
		rgbNormalColor = 0x4EA0B8;
		wTileHighlightActive = 0x4EA7F0;
		dwSoundBufferClear = 0x4EA848;
		nCurrentActionThingSoundID = 0x4EA854;
		nSoundPlayTicks = 0x4EA858;
		nActionThingSoundPlayTicks = 0x4EA8D4;
		nActionThingSoundPlayTicksCurrent = 0x4EA8D8;
		aDWav = 0x4EA8FC;
		aSounds = 0x4EA920;
		bTilesetLoadOutOfMemory = 0x4EAA24;
		smkBufOpenRet = 0x4EAA54;
		smkOpenRet = 0x4EAA58;
		MovieWndInitFinish = 0x4EAA5C;
		MovieWndExit = 0x4EAA60;
	};
}

-- Extra bits to clean up after the initialization
sc2k.zones["ZONE_BOUNDARY"] = 15
sc2k.sounds["SOUND_START"] = 500


---------------------------------
-- Member functions for tables --
---------------------------------

-- Calls ConsoleLog() with mod info prepended (like the LOG() macro for native code mods).
function sc2kfix.log(level, fmt, ...)
	msg = string.format(fmt, ...)
	sc2kfix.__ConsoleLog(level, msg)
end

-- Dump a Lua table with the keys sorted in alphabetical order. Hides entries starting with an
-- underscore unless showint is set to true.
function table.dump(t, showint, indent)
	local keys = {}
	indent = indent or 0
	
	for k in pairs(t) do table.insert(keys, k) end
	table.sort(keys)

	for _,k in ipairs(keys) do
		if k:sub(1,1) ~= "_" or showint ~= nil then 
			value = tostring(t[k])
			if type(t[k]) == "string" then
				value = '"' .. value .. '"'
			end
			io.write(string.format("%s%s = (%s) %s\n", string.rep(" ", indent * 4), k, type(t[k]), value))
			if type(t[k]) == "table" then
				table.dump(t[k], showint, indent + 1)
			end
		end
	end
end

-- As table.dump, but without sorting.
function table.udump(t, showint, indent)
	local keys = {}
	indent = indent or 0
	
	for k,_ in ipairs(t) do table.insert(keys, k) end

	for _,k in ipairs(keys) do
		if k:sub(1,1) ~= "_" or showint ~= nil then 
			value = tostring(t[k])
			if type(t[k]) == "string" then
				value = '"' .. value .. '"'
			end
			io.write(string.format("%s%s = (%s) %s\n", string.rep(" ", indent * 4), k, type(t[k]), value))
			if type(t[k]) == "table" then
				table.udump(t[k], showint, indent + 1)
			end
		end
	end
end