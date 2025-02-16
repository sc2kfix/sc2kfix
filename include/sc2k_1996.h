// sc2kfix sc2k_1996.h: defines specific to the 1996 Special Edition version
// (c) 2025 github.com/araxestroy - released under the MIT license

// !!! HIC SUNT DRACONES !!!
// This is basically a live interface to a bunch of the reverse engineering I've done. Some of it
// may have adverse effects if you start fudging the numbers using these offsets. Actually a lot
// of it will. It's best to use it just to check things.
//
// In particular I don't know if I've written rvalues right to be able to use structs in GAMEOFF
// macros. We'll find out eventually.
//
// I'm usually a C guy.

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
	DISASTER_HELICOPTER_MAYBE,
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

// Military base types
enum {
	MILITARY_BASE_NONE = 0,
	MILITARY_BASE_DECLINED,
	MILITARY_BASE_ARMY,
	MILITARY_BASE_AIR_FORCE,
	MILITARY_BASE_NAVY,
	MILITARY_BASE_MISSILE_SILOS
};

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
	XTHG_UNKNOWN_0 = 0,
	XTHG_AIRPLANE,
	XTHG_HELICOPTER,
	XTHG_CARGO_SHIP,
	XTHG_UNKNOWN_1,
	XTHG_MONSTER,
	XTHG_UNKNOWN_2,
	XTHG_DEPLOY_POLICE,
	XTHG_DEPLOY_FIRE,
	XTHG_SAILBOAT,
	XTHG_TRAIN_ENGINE,
	XTHG_TRAIN_CAR,
	XTHG_UNKNOWN_3,
	XTHG_UNKNOWN_4,
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
	GAME_SPEED_PAUSED = 0,
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

// Structs

typedef struct {
	BYTE bDunno[32];
} neighbor_city_t;


typedef struct {
	struct {
		WORD iTunnelLevels : 6; // how many levels below altitude should we display a grey block for a tunnel?
		WORD iWaterLevel : 5;   // not always accurate (rely on XTER value instead)
		WORD iLandAltitude : 5; // level / altitude
	} w[128];
} map_ALTM_t;

typedef struct {
	struct {
		BYTE iZoneType : 4;
		BYTE iCorners : 4;
	} b[128];
} map_XZON_t;

typedef struct {
	BYTE iBuildingID[128];
} map_XBLD_t;

typedef struct {
	BYTE iTileId[128]; // reference XTER map
} map_XTER_t;

typedef struct {
	BYTE iTileID[128];
} map_XUND_t;

typedef struct {
	BYTE bTextOverlay[128];
} map_XTXT_t;

typedef struct {
	struct {
		BYTE iPowerable : 1;
		BYTE iPowered : 1;
		BYTE iPiped : 1;
		BYTE iWatered : 1;
		BYTE iXVALMask : 1;
		BYTE iWater : 1;
		BYTE iRotated : 1;
		BYTE iSaltWater : 1;
	} b[128];
} map_XBIT_t;

typedef struct {
	BYTE iId : 1; // use xthg types enum to determine
	BYTE iDirection: 1; // use xthg directions enum; for types airplane, helicopter, cargo ship, monster, etc; not sure how this is used for "deployment" types like fire, police, etc
	BYTE iDunno1 : 1; // identifier? sequence number? type?
	BYTE iPositionX : 1;
	BYTE iPositionY : 1;
	BYTE iPositionZ : 1;
	BYTE iDunno2 : 1;
	BYTE iDunno3 : 1;
	BYTE iDunno4 : 1;
	BYTE iDunno5 : 1;
	BYTE iDunno6 : 1;
	BYTE iDunno7 : 1;
} map_XTHG_t;




// Pointers

GAMEOFF(WORD,	wSimulationSpeed,			0x4C7318)
GAMEOFF(WORD,	wDisasterType,				0x4CA420)
GAMEOFF(DWORD,	dwCityLandValue,			0x4CA440)
GAMEOFF(DWORD,	dwCityFunds,				0x4CA444)
GAMEOFF_ARR(WORD, dwTileCount,				0x4CA4C8)
GAMEOFF(WORD,	wCityStartYear,				0x4CA5F4)
GAMEOFF(DWORD,	dwCityPopulation,			0x4CAA74)
GAMEOFF(DWORD,	dwInScenario,				0x4CAD44)
GAMEOFF_ARR(neighbor_city_t, stNeighborCities, 0x4CAD58)	// stNeighborCities[4]
GAMEOFF(DWORD,	dwCityDays,					0x4CAE04)
GAMEOFF(WORD,	wCityProgression,			0x4CB010)
GAMEOFF(WORD,	wCityCurrentMonth,			0x4CB01C)
GAMEOFF(WORD,	wCityElapsedYears,			0x4CB020)
GAMEOFF(DWORD,	dwNewspaperSubscription,	0x4CB3D0)
GAMEOFF(WORD,	wCityCurrentSeason,			0x4CB3E8)
GAMEOFF(WORD,	wCityDifficulty,			0x4CB404)
GAMEOFF(BYTE,	bWeatherTrend,				0x4CB40C)
GAMEOFF(DWORD,	dwNewspaperExtra,			0x4CC4BC)
GAMEOFF(BOOL,	dwNoDisasters,				0x4CC4D4)
GAMEOFF(BYTE,	bMilitaryBaseType,			0x4CC4E4)
GAMEOFF(DWORD,	dwCityBonds,				0x4CC4E8)
GAMEOFF(DWORD,	dwPRNGState,				0x4CDB80)
GAMEOFF(DWORD,	dwSimulationSubtickCounter,	0x4E63D8)
GAMEOFF_ARR(DWORD, dwCityProgressionRequirements, 0x4E6984)
GAMEOFF(DWORD,	dwNextRefocusSongID,		0x4E6F8C)
GAMEOFF_ARR(DWORD, dwZoneNameStringIDs,		0x4E7140)
GAMEOFF_ARR(DWORD, dwCityNoticeStringIDs,	0x4E98B8)
GAMEOFF(DWORD,	dwCityRewardsUnlocked,		0x4E9A24)


// Pointers to map arrays

GAMEOFF_ARR(map_XTER_t,	dwMapXTER,	0x4C9F58)
GAMEOFF_ARR(map_XZON_t,	dwMapXZON,	0x4CA1F0)
GAMEOFF_ARR(map_XTXT_t,	dwMapXTXT,	0x4CA600)
GAMEOFF_ARR(map_XBIT_t,	dwMapXBIT,	0x4CAB10)
GAMEOFF_ARR(map_ALTM_t,	dwMapALTM,	0x4CAE10)
GAMEOFF_ARR(map_XUND_t,	dwMapXUND,	0x4CB1D0)
GAMEOFF_ARR(map_XBLD_t,	dwMapXBLD,	0x4CC4F0)
