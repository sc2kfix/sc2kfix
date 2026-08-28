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

-- Returns whether a string starts with a specific substring or not
function string.starts_with(str, substr)
	return string.sub(str, 1, string.len(substr)) == substr
end


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

	budgets = enumize({
		"BUDGET_RESFUND",
		"BUDGET_COMFUND",
		"BUDGET_INDFUND",
		"BUDGET_ORDINANCE",
		"BUDGET_BOND",
		"BUDGET_POLICE",
		"BUDGET_FIRE",
		"BUDGET_HEALTH",
		"BUDGET_SCHOOL",
		"BUDGET_COLLEGE",
		"BUDGET_ROAD",
		"BUDGET_HIGHWAY",
		"BUDGET_BRIDGE",
		"BUDGET_RAIL",
		"BUDGET_SUBWAY",
		"BUDGET_TUNNEL",
		"BUDGET_COUNT"
	});

	ordinances = enumize({
		"ORDINANCE_SALES_TAX",
		"ORDINANCE_INCOME_TAX",
		"ORDINANCE_LEGALIZED_GAMBLING",
		"ORDINANCE_PARKING_FINES",
		"ORDINANCE_VOLUNTEER_FIRE_DEPARTMENT",
		"ORDINANCE_PUBLIC_SMOKING_BAN",
		"ORDINANCE_FREE_CLINICS",
		"ORDINANCE_JUNIOR_SPORTS",
		"ORDINANCE_PRO_READING_CAMPAIGN",
		"ORDINANCE_ANTI_DRUG_CAMPAIGN",
		"ORDINANCE_CPR_TRAINING",
		"ORDINANCE_NEIGHBORHOOD_WATCH",
		"ORDINANCE_TOURIST_ADVERTISING",
		"ORDINANCE_BUSINESS_ADVERTISING",
		"ORDINANCE_CITY_BEAUTIFICATION",
		"ORDINANCE_ANNUAL_CARNIVAL",
		"ORDINANCE_ENERGY_CONSERVATION",
		"ORDINANCE_NUCLEAR_FREE_ZONE",
		"ORDINANCE_HOMELESS_SHELTER",
		"ORDINANCE_POLLUTION_CONTROLS"
	});

	militarytiles = enumize({
		"MILITARYTILE_OTHER",
		"MILITARYTILE_RUNWAY",
		"MILITARYTILE_RUNWAYCROSS",
		"MILITARYTILE_MPARKINGLOT",
		"MILITARYTILE_CARGOYARD",
		"MILITARYTILE_MRADAR",
		"MILITARYTILE_MWAREHOUSE",
		"MILITARYTILE_BUILDING1",
		"MILITARYTILE_BUILDING2",
		"MILITARYTILE_TOPSECRET",
		"MILITARYTILE_CRANE",
		"MILITARYTILE_MCONTROLTOWER",
		"MILITARYTILE_F15B",
		"MILITARYTILE_MHANGAR1",
		"MILITARYTILE_HANGAR2",
		"MILITARYTILE_MISSILESILO"
	});

	layers = enumize({
		"LAYER_UNDERGROUND",
		"LAYER_ZONES",
		"LAYER_INFRANATURE",
		"LAYER_SIGNS",
		"LAYER_BUILDINGS",

		"LAYER_COUNT"
	});

	zonepop_aggregates = enumize({
		"ZONEPOPAGR_RESIDENTIAL",
		"ZONEPOPAGR_COMMERCIAL",
		"ZONEPOPAGR_INDUSTRIAL",

		"ZONEPOPAGR_COUNT"
	});

	zonepops = enumize({
		"ZONEPOP_ALL",
		"ZONEPOP_RESLIGHT",
		"ZONEPOP_RESDENSE",
		"ZONEPOP_COMLIGHT",
		"ZONEPOP_COMDENSE",
		"ZONEPOP_INDLIGHT",
		"ZONEPOP_INDDENSE",
		"ZONEPOP_ABANDONED",

		"ZONEPOP_COUNT"
	});

	graphs = enumize({
		"GRP_CITYSIZE",
		"GRP_RESPOP",
		"GRP_COMPOP",
		"GRP_INDPOP",
		"GRP_TRAFFIC",
		"GRP_POLLUTION",
		"GRP_CITYVALUE",
		"GRP_CITYCRIME",
		"GRP_POWERPERCENT",
		"GRP_WATERPERCENT",
		"GRP_HEALTH",
		"GRP_EDUCATION",
		"GRP_UNEMPLOYMENTRATE",
		"GRP_GNP",
		"GRP_NATIONALPOP",
		"GRP_FEDRATE",
		"GRP_GRPCATCOUNT"
	});

	citytool_groups = enumize({
		"CITYTOOL_GROUP_BULLDOZER",
		"CITYTOOL_GROUP_NATURE",					-- I don't have a better name for this one.
		"CITYTOOL_GROUP_DISPATCH",
		"CITYTOOL_GROUP_POWER",
		"CITYTOOL_GROUP_WATER",
		"CITYTOOL_GROUP_REWARDS",
		"CITYTOOL_GROUP_ROADS",
		"CITYTOOL_GROUP_RAIL",
		"CITYTOOL_GROUP_PORTS",
		"CITYTOOL_GROUP_RESIDENTIAL",
		"CITYTOOL_GROUP_COMMERCIAL",
		"CITYTOOL_GROUP_INDUSTRIAL",
		"CITYTOOL_GROUP_EDUCATION",
		"CITYTOOL_GROUP_SERVICES",
		"CITYTOOL_GROUP_PARKS",
		"CITYTOOL_GROUP_SIGNS",
		"CITYTOOL_GROUP_QUERY",
		"CITYTOOL_GROUP_CENTERINGTOOL",

		"CITYTOOL_GROUP_COUNT"
	});

	citytool_buttons = enumize({
		"CITYTOOL_BUTTON_BULLDOZER",
		"CITYTOOL_BUTTON_NATURE",
		"CITYTOOL_BUTTON_DISPATCH",
		"CITYTOOL_BUTTON_POWER",
		"CITYTOOL_BUTTON_WATER",
		"CITYTOOL_BUTTON_REWARDS",
		"CITYTOOL_BUTTON_ROAD",
		"CITYTOOL_BUTTON_RAIL",
		"CITYTOOL_BUTTON_PORTS",
		"CITYTOOL_BUTTON_RESIDENTIAL",
		"CITYTOOL_BUTTON_COMMERCIAL",
		"CITYTOOL_BUTTON_INDUSTRIAL",
		"CITYTOOL_BUTTON_EDUCATION",
		"CITYTOOL_BUTTON_SERVICES",
		"CITYTOOL_BUTTON_PARKS",
		"CITYTOOL_BUTTON_SIGNS",
		"CITYTOOL_BUTTON_QUERY",
		"CITYTOOL_BUTTON_ROTATEANTICLOCKWISE",
		"CITYTOOL_BUTTON_ROTATECLOCKWISE",
		"CITYTOOL_BUTTON_ZOOMOUT",
		"CITYTOOL_BUTTON_ZOOMIN",
		"CITYTOOL_BUTTON_CENTERINGTOOL",
		"CITYTOOL_BUTTON_CITYMAP",
		"CITYTOOL_BUTTON_CITYPOPULATION",
		"CITYTOOL_BUTTON_CITYNEIGHBOURS",
		"CITYTOOL_BUTTON_CITYGRAPHS",
		"CITYTOOL_BUTTON_CITYINDUSTRY",
		"CITYTOOL_BUTTON_BUDGET",
		"CITYTOOL_BUTTON_DISPLAYBUILDINGS",
		"CITYTOOL_BUTTON_DISPLAYSIGNS",
		"CITYTOOL_BUTTON_DISPLAYINFRA",
		"CITYTOOL_BUTTON_DISPLAYZONES",
		"CITYTOOL_BUTTON_DISPLAYUNDERGROUND",
		"CITYTOOL_BUTTON_HELP"
	});

	subtools_bulldozer = enumize({
		"BULLDOZER_DEMOLISH",
		"BULLDOZER_LEVEL",
		"BULLDOZER_RAISE",
		"BULLDOZER_LOWER",
		"BULLDOZER_DEZONE",

		"BULLDOZER_COUNT"
	});

	subtools_naure = enumize({
		"NATURE_TREES",
		"NATURE_WATER",

		"NATURE_COUNT"
	});

	subtools_dispatch = enumize({
		"DISPATCH_POLICE",
		"DISPATCH_FIRE",
		"DISPATCH_MILITARY",

		"DISPATCH_COUNT"
	});

	subtools_power = enumize({
		"POWER_WIRES",
		"POWER_PLANTS",
		"POWER_PLANTS_COAL",
		"POWER_PLANTS_HYDRO",
		"POWER_PLANTS_OIL",
		"POWER_PLANTS_GAS",
		"POWER_PLANTS_NUCLEAR",
		"POWER_PLANTS_WIND",
		"POWER_PLANTS_SOLAR",
		"POWER_PLANTS_MICROWAVE",
		"POWER_PLANTS_FUSION",

		"POWER_COUNT"
	});

	subtools_water = enumize({
		"WATER_PIPES",
		"WATER_PUMP",
		"WATER_TOWER",
		"WATER_TREATMENT",
		"WATER_DESALINIZATION",

		"WATER_COUNT"
	});

	subtools_rewards = enumize({
		"REWARDS_MAYORSHOUSE",
		"REWARDS_CITYHALL",
		"REWARDS_STATUE",
		"REWARDS_BRAUNLLAMADOME",
		"REWARDS_ARCOLOGIES",
		"REWARDS_ARCOLOGIES_PLYMOUTH",
		"REWARDS_ARCOLOGIES_FOREST",
		"REWARDS_ARCOLOGIES_DARCO",
		"REWARDS_ARCOLOGIES_LAUNCH",

		"REWARDS_COUNT"
	});

	subtools_roads = enumize({
		"ROADS_ROAD",
		"ROADS_HIGHWAY",
		"ROADS_TUNNEL",
		"ROADS_ONRAMP",
		"ROADS_BUSSTATION",

		"ROADS_COUNT"
	});

	subtools_rails = enumize({
		"RAILS_RAIL",
		"RAILS_SUBWAY",
		"RAILS_DEPOT",
		"RAILS_SUBSTATION",
		"RAILS_SUBTORAIL",

		"RAILS_COUNT"
	});

	subtools_ports = enumize({
		"PORTS_SEAPORT",
		"PORTS_AIRPORT",

		"PORTS_COUNT"
	});

	subtools_zones = enumize({
		"ZONES_LOW",
		"ZONES_HIGH",

		"ZONES_COUNT"
	});

	subtools_education = enumize({
		"EDUCATION_SCHOOL",
		"EDUCATION_COLLEGE",
		"EDUCATION_LIBRARY",
		"EDUCATION_MUSEUM",

		"EDUCATION_COUNT"
	});

	subtools_services = enumize({
		"SERVICES_POLICE",
		"SERVICES_FIRESTATION",
		"SERVICES_HOSPITAL",
		"SERVICES_PRISON",

		"SERVICES_COUNT"
	});

	subtools_parks = enumize({
		"PARKS_SMALLPARK",
		"PARKS_BIGPARK",
		"PARKS_ZOO",
		"PARKS_STADIUM",
		"PARKS_MARINA",

		"PARKS_COUNT"
	});

	maptool_groups = enumize({
		"MAPTOOL_GROUP_BULLDOZER",
		"MAPTOOL_GROUP_RAISETERRAIN",
		"MAPTOOL_GROUP_LOWERTERRAIN",
		"MAPTOOL_GROUP_STRETCHTERRAIN",
		"MAPTOOL_GROUP_LEVELTERRAIN",
		"MAPTOOL_GROUP_WATER",
		"MAPTOOL_GROUP_STREAM",
		"MAPTOOL_GROUP_TREES",
		"MAPTOOL_GROUP_FOREST",
		"MAPTOOL_GROUP_CENTERINGTOOL"
	});

	maptool_buttons = enumize({
		"MAPTOOL_BUTTON_RAISETERRAIN",
		"MAPTOOL_BUTTON_LOWERTERRAIN",
		"MAPTOOL_BUTTON_STRETCHTERRAIN",
		"MAPTOOL_BUTTON_LEVELTERRAIN",
		"MAPTOOL_BUTTON_INCREASEWATERLEVEL",
		"MAPTOOL_BUTTON_DECREASEWATERLEVEL",
		"MAPTOOL_BUTTON_WATER",
		"MAPTOOL_BUTTON_STREAM",
		"MAPTOOL_BUTTON_TREES",
		"MAPTOOL_BUTTON_FOREST",
		"MAPTOOL_BUTTON_CENTERINGTOOL",
		"MAPTOOL_BUTTON_ZOOMOUT",
		"MAPTOOL_BUTTON_ZOOMIN",
		"MAPTOOL_BUTTON_ROTATEANTICLOCKWISE",
		"MAPTOOL_BUTTON_ROTATECLOCKWISE",
		"MAPTOOL_BUTTON_HELP",
		"MAPTOOL_BUTTON_TERRAINHILLS",
		"MAPTOOL_BUTTON_TERRAINWATER",
		"MAPTOOL_BUTTON_TERRAINTREES",
		"MAPTOOL_BUTTON_TOGGLEOCEAN",
		"MAPTOOL_BUTTON_TOGGLERIVER",
		"MAPTOOL_BUTTON_MAKE",
		"MAPTOOL_BUTTON_DONE",
		"MAPTOOL_BUTTON_COUNT"
	});

	cursors = enumize({
		"GAMECURSOR_ARROW",
		"GAMECURSOR_WAIT",
		"GAMECURSOR_BULLDOZER",
		"GAMECURSOR_TREE",
		"GAMECURSOR_RESIDENTIAL",
		"GAMECURSOR_COMMERCIAL",
		"GAMECURSOR_INDUSTRIAL",
		"GAMECURSOR_EDUCATION",
		"GAMECURSOR_POWER",
		"GAMECURSOR_UNKNOWNONE",
		"GAMECURSOR_UNKNOWNTWO",
		"GAMECURSOR_CENTER",
		"GAMECURSOR_RAISETERRAIN",
		"GAMECURSOR_LOWERTERRAIN",
		"GAMECURSOR_STRETCHTERRAIN",
		"GAMECURSOR_LEVELTERRAIN",
		"GAMECURSOR_POND",
		"GAMECURSOR_STREAM",
		"GAMECURSOR_FOREST",
		"GAMECURSOR_WATER",
		"GAMECURSOR_SERVICES",
		"GAMECURSOR_ROAD",
		"GAMECURSOR_RAIL",
		"GAMECURSOR_QUERY",
		"GAMECURSOR_REWARDS",
		"GAMECURSOR_PORTS",
		"GAMECURSOR_PARKS",
		"GAMECURSOR_SIGNS",
		"GAMECURSOR_DISPATCH",
		"GAMECURSOR_HELP"
	});

	viewrotations = enumize({
		"VIEWROTATION_NORTH",
		"VIEWROTATION_EAST",
		"VIEWROTATION_SOUTH",
		"VIEWROTATION_WEST",

		"VIEWROTATION_COUNT"
	});

	gameovers = enumize({
		"GAMEOVER_SCENARIO_FAILURE",
		"GAMEOVER_SCENARIO_VICTORY",
		"GAMEOVER_BANKRUPT"
	});

	demands = enumize({
		"DEMAND_RES",
		"DEMAND_COM",
		"DEMAND_IND",

		"DEMAND_COUNT"
	});

	tripgenerator = enumize({
		"TRANSIT_TYPE_NONE",
		"TRANSIT_TYPE_ROAD",
		"TRANSIT_TYPE_HIGHWAY",
		"TRANSIT_TYPE_TUNNEL",
		"TRANSIT_TYPE_ROADBRIDGE",
		"TRANSIT_TYPE_PEDESTRIAN",
		"TRANSIT_TYPE_PEDESTRIAN_UNDERPASS",
		"TRANSIT_TYPE_PEDESTRIAN_TUNNEL",
		"TRANSIT_TYPE_PEDESTRIAN_UNDERPASS2",
		"TRANSIT_TYPE_BUS",
		"TRANSIT_TYPE_SUBWAY_EXIT",
		"TRANSIT_TYPE_RAIL_ENTER",
		"TRANSIT_TYPE_SUBWAY_ENTER",
		"TRANSIT_TYPE_RAIL",
		"TRANSIT_TYPE_SUBWAY"
	}, -1);

	newspaper_types = enumize({
		"NEWSPAPER_TYPE_WEATHER",
		"NEWSPAPER_TYPE_JUNK",
		"NEWSPAPER_TYPE_FOUNDING",
		"NEWSPAPER_TYPE_GROWTH",
		"NEWSPAPER_TYPE_INVENTION",
		"NEWSPAPER_TYPE_INNOVATION",
		"NEWSPAPER_TYPE_WAR",
		"NEWSPAPER_TYPE_MARKET",
		"NEWSPAPER_TYPE_SPORTS",
		"NEWSPAPER_TYPE_FED_RATE_UP",
		"NEWSPAPER_TYPE_FED_RATE_DOWN",
		"NEWSPAPER_TYPE_POLITICS",
		"NEWSPAPER_TYPE_DIPLOMACY",
		"NEWSPAPER_TYPE_DISASTER",
		"NEWSPAPER_TYPE_MEDICAL",
		"NEWSPAPER_TYPE_UPBEAT",
		"NEWSPAPER_TYPE_HIGH_CRIME",
		"NEWSPAPER_TYPE_HIGH_TRAFFIC",
		"NEWSPAPER_TYPE_HIGH_POLLUTION",
		"NEWSPAPER_TYPE_POOR_EDUCATION",
		"NEWSPAPER_TYPE_POOR_HEALTH",
		"NEWSPAPER_TYPE_POOR_EMPLOYMENT",
		"NEWSPAPER_TYPE_DISASTER_FIRE",
		"NEWSPAPER_TYPE_DISASTER_FLOOD",
		"NEWSPAPER_TYPE_DISASTER_PLANECRASH",
		"NEWSPAPER_TYPE_DISASTER_HELICOPTERCRASH",
		"NEWSPAPER_TYPE_DISASTER_TORNADO",
		"NEWSPAPER_TYPE_DISASTER_EARTHQUAKE",
		"NEWSPAPER_TYPE_DISASTER_MONSTER",
		"NEWSPAPER_TYPE_DISASTER_MELTDOWN",
		"NEWSPAPER_TYPE_DISASTER_MICROWAVE",
		"NEWSPAPER_TYPE_DISASTER_VOLCANO",
		"NEWSPAPER_TYPE_DISASTER_POLLUTION",
		"NEWSPAPER_TYPE_DISASTER_CHEMICALSPILL",
		"NEWSPAPER_TYPE_DISASTER_HURRICANE",
		"NEWSPAPER_TYPE_DISASTER_RIOT",
		"NEWSPAPER_TYPE_POWER_PLANT_EOL",
		"NEWSPAPER_TYPE_OVERCROWDED_PRISONS",
		"NEWSPAPER_TYPE_ANGRY_TEACHERS",
		"NEWSPAPER_TYPE_BRIDGE_COLLAPSE",
		"NEWSPAPER_TYPE_ANGRY_BULLDOZER",
		"NEWSPAPER_TYPE_ORDINANCE",
		"NEWSPAPER_TYPE_OPINION_FRED",
		"NEWSPAPER_TYPE_OPINION_EDITOR",
		"NEWSPAPER_TYPE_SURVEY",
		"NEWSPAPER_TYPE_MISSIM",
		"NEWSPAPER_TYPE_DEMAND_POWER",
		"NEWSPAPER_TYPE_DEMAND_TRANSIT",
		"NEWSPAPER_TYPE_DEMAND_POLICE",
		"NEWSPAPER_TYPE_DEMAND_FIRE",
		"NEWSPAPER_TYPE_DEMAND_WATER",
		"NEWSPAPER_TYPE_DEMAND_HOSPITAL",
		"NEWSPAPER_TYPE_DEMAND_SCHOOL",
		"NEWSPAPER_TYPE_DEMAND_SEAPORT",
		"NEWSPAPER_TYPE_DEMAND_AIRPORT",
		"NEWSPAPER_TYPE_DEMAND_ZOO",
		"NEWSPAPER_TYPE_DEMAND_STADIUM",
		"NEWSPAPER_TYPE_DEMAND_MARINA",
		"NEWSPAPER_TYPE_DEMAND_PARK",
		"NEWSPAPER_TYPE_DEMAND_INDUSTRIAL_CONNECTION",
		"NEWSPAPER_TYPE_DEMAND_COMMERCIAL_CONNECTION",
		"NEWSPAPER_TYPE_LOW_CRIME",
		"NEWSPAPER_TYPE_LOW_TRAFFIC",
		"NEWSPAPER_TYPE_LOW_POLLUTION",
		"NEWSPAPER_TYPE_GOOD_EDUCATION",
		"NEWSPAPER_TYPE_GOOD_HEALTH",
		"NEWSPAPER_TYPE_GOOD_EMPLOYMENT",
		"NEWSPAPER_TYPE_COUNT"
	});

	-- !!! HIC SUNT DRACONES !!!
	-- Generated from sc2k_1996.h using the following:
	--    sed -E "s/GAMEOFF.*\([_a-zA-Z0-8 *]+\,[\t ](\w+),[\t ]*(0x[0-9A-F]+).+/\1 = \2;/"
	-- Do NOT modify this manually unless you know what you are doing!
	-- Last updated: 2026-08-26 (@araxestroy)
	addr = {
		pCSimcityAppThis = 0x4C7010;
		wCurrentTileCoordinates = 0x4C7A98;
		dwSystemMetricCYHScroll = 0x4C7A9C;
		wTileCoordinateX = 0x4C7AB0;
		wTileCoordinateY = 0x4C7AB4;
		gameViewPt = 0x4C7AC0;
		dwSystemMetricCYVScroll = 0x4C7AC8;
		dwSystemMetricCXVScroll = 0x4C7ACC;
		dwSystemMetricCXHScroll = 0x4C7AD4;
		wGameScreenAreaX = 0x4C7AD8;
		wGameScreenAreaY = 0x4C7ADC;
		crDlgColBtnShadow = 0x4C7AF0;
		wDlgNumAvailablePlants = 0x4C7B38;
		crDlgColWndFrame = 0x4C7B40;
		crDlgColBtnFace = 0x4C7B48;
		wDlgAvailablePlants = 0x4C7B50;
		crDlgColBtnText = 0x4C7B64;
		crDlgColBtnHighlight = 0x4C7B70;
		dwCityToolBarArcologyDialogCancel = 0x4C7B98;
		wDlgNumAvailableBridges = 0x4C7C50;
		vBridgeBits = 0x4C7C54;
		wDlgAvailableBridges = 0x4C7C58;
		wQueryTileID = 0x4C7C68;
		wQuerySpriteID = 0x4C7C6C;
		pQuerySpriteBits = 0x4C7C70;
		MainFontsArl = 0x4C7C88;
		wViewInitialCoordX = 0x4C7CB0;
		wViewInitialCoordY = 0x4C7CB4;
		wViewInitialZoom = 0x4C7CB8;
		pActiveSimDoc = 0x4C7CBC;
		nRetState = 0x4C7CC0;
		nDataOffset = 0x4C7CC4;
		currWndClientRect = 0x4C7CD0;
		wCurrentAngle = 0x4C7CF8;
		theSCVDC = 0x4C7D00;
		rcDst = 0x4C7D08;
		wCurrentPositionAngle = 0x4C7D18;
		g_wColorSpriteStart = 0x4C7D1C;
		g_wColorMapYOffs = 0x4C7D20;
		g_wColorLandAltScale = 0x4C7D24;
		g_wColorScale = 0x4C7D28;
		g_wColorMapXOffs = 0x4C7D2C;
		g_iColorMapOffSetX = 0x4C7D30;
		g_iColorMapOffSetY = 0x4C7D34;
		dirtCount = 0x4C7D58;
		actionZone = 0x4C7D5C;
		traceDir = 0x4C7D60;
		tileCount = 0x4C7D84;
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
		pRawPopRatioTable = 0x4C94B4;
		pEQRatioTable = 0x4C94BC;
		bCityHasOcean = 0x4C94C0;
		dwArcologyPopulation = 0x4C94C4;
		cityToolGroupStrings = 0x4C94C8;
		dwDisasterActive = 0x4C9EE8;
		wArrBondData = 0x4C9EF0;
		cStrDataArchiveNames = 0x4CA160;
		strUnusedString = 0x4CA188;
		dwCityOldResPopulation = 0x4CA194;
		pszCityName = 0x4CA1A0;
		wNationalEconomyTrend = 0x4CA1BC;
		pNewsArr = 0x4CA1C0;
		wPrisonBonus = 0x4CA1DC;
		wCityTerrainSliderHills = 0x4CA1E0;
		wClipXhigh = 0x4CA1E4;
		wIndustrialMixBonus = 0x4CA1E8;
		wCurrentMapToolGroup = 0x4CA1EC;
		wIndustryConnect = 0x4CA3F0;
		pIndividualIndDemands = 0x4CA3F4;
		dwInterestRateSum = 0x4CA400;
		EditData = 0x4CA404;
		wSubwayXUNDCount = 0x4CA41C;
		wSetTriggerDisasterType = 0x4CA420;
		pZonePops = 0x4CA428;
		wCityMode = 0x4CA42C;
		wOldArrests = 0x4CA430;
		colGameBackgndAbove = 0x4CA43C;
		dwCityLandValue = 0x4CA440;
		dwCityFunds = 0x4CA444;
		wTileCount = 0x4CA4C8;
		dwCityValue = 0x4CA4D0;
		bOptionsAutoGoto = 0x4CA5D8;
		dwCityGarbage = 0x4CA5F0;
		wCityStartYear = 0x4CA5F4;
		dwCityUnemployment = 0x4CA5F8;
		dwNeighborValue = 0x4CA804;
		wNewspaperChoice = 0x4CA808;
		wWaterLevel = 0x4CA818;
		wDisasterObject = 0x4CA81C;
		wClipYhigh = 0x4CA820;
		dwNationalPopulation = 0x4CA928;
		dwNeighborFame = 0x4CA92C;
		pTaxPops = 0x4CA930;
		wMilitaryTiles = 0x4CA934;
		wNationalFedRate = 0x4CA938;
		wCurrentDisasterType = 0x4CA93C;
		dwCityOrdinances = 0x4CAA40;
		MainBrushFace = 0x4CAA48;
		dwPowerUsedPercentage = 0x4CAA50;
		disasterPoint = 0x4CAA58;
		pLERatioTable = 0x4CAA70;
		dwCityPopulation = 0x4CAA74;
		wCityTerrainSliderWater = 0x4CAAF8;
		fileExcept = 0x4CAB00;
		pSomeWnd = 0x4CAC18;
		dwNeighborPopulation = 0x4CAD10;
		bMainFrameInactive = 0x4CAD14;
		iScreenOffSetX = 0x4CAD18;
		iScreenOffSetY = 0x4CAD1C;
		colGameBackgndUnder = 0x4CAD20;
		pPaperArr = 0x4CAD24;
		dwCityFame = 0x4CAD28;
		bYearEndFlag = 0x4CAD2C;
		iScreenPointX = 0x4CAD30;
		iScreenPointY = 0x4CAD34;
		strCityFilename = 0x4CAD38;
		bInScenario = 0x4CAD44;
		szNeighborCities = 0x4CAD58;
		wCityTerrainSliderTrees = 0x4CADD8;
		wConnectTiles = 0x4CADDC;
		bWeatherHeat = 0x4CADE0;
		dirtyRect = 0x4CAD48;
		stNeighborCities = 0x4CAD58;
		wClipXlow = 0x4CAE00;
		dwCityDays = 0x4CAE04;
		bWeatherWind = 0x4CAE0C;
		wCityProgression = 0x4CB010;
		dwNationalValue = 0x4CB014;
		dwCityAdvertising = 0x4CB018;
		wCityCurrentMonth = 0x4CB01C;
		wCityElapsedYears = 0x4CB020;
		wClipYlow = 0x4CB024;
		MainBrushBorder = 0x4CB1B0;
		pArrSpriteHeaders = 0x4CB1B8;
		wUnknownGameVarOne = 0x4CB1CC;
		bNewspaperSubscription = 0x4CB3D0;
		bWeatherRain = 0x4CB3D4;
		wSewerBonus = 0x4CB3DC;
		pIndividualIndTaxRate = 0x4CB3E0;
		wCityCurrentSeason = 0x4CB3E8;
		pMicrosimArr = 0x4CB3EC;
		pIndividualIndRatio = 0x4CB3F0;
		bCityHasRiver = 0x4CB3F8;
		colBtnFace = 0x4CB3FC;
		LastCursorX = 0x4CB400;
		wCityDifficulty = 0x4CB404;
		LastCursorY = 0x4CB408;
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
		wCommerceConnect = 0x4CC4D8;
		wStadiumSportsTeams = 0x4CC4E0;
		bMilitaryBaseType = 0x4CC4E4;
		dwCityBonds = 0x4CC4E8;
		dwCityTrafficCount = 0x4CC6F4;
		wCityDemand = 0x4CC8F8;
		dwCityPollution = 0x4CC910;
		scenarioAttrib = 0x4CC918;
		wSelectedSubtool = 0x4CC950;
		pCustomTileNamesFromSpriteID = 0x4CCEC8;
		wGrantedArcologies = 0x4CDB2C;
		wCurBndsX1 = 0x4CDB68;
		wCurBndsX2 = 0x4CDB6C;
		wCurBndsY1 = 0x4CDB70;
		wCurBndsY2 = 0x4CDB74;
		dwLFSRState = 0x4CDB7C;
		dwLCGState = 0x4CDB80;
		szSoundPath = 0x4CDB88;
		shapeLeft = 0x4CDE30;
		shapeTop = 0x4CDE34;
		shapeRight = 0x4CDE38;
		shapeBottom = 0x4CDE3C;
		shapeY = 0x4CDE40;
		shapeX = 0x4CDE44;
		shapeBits = 0x4CDE48;
		shapeCurrent = 0x4CDE4C;
		reqCaption = 0x4CDE58;
		pTileNames = 0x4CDE68;
		reqText = 0x4CE640;
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
		aGraphics = 0x4E61D8;
		aMusic = 0x4E622C;
		cBackslash = 0x4E6278;
		bRedraw = 0x4E62B4;
		dwSimulationSubtickCounter = 0x4E63D8;
		iCheatEntry = 0x4E6520;
		iCheatExpectedCharPos = 0x4E6524;
		szNewItem = 0x4E66EC;
		pCSimcityDoc = 0x4E66F8;
		pStartEngineStr = 0x4E67D0;
		wPreviousTileCoordinateX = 0x4E6808;
		gameStrHyphen = 0x4E6804;
		wPreviousTileCoordinateY = 0x4E680C;
		pCSimcityView = 0x4E682C;
		dwCityProgressionRequirements = 0x4E6984;
		wPowerPlantSpriteIDs = 0x4E6C68;
		wPowerPlantMWs = 0x4E6C80;
		dwPowerPlantInfoBtnIDs = 0x4E6C98;
		dwPowerPlantBtnIDs = 0x4E6CC0;
		aMw = 0x4E6E94;
		colRCI = 0x4E6F28;
		dwNextRefocusSongID = 0x4E6F8C;
		bInfraTile = 0x4E6FA0;
		dwBridgeBaseCost = 0x4E6FB8;
		dwBridgeStringIDs = 0x4E6FD8;
		aGraphicsDir = 0x4E70D0;
		aScenarioDir = 0x4E70EC;
		aScenarios = 0x4E70FC;
		dwZoneNameStringIDs = 0x4E7140;
		wFontHeightsArl = 0x4E71C0;
		aTilesets = 0x4E7244;
		aData = 0x4E728C;
		dwShowSCURK = 0x4E72AC;
		dwMapEditingMode = 0x4E72F0;
		aCities = 0x4E730C;
		aSavegame = 0x4E7338;
		dwBaseSpriteLoading = 0x4E7448;
		wBuildingPopLevel = 0x4E7458;
		bTileState = 0x4E7508;
		wBuildingPopulation = 0x4E75B0;
		showColor = 0x4E7624;
		nXTERTileIDs = 0x4E7628;
		wXTERToXUNDSpriteIDMap = 0x4E76B8;
		trafficSpriteOffsets = 0x4E772B;
		trafficSpriteOverlayLevels = 0x4E7798;
		BuiltUpZones = 0x4E77B8;
		curLockedDIBBits = 0x4E77C8;
		traversableTerrain = 0x4E7B28;
		crSignText = 0x4E7FA8;
		dwPlacePoliceThingFail = 0x4E7FC4;
		dwPlaceFireThingFail = 0x4E7FC8;
		dwPlaceMilitaryThingFail = 0x4E7FCC;
		wThingSprites = 0x4E8080;
		wThingZoomVisibility = 0x4E80A8;
		nNessieFlip = 0x4E80DC;
		nShipDirectionPos = 0x4E8118;
		nShipDirectionFlip = 0x4E8120;
		nThingDirectionPosition = 0x4E8140;
		nThingDirectionFlip = 0x4E8148;
		nThingZoomXDivisor = 0x4E8158;
		nThingZoomYDivisor = 0x4E815C;
		wDisasterWindy = 0x4E86B0;
		bCSimcityDocSC2InUse = 0x4E9744;
		bCSimcityDocSCNInUse = 0x4E9748;
		dwUnknownInitVarOne = 0x4E974C;
		dwCityNoticeStringIDs = 0x4E98B8;
		crSignShine = 0x4E9924;
		crSignBase = 0x4E9930;
		crSignShade = 0x4E9934;
		wActivePlanes = 0x4E99C0;
		wActiveHelicopters = 0x4E99C4;
		wActiveShips = 0x4E99C8;
		wActiveMaxisMan = 0x4E99CC;
		wSailingBoats = 0x4E99D0;
		wMonsterSpawned = 0x4E99D4;
		wActiveTrains = 0x4E99D8;
		wActiveTornadoes = 0x4E99DC;
		iInventionBaseYears = 0x4E99E8;
		dwGrantedItems = 0x4E9A10;
		dwCityRewardsUnlocked = 0x4E9A24;
		DisplayLayer = 0x4E9E48;
		wGrantedPowerPlants = 0x4E9E88;
		hDC_Global = 0x4EA03C;
		g_hBitmapOld = 0x4EA040;
		hLoColor = 0x4EA044;
		bHiColor = 0x4EA048;
		bLoColor = 0x4EA04C;
		bPaletteSet = 0x4EA050;
		rgbLoColor = 0x4EA058;
		rgbNormalColor = 0x4EA0B8;
		dwArcologySpriteIDs = 0x4EA748;
		dwArcologyPopStrIDs = 0x4EA778;
		wCursorActive = 0x4EA7F0;
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
		wMovButtonsUp = 0x4EAA90;
		wMovButtonsDown = 0x4EAAA8;
		dwMovButtons = 0x4EAAC0;
	};
}

-- Extra bits to clean up after the initialization
sc2k.zones["ZONE_BOUNDARY"] = 15
sc2k.sounds["SOUND_START"] = 500


-----------------------------------------------
-- Helper table/metatable for game addresses --
-----------------------------------------------

game = {}

-- Read helper
function game:__index(key)
	if string.starts_with(key, "sz") or string.starts_with(key, "a") or string.starts_with(key, "str") then
		return string.format("%s",sc2k.read_cstring(sc2k.addr[key]))
	elseif string.starts_with(key, "f") then
		return sc2k.read_float(sc2k.addr[key])
	elseif string.starts_with(key, "b") then
		return sc2k.read_byte(sc2k.addr[key])
	elseif string.starts_with(key, "w") then
		return sc2k.read_word(sc2k.addr[key])
	else
		return sc2k.read_dword(sc2k.addr[key])
	end
end

-- Write helper (slightly smarter than the read helper)
function game:__newindex(key, value)
	if string.starts_with(key, "f") then
		return sc2k.write_float(sc2k.addr[key], value)
	elseif string.starts_with(key, "b") then
		return sc2k.write_byte(sc2k.addr[key], value)
	elseif string.starts_with(key, "w") then
		return sc2k.write_word(sc2k.addr[key], value)
	elseif string.starts_with(key, "dw") or string.starts_with(key, "i") or string.starts_with(key, "u") then
		return sc2k.write_dword(sc2k.addr[key], value)
	else
		error("game:__newindex called with ambiguously-named key; please use a specific write function instead")
	end
end

setmetatable(game, game)

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
			if t == sc2k.addr then
				value = string.format("%08X", t[k])
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
			if t == sc2k.addr then
				value = string.format("%08X", t[k])
			end
			io.write(string.format("%s%s = (%s) %s\n", string.rep(" ", indent * 4), k, type(t[k]), value))
			if type(t[k]) == "table" then
				table.udump(t[k], showint, indent + 1)
			end
		end
	end
end