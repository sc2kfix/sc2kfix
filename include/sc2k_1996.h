// sc2kfix include/sc2k_1996.h: defines specific to the 1996 Special Edition version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is basically a live interface to a bunch of the reverse engineering I've done. Some of it
// may have adverse effects if you start fudging the numbers using these offsets. Actually a lot
// of it will. It's best to use it just to check things.
//
// In particular I don't know if I've written rvalues right to be able to use structs in GAMEOFF
// macros. We'll find out eventually.
//
// I'm usually a C guy.

// Do you want documentation? Because we've got documentation!
// Check it out here: https://wiki.sc2kfix.net/Known_global_variables

#pragma once

#ifdef GAMEOFF_IMPL
#define GAMEOFF(type, name, address) \
	type* __ptr__##name = (type*)address; \
	type& name = *__ptr__##name;
#define GAMEOFF_ARR(type, name, address) type* name = (type*)address;
#else
#define GAMEOFF(type, name, address) extern type& name;
#define GAMEOFF_ARR(type, name, address) extern type* name;
#endif

#define GAMEOFF_PTR GAMEOFF_ARR

#ifdef GAMEOFF_IMPL
#define GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name)(__VA_ARGS__); \
	GameFuncPtr_##name Game_##name = (GameFuncPtr_##name)address;
#else
#define GAMECALL(address, type, conv, name, ...) \
	typedef type (conv *GameFuncPtr_##name)(__VA_ARGS__);\
	extern GameFuncPtr_##name Game_##name;
#endif

#define GAMEJMP(address) { __asm push address __asm retn }


#define BITMASK(x) (1 << x)

#define MAX_CITY_INVENTION_YEARS 17

#define CORNER_NONE   0x0
#define CORNER_BLEFT  0x1
#define CORNER_BRIGHT 0x2
#define CORNER_TLEFT  0x4
#define CORNER_TRIGHT 0x8
#define CORNER_ALL    (CORNER_BLEFT|CORNER_BRIGHT|CORNER_TLEFT|CORNER_TRIGHT)

#define GAME_MAP_SIZE 128

class CMFC3XString;

// Enums

// Disaster IDs
enum {
	DISASTER_NONE = 0,
	DISASTER_FIRE,
	DISASTER_FLOOD,
	DISASTER_RIOT,
	DISASTER_TOXICSPILL,
	DISASTER_AIRCRASH_MAYBE,
	DISASTER_EARTHQUAKE,
	DISASTER_TORNADO,
	DISASTER_MONSTER,
	DISASTER_MELTDOWN,
	DISASTER_MICROWAVE,
	DISASTER_VOLCANO,
	DISASTER_FIRESTORM,
	DISASTER_MASSRIOTS,
	DISASTER_MASSFLOODS,
	DISASTER_POLLUTION,
	DISASTER_HURRICANE,
	DISASTER_HELICOPTERCRASH,
	DISASTER_PLANECRASH
};

// Zone types
enum {
	ZONE_NONE = 0,
	ZONE_LIGHT_RESIDENTIAL,
	ZONE_DENSE_RESIDENTIAL,
	ZONE_LIGHT_COMMERCIAL,
	ZONE_DENSE_COMMERCIAL,
	ZONE_LIGHT_INDUSTRIAL,
	ZONE_DENSE_INDUSTRIAL,
	ZONE_MILITARY,
	ZONE_AIRPORT,
	ZONE_SEAPORT
};

static inline const char* GetZoneName(int iZoneID) {
	switch (iZoneID) {
	case ZONE_NONE:
		return "Unzoned";
	case ZONE_LIGHT_RESIDENTIAL:
		return "Light Residential";
	case ZONE_DENSE_RESIDENTIAL:
		return "Dense Residential";
	case ZONE_LIGHT_COMMERCIAL:
		return "Light Commercial";
	case ZONE_DENSE_COMMERCIAL:
		return "Dense Commercial";
	case ZONE_LIGHT_INDUSTRIAL:
		return "Light Industrial";
	case ZONE_DENSE_INDUSTRIAL:
		return "Dense Industrial";
	case ZONE_MILITARY:
		return "Military";
	case ZONE_AIRPORT:
		return "Airport";
	case ZONE_SEAPORT:
		return "Seaport";
	default:
		return "INVALID";
	}
}

// Military base types
enum {
	MILITARY_BASE_NONE = 0,
	MILITARY_BASE_DECLINED,
	MILITARY_BASE_ARMY,
	MILITARY_BASE_AIR_FORCE,
	MILITARY_BASE_NAVY,
	MILITARY_BASE_MISSILE_SILOS
};

// Building (XBLD) tile IDs
enum {
	TILE_CLEAR,
	TILE_RUBBLE1,
	TILE_RUBBLE2,
	TILE_RUBBLE3,
	TILE_RUBBLE4,
	TILE_RADIOACTIVITY,
	TILE_TREES1,
	TILE_TREES2,
	TILE_TREES3,
	TILE_TREES4,
	TILE_TREES5,
	TILE_TREES6,
	TILE_TREES7,
	TILE_SMALLPARK,

	TILE_POWERLINES_LR,
	TILE_POWERLINES_TB,
	TILE_POWERLINES_HTB,
	TILE_POWERLINES_LHR,
	TILE_POWERLINES_THB,
	TILE_POWERLINES_HLR,
	TILE_POWERLINES_BR,
	TILE_POWERLINES_BL,
	TILE_POWERLINES_TL,
	TILE_POWERLINES_TR,
	TILE_POWERLINES_RTB,
	TILE_POWERLINES_LBR,
	TILE_POWERLINES_TLB,
	TILE_POWERLINES_LTR,
	TILE_POWERLINES_LTBR,

	TILE_ROAD_LR,
	TILE_ROAD_TB,
	TILE_ROAD_HTB,
	TILE_ROAD_LHR,
	TILE_ROAD_THB,
	TILE_ROAD_HLR,
	TILE_ROAD_BR,
	TILE_ROAD_BL,
	TILE_ROAD_TL,
	TILE_ROAD_TR,
	TILE_ROAD_RTB,
	TILE_ROAD_LBR,
	TILE_ROAD_TLB,
	TILE_ROAD_LTR,
	TILE_ROAD_LTBR,

	TILE_RAIL_LR,
	TILE_RAIL_TB,
	TILE_RAIL_HTB,
	TILE_RAIL_LHR,
	TILE_RAIL_THB,
	TILE_RAIL_HLR,
	TILE_RAIL_BR,
	TILE_RAIL_BL,
	TILE_RAIL_TL,
	TILE_RAIL_TR,
	TILE_RAIL_RTB,
	TILE_RAIL_LBR,
	TILE_RAIL_TLB,
	TILE_RAIL_LTR,
	TILE_RAIL_LTBR,
	TILE_RAIL_HHTB,
	TILE_RAIL_LHHR,
	TILE_RAIL_THHB,
	TILE_RAIL_HHLR,

	TILE_TUNNEL_T,
	TILE_TUNNEL_R,
	TILE_TUNNEL_B,
	TILE_TUNNEL_L,

	TILE_CROSSOVER_POWERTB_ROADLR,
	TILE_CROSSOVER_POWERLR_ROADTB,
	TILE_CROSSOVER_ROADLR_RAILTB,
	TILE_CROSSOVER_ROADTB_RAILLR,
	TILE_CROSSOVER_POWERTB_RAILLR,
	TILE_CROSSOVER_POWERLR_RAILTB,

	TILE_HIGHWAY_LR,
	TILE_HIGHWAY_TB,

	TILE_CROSSOVER_HIGHWAYLR_ROADTB,
	TILE_CROSSOVER_HIGHWAYTB_ROADLR,
	TILE_CROSSOVER_HIGHWAYLR_RAILTB,
	TILE_CROSSOVER_HIGHWAYTB_RAILLR,
	TILE_CROSSOVER_HIGHWAYLR_POWERTB,
	TILE_CROSSOVER_HIGHWAYTB_POWERLR,

	TILE_SUSPENSION_BRIDGE_START_B,
	TILE_SUSPENSION_BRIDGE_MIDDLE_B,
	TILE_SUSPENSION_BRIDGE_CENTER_B,
	TILE_SUSPENSION_BRIDGE_MIDDLE_T,
	TILE_SUSPENSION_BRIDGE_END_T,
	TILE_RAISING_BRIDGE_TOWER,
	TILE_CAUSEWAY_PYLON,
	TILE_RAISING_BRIDGE_LOWERED,
	TILE_RAISING_BRIDGE_RAISED,
	TILE_RAIL_BRIDGE_PYLON,
	TILE_RAIL_BRIDGE,
	TILE_ELEVATED_POWERLINES,

	TILE_ONRAMP_TL,
	TILE_ONRAMP_TR,
	TILE_ONRAMP_BL,
	TILE_ONRAMP_BR,

	TILE_HIGHWAY_HTB,
	TILE_HIGHWAY_LHR,
	TILE_HIGHWAY_THB,
	TILE_HIGHWAY_HLR,
	TILE_HIGHWAY_BR,
	TILE_HIGHWAY_BL,
	TILE_HIGHWAY_TL,
	TILE_HIGHWAY_TR,
	TILE_HIGHWAY_LTBR,

	TILE_REINFORCED_BRIDGE_PYLON,
	TILE_REINFORCED_BRIDGE,

	TILE_SUBTORAIL_T,
	TILE_SUBTORAIL_R,
	TILE_SUBTORAIL_B,
	TILE_SUBTORAIL_L,
	
	TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1,
	TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES2,
	TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES3,
	TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES4,
	TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES1,
	TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES2,
	TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES3,
	TILE_RESIDENTIAL_1X1_MIDDLECLASSHOMES4,
	TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES1,
	TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES2,
	TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES3,
	TILE_RESIDENTIAL_1X1_UPPERCLASSHOMES4,

	TILE_COMMERCIAL_1X1_GASSTATION1,
	TILE_COMMERCIAL_1X1_BEDANDBREAKFAST,
	TILE_COMMERCIAL_1X1_CONVENIENCESTORE,
	TILE_COMMERCIAL_1X1_GASSTATION2,
	TILE_COMMERCIAL_1X1_SMALLOFFICEBUILDING1,
	TILE_COMMERCIAL_1X1_SMALLOFFICEBUILDING2,
	TILE_COMMERCIAL_1X1_WAREHOUSE,
	TILE_COMMERCIAL_1X1_TOYSTORE,

	TILE_INDUSTRIAL_1X1_SMALLWAREHOUSE1,
	TILE_INDUSTRIAL_1X1_CHEMICALSTORAGE,
	TILE_INDUSTRIAL_1X1_SMALLWAREHOUSE2,
	TILE_INDUSTRIAL_1X1_SUBSTATION,

	TILE_MISC_1X1_CONSTRUCTION1,
	TILE_MISC_1X1_CONSTRUCTION2,
	TILE_MISC_1X1_ABANDONED1,
	TILE_MISC_1X1_ABANDONED2,

	TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS1,
	TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS2,
	TILE_RESIDENTIAL_2X2_SMALLAPARTMENTS3,
	TILE_RESIDENTIAL_2X2_MEDIUMAPARTMENTS1,
	TILE_RESIDENTIAL_2X2_MEDIUMAPARTMENTS2,
	TILE_RESIDENTIAL_2X2_MEDIUMCONDOS1,
	TILE_RESIDENTIAL_2X2_MEDIUMCONDOS2,
	TILE_RESIDENTIAL_2X2_MEDIUMCONDOS3,

	TILE_COMMERCIAL_2X2_SHOPPINGCENTER,
	TILE_COMMERCIAL_2X2_GROCERYSTORE,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE1,
	TILE_COMMERCIAL_2X2_RESORTHOTEL,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE2,
	TILE_COMMERCIAL_2X2_OFFICERETAIL,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE3,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE4,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE5,
	TILE_COMMERCIAL_2X2_MEDIUMOFFICE6,

	TILE_INDUSTRIAL_2X2_MEDIUMWAREHOUSE,
	TILE_INDUSTRIAL_2X2_CHEMICALPROCESSING,
	TILE_INDUSTRIAL_2X2_FACTORY1,
	TILE_INDUSTRIAL_2X2_FACTORY2,
	TILE_INDUSTRIAL_2X2_FACTORY3,
	TILE_INDUSTRIAL_2X2_FACTORY4,
	TILE_INDUSTRIAL_2X2_FACTORY5,
	TILE_INDUSTRIAL_2X2_FACTORY6,

	TILE_MISC_2X2_CONSTRUCTION1,
	TILE_MISC_2X2_CONSTRUCTION2,
	TILE_MISC_2X2_CONSTRUCTION3,
	TILE_MISC_2X2_CONSTRUCTION4,
	TILE_MISC_2X2_ABANDONED1,
	TILE_MISC_2X2_ABANDONED2,
	TILE_MISC_2X2_ABANDONED3,
	TILE_MISC_2X2_ABANDONED4,

	TILE_RESIDENTIAL_3X3_LARGEAPARTMENTS1,
	TILE_RESIDENTIAL_3X3_LARGEAPARTMENTS2,
	TILE_RESIDENTIAL_3X3_LARGECONDOS1,
	TILE_RESIDENTIAL_3X3_LARGECONDOS2,

	TILE_COMMERCIAL_3X3_OFFICEPARK,
	TILE_COMMERCIAL_3X3_OFFICETOWER1,
	TILE_COMMERCIAL_3X3_MINIMALL,
	TILE_COMMERCIAL_3X3_THEATERSQUARE,
	TILE_COMMERCIAL_3X3_DRIVEINTHEATER,
	TILE_COMMERCIAL_3X3_OFFICETOWER2,
	TILE_COMMERCIAL_3X3_OFFICETOWER3,
	TILE_COMMERCIAL_3X3_PARKINGLOT,
	TILE_COMMERCIAL_3X3_HISTORICOFFICE,
	TILE_COMMERCIAL_3X3_CORPORATEHQ,

	TILE_INDUSTRIAL_3X3_CHEMICALPROCESSING,
	TILE_INDUSTRIAL_3X3_LARGEFACTORY,
	TILE_INDUSTRIAL_3X3_THINGAMAJIG,
	TILE_INDUSTRIAL_3X3_MEDIUMFACTORY,
	TILE_INDUSTRIAL_3X3_LARGEWAREHOUSE1,
	TILE_INDUSTRIAL_3X3_LARGEWAREHOUSE2,

	TILE_MISC_3X3_CONSTRUCTION1,
	TILE_MISC_3X3_CONSTRUCTION2,
	TILE_MISC_3X3_ABANDONED1,
	TILE_MISC_3X3_ABANDONED2,

	TILE_POWERPLANT_HYDRO1,
	TILE_POWERPLANT_HYDRO2,
	TILE_POWERPLANT_WIND,
	TILE_POWERPLANT_GAS,
	TILE_POWERPLANT_OIL,
	TILE_POWERPLANT_NUCLEAR,
	TILE_POWERPLANT_SOLAR,
	TILE_POWERPLANT_MICROWAVE,
	TILE_POWERPLANT_FUSION,
	TILE_POWERPLANT_COAL,

	TILE_SERVICES_CITYHALL,
	TILE_SERVICES_HOSPITAL,
	TILE_SERVICES_POLICE,
	TILE_SERVICES_FIRE,
	TILE_SERVICES_MUSEUM,
	TILE_SERVICES_BIGPARK,
	TILE_SERVICES_SCHOOL,
	TILE_SERVICES_STADIUM,
	TILE_SERVICES_PRISON,
	TILE_SERVICES_COLLEGE,
	TILE_SERVICES_ZOO,
	TILE_SERVICES_STATUE,
	
	TILE_INFRASTRUCTURE_WATERPUMP,
	TILE_INFRASTRUCTURE_RUNWAY,
	TILE_INFRASTRUCTURE_RUNWAYCROSS,
	TILE_INFRASTRUCTURE_PIER,
	TILE_INFRASTRUCTURE_CRANE,
	TILE_INFRASTRUCTURE_CONTROLTOWER_CIV,
	TILE_MILITARY_CONTROLTOWER,
	TILE_MILITARY_WAREHOUSE,
	TILE_INFRASTRUCTURE_BUILDING1,
	TILE_INFRASTRUCTURE_BUILDING2,
	TILE_MILITARY_TARMAC,
	TILE_MILITARY_F15B,
	TILE_MILITARY_HANGAR1,
	TILE_INFRASTRUCTURE_SUBWAYSTATION,
	TILE_MILITARY_RADAR,
	TILE_INFRASTRUCTURE_WATERTOWER,
	TILE_INFRASTRUCTURE_BUSDEPOT,
	TILE_INFRASTRUCTURE_RAILSTATION,
	TILE_INFRASTRUCTURE_PARKINGLOT,
	TILE_MILITARY_PARKINGLOT,
	TILE_MILITARY_LOADINGBAY,
	TILE_MILITARY_TOPSECRET,
	TILE_INFRASTRUCTURE_CARGOYARD,
	TILE_INFRASTRUCTURE_MAYORSHOUSE,
	TILE_INFRASTRUCTURE_WATERTREATMENT,
	TILE_INFRASTRUCTURE_LIBRARY,
	TILE_INFRASTRUCTURE_HANGAR2,
	TILE_INFRASTRUCTURE_CHURCH,
	TILE_INFRASTRUCTURE_MARINA,
	TILE_MILITARY_MISSILESILO,
	TILE_INFRASTRUCUTRE_DESALINIZATIONPLANT,

	TILE_ARCOLOGY_PLYMOUTH,
	TILE_ARCOLOGY_FOREST,
	TILE_ARCOLOGY_DARCO,
	TILE_ARCOLOGY_LAUNCH,
	
	TILE_OTHER_BRAUNLLAMADOME
};

enum {
	UNDER_TILE_CLEAR,

	UNDER_TILE_SUBWAY_LR,
	UNDER_TILE_SUBWAY_TB,
	UNDER_TILE_SUBWAY_HTB,
	UNDER_TILE_SUBWAY_LHR,
	UNDER_TILE_SUBWAY_THB,
	UNDER_TILE_SUBWAY_HLR,
	UNDER_TILE_SUBWAY_BR,
	UNDER_TILE_SUBWAY_BL,
	UNDER_TILE_SUBWAY_TL,
	UNDER_TILE_SUBWAY_TR,
	UNDER_TILE_SUBWAY_RTB,
	UNDER_TILE_SUBWAY_LBR,
	UNDER_TILE_SUBWAY_TLB,
	UNDER_TILE_SUBWAY_LTR,
	UNDER_TILE_SUBWAY_LTBR,

	UNDER_TILE_PIPES_LR,
	UNDER_TILE_PIPES_TB,
	UNDER_TILE_PIPES_HTB,
	UNDER_TILE_PIPES_LHR,
	UNDER_TILE_PIPES_THB,
	UNDER_TILE_PIPES_HLR,
	UNDER_TILE_PIPES_BR,
	UNDER_TILE_PIPES_BL,
	UNDER_TILE_PIPES_TL,
	UNDER_TILE_PIPES_TR,
	UNDER_TILE_PIPES_RTB,
	UNDER_TILE_PIPES_LBR,
	UNDER_TILE_PIPES_TLB,
	UNDER_TILE_PIPES_LTR,
	UNDER_TILE_PIPES_LTBR,

	UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR,
	UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB,
	UNDER_TILE_UNKNOWN,
	UNDER_TILE_MISSILESILO,
	UNDER_TILE_SUBWAYENTRANCE
};

extern const char* szTileNames[256];
extern const char* szUndergroundNames[36];
extern const char* szOnIdleStateEnums[20];

#define TILE_IS_MILITARY(iTileID) \
	((iTileID == 0xDD) || (iTileID == 0xDE) || (iTileID == 0xEF) || (iTileID == 0xF2) || (iTileID == 0xEA) || (iTileID == 0xE3) \
	|| (iTileID == 0xE4) || (iTileID == 0xE5) || (iTileID == 0xF1) || (iTileID == 0xE0) || (iTileID == 0xE2) || (iTileID == 0xE7) \
	|| (iTileID == 0xE8) || (iTileID == 0xF6) || (iTileID == 0xF9))

// XTER map
//
// Terrain tiles only need to consider the surface display (terrain tile id)
//
// Water tiles need to consider the surface display (water tile id) and
// the subsurface display (terrain tile id)
enum {
	TERRAIN_00 = 0x00,         // terrain tile id: 256
	TERRAIN_01,                // terrain tile id: 257
	TERRAIN_02,                // terrain tile id: 258
	TERRAIN_03,                // terrain tile id: 259
	TERRAIN_04,                // terrain tile id: 260
	TERRAIN_05,                // terrain tile id: 261
	TERRAIN_06,                // terrain tile id: 262
	TERRAIN_07,                // terrain tile id: 263
	TERRAIN_08,                // terrain tile id: 264
	TERRAIN_09,                // terrain tile id: 265
	TERRAIN_10,                // terrain tile id: 266
	TERRAIN_11,                // terrain tile id: 267
	TERRAIN_12,                // terrain tile id: 268
	TERRAIN_13,                // terrain tile id: 269
	SUBMERGED_00 = 0x10,       // terrain tild id: 256, water tile id: 270
	SUBMERGED_01,              // terrain tild id: 257, water tile id: 270
	SUBMERGED_02,              // terrain tild id: 258, water tile id: 270
	SUBMERGED_03,              // terrain tild id: 259, water tile id: 270
	SUBMERGED_04,              // terrain tild id: 260, water tile id: 270
	SUBMERGED_05,              // terrain tild id: 261, water tile id: 270
	SUBMERGED_06,              // terrain tild id: 262, water tile id: 270
	SUBMERGED_07,              // terrain tild id: 263, water tile id: 270
	SUBMERGED_08,              // terrain tild id: 264, water tile id: 270
	SUBMERGED_09,              // terrain tild id: 265, water tile id: 270
	SUBMERGED_10,              // terrain tild id: 266, water tile id: 270
	SUBMERGED_11,              // terrain tild id: 267, water tile id: 270
	SUBMERGED_12,              // terrain tild id: 268, water tile id: 270
	SUBMERGED_13,              // terrain tild id: 269, water tile id: 270
	COAST_00 = 0x20,           // terrain tile id: 256, water tile id: 270
	COAST_01,                  // terrain tile id: 257, water tile id: 271
	COAST_02,                  // terrain tile id: 258, water tile id: 272
	COAST_03,                  // terrain tile id: 259, water tile id: 273
	COAST_04,                  // terrain tile id: 260, water tile id: 274
	COAST_05,                  // terrain tile id: 261, water tile id: 275
	COAST_06,                  // terrain tile id: 262, water tile id: 276
	COAST_07,                  // terrain tile id: 263, water tile id: 277
	COAST_08,                  // terrain tile id: 264, water tile id: 278
	COAST_09,                  // terrain tile id: 265, water tile id: 279
	COAST_10,                  // terrain tile id: 266, water tile id: 280
	COAST_11,                  // terrain tile id: 267, water tile id: 281
	COAST_12,                  // terrain tile id: 268, water tile id: 282
	COAST_13,                  // terrain tile id: 269, water tile id: 283
	SURFACE_WATER_00 = 0x30,   // terrain tile id: 256, water tile id: 270
	SURFACE_WATER_01,          // terrain tile id: 256, water tile id: 271
	SURFACE_WATER_02,          // terrain tile id: 256, water tile id: 272
	SURFACE_WATER_03,          // terrain tile id: 256, water tile id: 273
	SURFACE_WATER_04,          // terrain tile id: 256, water tile id: 274
	SURFACE_WATER_05,          // terrain tile id: 256, water tile id: 275
	SURFACE_WATER_06,          // terrain tile id: 256, water tile id: 276
	SURFACE_WATER_07,          // terrain tile id: 256, water tile id: 277
	SURFACE_WATER_08,          // terrain tile id: 256, water tile id: 278
	SURFACE_WATER_09,          // terrain tile id: 256, water tile id: 279
	SURFACE_WATER_10,          // terrain tile id: 256, water tile id: 280
	SURFACE_WATER_11,          // terrain tile id: 256, water tile id: 281
	SURFACE_WATER_12,          // terrain tile id: 256, water tile id: 282
	SURFACE_WATER_13,          // terrain tile id: 256, water tile id: 283
	WATERFALL = 0x3e,          // terrain tile id: 269, water tile id: 284
	SURFACE_STREAM_00 = 0x40,  // terrain tile id: 256, water tile id: 285
	SURFACE_STREAM_01,         // terrain tile id: 256, water tile id: 286
	SURFACE_STREAM_02,         // terrain tile id: 256, water tile id: 287
	SURFACE_STREAM_03,         // terrain tile id: 256, water tile id: 288
	SURFACE_STREAM_04,         // terrain tile id: 256, water tile id: 289
	SURFACE_STREAM_05          // terrain tile id: 256, water tile id: 290
};

// XTHG types
enum {
	XTHG_NONE = 0,
	XTHG_AIRPLANE,
	XTHG_HELICOPTER,
	XTHG_CARGO_SHIP,
	XTHG_BULLDOZER,
	XTHG_MONSTER,
	XTHG_EXPLOSION,
	XTHG_DEPLOY_POLICE,
	XTHG_DEPLOY_FIRE,
	XTHG_SAILBOAT,
	XTHG_TRAIN_ENGINE,
	XTHG_TRAIN_CAR,
	XTHG_UNKNOWN_3,						// Seems to be a variant of TRAIN_ENGINE
	XTHG_UNKNOWN_4,						// Likewise for TRAIN_CAR
	XTHG_DEPLOY_MILITARY,
	XTHG_TORNADO,
	XTHG_MAXIS_MAN
};

enum {
	XTHG_DIRECTION_NORTH = 0,
	XTHG_DIRECTION_NORTH_EAST,
	XTHG_DIRECTION_EAST,
	XTHG_DIRECTION_SOUTH_EAST,
	XTHG_DIRECTION_SOUTH,
	XTHG_DIRECTION_SOUTH_WEST,
	XTHG_DIRECTION_WEST,
	XTHG_DIRECTION_NORTH_WEST
};

enum {
	GAME_MODE_TERRAIN_EDIT = 0,
	GAME_MODE_CITY,
	GAME_MODE_DISASTER
};

enum {
	GAME_DIFFICULTY_NONE = 0,
	GAME_DIFFICULTY_EASY,
	GAME_DIFFICULTY_MEDIUM,
	GAME_DIFFICULTY_HARD
};

enum {
	GAME_SPEED_PAUSED = 1,
	GAME_SPEED_TURTLE,
	GAME_SPEED_LLAMA,
	GAME_SPEED_CHEETAH,
	GAME_SPEED_AFRICAN_SWALLOW
};

enum {
	CITY_INDUSTRY_STEEL_OR_MINING = 0,
	CITY_INDUSTRY_TEXTILES,
	CITY_INDUSTRY_PETROCHEMICAL,
	CITY_INDUSTRY_FOOD,
	CITY_INDUSTRY_CONSTRUCTION,
	CITY_INDUSTRY_AUTOMOTIVE,
	CITY_INDUSTRY_AEROSPACE,
	CITY_INDUSTRY_FINANCE,
	CITY_INDUSTRY_MEDIA,
	CITY_INDUSTRY_ELECTRONICS,
	CITY_INDUSTRY_TOURISM
};

enum {
	WEATHER_TREND_COLD = 0,
	WEATHER_TREND_CLEAR,
	WEATHER_TREND_HOT,
	WEATHER_TREND_FOGGY,
	WEATHER_TREND_CHILLY,
	WEATHER_TREND_OVERCAST,
	WEATHER_TREND_SNOW,
	WEATHER_TREND_RAIN,
	WEATHER_TREND_WINDY,
	WEATHER_TREND_BLIZZARD,
	WEATHER_TREND_HURRICANE,
	WEATHER_TREND_TORNADO
};

enum {
	SOUND_BUILD = 500,
	SOUND_ERROR,
	SOUND_WIND,
	SOUND_PLOP,
	SOUND_EXPLODE,
	SOUND_CLICK,
	SOUND_POLICE,
	SOUND_FIRE,
	SOUND_BULLDOZER,
	SOUND_FIRETRUCK,
	SOUND_SIMCOPTER,
	SOUND_FLOOD,
	SOUND_BOOS,
	SOUND_CHEERS,
	SOUND_ZAP,
	SOUND_MAYDAY,
	SOUND_IMHIT,
	SOUND_SHIP,
	SOUND_TAKEOFF,
	SOUND_LAND,
	SOUND_SIREN,
	SOUND_HORNS,
	SOUND_PRISON,
	SOUND_SCHOOL,
	SOUND_TRAIN,
	SOUND_MILITARY,
	SOUND_ARCO,
	SOUND_MONSTER,
	SOUND_BULLDOZER2,					// identical to SOUND_BULLDOZER
	SOUND_RETICULATINGSPLINES,
	SOUND_SILENT
};

enum {
	BUDGET_RESFUND,
	BUDGET_COMFUND,
	BUDGET_INDFUND,
	BUDGET_ORDINANCE,
	BUDGET_BOND,
	BUDGET_POLICE,
	BUDGET_FIRE,
	BUDGET_HEALTH,
	BUDGET_SCHOOL,
	BUDGET_COLLEGE,
	BUDGET_ROAD,
	BUDGET_HIGHWAY,
	BUDGET_BRIDGE,
	BUDGET_RAIL,
	BUDGET_SUBWAY,
	BUDGET_TUNNEL,
	BUDGET_COUNT
};

enum {
	ORDINANCE_SALES_TAX = 0,
	ORDINANCE_INCOME_TAX,
	ORDINANCE_LEGALIZED_GAMBLING,
	ORDINANCE_PARKING_FINES,
	ORDINANCE_VOLUNTEER_FIRE_DEPARTMENT,
	ORDINANCE_PUBLIC_SMOKING_BAN,
	ORDINANCE_FREE_CLINICS,
	ORDINANCE_JUNIOR_SPORTS,
	ORDINANCE_PRO_READING_CAMPAIGN,
	ORDINANCE_ANTI_DRUG_CAMPAIGN,
	ORDINANCE_CPR_TRAINING,
	ORDINANCE_NEIGHBORHOOD_WATCH,
	ORDINANCE_TOURIST_ADVERTISING,
	ORDINANCE_BUSINESS_ADVERTISING,
	ORDINANCE_CITY_BEAUTIFICATION,
	ORDINANCE_ANNUAL_CARNIVAL,
	ORDINANCE_ENERGY_CONSERVATION,
	ORDINANCE_NUCLEAR_FREE_ZONE,
	ORDINANCE_HOMELESS_SHELTER,
	ORDINANCE_POLLUTION_CONTROLS
};

enum {
	MILITARYTILE_OTHER, // This one applies for any other item that isn't categorised below.
	MILITARYTILE_RUNWAY,
	MILITARYTILE_RUNWAYCROSS,
	MILITARYTILE_MPARKINGLOT,
	MILITARYTILE_CARGOYARD,
	MILITARYTILE_MRADAR,
	MILITARYTILE_MWAREHOUSE,
	MILITARYTILE_BUILDING1,
	MILITARYTILE_BUILDING2,
	MILITARYTILE_TOPSECRET,
	MILITARYTILE_CRANE,
	MILITARYTILE_MCONTROLTOWER,
	MILITARYTILE_F15B,
	MILITARYTILE_MHANGAR1,
	MILITARYTILE_HANGAR2,
	MILITARYTILE_MISSILESILO
};

enum {
	ZONEPOP_ALL,
	ZONEPOP_RESLIGHT,
	ZONEPOP_RESDENSE,
	ZONEPOP_COMLIGHT,
	ZONEPOP_COMDENSE,
	ZONEPOP_INDLIGHT,
	ZONEPOP_INDDENSE,
	ZONEPOP_ABANDONED
};

enum {
	TOOL_GROUP_BULLDOZER = 0,
	TOOL_GROUP_NATURE,					// I don't have a better name for this one.
	TOOL_GROUP_DISPATCH,
	TOOL_GROUP_POWER,
	TOOL_GROUP_WATER,
	TOOL_GROUP_REWARDS,
	TOOL_GROUP_ROADS,
	TOOL_GROUP_RAIL,
	TOOL_GROUP_PORTS,
	TOOL_GROUP_RESIDENTIAL,
	TOOL_GROUP_COMMERCIAL,
	TOOL_GROUP_INDUSTRIAL,
	TOOL_GROUP_EDUCATION,
	TOOL_GROUP_SERVICES,
	TOOL_GROUP_PARKS,
	TOOL_GROUP_SIGNS,
	TOOL_GROUP_QUERY,
	TOOL_GROUP_CENTERINGTOOL
};

enum {
	MAPTOOL_GROUP_BULLDOZER = 0,
	MAPTOOL_GROUP_RAISETERRAIN,
	MAPTOOL_GROUP_LOWERTERRAIN,
	MAPTOOL_GROUP_STRETCHTERRAIN,
	MAPTOOL_GROUP_LEVELTERRAIN,
	MAPTOOL_GROUP_WATER,
	MAPTOOL_GROUP_STREAM,
	MAPTOOL_GROUP_TREES,
	MAPTOOL_GROUP_FOREST,
	MAPTOOL_GROUP_CENTERINGTOOL
};

enum {
	VIEWROTATION_NORTH = 0,
	VIEWROTATION_EAST,
	VIEWROTATION_SOUTH,
	VIEWROTATION_WEST
};

enum {
	ONIDLE_STATE_INGAME = -1,
	ONIDLE_STATE_NONE,
	ONIDLE_STATE_DISPLAYMAXIS,
	ONIDLE_STATE_WAITMAXIS,
	ONIDLE_STATE_DISPLAYTITLE,
	ONIDLE_STATE_DIALOGFINISH,
	ONIDLE_STATE_DISPLAYREGISTRATION,
	ONIDLE_STATE_CLOSEREGISTRATION,
	ONIDLE_STATE_MAINMENU,
	ONIDLE_STATE_NONE_8,
	ONIDLE_STATE_NONE_9,
	ONIDLE_STATE_NONE_10,
	ONIDLE_STATE_RETURN_11,
	ONIDLE_STATE_RETURN_12,
	ONIDLE_STATE_RETURN_13,
	ONIDLE_STATE_MENUDIALOG,
	ONIDLE_STATE_UNKNOWN_15,
	ONIDLE_STATE_INTROVIDEO,
	ONIDLE_STATE_UNKNOWN_17,
	ONIDLE_STATE_UNKNOWN_18
};

// Structs

typedef struct {
	BYTE bDunno[32];
} neighbor_city_t;

typedef struct {
	int iCountMonth[12];
	int iFundMonth[12];
	int iCurrentCosts;
	int iFundingPercent;
	int iYearToDateCost;
	int iEstimatedCost;
} budget_t;

typedef struct {
	BYTE bTileID;
	BYTE bMicrosimData[7];
} microsim_t;

typedef struct {
	DWORD dwAddress;
	WORD wHeight;
	WORD wWidth;
} sprite_header_t;

typedef struct {
	WORD iLandAltitude : 5; // level / altitude
	WORD iWaterLevel : 5;   // not always accurate (rely on XTER value instead)
	WORD iTunnelLevels : 6; // how many levels below altitude should we display a grey block for a tunnel?
} map_ALTM_attribs_t;

typedef struct {
	map_ALTM_attribs_t w;
} map_ALTM_t;

typedef struct {
	BYTE iZoneType : 4;
	BYTE iCorners : 4;
} map_XZON_attribs_t;

typedef struct {
	map_XZON_attribs_t b;
} map_XZON_t;

typedef struct {
	BYTE iTileID;
} map_XBLD_t;

typedef struct {
	BYTE iTileID; // reference XTER map
} map_XTER_t;

typedef struct {
	BYTE iTileID;
} map_XUND_t;

typedef struct {
	BYTE bTextOverlay;
} map_XTXT_t;

typedef struct {
	BYTE bBlock;
} map_mini64_t;

typedef struct {
	BYTE bBlock;
} map_mini32_t;

typedef struct {
	BYTE iSaltWater : 1;
	BYTE iRotated : 1;
	BYTE iWater : 1;
	BYTE iXVALMask : 1;
	BYTE iWatered : 1;
	BYTE iPiped : 1;
	BYTE iPowered : 1;
	BYTE iPowerable : 1;
} map_XBIT_bits_t;

typedef struct {
	map_XBIT_bits_t b;
} map_XBIT_t;

typedef struct {
	BYTE iId; // use xthg types enum to determine
	BYTE iDirection; // use xthg directions enum; for types airplane, helicopter, cargo ship, monster, etc; not sure how this is used for "deployment" types like fire, police, etc
	BYTE iDunno1; // identifier? sequence number? type?
	BYTE iPositionX;
	BYTE iPositionY;
	BYTE iPositionZ;
	BYTE iDunno2;
	BYTE iDunno3;
	BYTE iDunno4;
	BYTE iDunno5;
	BYTE iDunno6;
	BYTE iDunno7;
} map_XTHG_t;

typedef struct {
	char szLabel[24];
	BYTE bPadding;
} map_XLAB_t;

// Function pointers

GAMECALL(0x40103C, int, __thiscall, PreGameMenuDialogToggle, void *pThis, int iShow)
GAMECALL(0x40106E, int, __cdecl, PlaceRoadAtCoordinates, __int16 x, __int16 y)
GAMECALL(0x401096, int, __thiscall, SoundPlaySound, void* pThis, int iSoundID)
GAMECALL(0x4011E5, int, __thiscall, MapToolSoundTrigger, void* pThis)
GAMECALL(0x4012C1, int, __cdecl, SpawnItem, __int16 x, __int16 y)
GAMECALL(0x401460, char, __cdecl, SimulationProvisionMicrosim, __int16, int, __int16 iTileID) // The first two arguments aren't clear, though they "could" be the X/Y tile coordinates.
GAMECALL(0x4014CE, int, __cdecl, SpawnAeroplane, __int16 x, __int16 y, __int16 iDirection)
GAMECALL(0x4014F1, int, __thiscall, TileHighlightUpdate, void *pThis)
GAMECALL(0x40150A, int, __thiscall, ExitRequester, void *pThis, int iSource)
GAMECALL(0x4015A0, void, __thiscall, DoSaveCity, void *pThis)
GAMECALL(0x401519, void, __thiscall, ToolMenuEnable, void* pThis)
GAMECALL(0x4016D1, int, __thiscall, CenterOnNewScreenCoordinates, void *pThis, __int16 iNewScreenPointX, __int16 iNewScreenPointY)
GAMECALL(0x4016F9, int, __cdecl, PlaceChurch, __int16 x, __int16 y)
GAMECALL(0x40178F, __int16, __cdecl, PlaceTileWithMilitaryCheck, __int16 x, __int16 y, __int16 iTileID)
GAMECALL(0x401857, int, __cdecl, MapToolPlaceTree, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x40198D, int, __cdecl, MapToolPlaceStream, __int16 iTileTargetX, __int16 iTileTargetY, __int16) // XXX - the last parameter isn't entirely clear, perhaps area or offset?
GAMECALL(0x401997, int, __cdecl, MapToolPlaceWater, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x4019A1, char, __cdecl, CheckAndAdjustTraversableTerrain, __int16 x, __int16 y)
GAMECALL(0x4019EC, int, __cdecl, CenterOnTileCoords, __int16 x, __int16 y)
GAMECALL(0x401A37, int, __cdecl, MaybeRoadViabilityAlongPath, __int16* x, __int16* y)
GAMECALL(0x401A3C, char, __cdecl, PerhapsGeneralZoneStartBuilding, signed __int16 x, signed __int16 y, __int16 iBuildingPopLevel, __int16 iZoneType)
GAMECALL(0x401AB4, int, __cdecl, MapToolRaiseTerrain, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x401AF0, int, __cdecl, MaybeCheckViablePlacementPath, __int16 x1, __int16 y1, __int16 x2, __int16 y2)
GAMECALL(0x401B40, int, __cdecl, IsZonedTilePowered, __int16 x, __int16 y)
GAMECALL(0x401CCB, int, __stdcall, ResetTileDirection, void)
GAMECALL(0x401D16, __int16, __cdecl, GetTileCoordsFromScreenCoords, __int16 x, __int16 y)
GAMECALL(0x401E38, int, __cdecl, PlaceUndergroundTiles, __int16 x, __int16 y, __int16 iUndergroundTileID)
GAMECALL(0x401E47, BOOL, __cdecl, UseBulldozer, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x401EA1, int, __cdecl, MapToolLowerTerrain, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x401FA0, int, __cdecl, CheckAdjustTerrainAndPlacePowerLines, __int16 x, __int16 y)
GAMECALL(0x40209F, __int16, __cdecl, SpawnTrain, __int16 x, __int16 y)
GAMECALL(0x402211, unsigned int, __thiscall, DestroyStructure, DWORD *pThis, __int16 x, __int16 y, int iExplosion)
GAMECALL(0x40226B, int, __thiscall, UpdateAreaPortionFill, void *) // This appears to do a partial update of selected/highlighted area while appearing to dispense with immediate color updates.
GAMECALL(0x402289, char, __cdecl, PerhapsGeneralZoneChooseAndPlaceBuilding, __int16 x, __int16 y, __int16 iBuildingPopLevel, __int16)
GAMECALL(0x40235B, int, __stdcall, DrawSquareHighlight, WORD wX1, WORD wY1, WORD wX2, WORD wY2)
GAMECALL(0x4023B0, int, __cdecl, IsValidTransitItems, __int16 x, __int16 y)
GAMECALL(0x4023EC, void, __stdcall, ToolMenuUpdate, void)
GAMECALL(0x402414, int, __thiscall, MusicPlay, DWORD pThis, int iSongID)
GAMECALL(0x402478, int, __cdecl, SpawnHelicopter, __int16 x, __int16 y)
GAMECALL(0x4024FA, char, __cdecl, PerhapsGeneralZoneChangeBuilding, __int16 x, __int16 y, __int16 iBuldingPopLevel, int iTileID)
GAMECALL(0x40258B, int, __cdecl, GetScreenCoordsFromTileCoords, __int16 iTileTargetX, __int16 iTileTargetY, WORD *wNewScreenPointX, WORD *wNewScreenPointY)
GAMECALL(0x402603, __int16, __cdecl, ZonedBuildingTileDeletion, __int16 x, __int16 y)
GAMECALL(0x402699, int, __thiscall, PointerToCSimcityViewClass, void* CSimcityAppThis)
GAMECALL(0x4026B2, int, __cdecl, SimulationGrowSpecificZone, __int16 x, __int16 y, __int16 iTileID, __int16 iZoneType)
GAMECALL(0x402725, int, __cdecl, PlacePowerLinesAtCoordinates, __int16 x, __int16 y)
GAMECALL(0x402798, int, __cdecl, MapToolPlaceForest, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x4027A7, int, __thiscall, CSimCityView_OnVScroll, DWORD pThis, int nSBCode, __int16 nPos, int pScrollBar)
GAMECALL(0x4027F2, int, __cdecl, ItemPlacementCheck, __int16 x, int y, __int16 iTileID, __int16 iTileArea)
GAMECALL(0x402810, int, __thiscall, UpdateAreaCompleteColorFill, void *) // This appears to be a more comprehensive update that'll occur for highlighted/selected area or when you're moving the game area.
GAMECALL(0x40281F, int, __cdecl, UpdateDisasterAndTransitStats, __int16 x, __int16 y, __int16 iZoneType, __int16 iBuildingPopLevel, __int16)
GAMECALL(0x402829, void, __cdecl, SpawnShip, __int16 x, __int16 y)
GAMECALL(0x402900, int, __cdecl, NewspaperStoryGenerator, __int16 iType, char iValue)
GAMECALL(0x402937, void, __thiscall, ToolMenuDisable, void* pThis)
GAMECALL(0x402978, int, __cdecl, SpawnSailBoat, __int16 x, __int16 y)
GAMECALL(0x4029C3, int, __cdecl, CSimcityViewMouseMoveOrLeftClick, void* pThis, LPPOINT lpPoint)
GAMECALL(0x402B2B, int, __cdecl, MapToolStretchTerrain, __int16 iTileTargetX, __int16 iTileTargetY, __int16 iScreenTargetPointY)
GAMECALL(0x402B44, __int16, __cdecl, MapToolMenuAction, int iMouseKeys, POINT pt)
GAMECALL(0x402B94, int, __cdecl, MapToolLevelTerrain, __int16 iTileTargetX, __int16 iTileTargetY)
GAMECALL(0x402C25, int, __cdecl, CityToolMenuAction, int iMouseKeys, POINT pt)
GAMECALL(0x402CF2, void, __thiscall, SimcityAppSetGameCursor, void *pThis, int iNewCursor, BOOL bActive)
GAMECALL(0x402F9A, int, __thiscall, GetScreenAreaInfo, DWORD pThis, LPRECT lpRect)
GAMECALL(0x480140, void, __stdcall, LoadSoundBuffer, int iSoundID, void* pBuffer)
GAMECALL(0x48A810, DWORD, __cdecl, Direct_MovieCheck, char *sMovStr)


// MFC function pointers. Use with care.
GAMECALL(0x4017B2, void, __thiscall, RefreshTitleBar, void* pThis)
GAMECALL(0x40C3E0, void, __thiscall, CFrameWnd_ShowStatusBar, HWND* pThis, HWND hWnd)
GAMECALL(0x4A3BDF, struct CWnd *, __stdcall, CWnd_FromHandle, HWND hWnd)
GAMECALL(0x4AA573, void, __thiscall, CWinApp_OnAppExit, void *pThis)
GAMECALL(0x4AE0BC, void, __thiscall, CDocument_UpdateAllViews, void* pThis, void* pSender, int lHint, void* pHint)
GAMECALL(0x4B234F, int, __stdcall, AfxMessageBox, unsigned int nIDPrompt, unsigned int nType, unsigned int nIDHelp)

// Random calls.
GAMECALL(0x40116D, __int16, __cdecl, RandomWordLCGMod, __int16 iSeed)
GAMECALL(0x402261, __int16, __stdcall, RandomWordLFSRMod4, void)
GAMECALL(0x402B3F, __int16, __stdcall, RandomWordLFSRMod128, int seed)

// Unknown functions that do something we might need them to. Use with extreme care.

// Pointers

GAMEOFF(DWORD,	pCSimcityAppThis,			0x4C7010)
GAMEOFF(void*,	pCWndRootWindow,			0x4C702C)		// CMainFrame
GAMEOFF(DWORD,	dwSCAGameAutoSave,			0x4D70E8)
GAMEOFF(DWORD,	dwCursorGameHit,			0x4C70EC)
GAMEOFF(BOOL,	bPriscillaActivated,		0x4C7104)
GAMEOFF(DWORD*, dwAudioHandle,				0x4C7158)		// Various checks have pointed towards audio (sound and/or midi - perhaps stoppage given some context elsewhere)
GAMEOFF(BOOL,	bOptionsMusicEnabled,		0x4C71F0)
GAMEOFF(WORD,	wSimulationSpeed,			0x4C7318)
GAMEOFF(WORD,	wCurrentTileCoordinates,	0x4C7A98)
GAMEOFF(WORD,	wTileCoordinateX,			0x4C7AB0)
GAMEOFF(WORD,	wTileCoordinateY,			0x4C7AB4)
GAMEOFF(WORD,	wGameScreenAreaX,			0x4C7AD8)		// Used here in CSimcityView_WM_LBUTTONDOWN and CSimcityView_WM_MOUSEFIRST
GAMEOFF(WORD,	wGameScreenAreaY,			0x4C7ADC)		// Used here in CSimcityView_WM_LBUTTONDOWN and CSimcityView_WM_MOUSEFIRST
GAMEOFF(WORD,	wCurrentAngle,				0x4C7CF8)
GAMEOFF(WORD,	wTileDirection,				0x4C7D60)
GAMEOFF(WORD,	wMaybeActiveToolGroup,		0x4C7D88)
GAMEOFF(WORD,	wViewRotation,				0x4C942C)
GAMEOFF(BOOL,	bCityHasOcean,				0x4C94C0)
GAMEOFF(DWORD,	dwArcologyPopulation,		0x4C94C4)
GAMEOFF(DWORD,	dwDisasterActive,			0x4C9EE8)
GAMEOFF(DWORD,	dwCityResidentialPopulation,	0x4CA194)
GAMEOFF(CMFC3XString, pszCityName,				0x4CA1A0)
GAMEOFF(WORD,	wNationalEconomyTrend,		0x4CA1BC)
GAMEOFF(WORD,	wCurrentMapToolGroup,		0x4CA1EC)
GAMEOFF(WORD,	wCityNeighborConnections1500,	0x4CA3F0)
GAMEOFF(WORD,	wSubwayXUNDCount,			0x4CA41C)
GAMEOFF(WORD,	wDisasterType,				0x4CA420)
GAMEOFF(DWORD *,	pZonePops,					0x4CA428)
GAMEOFF(WORD,	wCityMode,					0x4CA42C)
GAMEOFF(int,	dwCityLandValue,			0x4CA440)
GAMEOFF(int,	dwCityFunds,				0x4CA444)
GAMEOFF(WORD*, dwTileCount,					0x4CA4C8)		// WORD dwTileCount[256]
GAMEOFF(DWORD,	dwCityValue,				0x4CA4D0)
GAMEOFF(DWORD,	dwCityGarbage,				0x4CA5F0)		// Unused in vanilla game (sort of)
GAMEOFF(WORD,	wCityStartYear,				0x4CA5F4)
GAMEOFF(DWORD,	dwCityUnemployment,			0x4CA5F8)
GAMEOFF_ARR(DWORD, dwNeighborValue,			0x4CA804)		// DWORD dwNeighborValue[4]
GAMEOFF(WORD,	wNewspaperChoice,			0x4CA808)
GAMEOFF(short,	wWaterLevel,				0x4CA818)
GAMEOFF(WORD,	wMonsterXTHGIndex,			0x4CA81C)
GAMEOFF(DWORD,	dwNationalPopulation,		0x4CA928)
GAMEOFF_ARR(DWORD, dwNeighborFame,			0x4CA92C)		// DWORD dwNeighborFame[4]
GAMEOFF(WORD*,	dwMilitaryTiles,			0x4CA934)
GAMEOFF(WORD,	wNationalTax,				0x4CA938)
GAMEOFF(WORD,	wCurrentDisasterID,			0x4CA93C)
GAMEOFF(DWORD,	dwCityOrdinances,			0x4CAA40)
GAMEOFF(DWORD,	dwPowerUsedPercentage,		0x4CAA50)
GAMEOFF(DWORD,	dwCityPopulation,			0x4CAA74)
GAMEOFF_ARR(DWORD, dwNeighborPopulation,	0x4CAD10)		// DWORD dwNeighborPopulation[4]
GAMEOFF(DWORD,	dwCityFame,					0x4CAD28)		// Unused in vanilla game
GAMEOFF(BOOL,	bYearEndFlag,				0x4CAD2C)
GAMEOFF(WORD,	wScreenPointX,				0x4CAD30)		// Used here in MapToolMenuAction
GAMEOFF(WORD,	wScreenPointY,				0x4CAD34)		// Used here in MapToolMenuAction
GAMEOFF(BOOL,	bInScenario,				0x4CAD44)
GAMEOFF_ARR(char, szNeighborNameSouth,		0x4CAD58)		// char[32]
GAMEOFF_ARR(char, szNeighborNameWest,		0x4CAD78)		// char[32]
GAMEOFF_ARR(char, szNeighborNameNorth,		0x4CAD98)		// char[32]
GAMEOFF_ARR(char, szNeighborNameEast,		0x4CADB8)		// char[32]
GAMEOFF(BYTE,	bWeatherHeat,				0x4CADE0)
GAMEOFF(RECT,	rcDst,						0x4CAD48)
GAMEOFF(DWORD,	dwCityDays,					0x4CAE04)
GAMEOFF(BYTE,	bWeatherWind,				0x4CAE0C)
GAMEOFF(WORD,	wCityProgression,			0x4CB010)
GAMEOFF(DWORD,	dwNationalValue,			0x4CB014)
GAMEOFF(DWORD,	dwCityAdvertising,			0x4CB018)		// Unused in vanilla game
GAMEOFF(WORD,	wCityCurrentMonth,			0x4CB01C)
GAMEOFF(WORD,	wCityElapsedYears,			0x4CB020)
GAMEOFF_ARR(sprite_header_t*, pArrSpriteHeaders, 0x4CB1B8)
GAMEOFF(BOOL,	bNewspaperSubscription,		0x4CB3D0)
GAMEOFF(BYTE,	bWeatherHumidity,			0x4CB3D4)
GAMEOFF(WORD,	wCityCurrentSeason,			0x4CB3E8)
GAMEOFF(microsim_t*, pMicrosimArr,			0x4CB3EC)
GAMEOFF(BOOL,	bCityHasRiver,				0x4CB3F8)
GAMEOFF(WORD,	wCityDifficulty,			0x4CB404)
GAMEOFF(BYTE,	bWeatherTrend,				0x4CB40C)
GAMEOFF(DWORD,	dwCityWorkforceLE,			0x4CB410)
GAMEOFF_ARR(WORD,	wCityInventionYears,	0x4CB430)
GAMEOFF(DWORD,	dwCityCrime,				0x4CB454)
GAMEOFF(WORD,	wCityCenterX,				0x4CB458)
GAMEOFF(WORD,	wCityCenterY,				0x4CB45C)
GAMEOFF(DWORD,	dwCityWorkforcePercent,		0x4CB460)
GAMEOFF(WORD,	wCurrentCityToolGroup,		0x4CB464)
GAMEOFF(DWORD,	dwCityWorkforceEQ,			0x4CC4B4)
GAMEOFF(DWORD,	dwWaterUsedPercentage,		0x4CC4B8)
GAMEOFF(BOOL,	bNewspaperExtra,			0x4CC4BC)
GAMEOFF(budget_t*,	pBudgetArr,				0x4CC4CC)		// Needs reverse engineering. See wiki.
GAMEOFF(BOOL,	bNoDisasters,				0x4CC4D4)
GAMEOFF_ARR(WORD, wNeighborNameIdx,			0x4CC4DC)		// WORD wNeighborNameIdx[4]
GAMEOFF(WORD,	wCityNeighborConnections1000,	0x4CC4D8)
GAMEOFF(BYTE,	bMilitaryBaseType,			0x4CC4E4)
GAMEOFF(int,	dwCityBonds,				0x4CC4E8)
GAMEOFF(DWORD,	dwCityTrafficUnknown,		0x4CC6F4)
GAMEOFF_ARR(WORD,	wCityResidentialDemand,		0x4CC8F8)
GAMEOFF(WORD,	wCityCommericalDemand,		0x4CC8FA)
GAMEOFF(WORD,	wCityIndustrialDemand,		0x4CC8FC)
GAMEOFF(DWORD,	dwCityPollution,			0x4CC910)		// Needs reverse engineering. See wiki.
GAMEOFF(WORD,	wScenarioTimeLimitMonths,	0x4CC91C)
GAMEOFF(DWORD,	dwScenarioCitySize,			0x4CC91E)
GAMEOFF(DWORD,	dwScenarioResPopulation,	0x4CC922)
GAMEOFF(DWORD,	dwScenarioComPopulation,	0x4CC926)
GAMEOFF(DWORD,	dwScenarioIndPopulation,	0x4CC92A)
GAMEOFF(DWORD,	dwScenarioCashGoal,			0x4CC92E)
GAMEOFF(DWORD,	dwScenarioLandValueGoal,	0x4CC932)
GAMEOFF(WORD,	wScenarioLEGoal,			0x4CC936)
GAMEOFF(WORD,	wScenarioEQGoal,			0x4CC938)
GAMEOFF(DWORD,	dwScenarioPollutionLimit,	0x4CC93A)
GAMEOFF(DWORD,	dwScenarioCrimeLimit,		0x4CC93E)
GAMEOFF(DWORD,	dwScenarioTrafficLimit,		0x4CC942)
GAMEOFF(BYTE,	bScenarioBuildingGoal1,		0x4CC946)
GAMEOFF(BYTE,	bScenarioBuildingGoal2,		0x4CC947)
GAMEOFF(WORD,	wScenarioBuildingGoal1Count,0x4CC948)
GAMEOFF(WORD,	wScenarioBuildingGoal2Count,0x4CC94A)
GAMEOFF_ARR(WORD,	wSelectedSubtool,		0x4CC950)
GAMEOFF(WORD,	wHighlightedTileX1,			0x4CDB68)
GAMEOFF(WORD,	wHighlightedTileX2,			0x4CDB6C)
GAMEOFF(WORD,	wHighlightedTileY1,			0x4CDB70)
GAMEOFF(WORD,	wHighlightedTileY2,			0x4CDB74)
GAMEOFF(DWORD,	dwLFSRState,				0x4CDB7C)
GAMEOFF(DWORD,	dwLCGState,					0x4CDB80)
GAMEOFF(void*,	pCWinApp,					0x4CE8C0)
GAMEOFF_ARR(WORD,	wPositionAngle,			0x4DC4C8)
GAMEOFF_ARR(DWORD,	dwDisasterStringIndex,	0x4E6010)
GAMEOFF(DWORD,	dwSimulationSubtickCounter,	0x4E63D8)
GAMEOFF(void*,	pCDocumentMainWindow,		0x4E66F8)
GAMEOFF(WORD,	wPreviousTileCoordinateX,	0x4E6808)
GAMEOFF(WORD,	wPreviousTileCoordinateY,	0x4E680C)
GAMEOFF(void*,	pCSimcityView,				0x4E682C)
GAMEOFF_ARR(DWORD, dwCityProgressionRequirements, 0x4E6984)
GAMEOFF(DWORD,	dwNextRefocusSongID,		0x4E6F8C)
GAMEOFF_ARR(DWORD, dwZoneNameStringIDs,		0x4E7140)
GAMEOFF_ARR(WORD,	wBuildingPopLevel,		0x4E7458)
GAMEOFF_ARR(BYTE,	bAreaState,				0x4E7508)
GAMEOFF_ARR(WORD,	wBuildingPopulation,	0x4E75B0)
GAMEOFF_ARR(WORD, wXTERToSpriteIDMap,		0x4E7628)
GAMEOFF_ARR(WORD, wXTERToXUNDSpriteIDMap,	0x4E76B8)
GAMEOFF_ARR(DWORD, dwCityNoticeStringIDs,	0x4E98B8)
GAMEOFF(WORD,	wActiveTrains,				0x4E99D8)
GAMEOFF(WORD,	wSailingBoats,				0x4E99D0)
GAMEOFF(DWORD,	dwCityRewardsUnlocked,		0x4E9A24)
GAMEOFF(WORD,	wTileHighlightActive,		0x4EA7F0)

// Pending classification
GAMEOFF_ARR(WORD, wTileAreaBottomLeftCorner,	0x4DC4D0)
GAMEOFF_ARR(WORD, wTileAreaBottomRightCorner,	0x4DC4D2)
GAMEOFF_ARR(WORD, wTileAreaTopLeftCorner,	0x4DC4D4)
GAMEOFF_ARR(WORD, wTileAreaTopRightCorner,	0x4DC4D6)
GAMEOFF_ARR(WORD, wSomePierLengthWays,		0x4E75C0)
GAMEOFF_ARR(WORD, wSomePierDepthWays,		0x4E75C8)

// Pointers to map arrays

// 128x128
GAMEOFF_ARR(map_XTER_t*,	dwMapXTER,	0x4C9F58)
GAMEOFF_ARR(map_XZON_t*,	dwMapXZON,	0x4CA1F0)
GAMEOFF_ARR(map_XTXT_t*,	dwMapXTXT,	0x4CA600)
GAMEOFF_ARR(map_XBIT_t*,	dwMapXBIT,	0x4CAB10)
GAMEOFF_ARR(map_ALTM_t*,	dwMapALTM,	0x4CAE10)
GAMEOFF_ARR(map_XUND_t*,	dwMapXUND,	0x4CB1D0)
GAMEOFF_ARR(map_XBLD_t*,	dwMapXBLD,	0x4CC4F0)

// 64x64
GAMEOFF_ARR(map_mini64_t*,	dwMapXCRM,	0x4CA4D8)
GAMEOFF_ARR(map_mini64_t*,	dwMapXPLT,	0x4CA828)
GAMEOFF_ARR(map_mini64_t*,	dwMapXTRF,	0x4CA940)
GAMEOFF_ARR(map_mini64_t*,	dwMapXVAL,	0x4CB0A8)

// 32x32
GAMEOFF_ARR(map_mini32_t*,	dwMapXPLC,	0x4C9430)
GAMEOFF_ARR(map_mini32_t*,	dwMapXPOP,	0x4CA448)
GAMEOFF_ARR(map_mini32_t*,	dwMapXFIR,	0x4CAA78)
GAMEOFF_ARR(map_mini32_t*,	dwMapXROG,	0x4CB028)

// totally different
GAMEOFF_ARR(map_XLAB_t*,	dwMapXLAB,	0x4CA198)
GAMEOFF_ARR(map_XTHG_t*,	dwMapXTHG,	0x4CA434)
GAMEOFF_ARR(DWORD,			dwMapXGRP,	0x4CC470)


static inline int GetTileID(int iTileX, int iTileY) {
	if (iTileX >= 0 && iTileX < GAME_MAP_SIZE && iTileY >= 0 && iTileY < GAME_MAP_SIZE)
		return dwMapXBLD[iTileX][iTileY].iTileID;
	else
		return -1;
}

static inline const char* GetXLABEntry(int iLabelID) {
	return (*dwMapXLAB + iLabelID)->szLabel;
}

static inline sprite_header_t* GetSpriteHeader(int iSpriteID) {
	return (*pArrSpriteHeaders + iSpriteID);
}

static inline HPALETTE GameGetPalette(void) {
	DWORD* CSimcityAppThis = &pCSimcityAppThis;
	DWORD* CPalette;

	// Exactly what sub_4069B0 does.
	if (CSimcityAppThis[59])
		CPalette = (DWORD*)CSimcityAppThis[67];
	else
		CPalette = (DWORD*)CSimcityAppThis[68];

	// ...and this is what CDC::SelectPalette does.
	return (HPALETTE)CPalette[1];
}

static inline HWND GameGetRootWindowHandle(void) {
	return (HWND)((DWORD*)pCWndRootWindow)[7];
}

static inline DWORD SwapDWORD(DWORD dwData) {
	return _byteswap_ulong(dwData);
}
