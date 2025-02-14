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

typedef struct {
	BYTE bDunno[32];
} neighbor_city_t;

// Pointers

GAMEOFF(WORD,	wSimulationSpeed,			0x4C7318)
GAMEOFF(WORD,	wDisasterType,				0x4CA420)
GAMEOFF(DWORD,	dwCityLandValue,			0x4CA440)
GAMEOFF(DWORD,	dwCityFunds,				0x4CA444)
GAMEOFF(WORD,	wCityStartYear,				0x4CA5F4)
GAMEOFF(DWORD,	dwCityPopulation,			0x4CAA74)
GAMEOFF(DWORD,	dwInScenario,				0x4CAD44)
GAMEOFF_ARR(neighbor_city_t, stNeighborCities, 0x4CAD58)
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

