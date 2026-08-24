// sc2kfix hooks/hook_tilegrowthorplacement.cpp: tile placement and growth handling
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE 2026-08-23 (araxestroy): This is rapidly becoming a large chunk of the simulation engine
// and needs to be split out a bit before it turns into another hook_sc2k1996_miscellaneous.cpp.

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

#pragma intrinsic(_ReturnAddress)

#if MAP_EDGE_BUILDING == 2
#define ABSOLUTE_MIN_EDGE MAP_EDGE_MIN
#define ABSOLUTE_MAX_EDGE MAP_EDGE_MAX

#define AREA_2x2_MIN_EDGE MAP_EDGE_MIN
#define AREA_2x2_MAX_EDGE MAP_EDGE_MAX - 1

#define AREA_3x3_MIN_EDGE MAP_EDGE_MIN + 1
#define AREA_3x3_MAX_EDGE AREA_2x2_MAX_EDGE

#define AREA_4x4_MIN_EDGE AREA_3x3_MIN_EDGE
#define AREA_4x4_MAX_EDGE MAP_EDGE_MAX - 2
#else

#if MAP_EDGE_BUILDING == 1
#define ABSOLUTE_MIN_EDGE MAP_EDGE_MIN
#define ABSOLUTE_MAX_EDGE MAP_EDGE_MAX
#else
#define ABSOLUTE_MIN_EDGE MAP_EDGE_MIN + 1
#define ABSOLUTE_MAX_EDGE MAP_EDGE_MAX - 1
#endif

#define AREA_2x2_MIN_EDGE MAP_EDGE_MIN + 1
#define AREA_2x2_MAX_EDGE MAP_EDGE_MAX - 2

#define AREA_3x3_MIN_EDGE MAP_EDGE_MIN + 2
#define AREA_3x3_MAX_EDGE AREA_2x2_MAX_EDGE

#define AREA_4x4_MIN_EDGE AREA_3x3_MIN_EDGE
#define AREA_4x4_MAX_EDGE MAP_EDGE_MAX - 3
#endif

// Internal defines to turn bits of the reimplemented trip generator on and off, if the command
// line option -experiment=tripgenerator is passed (or dwExperimentsEnabled is set to
// EXPERIMENT_TRIPGENERATOR by default).
// WARNING: USE_NATIVE_STACKS breaks things and is intended for experimentation. It will soon be
// replaced with something a little less finicky.

#define USE_NEW_TRIP_GENERATOR		1		// use the recompiled trip generator
#define USE_NEW_STARTINGCOORDS		1		// use our own starting coords code
#define USE_NATIVE_STACKS			1		// use std::stack instead of SC2K's shared point stack

#define SHUFFLE_TRIP_GENERATOR		0		// shuffles dwTripStartingCoords for added randomness

// Debug flags
// TILEBUILD_DEBUG_TRIP_* are meant to turn on deeper levels of trip generator output when
// debugging the reimplementation and extensions. Be very careful turning them on.

#define TILEBUILD_DEBUG_OTHER		1
#define TILEBUILD_DEBUG_SPRITES		2
#define TILEBUILD_DEBUG_TILESETS	4
#define TILEBUILD_DEBUG_BUILDING	8
#define TILEBUILD_DEBUG_TRIP_DUMP	16		// dumps trip start/end info to console + log
#define TILEBUILD_DEBUG_TRIP_LOG	32		// I AM DEATH INCARNATE
#define TILEBUILD_DEBUG_DEMOLISH	64

#define TILEBUILD_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef TILEBUILD_DEBUG
#define TILEBUILD_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT tilebuild_debug = TILEBUILD_DEBUG;

#if USE_NEW_STARTINGCOORDS
CPoint dwTripStartingCoords[24] = {
	{ 0, 1 },
	{ 1, 0 },
	{ 0, -1 },
	{ -1, 0 },
	{ 0, 2 },
	{ 2, 0 },
	{ 0, -2 },
	{ -2, 0 },
	{ 0, 3 },
	{ 3, 0 },
	{ 0, -3 },
	{ -3, 0 },
	{ 1, 1 },
	{ -1, 1 },
	{ 1, -1 },
	{ -1, -1 },
	{ 2, 1 },
	{ -2, 1 },
	{ 2, -1 },
	{ -2, -1 },
	{ 1, 2 },
	{ -1, 2 },
	{ 1, -2 },
	{ -1, -2 }
};

static int IsValidTransitItems(mapcoord_t x, mapcoord_t y) {
	for (int i = 0; i < 24; i++) {
		mapcoord_t newX = x + (mapcoord_t)dwTripStartingCoords[i].x;
		mapcoord_t newY = y + (mapcoord_t)dwTripStartingCoords[i].y;

		if ((newX < MAP_EDGE_MIN || newX > MAP_EDGE_MAX) ||
			(newY < MAP_EDGE_MIN || newY > MAP_EDGE_MAX))
			continue;

		uint32_t iTileID = GetTileID(newX, newY);

		if (iTileID >= TILE_ROAD_LR && iTileID < TILE_RAIL_LR)
			return 1;

		if (iTileID >= TILE_TUNNEL_T && iTileID < TILE_CROSSOVER_POWERTB_RAILLR
			|| iTileID == TILE_CROSSOVER_HIGHWAYLR_ROADTB
			|| iTileID == TILE_CROSSOVER_HIGHWAYTB_ROADLR
			|| iTileID >= TILE_ONRAMP_TL && iTileID < TILE_HIGHWAY_HTB)
			return 1;

		if (iTileID == TILE_INFRASTRUCTURE_BUSDEPOT
			|| iTileID == TILE_INFRASTRUCTURE_RAILSTATION
			|| iTileID == TILE_INFRASTRUCTURE_SUBWAYSTATION)
			return 1;
	}

	return 0;
}
#else
static CPoint* dwTripStartingCoords = (CPoint*)0x4C92C0;
#endif

#define TRIP_SCALE_FACTOR_STEP		1		// scaling for cars/busses/pedestrians
#define TRIP_SCALE_FACTOR_CHANGE	1		// scaling for transit type changes
#define TRIP_SCALE_FACTOR_RAPID		1		// scaling for rapid transit (highway/rail/subway)

#define TRIP_MAX_STEPS_VANILLA		100
#define TRIP_MAX_STEPS_SCALE		1

#define TRIP_MAX_ATTEMPTS			4		// vanilla: 4
#define TRIP_ITERATION_LOG_SIZE		512		// vanilla: 512

// TODO: get these cleaned up and moved into sc2k_1996.h

mapcoord_t wTripX[] = { 0, 1, 0, -1 };
mapcoord_t wTripY[] = { -1, 0, 1, 0 };
int16_t& FTop = *(int16_t*)0x4CA424;
int16_t& FBot = *(int16_t*)0x4CC908;
WORD* wArrZoneDestinations = (WORD*)0x4E8570;
BYTE* byte_4E858C = (BYTE*)0x4E858C;

#pragma pack(push, 1)
typedef struct {
	int16_t iCurrentTransitType;
	int16_t iIterationCount;
	int16_t iDunno1;
	int16_t iMaybeDirection;
	int16_t iDunno2;
	WORD wDunno3;
	DWORD bTripCompleted;
	CMFC3XPoint ptTripNextLocation;
	CMFC3XPoint ptTripCurrentLocation;
	int array1[TRIP_ITERATION_LOG_SIZE];
	int array2[TRIP_ITERATION_LOG_SIZE];
	int array3[TRIP_ITERATION_LOG_SIZE];
} tripStruct;
#pragma pack(pop)

static int iTotalTripCount = 0;

// This is REALLY rough and has only had enough cleanup to make sense in my head so it probably
// won't make much sense to anyone else yet.
NEWENGINE int Simulation_RunTripGenerator(mapcoord_t x, mapcoord_t y, int16_t nZoneType, int nBuildingPopLevel, int nTripMaxSteps) {
	uint32_t iTileID;
	int16_t iTripCurrentSteps;
	int v9;
	int16_t var_61A;
	int16_t n15_1;
	BOOL bUsedRail = FALSE;
	BOOL bUsedSubway = FALSE;
	BOOL bUsedBus = FALSE;
	int16_t iTransitType;

	map_mini_half_t* bXTRFData;
	unsigned int iBuffer = 0;
	unsigned int iBuffer2 = 0;
	tripStruct stTripData;
	std::stack<CPoint> stackTripPoints;
		
	memset(&stTripData, 0, sizeof(tripStruct));
	iTotalTripCount++;

	// Shuffle the trip generator's search order if requested. This adds a bit more randomness to
	// the traffic simulation, but at unknown costs (seriously, I don't know how this will affect
	// gameplay, test at your own risk).
	if (SHUFFLE_TRIP_GENERATOR && USE_NEW_STARTINGCOORDS)
		std::shuffle(dwTripStartingCoords, dwTripStartingCoords + 24, mtMersenneTwister);

	// See if we can find a transit type within 3 tiles.
	stTripData.iCurrentTransitType = TRANSIT_TYPE_NONE;
	for (int iTripStartAttempt = 0; iTripStartAttempt < 24; iTripStartAttempt++) {
		if (stTripData.iCurrentTransitType >= 0)
			break;

		SetCPoint(&stTripData.ptTripCurrentLocation, x + dwTripStartingCoords[iTripStartAttempt].x, y + dwTripStartingCoords[iTripStartAttempt].y);

		if (stTripData.ptTripCurrentLocation.x < GAME_MAP_SIZE && stTripData.ptTripCurrentLocation.y < GAME_MAP_SIZE) {
			iTileID = GetTileID((short)stTripData.ptTripCurrentLocation.x, (short)stTripData.ptTripCurrentLocation.y);
			if (TILE_IS_ROAD(iTileID))
				stTripData.iCurrentTransitType = TRANSIT_TYPE_ROAD;
			else {
				switch (iTileID) {
				case TILE_INFRASTRUCTURE_BUSDEPOT:
					stTripData.iCurrentTransitType = TRANSIT_TYPE_BUS;
					break;
				case TILE_INFRASTRUCTURE_RAILSTATION:
					stTripData.iCurrentTransitType = TRANSIT_TYPE_RAIL_ENTER;
					break;
				case TILE_INFRASTRUCTURE_SUBWAYSTATION:
					stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY_ENTER;
					break;
				}
			}
		}
	}

	// If we didn't find a transit type within 3 tiles, bail out now.
	if (stTripData.iCurrentTransitType == TRANSIT_TYPE_NONE)
		return 0;

#if !USE_NATIVE_STACKS
	Game_InitStack((short)stTripData.ptTripCurrentLocation.x, (short)stTripData.ptTripCurrentLocation.y);
#endif

	// Set up the initial variables for the trip
	iTripCurrentSteps = 0;
	stTripData.iDunno1 = 15;
	stTripData.array2[0] = 15;
	stTripData.array1[0] = stTripData.iCurrentTransitType;
	stTripData.bTripCompleted = 0;
	stTripData.iIterationCount = 1;
	*(DWORD*)&stTripData.wDunno3 = (unsigned __int16)(2 * (rand() & 1) + 1);
	if (nBuildingPopLevel == 1)
		nTripMaxSteps -= nTripMaxSteps >> 2;

	if (tilebuild_debug & TILEBUILD_DEBUG_TRIP_DUMP) {
		ConsoleLog(LOG_DEBUG, "TRIP: New trip! Starting tile is (%d, %d), zone type %d.\n", x, y, nZoneType);
		ConsoleLog(LOG_DEBUG, "TRIP: Starting with %s from position (%d, %d).\n", GetTransitTypeName(stTripData.iCurrentTransitType), stTripData.ptTripCurrentLocation.x, stTripData.ptTripCurrentLocation.y);
	}

	while (nTripMaxSteps > iTripCurrentSteps) {
		stackTripPoints = {};
		v9 = 0;
		stTripData.iMaybeDirection = rand() & 3;
		stTripData.iDunno2 = 0;
		do {
			if (v9)
				goto FINISHTRIP;

			var_61A = (LOBYTE(stTripData.iMaybeDirection) + LOBYTE(stTripData.wDunno3)) & 3;
			stTripData.iMaybeDirection = var_61A;
			if ((stTripData.iDunno1 & (1 << var_61A)) != 0) {
				stTripData.iDunno1 += 0xFFFF << var_61A;
				SetCPoint(
					&stTripData.ptTripNextLocation,
					LOWORD(stTripData.ptTripCurrentLocation.x) + wTripX[var_61A],
					LOWORD(stTripData.ptTripCurrentLocation.y) + wTripY[var_61A]);
				if (stTripData.ptTripNextLocation.x >= GAME_MAP_SIZE || stTripData.ptTripNextLocation.y >= GAME_MAP_SIZE) {
					if (dwMapXTXT[stTripData.ptTripCurrentLocation.x][stTripData.ptTripCurrentLocation.y].bTextOverlay == 250) {
TRIPSUCCESS:
						v9 = 1;
						stTripData.bTripCompleted = 1;
						continue;
					}
				} else {
					switch (stTripData.iCurrentTransitType) {
					case TRANSIT_TYPE_ROAD:
						if (((unsigned __int16)(1 << (*(BYTE*)&dwMapXZON[SLOWORD(stTripData.ptTripNextLocation.x)][SLOWORD(stTripData.ptTripNextLocation.y)].b & 0xF)) & (unsigned __int16)wArrZoneDestinations[nZoneType]) != 0)
							goto TRIPSUCCESS;

						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);
						
						// TODO: rework this for clarity
						if (iTileID < TILE_TUNNEL_T || iTileID >= TILE_CROSSOVER_POWERTB_ROADLR) {
							if (iTileID >= TILE_SUSPENSION_BRIDGE_START_B
								&& iTileID < TILE_ONRAMP_TL
								|| iTileID == TILE_REINFORCED_BRIDGE_PYLON
								|| iTileID == TILE_REINFORCED_BRIDGE) {
								iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_ROADBRIDGE;
							} else if (iTileID < TILE_ONRAMP_TL || iTileID >= TILE_HIGHWAY_HTB) {
								if (TILE_IS_ROAD_WITH_SIDEWALKS(iTileID)) {
									iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
									v9 = 1;
								} else {
									switch (iTileID) {
									case TILE_INFRASTRUCTURE_BUSDEPOT:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_BUS;
										break;
									case TILE_INFRASTRUCTURE_RAILSTATION:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_RAIL_ENTER;
										break;
									case TILE_INFRASTRUCTURE_SUBWAYSTATION:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY_ENTER;
										break;
									default:
										if (iTileID > TILE_INFRASTRUCTURE_DESALINIZATIONPLANT)
											goto TRIPSUCCESS;
										break;
									}
								}
							} else {
								iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_HIGHWAY;
							}
						} else {
							iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_TUNNEL;
						}

						break;

					case TRANSIT_TYPE_HIGHWAY:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if ((iTileID < 0x61u || iTileID >= 0x6Cu) && (iTileID < 0x49u || iTileID >= 0x51u)) {
							if (iTileID >= 0x5Du && iTileID < 0x61u) {
								stTripData.iCurrentTransitType = TRANSIT_TYPE_ROAD;
								v9 = 1;
								iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
							}
						} else {
							v9 = 1;
							iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
						}

						break;

					case TRANSIT_TYPE_TUNNEL:
						if ((*((BYTE*)&dwMapALTM[stTripData.ptTripNextLocation.x][stTripData.ptTripNextLocation.y].w + 1) & 0x7C) != 0) {
							iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
						} else {
							iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

							if (TILE_IS_ROAD(iTileID)) {
								iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_ROAD;
							}
						}

						break;

					case TRANSIT_TYPE_ROADBRIDGE:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID >= (unsigned int)TILE_SUSPENSION_BRIDGE_START_B && iTileID < (unsigned int)TILE_ONRAMP_TL
							|| iTileID == TILE_REINFORCED_BRIDGE_PYLON
							|| iTileID == TILE_REINFORCED_BRIDGE) {
							iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
						} else if (TILE_IS_ROAD(iTileID)) {
							iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_ROAD;
						}

						break;

					case TRANSIT_TYPE_PEDESTRIAN:
						if (((unsigned __int16)(1 << (*(BYTE*)&dwMapXZON[SLOWORD(stTripData.ptTripNextLocation.x)][SLOWORD(stTripData.ptTripNextLocation.y)].b & 0xF)) & (unsigned __int16)wArrZoneDestinations[nZoneType]) != 0)
							goto TRIPSUCCESS;

						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID < TILE_TUNNEL_T || iTileID >= TILE_CROSSOVER_POWERTB_ROADLR) {
							if (iTileID >= TILE_TUNNEL_T && iTileID < TILE_ONRAMP_TL || iTileID == TILE_REINFORCED_BRIDGE_PYLON || iTileID == TILE_REINFORCED_BRIDGE) {
								iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN_UNDERPASS2;
							} else if (iTileID < (unsigned int)TILE_ONRAMP_TL || iTileID >= (unsigned int)TILE_HIGHWAY_HTB) {
								if (TILE_IS_ROAD_WITH_SIDEWALKS(iTileID)) {
									iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
									v9 = 1;
								} else {
									switch (iTileID) {
									case TILE_INFRASTRUCTURE_BUSDEPOT:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_BUS;
										break;
									case TILE_INFRASTRUCTURE_RAILSTATION:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_RAIL_ENTER;
										break;
									case TILE_INFRASTRUCTURE_SUBWAYSTATION:
										iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
										v9 = 1;
										stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY_ENTER;
										break;
									default:
										if (iTileID > (unsigned int)TILE_INFRASTRUCTURE_DESALINIZATIONPLANT)
											goto TRIPSUCCESS;
										break;
									}
								}
							} else {
								iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN_UNDERPASS;
							}
						} else {
							iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN_TUNNEL;
						}

						break;

					// TODO: determine if this is actually what it seems to be
					case TRANSIT_TYPE_PEDESTRIAN_UNDERPASS:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if ((iTileID < (unsigned int)TILE_HIGHWAY_HTB || iTileID >= (unsigned int)TILE_SUBTORAIL_T)
							&& (iTileID < (unsigned int)TILE_HIGHWAY_LR || iTileID >= (unsigned int)TILE_SUSPENSION_BRIDGE_START_B)) {
							if (iTileID >= (unsigned int)TILE_ONRAMP_TL && iTileID < (unsigned int)TILE_HIGHWAY_HTB) {
								stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN;
								v9 = 1;
								iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_STEP;
							}
						} else {
							v9 = 1;
							iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_STEP;
						}

						break;

					// TODO: determine if this is actually what it seems to be
					case TRANSIT_TYPE_PEDESTRIAN_TUNNEL:
						if ((*((BYTE*)&dwMapALTM[stTripData.ptTripNextLocation.x][stTripData.ptTripNextLocation.y].w + 1) & 0x7C) != 0) {
							iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
						} else {
							iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

							if (TILE_IS_ROAD(iTileID)) {
								iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
								v9 = 1;
								stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN;
							}
						}

						break;

					// TODO: determine if this is actually what it seems to be
					case TRANSIT_TYPE_PEDESTRIAN_UNDERPASS2:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID >= TILE_SUSPENSION_BRIDGE_START_B && iTileID < TILE_ONRAMP_TL || iTileID == TILE_REINFORCED_BRIDGE_PYLON || iTileID == TILE_REINFORCED_BRIDGE) {
							iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
						} else if (TILE_IS_ROAD(iTileID)) {
							iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN;
						}

						break;

					case TRANSIT_TYPE_BUS:
						if (((unsigned __int16)(1 << (*(BYTE*)&dwMapXZON[SLOWORD(stTripData.ptTripNextLocation.x)][SLOWORD(stTripData.ptTripNextLocation.y)].b & 0xF)) & (unsigned __int16)wArrZoneDestinations[nZoneType]) != 0)
							goto TRIPSUCCESS;

						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID == TILE_INFRASTRUCTURE_BUSDEPOT) {
							iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
							v9 = 1;
						} else if (TILE_IS_ROAD(iTileID)) {
							iTripCurrentSteps += 2 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_PEDESTRIAN;
						}

						break;

					case TRANSIT_TYPE_SUBWAY_EXIT:
						if (((unsigned __int16)(1 << (*(BYTE*)&dwMapXZON[SLOWORD(stTripData.ptTripNextLocation.x)][SLOWORD(stTripData.ptTripNextLocation.y)].b & 0xF)) & (unsigned __int16)wArrZoneDestinations[nZoneType]) != 0)
							goto TRIPSUCCESS;

						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID == TILE_INFRASTRUCTURE_BUSDEPOT || iTileID == TILE_INFRASTRUCTURE_RAILSTATION) {
							iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
							v9 = 1;
						} else if (TILE_IS_ROAD(iTileID)) {
							iTripCurrentSteps += 3 * TRIP_SCALE_FACTOR_STEP;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_ROAD;
						}

						break;

					case TRANSIT_TYPE_RAIL_ENTER:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID == TILE_INFRASTRUCTURE_RAILSTATION) {
							iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
							v9 = 1;
						} else if (TILE_IS_RAIL(iTileID)) {
							stTripData.iCurrentTransitType = TRANSIT_TYPE_RAIL;
							v9 = 1;
							iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
						}

						break;

					case TRANSIT_TYPE_SUBWAY_ENTER:
						iTileID = dwMapXUND[stTripData.ptTripNextLocation.x][stTripData.ptTripNextLocation.y].iTileID;

						if (iTileID && iTileID < UNDER_TILE_PIPES_LR
							|| iTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR
							|| iTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB
							|| iTileID == UNDER_TILE_MISSILESILO	// I assure you, I have clearance
							|| iTileID == UNDER_TILE_SUBWAYENTRANCE) {
							stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY;
							v9 = 1;
							iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
						}

						break;

					case TRANSIT_TYPE_RAIL:
						iTileID = GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);

						if (iTileID == TILE_INFRASTRUCTURE_RAILSTATION) {
							iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY_EXIT;
						} else if (TILE_IS_RAIL(iTileID)) {
							v9 = 1;
							iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
						} else if (iTileID > 0xFAu)
							goto TRIPSUCCESS;

						break;

					case TRANSIT_TYPE_SUBWAY:
						if (GetTileID((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y) == TILE_INFRASTRUCTURE_SUBWAYSTATION) {
							iTripCurrentSteps += 4 * TRIP_SCALE_FACTOR_CHANGE;
							v9 = 1;
							stTripData.iCurrentTransitType = TRANSIT_TYPE_SUBWAY_EXIT;
						} else {
							iTileID = dwMapXUND[stTripData.ptTripNextLocation.x][stTripData.ptTripNextLocation.y].iTileID;
							if (iTileID && iTileID < UNDER_TILE_PIPES_LR
								|| iTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR
								|| iTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB
								|| iTileID == UNDER_TILE_MISSILESILO	// I assure you, I *still* have clearance
								|| iTileID == UNDER_TILE_SUBWAYENTRANCE) {
								v9 = 1;
								iTripCurrentSteps += 1 * TRIP_SCALE_FACTOR_RAPID;
							}
						}

						break;

					default:
						ConsoleLog(LOG_NOTICE, "TRIP: Got weird iCurrentTransitType %d. Maybe tell a developer if this keeps happening.\n", stTripData.iCurrentTransitType);
						break;
					}
				}
			}
			++stTripData.iDunno2;
		} while (stTripData.iDunno2 < TRIP_MAX_ATTEMPTS);

		if (v9) {
FINISHTRIP:
			if (stTripData.bTripCompleted)
				break;
			stTripData.ptTripCurrentLocation = stTripData.ptTripNextLocation;
			stTripData.array2[stTripData.iIterationCount - 1] = stTripData.iDunno1;

#if USE_NATIVE_STACKS
			stackTripPoints.push({ stTripData.ptTripNextLocation.x, stTripData.ptTripNextLocation.y });
#else
			Game_StackPush((short)stTripData.ptTripNextLocation.x, (short)stTripData.ptTripNextLocation.y);
#endif

			if (stTripData.iCurrentTransitType == TRANSIT_TYPE_ROADBRIDGE || stTripData.iCurrentTransitType == TRANSIT_TYPE_PEDESTRIAN_UNDERPASS2)
				n15_1 = 1 << SLOBYTE(stTripData.iMaybeDirection);
			else {
				if (stTripData.iCurrentTransitType == TRANSIT_TYPE_SUBWAY_ENTER) {
					stTripData.iDunno1 = 15;
LABEL_229:
					stTripData.array2[stTripData.iIterationCount] = stTripData.iDunno1;
					stTripData.array3[stTripData.iIterationCount] = iTripCurrentSteps;
					stTripData.array1[stTripData.iIterationCount] = stTripData.iCurrentTransitType;

					if (tilebuild_debug & TILEBUILD_DEBUG_TRIP_DUMP)
						if (tilebuild_debug & TILEBUILD_DEBUG_TRIP_LOG)
							ConsoleLog(LOG_DEBUG, "TRIP: Iteration %d (%d steps): (%d, %d), %s.\n",
								stTripData.iIterationCount, iTripCurrentSteps,
								stTripData.ptTripCurrentLocation.x, stTripData.ptTripCurrentLocation.y,
								GetTransitTypeName(stTripData.iCurrentTransitType));
						else
							printf("[DEBUG] TRIP: Iteration %d (%d steps): (%d, %d), %s. [nolog]\n",
								stTripData.iIterationCount, iTripCurrentSteps,
								stTripData.ptTripCurrentLocation.x, stTripData.ptTripCurrentLocation.y,
								GetTransitTypeName(stTripData.iCurrentTransitType));

					stTripData.iIterationCount++;
					goto LABEL_236;
				}
				n15_1 = (unsigned __int8)byte_4E858C[stTripData.iMaybeDirection];
			}
			stTripData.iDunno1 = n15_1;
			goto LABEL_229;
		}
		do {
			if (--stTripData.iIterationCount > 0) {
#if USE_NATIVE_STACKS
				if (!stackTripPoints.empty()) {
					stackTripPoints.pop();
					stTripData.ptTripCurrentLocation.x = stackTripPoints.top().x;
					stTripData.ptTripCurrentLocation.y = stackTripPoints.top().y;
				}
#else
				Game_StackPop(&stTripData.ptTripCurrentLocation);
				Game_StackPeek(&stTripData.ptTripCurrentLocation);
#endif

				stTripData.iCurrentTransitType = *((unsigned __int8*)&stTripData.ptTripCurrentLocation.y + stTripData.iIterationCount + 3); // XXX - wtf?
				iTripCurrentSteps = stTripData.array3[stTripData.iIterationCount - 1];
				stTripData.iDunno1 = stTripData.array2[stTripData.iIterationCount - 1];
			}
		} while (!stTripData.iDunno1 && stTripData.iIterationCount > 0);
		if (!stTripData.iIterationCount)
			iTripCurrentSteps = nTripMaxSteps;
LABEL_236:
		if (stTripData.bTripCompleted)
			break;
	}

	// Iterate through our trip data to see what kinds of effects we had (traffic usage, public
	// transit load, etc).
	if (stTripData.bTripCompleted) {
		if (tilebuild_debug & TILEBUILD_DEBUG_TRIP_DUMP)
			ConsoleLog(LOG_DEBUG, "TRIP: Trip completed, made it to (%d, %d).\n", stTripData.ptTripNextLocation.x, stTripData.ptTripNextLocation.y);

		// TODO: don't manipulate the point-stack data directly.
		// TODO: magic numbers
#if USE_NATIVE_STACKS
		if (nBuildingPopLevel > 0 && !stackTripPoints.empty()) {
#else
		if (nBuildingPopLevel > 0 && FBot != FTop) {
#endif
			do {
#if USE_NATIVE_STACKS
				stTripData.ptTripCurrentLocation.x = stackTripPoints.top().x;
				stTripData.ptTripCurrentLocation.y = stackTripPoints.top().y;
				stackTripPoints.pop();
#else
				Game_StackPop(&stTripData.ptTripCurrentLocation);
#endif

				if (--stTripData.iIterationCount < 0)
					stTripData.iIterationCount = 0;
				iTransitType = stTripData.array1[stTripData.iIterationCount];

				if (iTransitType == TRANSIT_TYPE_SUBWAY_ENTER)
					bUsedSubway = TRUE;
				if (iTransitType == TRANSIT_TYPE_RAIL_ENTER)
					bUsedRail = TRUE;
				if (iTransitType == TRANSIT_TYPE_BUS)
					bUsedBus = TRUE;

				if (iTransitType < TRANSIT_TYPE_TUNNEL || iTransitType == TRANSIT_TYPE_ROADBRIDGE) {
					bXTRFData = &dwMapXTRF[stTripData.ptTripCurrentLocation.x / 2][stTripData.ptTripCurrentLocation.y / 2];
					bXTRFData->bBlock = ((nBuildingPopLevel + bXTRFData->bBlock) > 0xFF ? 0xFF : (nBuildingPopLevel + bXTRFData->bBlock));
				}
#if USE_NATIVE_STACKS
			} while (!stackTripPoints.empty());
#else
			} while (FBot != FTop);
#endif
		}
		//printf("\n");
	} else
		if (tilebuild_debug & TILEBUILD_DEBUG_TRIP_DUMP)
			ConsoleLog(LOG_DEBUG, "TRIP: Failed to generate trip. :(\n", stTripData.ptTripNextLocation.x, stTripData.ptTripNextLocation.y);

	if (bUsedSubway)
		dwSubwayPassengers += nBuildingPopLevel;
	if (bUsedRail)
		dwRailPassengers += nBuildingPopLevel;
	if (bUsedBus)
		dwBusPassengers += nBuildingPopLevel;
	return stTripData.bTripCompleted;
}

static void GetItemPlacementAreaAndFarPosition(mapcoord_t m_x, mapcoord_t m_y, int16_t iTileArea, mapcoord_t* outX, mapcoord_t* outY, mapcoord_t* outFarX, mapcoord_t* outFarY, int16_t* outArea) {
	mapcoord_t x, y;
	int16_t iArea;

	x = m_x;
	y = m_y;

	iArea = iTileArea - 1;
	if (iArea > 1) {
		--x;
		--y;
	}

	*outX = x;
	*outY = y;
	*outFarX = iArea + x;
	*outFarY = iArea + y;
	*outArea = iArea;
}

static bool IsValidGeneralPosPlacementMain(mapcoord_t x, mapcoord_t y, mapcoord_t iFarX, mapcoord_t iFarY, int16_t iArea, BYTE iTileID, bool bDoSilo, bool bSiloPlotCheck, int16_t* outMarinaWaterTileCount) {
	mapcoord_t iCurX, iCurY;
	int16_t iMarinaWaterTileCount;
	bool bCanBeMarinaTile;
	BYTE iCurTile;

	iMarinaWaterTileCount = 0;
	for (iCurX = x; iCurX <= iFarX; ++iCurX) {
		for (iCurY = y; iCurY <= iFarY; ++iCurY) {
			// if the extended iArea is zero (or below..) and the current X or Y
			// tiles are equal to or exceed GAME_MAP_SIZE.. definitely abort.
			if (iArea <= 0) {
				if (iCurX < MAP_EDGE_MIN || iCurY < MAP_EDGE_MIN || iCurX > MAP_EDGE_MAX || iCurY > MAP_EDGE_MAX)
					return false;
			}
			else if (iCurX < ABSOLUTE_MIN_EDGE || iCurY < ABSOLUTE_MIN_EDGE || iCurX > ABSOLUTE_MAX_EDGE || iCurY > ABSOLUTE_MAX_EDGE) {
				// Added this due to legacy military plot drops,
				// this allows > 1x1 type buildings to develop
				// if the plot is on the edge of the map.
				if (!bDoSilo) {
					if (XZONReturnZone(iCurX, iCurY) == ZONE_MILITARY && (iCurX < MAP_EDGE_MIN || iCurY < MAP_EDGE_MIN || iCurX > MAP_EDGE_MAX || iCurY > MAP_EDGE_MAX))
						return false;
					else
						return false;
				}
				else
					return false;
			}

			// If the current tile has the referenced
			// item.
			iCurTile = GetTileID(iCurX, iCurY);
			if (iCurTile >= TILE_ROAD_LR)
				return false;

			if (iCurTile == TILE_RADIOACTIVITY)
				return false;

			if (iCurTile == TILE_SMALLPARK)
				return false;

			// !bDoSilo case:
			// Originally in the Win95 version this check
			// only did a comparison regarding the current
			// tile zone being ZONE_MILITARY, as a result
			// it would return 0 and military bases wouldn't
			// grow; now it checks to see whether the current
			// tile zone is ZONE_MILITARY, and whether the
			// tile item is a runwaycross, or certain road tiles -
			// this then prevents either:
			// a) erroneous growth attempts if said tiles are
			//    destroyed (particularly on Army Base plots)
			// b) blank sections being left on Army Base plots
			//    as a result of the presence of said tiles -
			//    particular the road tiles - you'd then see
			//    the 'Hanger' (nice typing error there..)
			//    constantly spawn and despawn resulting
			//    in many unnecessary calls.
			if (!bDoSilo) {
				if (XZONReturnZone(iCurX, iCurY) == ZONE_MILITARY) {
					if (TILE_IS_MILITARY(iCurTile) ||
						iCurTile == TILE_ROAD_LR ||
						iCurTile == TILE_ROAD_TB)
						return false;
				}
			}
			else {
				if (bSiloPlotCheck) {
					if (XZONReturnZone(iCurX, iCurY) != ZONE_NONE)
						return false;
				}
				else {
					if (XZONReturnZone(iCurX, iCurY) != ZONE_MILITARY)
						return false;
					else {
						if (TILE_IS_MILITARY(iCurTile) ||
							iCurTile == TILE_ROAD_LR ||
							iCurTile == TILE_ROAD_TB)
							return false;
					}
				}
			}

			// Marina being an exception, this 'if' block
			// checks to see whether the prospective area
			// is suitable for placement.
			bCanBeMarinaTile = false;
			if (iTileID == TILE_INFRASTRUCTURE_MARINA) {
				if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE && XBITReturnIsWater(iCurX, iCurY)) {
					++iMarinaWaterTileCount;
					bCanBeMarinaTile = true;
				}
			}

			// This check shouldn't occur if 'bCanBeMarinaTile'
			// is true, since the Marina needs to be placed
			// across shorelines, and a block to prevent
			// placement on shores or water bearing tiles
			// would negate that entirely.
			if (!bCanBeMarinaTile) {
				if (GetTerrainTileID(iCurX, iCurY))
					return false;

				if (bDoSilo) {
					if (GetUndergroundTileID(iCurX, iCurY))
						return false;
				}

				if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE && XBITReturnIsWater(iCurX, iCurY))
					return false;
			}
		}
	}

	*outMarinaWaterTileCount = iMarinaWaterTileCount;
	return true;
}

int IsValidSiloPosCheck(mapcoord_t m_x, mapcoord_t m_y) {
	mapcoord_t x, y;
	mapcoord_t iFarX, iFarY;
	int16_t iArea;
	int16_t iDummy;

	GetItemPlacementAreaAndFarPosition(m_x, m_y, AREA_3x3, &x, &y, &iFarX, &iFarY, &iArea);

	return IsValidGeneralPosPlacementMain(x, y, iFarX, iFarY, iArea, TILE_MILITARY_MISSILESILO, true, true, &iDummy);
}

static int IsValidGeneralPosPlacement(mapcoord_t x, mapcoord_t y, mapcoord_t iFarX, mapcoord_t iFarY, int16_t iArea, BYTE iTileID, bool bDoSilo, int16_t* outMarinaWaterTileCount) {
	return IsValidGeneralPosPlacementMain(x, y, iFarX, iFarY, iArea, iTileID, bDoSilo, false, outMarinaWaterTileCount);
}

int L_ItemPlacementCheck(mapcoord_t m_x, mapcoord_t m_y, BYTE iTileID, int16_t iTileArea, bool bDoSilo) {
	mapcoord_t x, y;
	mapcoord_t iCurX, iCurY;
	mapcoord_t iFarX, iFarY;
	int16_t iArea;
	int16_t iMarinaWaterTileCount;
	BYTE iTileBitMask;
	BYTE bTextOverlay;

	GetItemPlacementAreaAndFarPosition(m_x, m_y, iTileArea, &x, &y, &iFarX, &iFarY, &iArea);

	iMarinaWaterTileCount = 0;
	if (!IsValidGeneralPosPlacement(x, y, iFarX, iFarY, iArea, iTileID, bDoSilo, &iMarinaWaterTileCount))
		return 0;

	if (iTileID == TILE_INFRASTRUCTURE_MARINA && (iMarinaWaterTileCount == MARINA_TILES_ALLDRY || iMarinaWaterTileCount == MARINA_TILES_ALLWET)) {
		GameMain_AfxMessageBoxID(107, 0, -1);
		return 0;
	}
	else {
		iTileBitMask = 0;
		if (!bDoSilo) {
			iTileBitMask = (XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
			if (iTileID == TILE_SERVICES_BIGPARK || iTileID == TILE_SMALLPARK)
				iTileBitMask = (XBIT_PIPED);
		}
		if (iTileID == TILE_SMALLPARK && GetTileID(x, y) >= TILE_SMALLPARK)
			return 0;
		else {
			if (!bDoSilo)
				bTextOverlay = Game_SimulationProvisionMicrosim(x, y, iTileID);
			if (iFarX >= x) {
				for (iCurX = x; iCurX <= iFarX; ++iCurX) {
					for (iCurY = y; iCurY <= iFarY; ++iCurY) {
						if (iCurX >= 0) {
							if (bDoSilo) {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITClearBits(iCurX, iCurY, XBIT_WATERED | XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
							}
							else {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITClearBits(iCurX, iCurY, XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XBITSetBits(iCurX, iCurY, iTileBitMask);
							}
						}
						Game_PlaceTile(iCurX, iCurY, iTileID);
						if (bDoSilo)
							Game_PlaceUndergroundTiles(iCurX, iCurY, UNDER_TILE_MISSILESILO);
						else {
							if (iCurX >= 0) {
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XZONClearZone(iCurX, iCurY);
								if (iCurX < GAME_MAP_SIZE && iCurY < GAME_MAP_SIZE)
									XZONClearCorners(iCurX, iCurY);
							}
							if (bTextOverlay)
								XTXTSetTextOverlayID(iCurX, iCurY, bTextOverlay);
						}
					}
				}
			}
			if (iArea) {
				if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					XZONSetCornerAngle(x, y, wCornerStartBottomLeft[wViewRotation]);
				if (iFarX >= 0 && iFarX < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
					XZONSetCornerAngle(iFarX, y, wCornerStartBottomRight[wViewRotation]);
				if (iFarX < GAME_MAP_SIZE && iFarY >= 0 && iFarY < GAME_MAP_SIZE)
					XZONSetCornerAngle(iFarX, iFarY, wCornerStartTopLeft[wViewRotation]);
				if (x < GAME_MAP_SIZE && iFarY >= 0 && iFarY < GAME_MAP_SIZE)
					XZONSetCornerAngle(x, iFarY, wCornerStartTopRight[wViewRotation]);
			}
			else if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XZONSetCornerMask(x, y, CORNER_ALL);
			Game_DirtyTile(x, iFarY);
			return 1;
		}
	}
}

static bool IsTileThresholdReached(BYTE iTileID, DWORD nTarget, bool bMilitary, unsigned uComparator, DWORD nDiv, DWORD nMult) {
	WORD wTileIDCount;
	DWORD nCount;

	// Key:
	//
	// wTileIDCount: This is the returned number of 1x1 tiles covered by iTileID (the bMilitary flag when true
	//               will get you the count of military-specific tiles for the target iTileID).
	//
	// nDiv: This factor you divide against dwTileIDCount in order to get the number of groups of dwTileIDCount.
	//
	// nMult: This factor you multiply against (wTileIDCount / nDiv) in order to get the expected
	//                   multiplied returned count.
	//
	// uComparator: Specify whether you want to compare nCount against nTarget as greater than,
	//              greater or equal, equal, less or equal, or less than.

	wTileIDCount = GetFlaggedTileCount(iTileID, bMilitary);

	if (nDiv < 1)
		nDiv = 1;

	if (nMult < 1)
		nMult = 1;

	nCount = (wTileIDCount / nDiv) * nMult;
	if (uComparator == CMP_GREATERTHAN)
		return (nCount > nTarget);
	else if (uComparator == CMP_GREATEROREQUAL)
		return (nCount >= nTarget);
	else if (uComparator == CMP_EQUAL)
		return (nCount == nTarget);
	else if (uComparator == CMP_LESSOREQUAL)
		return (nCount <= nTarget);
	else
		return (nCount < nTarget);
}

// Use case:
//
// IsTileMultipliedThresholdReached() - Use this one if you want N iTileIDs to be multiplied in order to get a higher expected count
//                                      prior to comparing against the target.
//
// IsTileDividedThresholdReached() - Use this one if you want to divide the returned iTileID count (usually for grouping purposes)
//                                   before then comparing against the target.
//
// IsTileNormalThresholdReached() - Use this one to check the direct iTileID count against the target.

static bool IsTileMultipliedThresholdReached(BYTE iTileID, DWORD nTarget, bool bMilitary, unsigned uComparator, DWORD nMult) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, 1, nMult);
}

static bool IsTileDividedThresholdReached(BYTE iTileID, DWORD nTarget, bool bMilitary, unsigned uComparator, DWORD nDiv) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, nDiv, 1);
}

static bool IsTileNormalThresholdReached(BYTE iTileID, DWORD nTarget, bool bMilitary, unsigned uComparator) {
	return IsTileThresholdReached(iTileID, nTarget, bMilitary, uComparator, 1, 1);
}

static void Simulation_DoArmyBaseGrowth(mapcoord_t iX, mapcoord_t iY, int16_t iCurrZoneType) {
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	if ((rand() & 3) == 0) {
		wFlaggedTileCount = GetFlaggedTileCount(TILE_MILITARY_PARKINGLOT, true) / GetTileArea(AREA_2x2);
		iSelectedTileID = TILE_MILITARY_PARKINGLOT;
		iFirstCheckedTileID = TILE_MILITARY_TOPSECRET;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, true, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = TILE_MILITARY_HANGAR1;
			if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, true, CMP_GREATERTHAN, 8))
				iSelectedTileID = iFirstCheckedTileID;
		}
		if (!Simulation_GrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType))
			Simulation_GrowSpecificZone(iX, iY, TILE_MILITARY_HANGAR1, iCurrZoneType);
	}
}

static void Simulation_DoAirportGrowth(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID, int16_t iCurrZoneType) {
	bool bMilitary, bTakeOffNorthSouth;
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	bMilitary = (iCurrZoneType == ZONE_MILITARY) ? true : false;

	if ((rand() & 3) != 0) {
		// This section is for handling the spawning of aeroplanes and helicopters.
		// Only executed for civilian airports.
		if (!bMilitary) {
			if (iCurrentTileID == TILE_INFRASTRUCTURE_RUNWAY && (rand() % 30) == 0) {
				if (XBITReturnIsPowered(iX, iY)) {
					if (rand() % 10 < 4) {
						Game_SpawnHelicopter(iX, iY);
						return;
					}
					bTakeOffNorthSouth = false;
					if (!IsEven(wViewRotation)) {
						if (XBITReturnIsFlipped(iX, iY))
							bTakeOffNorthSouth = true;
					}
					else {
						if (!XBITReturnIsFlipped(iX, iY))
							bTakeOffNorthSouth = true;
					}
					Game_SpawnAeroplane(iX, iY, (bTakeOffNorthSouth) ? XTHG_DIRECTION_NORTH : XTHG_DIRECTION_EAST);
				}
			}
		}
	}
	else {
		// Aside from certain selected building substitions, the tile selection criteria are the same.
		wFlaggedTileCount = (GetFlaggedTileCount(TILE_INFRASTRUCTURE_RUNWAY, bMilitary) + GetFlaggedTileCount(TILE_INFRASTRUCTURE_RUNWAYCROSS, bMilitary)) / 5;
		iSelectedTileID = TILE_INFRASTRUCTURE_RUNWAY;
		iFirstCheckedTileID = (bMilitary) ? TILE_MILITARY_PARKINGLOT : TILE_INFRASTRUCTURE_PARKINGLOT;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, bMilitary, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = (bMilitary) ? TILE_MILITARY_CONTROLTOWER : TILE_INFRASTRUCTURE_CONTROLTOWER_CIV;
			if (IsTileMultipliedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
				iSelectedTileID = TILE_MILITARY_RADAR;
				if (IsTileMultipliedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
					iSelectedTileID = (bMilitary) ? TILE_MILITARY_F15B : TILE_MILITARY_TARMAC;
					if (IsTileNormalThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL)) {
						iSelectedTileID = TILE_INFRASTRUCTURE_BUILDING1;
						if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
							iSelectedTileID = TILE_INFRASTRUCTURE_BUILDING2;
							if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 2)) {
								iSelectedTileID = TILE_INFRASTRUCTURE_HANGAR2;
								if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, GetTileArea(AREA_2x2)))
									iSelectedTileID = iFirstCheckedTileID;
							}
						}
					}	
				}	
			}
		}
		Simulation_GrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType);
	}
}

static void Simulation_DoSeaportGrowth(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID, int16_t iCurrZoneType) {
	bool bMilitary;
	BYTE iFirstCheckedTileID, iSelectedTileID;
	WORD wFlaggedTileCount;

	bMilitary = (iCurrZoneType == ZONE_MILITARY) ? true : false;

	if ((rand() & 3) != 0) {
		// This section is for handling the spawning of ships.
		// Only executed for civilian seaports.
		if (!bMilitary) {
			if (iCurrentTileID == TILE_INFRASTRUCTURE_CRANE && (rand() & 3) == 0)
				Game_SpawnShip(iX, iY);
		}
	}
	else {
		wFlaggedTileCount = GetFlaggedTileCount(TILE_INFRASTRUCTURE_CRANE, bMilitary);
		iSelectedTileID = TILE_INFRASTRUCTURE_CRANE;
		iFirstCheckedTileID = TILE_INFRASTRUCTURE_CARGOYARD;
		if (IsTileDividedThresholdReached(iFirstCheckedTileID, wFlaggedTileCount, bMilitary, CMP_LESSTHAN, GetTileArea(AREA_2x2))) {
			iSelectedTileID = (bMilitary) ? TILE_MILITARY_TOPSECRET : TILE_INFRASTRUCTURE_LOADINGBAY;
			if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, GetTileArea(AREA_2x2))) {
				iSelectedTileID = TILE_MILITARY_WAREHOUSE;
				if (IsTileDividedThresholdReached(iSelectedTileID, wFlaggedTileCount, bMilitary, CMP_GREATEROREQUAL, 3))
					iSelectedTileID = iFirstCheckedTileID;
			}
		}
		if (!Simulation_GrowSpecificZone(iX, iY, iSelectedTileID, iCurrZoneType))
			Simulation_GrowSpecificZone(iX, iY, TILE_MILITARY_WAREHOUSE, iCurrZoneType);
	}
}

static void Simulation_DoSiloGrowth(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID, int16_t iCurrZoneType) {
	if (iCurrentTileID != TILE_MILITARY_MISSILESILO)
		Simulation_GrowSpecificZone(iX, iY, TILE_MILITARY_MISSILESILO, iCurrZoneType);
}

static void Simulation_DoUpdateMicrosimGrowthTick(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	BYTE iTextOverlay;
	BYTE iMicrosimIdx;
	BYTE iTileID;
	BYTE iMicrosimDataStat0;

	if (iCurrentTileID >= TILE_INFRASTRUCTURE_RAILSTATION) {
		if (iCurrentTileID == TILE_INFRASTRUCTURE_RAILSTATION && XBITReturnIsPowered(iX, iY) && !Game_RandomWordLFSRMod4()) {
			if (IsTileDividedThresholdReached(TILE_INFRASTRUCTURE_RAILSTATION, wActiveTrains, FALSE, CMP_GREATERTHAN, 4))
				Game_SpawnTrain(iX, iY);
		}
		else if (iCurrentTileID == TILE_INFRASTRUCTURE_MARINA && XBITReturnIsPowered(iX, iY) && !Game_RandomWordLFSRMod4()) {
			if (IsTileDividedThresholdReached(TILE_INFRASTRUCTURE_MARINA, wSailingBoats, FALSE, CMP_GREATERTHAN, 9))
				Game_SpawnSailBoat(iX, iY);
		}
		else if (iCurrentTileID >= TILE_ARCOLOGY_PLYMOUTH && iCurrentTileID <= TILE_ARCOLOGY_LAUNCH && XZONCornerAbsoluteCheckMask(iX, iY, CORNER_TRIGHT)) {
			iTextOverlay = XTXTGetTextOverlayID(iX, iY);
			if (iTextOverlay >= MIN_SIM_TEXT_ENTRIES && iTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
				iMicrosimIdx = MICROSIMID_ENTRY(iTextOverlay);
				iTileID = GetMicroSimulatorTileID(iMicrosimIdx);
				if (iTileID >= TILE_ARCOLOGY_PLYMOUTH && iTileID <= TILE_ARCOLOGY_LAUNCH) {
					iMicrosimDataStat0 = (GetXVALByteDataWithNormalCoordinates(iX, iY) >> 5)
						- (GetXCRMByteDataWithNormalCoordinates(iX, iY) >> 5)
						- (GetXPLTByteDataWithNormalCoordinates(iX, iY) >> 5)
						+ 12;
					if (!XBITReturnIsPowered(iX, iY))
						iMicrosimDataStat0 /= 2;
					if (!XBITReturnIsWatered(iX, iY))
						iMicrosimDataStat0 /= 2;
					if (iMicrosimDataStat0 < 0)
						iMicrosimDataStat0 = 0;
					if (iMicrosimDataStat0 > 12)
						iMicrosimDataStat0 = 12;
					SetMicroSimulatorStat0(iMicrosimIdx, iMicrosimDataStat0);
				}
			}
		}
	}
}

static bool Simulation_DoBudgetRoadCheck(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	int iFundingPercent;

	if (iCurrentTileID >= TILE_ROAD_LR && iCurrentTileID < TILE_RAIL_LR ||
		iCurrentTileID >= TILE_CROSSOVER_POWERTB_ROADLR && iCurrentTileID < TILE_CROSSOVER_POWERTB_RAILLR ||
		iCurrentTileID == TILE_CROSSOVER_HIGHWAYLR_ROADTB ||
		iCurrentTileID == TILE_CROSSOVER_HIGHWAYTB_ROADLR ||
		iCurrentTileID >= TILE_ONRAMP_TL && iCurrentTileID < TILE_HIGHWAY_HTB) {
		// Transportation budget, roads - if below 100% related tiles will be replaced with rubble.
		iFundingPercent = pBudgetArr[BUDGET_ROAD].iFundingPercent;
		if (iFundingPercent != 100 && (rand() % 100) >= iFundingPercent) {
			Game_PlaceTile(iX, iY, GetRubbleTileID());
			XBITClearBits(iX, iY, XBIT_POWERABLE);
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
		}
		return true;
	}
	return false;
}

static bool Simulation_DoBudgetRailCheck(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	int iFundingPercent;

	if (iCurrentTileID >= TILE_RAIL_LR && iCurrentTileID < TILE_TUNNEL_T ||
		iCurrentTileID >= TILE_CROSSOVER_ROADLR_RAILTB && iCurrentTileID < TILE_HIGHWAY_LR ||
		iCurrentTileID >= TILE_SUBTORAIL_T && iCurrentTileID < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 ||
		iCurrentTileID == TILE_CROSSOVER_HIGHWAYLR_RAILTB ||
		iCurrentTileID == TILE_CROSSOVER_HIGHWAYTB_RAILLR) {
		// Transportation budget, rails - if below 100% related tiles will be replaced with rubble.
		iFundingPercent = pBudgetArr[BUDGET_RAIL].iFundingPercent;
		if (iFundingPercent != 100 && (rand() % 100) >= iFundingPercent) {
			Game_PlaceTile(iX, iY, GetRubbleTileID());
			XBITClearBits(iX, iY, XBIT_POWERABLE);
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
		}
		return true;
	}
	return false;
}

static bool Simulation_DoBudgetBridgeCheck(CSimcityView *pSCView, mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	int iFundingPercent;

	if (iCurrentTileID >= TILE_SUSPENSION_BRIDGE_START_B && iCurrentTileID < TILE_ONRAMP_TL ||
		iCurrentTileID == TILE_REINFORCED_BRIDGE_PYLON ||
		iCurrentTileID == TILE_REINFORCED_BRIDGE) {
		iFundingPercent = pBudgetArr[BUDGET_BRIDGE].iFundingPercent;
		// Transportation budget, bridges - if below 100% and the weather isn't favourable, there's a chance of destruction.
		if (iFundingPercent != 100 && (int)(bWeatherWind + rand() % 50) >= iFundingPercent) {
			//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Bridge. Weather Vulnerable\n", iStep, iSubStep);
			Game_CenterOnTileCoords(iX, iY);
			Game_SimcityView_Demolish(pSCView, iX, iY, 1);
			Game_NewspaperStoryGenerator(NEWSPAPER_TYPE_BRIDGE_COLLAPSE, 0);
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
		}
		return true;
	}
	return false;
}

static bool Simulation_DoBudgetHighwayCheck(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	mapcoord_t iNextX, iNextY;
	int iFundingPercent;

	if (iCurrentTileID < TILE_TUNNEL_T || iCurrentTileID >= TILE_CROSSOVER_POWERTB_ROADLR) {
		if ((iCurrentTileID < TILE_HIGHWAY_HTB || iCurrentTileID >= TILE_SUBTORAIL_T) &&
			(iCurrentTileID < TILE_HIGHWAY_LR || iCurrentTileID >= TILE_SUSPENSION_BRIDGE_START_B))
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
		else {
			if (IsEven(iX) && IsEven(iY)) {
				iFundingPercent = pBudgetArr[BUDGET_HIGHWAY].iFundingPercent;
				if (iFundingPercent != 100 && (rand() % 100) >= iFundingPercent) {
					iNextX = iX + 1;
					iNextY = iY + 1;

					// each individual highway tile within the 2x2 block.
					if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iY))
						Game_PlaceTile(iX, iY, 0);
					else
						Game_PlaceTile(iX, iY, GetRubbleTileID());

					if (iNextX >= 0 && iNextX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE &&  XBITReturnIsWater(iNextX, iY))
						Game_PlaceTile(iNextX, iY, 0);
					else
						Game_PlaceTile(iNextX, iY, GetRubbleTileID());

					if (iX < GAME_MAP_SIZE && iNextY >= 0 && iNextY < GAME_MAP_SIZE && XBITReturnIsWater(iX, iNextY))
						Game_PlaceTile(iX, iNextY, 0);
					else
						Game_PlaceTile(iX, iNextY, GetRubbleTileID());

					if (iNextX < GAME_MAP_SIZE && iNextY < GAME_MAP_SIZE && XBITReturnIsWater(iNextX, iNextY))
						Game_PlaceTile(iNextX, iNextY, 0);
					else
						Game_PlaceTile(iNextX, iNextY, GetRubbleTileID());

					Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
				}
			}
		}
		return true;
	}
	return false;
}

static bool Simulation_DoBudgetTunnelCheck(CSimcityView *pSCView, mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	int iFundingPercent;

	if (iCurrentTileID >= TILE_TUNNEL_T && iCurrentTileID <= TILE_TUNNEL_L) {
		iFundingPercent = pBudgetArr[BUDGET_TUNNEL].iFundingPercent;
		if (iFundingPercent != 100 && (rand() % 100) >= iFundingPercent) {
			//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Tunnel. Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID]);
			Game_CenterOnTileCoords(iX, iY);
			Game_SimcityView_Demolish(pSCView, iX, iY, 1);
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
		}
		return true;
	}
	return false;
}

static void Simulation_DoBudgetOvergroundTransportCheck(CSimcityView *pSCView, mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID) {
	if (iCurrentTileID >= TILE_ROAD_LR) {
		if (!Game_RandomWordLFSRMod128()) {
			if (Simulation_DoBudgetRoadCheck(iX, iY, iCurrentTileID))
				return;
			else if (Simulation_DoBudgetRailCheck(iX, iY, iCurrentTileID))
				return;
			else if (Simulation_DoBudgetBridgeCheck(pSCView, iX, iY, iCurrentTileID))
				return;
			else if (Simulation_DoBudgetHighwayCheck(iX, iY, iCurrentTileID))
				return;
			else if (Simulation_DoBudgetTunnelCheck(pSCView, iX, iY, iCurrentTileID))
				return;
		}
		else
			Simulation_DoUpdateMicrosimGrowthTick(iX, iY, iCurrentTileID);
	}
}

static void Simulation_DoBudgetSubwayCheck(CSimcityView *pSCView, mapcoord_t iX, mapcoord_t iY) {
	BOOL bRemoveUndergroundTile;
	BYTE iCurrentUndergroundTileID;
	BYTE iReplaceUndergroundTile;
	int iFundingPercent;

	if (!Game_RandomWordLFSRMod128()) {
		iCurrentUndergroundTileID = GetUndergroundTileID(iX, iY);
		if (iCurrentUndergroundTileID >= UNDER_TILE_SUBWAY_LR && iCurrentUndergroundTileID < UNDER_TILE_PIPES_LR ||
			iCurrentUndergroundTileID == UNDER_TILE_SUBWAYENTRANCE ||
			iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR ||
			iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB) {
			iFundingPercent = pBudgetArr[BUDGET_SUBWAY].iFundingPercent;
			if (iFundingPercent != 100 && (rand() % 100) >= iFundingPercent) {
				//ConsoleLog(LOG_DEBUG, "DBG: SimulationGrowthTick(%d, %d) - Subway. Item(%s) / Underground Item(%s)\n", iStep, iSubStep, szTileNames[iCurrentTileID], (iCurrentUndergroundTileID > UNDER_TILE_SUBWAYENTRANCE) ? "** Unknown **" : szUndergroundNames[iCurrentUndergroundTileID]);
				bRemoveUndergroundTile = FALSE;
				iReplaceUndergroundTile = UNDER_TILE_CLEAR;
				if (iCurrentUndergroundTileID == UNDER_TILE_SUBWAYENTRANCE) {
					Game_SimcityView_Demolish(pSCView, iX, iY, 0);
					bRemoveUndergroundTile = TRUE;
				}
				else {
					if (iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESTB_SUBWAYLR)
						iReplaceUndergroundTile = UNDER_TILE_PIPES_TB;
					else if (iCurrentUndergroundTileID == UNDER_TILE_CROSSOVER_PIPESLR_SUBWAYTB)
						iReplaceUndergroundTile = UNDER_TILE_PIPES_LR;
					bRemoveUndergroundTile = TRUE;
				}
				if (bRemoveUndergroundTile)
					Game_PlaceUndergroundTiles(iX, iY, iReplaceUndergroundTile);
			}
		}
	}
}

static bool GetPopulatedTileAndLevel(mapcoord_t iX, mapcoord_t iY, BYTE iCurrentTileID, WORD *p_iPopulatedTile, WORD *p_iTilePopLevel) {
	WORD iPopulatedTile = 0;
	WORD iTilePopLevel = 0;

	if (iCurrentTileID >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
		// The !XZONCornerCheck() case is only hit when
		// a 2x2 >= building is being handled and the
		// otherr parts of it DON'T contain the wCurrentAngle
		// bit (In order for those other tiles to not be
		// included as part of the populated tile / populated level).
		if (!XZONCornerCheck(iX, iY, wCurrentAngle))
			return false;
		iPopulatedTile = iCurrentTileID - TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1;
		iTilePopLevel = wBuildingPopLevel[iPopulatedTile];
	}
	else {
		if (dwExperimentsEnabled & EXPERIMENT_TRIPGENERATOR && USE_NEW_TRIP_GENERATOR && USE_NEW_STARTINGCOORDS) {
			if (iCurrentTileID >= TILE_ROAD_LR || !IsValidTransitItems(iX, iY))
				return false;
		} else {
			if (iCurrentTileID >= TILE_ROAD_LR || !Game_IsValidTransitItems(iX, iY))
				return false;
		}
	}

	*p_iPopulatedTile = iPopulatedTile;
	*p_iTilePopLevel = iTilePopLevel;
	return true;
}

extern int iChurchVirus;

NEWENGINE void Simulation_DoGrowthTick(int iStep, int iSubStep) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	mapcoord_t iX, iY;
	bool bPlaceChurch;
	int16_t iCurrZoneType;
	BYTE iCurrentTileID;
	BYTE iTileState;
	int iGrowthState, iPrevGrowthState;
	// 'iDemandThreshold' must be 'int' (or a 32-bit integer at the very least),
	// otherwise building growth will not correctly occur and you'll end up with a very
	// high number of 1x1 abandonded buildings.
	// TODO (araxestroy): we should probably figure out why this happens
	int iDemandThreshold;
	WORD iTilePopLevel;
	WORD iPopulatedTile;
	int16_t iCurrentDemand, iRemainderDemand;
	BYTE iReplaceTile;

	// Key:
	// iStep: iX += 4 with each loop as long as it is < GAME_MAP_SIZE.
	// iSubStep: iY += 4 with each loop as long as it is < GAME_MAP_SIZE.

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	// The calculation here is otherwise 2500 population multiplied by
	// number of church tiles is less than the city population, in which
	// case build a church (when the Church virus isn't active...).
	bPlaceChurch = (iChurchVirus > 0) ? true : IsTileMultipliedThresholdReached(TILE_INFRASTRUCTURE_CHURCH, dwCityPopulation, FALSE, CMP_LESSTHAN, 2500);
	wCurrentAngle = wPositionAngle[wViewRotation];
	for (iX = iStep; iX < GAME_MAP_SIZE; iX += 4) {
		for (iY = iSubStep; iY < GAME_MAP_SIZE; iY += 4) {
			iCurrZoneType = XZONReturnZone(iX, iY);
			iCurrentTileID = GetTileID(iX, iY);
			if (iCurrZoneType == ZONE_NONE)
				Simulation_DoBudgetOvergroundTransportCheck(pSCView, iX, iY, iCurrentTileID);
			else {
				if (iCurrZoneType > ZONE_DENSE_INDUSTRIAL) {
					if (iCurrZoneType == ZONE_MILITARY) {
						if (bMilitaryBaseType == MILITARY_BASE_ARMY)
							Simulation_DoArmyBaseGrowth(iX, iY, iCurrZoneType);
						else if (bMilitaryBaseType == MILITARY_BASE_AIR_FORCE)
							Simulation_DoAirportGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
						else if (bMilitaryBaseType == MILITARY_BASE_NAVY)
							Simulation_DoSeaportGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
						else if (bMilitaryBaseType == MILITARY_BASE_MISSILE_SILOS)
							Simulation_DoSiloGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
					}
					else if (iCurrZoneType == ZONE_AIRPORT)
						Simulation_DoAirportGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
					else if (iCurrZoneType == ZONE_SEAPORT)
						Simulation_DoSeaportGrowth(iX, iY, iCurrentTileID, iCurrZoneType);
				}
				else {
					iPopulatedTile = 0;
					iTilePopLevel = TILEPOPLEVEL_NONE;
					if (GetPopulatedTileAndLevel(iX, iY, iCurrentTileID, &iPopulatedTile, &iTilePopLevel)) {
						iCurrentDemand = 0;
						iRemainderDemand = 4000;
						if (Game_IsZonedTilePowered(iX, iY)) {
							int iTripResult = 0;
							if (dwExperimentsEnabled & EXPERIMENT_TRIPGENERATOR && USE_NEW_TRIP_GENERATOR)
								iTripResult = Simulation_RunTripGenerator(iX, iY, iCurrZoneType, iTilePopLevel, TRIP_MAX_STEPS_VANILLA * TRIP_MAX_STEPS_SCALE);
							else
								iTripResult = Game_RunTripGenerator(iX, iY, iCurrZoneType, iTilePopLevel, GROWTH_TILE_MAX_TRIP_STEPS);

							if (iTripResult) {
								iCurrentDemand = wCityDemand[GET_GENERAL_RCI_ZONE(iCurrZoneType)] + 2000;
								iRemainderDemand = 4000 - iCurrentDemand;
							} else {
								//printf("!!! iTripResult failed, iRunTimes = %d\n", iRunTimes);
							}
						}

						// The general apparent chain of events:
						// 0) GROWTH_START - initial priming before any of the subsequent 'if' blocks are hit
						// 1) GROWTH_CONSIDERCHANGE - a building has already been placed (with an 'TilePopLevel' above 0).
						// 2) GROWTH_CHANGE - change current building due to 'iDemandThreshold' not being exceeded.
						// 3) GROWTH_CONSIDERCONSTRUCTION - consider whether construction is to be completed.
						// 4) GROWTH_COMPLETECONSTRUCTION - complete building/church construction based on the 'iDemandThreshold' not being exceeded.
						// 5) GROWTH_CONSIDERABANDON - consider changing the current building to the abandoned type (or just updating the figure until the next round)
						// 6) GROWTH_ABANDON - change building to the abandoned type based on 'iDemandThreshold' not being exceeded.
						// 7) GROWTH_CONSIDERCOMMIT - this is the final potential fall-through before committing to "starting"
						// 8) GROWTH_COMMIT - 'iDemandThreshold' exceeds the returned random number, we commit to "starting". (likely for construction sites, area changes 1x1 <-> 2x2 <-> 3x3)

						iTileState = bTileState[iPopulatedTile];
						iGrowthState = GROWTH_START;
						if (iTilePopLevel > TILEPOPLEVEL_NONE && iTileState == BUILD_START) {
							iGrowthState = GROWTH_CONSIDERCHANGE;
							//ConsoleLog(LOG_DEBUG, "BUILD_START(%u) - (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], iTilePopLevel(%u)\n", iTileState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, iTilePopLevel);
							pZonePops[iCurrZoneType] += wBuildingPopulation[iTilePopLevel]; // Values appear to be: 1[1], 8[2], 12[3], 36[4] (wBuildingPopulation[iTilePopLevel] format.
							iDemandThreshold = (iRemainderDemand / iTilePopLevel);
							if ((unsigned __int16)rand() < iDemandThreshold) {
								iGrowthState = GROWTH_CHANGE;
								iReplaceTile = rand() & 1;
								Game_PerhapsGeneralZoneChangeBuilding(iX, iY, iTilePopLevel, iReplaceTile);
							}
						}

						// Continue if iGrowthStart <= GROWTH_CONSIDERCHANGE, otherwise fallthrough.
						if (iGrowthState <= GROWTH_CONSIDERCHANGE) {
							if (iTileState == BUILD_THINK) {
								iGrowthState = GROWTH_CONSIDERCONSTRUCTION;
								iDemandThreshold = 16384 / iTilePopLevel;
								if ((unsigned __int16)rand() < iDemandThreshold) {
									iGrowthState = GROWTH_COMPLETECONSTRUCTION;
									//ConsoleLog(LOG_DEBUG, "BUILD_THINK(%u) - (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], (%u)wBuildingPopulation[%u], iTilePopLevel(%u)\n", iTileState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, wBuildingPopulation[iTilePopLevel], iTilePopLevel, iTilePopLevel);
									if (bPlaceChurch && (iTilePopLevel & 2) != 0 && iCurrZoneType < ZONE_LIGHT_COMMERCIAL) {
										//ConsoleLog(LOG_DEBUG, "BUILD_THINK(%u) - CHURCH (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], (%u)wBuildingPopulation[%u], iTilePopLevel(%u)\n", iTileState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, wBuildingPopulation[iTilePopLevel], iTilePopLevel, iTilePopLevel);
										Game_PlaceChurch(iX, iY);
									}
									else
										Game_PerhapsGeneralZoneChooseAndPlaceBuilding(iX, iY, iTilePopLevel, GET_GENERAL_RCI_ZONE(iCurrZoneType));
								}
							}

							// Continue if iGrowthStart <= GROWTH_CONSIDERCONSTRUCTION, otherwise fallthrough.
							if (iGrowthState <= GROWTH_CONSIDERCONSTRUCTION) {
								if (iTileState == BUILD_ABANDON) {
									iGrowthState = GROWTH_CONSIDERABANDON;
									//ConsoleLog(LOG_DEBUG, "BUILD_ABANDON(%u) - (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], (%u)wBuildingPopulation[%u], iTilePopLevel(%u)\n", iTileState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, wBuildingPopulation[iTilePopLevel], iTilePopLevel, iTilePopLevel);
									pZonePops[ZONEPOP_ABANDONED] += wBuildingPopulation[iTilePopLevel];
									iDemandThreshold = 15 * iCurrentDemand / iTilePopLevel;
									if ((unsigned __int16)rand() < iDemandThreshold) {
										iGrowthState = GROWTH_ABANDON;
										Game_PerhapsGeneralZoneChooseAndPlaceBuilding(iX, iY, iTilePopLevel, GET_GENERAL_RCI_ZONE(iCurrZoneType));
									}
								}

								// Continue if iGrowthStart <= GROWTH_CONSIDERCONSTRUCTION (this is unchanged), otherwise fallthrough.
								if (iGrowthState <= GROWTH_CONSIDERCONSTRUCTION) {
									if (iTilePopLevel != TILEPOPLEVEL_VERYHIGH &&
										(IsEven(iCurrZoneType) || iTilePopLevel <= TILEPOPLEVEL_NONE) &&
										(iCurrZoneType >= ZONE_LIGHT_INDUSTRIAL ||
										(iTilePopLevel != TILEPOPLEVEL_LOW || GetXVALByteDataWithNormalCoordinates(iX, iY) >= XVALPOPLEVEL_LOW) &&
										(iTilePopLevel != TILEPOPLEVEL_MEDIUM || GetXVALByteDataWithNormalCoordinates(iX, iY) >= XVALPOPLEVEL_MEDIUM) &&
										(iTilePopLevel != TILEPOPLEVEL_HIGH || GetXVALByteDataWithNormalCoordinates(iX, iY) >= XVALPOPLEVEL_HIGH))) {
										iPrevGrowthState = iGrowthState;
										// Let's cut down on the noise a bit during initial thinking for GROWTH_START alone.
										//if (iPrevGrowthState)
										//	ConsoleLog(LOG_DEBUG, "THINKING -(%u/%d) - (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], (%u)wBuildingPopulation[%u], iTilePopLevel(%u)\n", iTileState, iGrowthState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, wBuildingPopulation[iTilePopLevel], iTilePopLevel, iTilePopLevel);
										iGrowthState = GROWTH_CONSIDERCOMMIT;
										iDemandThreshold = 3 * iCurrentDemand / (iTilePopLevel + 1);
										if (iDemandThreshold > (unsigned __int16)rand()) {
											//ConsoleLog(LOG_DEBUG, "CONSIDERCOMMIT -> COMMIT -(%u) iGrowthState(%d), iPrevGrowthState(%d) - (%d, %d) [%s] bTileState[%u], wBuildingPopLevel[%u], (%u)wBuildingPopulation[%u], iTilePopLevel(%u)\n", iTileState, iGrowthState, iPrevGrowthState, iX, iY, szTileNames[iCurrentTileID], iPopulatedTile, iPopulatedTile, wBuildingPopulation[iTilePopLevel], iTilePopLevel, iTilePopLevel);
											iGrowthState = GROWTH_COMMIT;
										}
										if (iGrowthState == GROWTH_COMMIT)
											Game_PerhapsGeneralZoneStartBuilding(iX, iY, iTilePopLevel, iCurrZoneType);
									}
								}
							}
						}
					}
				}
			}
			Simulation_DoBudgetSubwayCheck(pSCView, iX, iY);
		}
	}
	dirtyRect.top = -1000;
}

static void DeleteTilePortion(mapcoord_t x, mapcoord_t y) {
	if (GetTileID(x, y) >= TILE_SMALLPARK)
		Game_ZonedBuildingTileDeletion(x, y);
}

static void SetNewZoneOnTilePortion(mapcoord_t x, mapcoord_t y, int16_t iZoneType) {
	if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
		XZONSetNewZone(x, y, iZoneType);
}

static void MilitaryUnsetBitsOnTilePortion(mapcoord_t x, mapcoord_t y, int16_t iZoneType) {
	if (iZoneType == ZONE_MILITARY) {
		if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
			XBITClearBits(x, y, XBIT_WATERED | XBIT_PIPED | XBIT_POWERED | XBIT_POWERABLE);
	}
}

static bool SetMoveRunwayTileAxis(mapcoord_t primaryAxis, mapcoord_t secondaryAxis, bool* bMovePrimaryAxis, bool* bMoveSecondaryAxis) {
	if (!IsEven(primaryAxis))
		*bMovePrimaryAxis = true;
	if (!*bMovePrimaryAxis) {
		if (IsEven(secondaryAxis))
			return FALSE;

		*bMoveSecondaryAxis = true;
	}
	return true;
}

static bool ShouldCoupledObjectTileFlip(mapcoord_t mapCoord) {
	if (mapCoord == 0) {
		if (!IsEven(wViewRotation))
			return true;
	}
	else {
		if (IsEven(wViewRotation))
			return true;
	}

	return false;
}

// XXX (araxestroy): should the last two args be bools?
static bool GetRunwayTilePositionalOffset(mapcoord_t x, mapcoord_t y, int16_t iZoneType, int16_t* iMoveX, int16_t* iMoveY) {
	bool bMoveXAxis;
	bool bMoveYAxis;
	WORD wTileCountType;

	bMoveXAxis = false;
	bMoveYAxis = false;
	// Slight change here: distinguish between military and standard runway tiles.
	wTileCountType = (iZoneType == ZONE_MILITARY) ? wMilitaryTiles[MILITARYTILE_RUNWAY] : wTileCount[TILE_INFRASTRUCTURE_RUNWAY];
	if (IsEven(wTileCountType)) {
		if (!SetMoveRunwayTileAxis(x, y, &bMoveXAxis, &bMoveYAxis))
			return false;
	}
	else {
		if (!SetMoveRunwayTileAxis(y, x, &bMoveYAxis, &bMoveXAxis))
			return false;
	}

	*iMoveX = (bMoveXAxis) ? 1 : 0;
	*iMoveY = (bMoveYAxis) ? 1 : 0;
	return true;
}

static bool RunwayTileMilitaryCheck(int16_t x, int16_t y, int16_t iZoneType) {
	if (iZoneType == ZONE_MILITARY) {
		if ((GetTileID(x, y) >= TILE_ROAD_LR && GetTileID(x, y) <= TILE_ROAD_LTBR) ||
			GetTileID(x, y) == TILE_INFRASTRUCTURE_CRANE || GetTileID(x, y) == TILE_MILITARY_MISSILESILO)
			return false;
		if (GetTerrainTileID(x, y))
			return false;
		if (GetUndergroundTileID(x, y))
			return false;
	}
	return true;
}

static bool RunwayStripLengthCheck(int iRunwayStripTileCount) {
	// Does the runway strip equal or exceed the defined number of max tiles?
	return (iRunwayStripTileCount >= RUNWAYSTRIP_MAXTILES) ? true : false;
}

static bool IsRunwayTypeTile(mapcoord_t x, mapcoord_t y) {
	return (GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAY || GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAYCROSS) ? true : false;
}

static bool TwoByTwoGeneralBlockTileCheck(mapcoord_t x, mapcoord_t y) {
	BYTE iTileID;

	iTileID = GetTileID(x, y);
	return (iTileID == TILE_INFRASTRUCTURE_RUNWAY || iTileID == TILE_INFRASTRUCTURE_RUNWAYCROSS ||
		iTileID == TILE_INFRASTRUCTURE_CRANE || iTileID == TILE_MILITARY_MISSILESILO) ? true : false;
}

static bool TwoByTwoMismatchAndMilitaryBlockTileCheck(mapcoord_t x, mapcoord_t y, int16_t iZoneType) {
	if (XZONReturnZone(x, y) != iZoneType)
		return true;
	if (iZoneType == ZONE_MILITARY) {
		if (XZONReturnZone(x, y) == ZONE_MILITARY) {
			if (GetTileID(x, y) >= TILE_ROAD_LR && GetTileID(x, y) <= TILE_ROAD_LTBR)
				return true;
		}
		if (GetUndergroundTileID(x, y))
			return true;
	}
	return false;
}

// Function prototype: HOOKCB void Hook_Simulation_GrowSpecificZone_Success(mapcoord_t iX, mapcoord_t iY, uint32_t iTileID, int16_t iZoneType)
// Called if Simulation_GrowSpecificZone succeeds. Cannot be ignored.
std::vector<hook_function_t> stHooks_Simulation_GrowSpecificZone_Success;

NEWENGINE bool Simulation_GrowSpecificZone(mapcoord_t iX, mapcoord_t iY, uint32_t iTileID, int16_t iZoneType) {
	mapcoord_t x, y;
	mapcoord_t iMoveX, iMoveY;
	mapcoord_t iCurrX, iCurrY;
	mapcoord_t iNextX, iNextY;
	int16_t iInitialRunwayStripTileCount;
	int16_t iBranchingRunwayStripTileCount;
	bool bToFlip;
	bool bTileFlipped;
	int16_t iPierTileCount;
	int16_t iPierPathTileCount;
	int16_t iPierLength;

	x = iX;
	y = iY;
	if (iZoneType != ZONE_MILITARY)
		if (!Game_IsZonedTilePowered(x, y))
			return false;

	switch (iTileID) {
	case TILE_INFRASTRUCTURE_RUNWAY:
		iMoveX = 0;
		iMoveY = 0;

		if (!GetRunwayTilePositionalOffset(x, y, iZoneType, &iMoveX, &iMoveY))
			return false;

		iCurrX = x;
		iCurrY = y;
		iInitialRunwayStripTileCount = 0;
		while (iCurrX < GAME_MAP_SIZE && iCurrY < GAME_MAP_SIZE) {
			if (XZONReturnZone(iCurrX, iCurrY) != iZoneType)
				return false;
			if (!RunwayTileMilitaryCheck(iCurrX, iCurrY, iZoneType))
				return false;
			// With this check if there's a hit on an existing runway
			// tile then we want to decrease the count until it reaches
			// the first vacant tile.
			if (IsRunwayTypeTile(iCurrX, iCurrY))
				--iInitialRunwayStripTileCount;
			iCurrX += iMoveY;
			iCurrY += iMoveX;
			++iInitialRunwayStripTileCount;
			if (RunwayStripLengthCheck(iInitialRunwayStripTileCount)) {
				bToFlip = ShouldCoupledObjectTileFlip(iMoveY);

				iBranchingRunwayStripTileCount = 0;
				while (1) {
					if (x >= 0) {
						// With this check if true and there's a hit on
						// an existing runway tile then the branching strip
						// counter is decreased, if the tile is specifically
						// the standard runway-type (not cross) do a tile-flip
						// check, if the result is not equivalent to iToFlip
						// then replace the tile in question with the
						// runwaycross type and unset the flipped bit at the end.
						if (IsRunwayTypeTile(x, y)) {
							--iBranchingRunwayStripTileCount;
							if (GetTileID(x, y) == TILE_INFRASTRUCTURE_RUNWAY) {
								bTileFlipped = (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE && XBITReturnIsFlipped(x, y));
								if (bTileFlipped != bToFlip) {
									Game_PlaceTile(x, y, TILE_INFRASTRUCTURE_RUNWAYCROSS);
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XZONSetCornerMask(x, y, CORNER_ALL);
									if (iZoneType != ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XBITSetBits(x, y, XBIT_POWERED | XBIT_POWERABLE);
									if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
										XBITClearBits(x, y, XBIT_FLIPPED);
								}
							}
						}
						else {
							if (!RunwayTileMilitaryCheck(x, y, iZoneType))
								return false;
							DeleteTilePortion(x, y);
							Game_PlaceTile(x, y, TILE_INFRASTRUCTURE_RUNWAY);
							if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XZONSetCornerMask(x, y, CORNER_ALL);
							if (iZoneType != ZONE_MILITARY && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XBITSetBits(x, y, XBIT_POWERED | XBIT_POWERABLE);
							if (bToFlip && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
								XBITSetBits(x, y, XBIT_FLIPPED);
						}
					}
					x += iMoveY;
					y += iMoveX;
					++iBranchingRunwayStripTileCount;
					if (RunwayStripLengthCheck(iBranchingRunwayStripTileCount))
						goto PLACEMENT_SUCCESS;
					continue;
				}
			}
		}
		goto PLACEMENT_SUCCESS;
	case TILE_INFRASTRUCTURE_CRANE:
		if (x < MAP_EDGE_MIN || x > MAP_EDGE_MAX || y < MAP_EDGE_MIN || y > MAP_EDGE_MAX)
			return false;
		for (iPierTileCount = 0; iPierTileCount < PIER_MAXTILES; iPierTileCount++) {
			iMoveX = x + wRotateCoordShiftX[iPierTileCount];
			if (iMoveX >= MAP_EDGE_MIN && iMoveX <= MAP_EDGE_MAX) {
				iMoveY = y + wRotateCoordShiftY[iPierTileCount];
				if (iMoveY >= MAP_EDGE_MIN && iMoveY <= MAP_EDGE_MAX && XBITReturnIsWater(iMoveX, iMoveY))
					break;
			}
		}
		if (iPierTileCount == PIER_MAXTILES || iMoveX < MAP_EDGE_MIN || iMoveX > MAP_EDGE_MAX || iMoveY < MAP_EDGE_MIN || iMoveY > MAP_EDGE_MAX)
			return false;
		iMoveY = wRotateCoordShiftY[iPierTileCount];
		if (iMoveY && !IsEven(x))
			return false;
		iMoveX = wRotateCoordShiftX[iPierTileCount];
		if (iMoveX && !IsEven(y))
			return false;
		iPierPathTileCount = 0;
		iCurrX = x;
		iCurrY = y;
		do {
			iCurrX += iMoveX;
			iCurrY += iMoveY;
			if (iCurrX >= GAME_MAP_SIZE || iCurrY >= GAME_MAP_SIZE || !XBITReturnIsWater(iCurrX, iCurrY))
				return false;
			if (GetTileID(iCurrX, iCurrY))
				return false;
			++iPierPathTileCount;
		} while (iPierPathTileCount <= PIER_MAXTILES);
		if (ALTMReturnWaterLevel(iCurrX, iCurrY) < ALTMReturnLandAltitude(iCurrX, iCurrY) + 2)
			return false;
		DeleteTilePortion(x, y);
		L_ItemPlacementCheck(x, y, TILE_INFRASTRUCTURE_CRANE, AREA_1x1, false);
		SetNewZoneOnTilePortion(x, y, iZoneType);
		MilitaryUnsetBitsOnTilePortion(x, y, iZoneType);

		bToFlip = ShouldCoupledObjectTileFlip(iMoveX);

		iPierLength = PIER_MAXTILES;
		do {
			x += wRotateCoordShiftX[iPierTileCount];
			y += wRotateCoordShiftY[iPierTileCount];
			Game_PlaceTile(x, y, TILE_INFRASTRUCTURE_PIER);
			if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XZONSetCornerMask(x, y, CORNER_ALL);
			if (bToFlip && x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
				XBITSetBits(x, y, XBIT_FLIPPED);
			--iPierLength;
		} while (iPierLength);
		goto PLACEMENT_SUCCESS;
	case TILE_INFRASTRUCTURE_CONTROLTOWER_CIV:
	case TILE_MILITARY_CONTROLTOWER:
	case TILE_MILITARY_WAREHOUSE:
	case TILE_INFRASTRUCTURE_BUILDING1:
	case TILE_INFRASTRUCTURE_BUILDING2:
	case TILE_MILITARY_TARMAC:
	case TILE_MILITARY_F15B:
	case TILE_MILITARY_HANGAR1:
	case TILE_MILITARY_RADAR:
		if (GetTileID(x, y) < TILE_SMALLPARK) {
			L_ItemPlacementCheck(x, y, iTileID, AREA_1x1, false);
			SetNewZoneOnTilePortion(x, y, iZoneType);
			MilitaryUnsetBitsOnTilePortion(x, y, iZoneType);
		}
		goto PLACEMENT_SUCCESS;
	case TILE_INFRASTRUCTURE_PARKINGLOT:
	case TILE_MILITARY_PARKINGLOT:
	case TILE_INFRASTRUCTURE_LOADINGBAY:
	case TILE_MILITARY_TOPSECRET:
	case TILE_INFRASTRUCTURE_CARGOYARD:
	case TILE_INFRASTRUCTURE_HANGAR2:
		// Odd numbered x/y coordinates are subtracted by 1.
		// If this isn't done then buildings of this nature
		// will end up overlapping.
		iCurrX = (IsEven(x)) ? x : x - 1;
		iCurrY = (IsEven(y)) ? y : y - 1;

		iNextX = iCurrX + 1;
		iNextY = iCurrY + 1;
		if (GetTileID(iCurrX, iCurrY) >= TILE_INFRASTRUCTURE_WATERTOWER)
			return false;
		if (TwoByTwoGeneralBlockTileCheck(iCurrX, iCurrY) ||
			TwoByTwoGeneralBlockTileCheck(iNextX, iCurrY) ||
			TwoByTwoGeneralBlockTileCheck(iCurrX, iNextY) ||
			TwoByTwoGeneralBlockTileCheck(iNextX, iNextY))
			return false;
		if (TwoByTwoMismatchAndMilitaryBlockTileCheck(iCurrX, iCurrY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iNextX, iCurrY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iCurrX, iNextY, iZoneType) ||
			TwoByTwoMismatchAndMilitaryBlockTileCheck(iNextX, iNextY, iZoneType))
			return false;
		DeleteTilePortion(iCurrX, iCurrY);
		DeleteTilePortion(iNextX, iCurrY);
		DeleteTilePortion(iCurrX, iNextY);
		DeleteTilePortion(iNextX, iNextY);
		L_ItemPlacementCheck(iCurrX, iCurrY, iTileID, AREA_2x2, false);
		SetNewZoneOnTilePortion(iCurrX, iCurrY, iZoneType);
		SetNewZoneOnTilePortion(iNextX, iCurrY, iZoneType);
		SetNewZoneOnTilePortion(iCurrX, iNextY, iZoneType);
		SetNewZoneOnTilePortion(iNextX, iNextY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iCurrX, iCurrY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iNextX, iCurrY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iCurrX, iNextY, iZoneType);
		MilitaryUnsetBitsOnTilePortion(iNextX, iNextY, iZoneType);
		goto PLACEMENT_SUCCESS;
	case TILE_MILITARY_MISSILESILO:
		L_ItemPlacementCheck(x, y, TILE_MILITARY_MISSILESILO, AREA_3x3, true);
		goto PLACEMENT_SUCCESS;
	default:
		goto PLACEMENT_SUCCESS;
	}

PLACEMENT_SUCCESS:
	for (const auto& hook : stHooks_Simulation_GrowSpecificZone_Success) {
		if (hook.iType == HOOKFN_TYPE_NATIVE && hook.bEnabled) {
			void(*fnHook)(mapcoord_t, mapcoord_t, uint32_t, uint32_t) = (void(*)(mapcoord_t, mapcoord_t, uint32_t, uint32_t))hook.pFunction;
			fnHook(iX, iY, iTileID, iZoneType);
		}
	}
	return true;
}

static void PlacePowerLineTile(mapcoord_t x, mapcoord_t y, BYTE iTileID) {
	Game_PlaceTile(x, y, iTileID);
	if (x < GAME_MAP_SIZE && y < GAME_MAP_SIZE)
		XBITSetBits(x, y, XBIT_POWERABLE);
	Game_CheckAdjustTerrainAndPlacePowerLines(x, y);
	if (x > 0)
		Game_CheckAdjustTerrainAndPlacePowerLines(x - 1, y);
	if (x < GAME_MAP_SIZE-1)
		Game_CheckAdjustTerrainAndPlacePowerLines(x + 1, y);
	if (y > 0)
		Game_CheckAdjustTerrainAndPlacePowerLines(x, y - 1);
	if (y < GAME_MAP_SIZE-1)
		Game_CheckAdjustTerrainAndPlacePowerLines(x, y + 1);
}

extern "C" void __cdecl Hook_PlacePowerLinesAtCoordinates(mapcoord_t x, mapcoord_t y) {
	BYTE iTileID;
	BYTE iBuildTileID;

	if ((x >= 0 || x < GAME_MAP_SIZE) && (y >= 0 || y < GAME_MAP_SIZE) && XBITReturnMask(x, y) >= 0) {
		iTileID = GetTileID(x, y);
		if (iTileID < TILE_POWERLINES_LR)
			iBuildTileID = TILE_POWERLINES_LR;
		else if (iTileID == TILE_ROAD_LR)
			iBuildTileID = TILE_CROSSOVER_POWERTB_ROADLR;
		else if (iTileID == TILE_ROAD_TB)
			iBuildTileID = TILE_CROSSOVER_POWERLR_ROADTB;
		else if (iTileID == TILE_RAIL_LR)
			iBuildTileID = TILE_CROSSOVER_POWERTB_RAILLR;
		else if (iTileID == TILE_RAIL_TB)
			iBuildTileID = TILE_CROSSOVER_POWERLR_RAILTB;
		else if (iTileID == TILE_HIGHWAY_LR)
			iBuildTileID = TILE_CROSSOVER_HIGHWAYLR_POWERTB;
		else if (iTileID == TILE_HIGHWAY_TB)
			iBuildTileID = TILE_CROSSOVER_HIGHWAYTB_POWERLR;
		else
			return;

		PlacePowerLineTile(x, y, iBuildTileID);
	}
}

extern "C" int __cdecl Hook_ItemPlacementCheck(mapcoord_t m_x, mapcoord_t m_y, BYTE iTileID, int16_t iTileArea) {
	return L_ItemPlacementCheck(m_x, m_y, iTileID, iTileArea, false);
}

extern "C" int __cdecl Hook_CityToolPlaceSelectedBuilding(mapcoord_t iX, mapcoord_t iY, int16_t iTool, int16_t iSubTool) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	int iItemCost;
	mapcoord_t iResAreaNearX, iResAreaFarX;
	mapcoord_t iResAreaNearY, iResAreaFarY;
	mapcoord_t iResAreaCurX, iResAreaCurY;
	int16_t nResSelCnt;
	BOOL bFail, bBadPlacement;
	BYTE tileFromSubTool[CITY_MENUTOOL_TOTAL];
	CMFC3XString errString;
	BYTE iPos;
	BYTE iTile;
	BYTE nBuildArea;
	BYTE iZone;
	CStadiumSelectTeamDialog stadiumDlg;

	memset(tileFromSubTool, 0, sizeof(tileFromSubTool));

	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_COAL, CITYTOOL_GROUP_POWER)]             = TILE_POWERPLANT_COAL;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_OIL, CITYTOOL_GROUP_POWER)]              = TILE_POWERPLANT_OIL;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_GAS, CITYTOOL_GROUP_POWER)]              = TILE_POWERPLANT_GAS;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_NUCLEAR, CITYTOOL_GROUP_POWER)]          = TILE_POWERPLANT_NUCLEAR;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_WIND, CITYTOOL_GROUP_POWER)]             = TILE_POWERPLANT_WIND;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_SOLAR, CITYTOOL_GROUP_POWER)]            = TILE_POWERPLANT_SOLAR;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_MICROWAVE, CITYTOOL_GROUP_POWER)]        = TILE_POWERPLANT_MICROWAVE;
	tileFromSubTool[CITY_MENUTOOL_POS(POWER_PLANTS_FUSION, CITYTOOL_GROUP_POWER)]           = TILE_POWERPLANT_FUSION;

	tileFromSubTool[CITY_MENUTOOL_POS(WATER_PUMP, CITYTOOL_GROUP_WATER)]                    = TILE_INFRASTRUCTURE_WATERPUMP;
	tileFromSubTool[CITY_MENUTOOL_POS(WATER_TOWER, CITYTOOL_GROUP_WATER)]                   = TILE_INFRASTRUCTURE_WATERTOWER;
	tileFromSubTool[CITY_MENUTOOL_POS(WATER_TREATMENT, CITYTOOL_GROUP_WATER)]               = TILE_INFRASTRUCTURE_WATERTREATMENT;
	tileFromSubTool[CITY_MENUTOOL_POS(WATER_DESALINIZATION, CITYTOOL_GROUP_WATER)]          = TILE_INFRASTRUCTURE_DESALINIZATIONPLANT;

	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_MAYORSHOUSE, CITYTOOL_GROUP_REWARDS)]         = TILE_INFRASTRUCTURE_MAYORSHOUSE;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_CITYHALL, CITYTOOL_GROUP_REWARDS)]            = TILE_SERVICES_CITYHALL;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_STATUE, CITYTOOL_GROUP_REWARDS)]              = TILE_SERVICES_STATUE;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_BRAUNLLAMADOME, CITYTOOL_GROUP_REWARDS)]      = TILE_OTHER_BRAUNLLAMADOME;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_ARCOLOGIES_PLYMOUTH, CITYTOOL_GROUP_REWARDS)] = TILE_ARCOLOGY_PLYMOUTH;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_ARCOLOGIES_FOREST, CITYTOOL_GROUP_REWARDS)]   = TILE_ARCOLOGY_FOREST;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_ARCOLOGIES_DARCO, CITYTOOL_GROUP_REWARDS)]    = TILE_ARCOLOGY_DARCO;
	tileFromSubTool[CITY_MENUTOOL_POS(REWARDS_ARCOLOGIES_LAUNCH, CITYTOOL_GROUP_REWARDS)]   = TILE_ARCOLOGY_LAUNCH;

	tileFromSubTool[CITY_MENUTOOL_POS(ROADS_BUSSTATION, CITYTOOL_GROUP_ROADS)]              = TILE_INFRASTRUCTURE_BUSDEPOT;

	tileFromSubTool[CITY_MENUTOOL_POS(RAILS_DEPOT, CITYTOOL_GROUP_RAIL)]                    = TILE_INFRASTRUCTURE_RAILSTATION;
	tileFromSubTool[CITY_MENUTOOL_POS(RAILS_SUBSTATION, CITYTOOL_GROUP_RAIL)]               = TILE_INFRASTRUCTURE_SUBWAYSTATION;

	tileFromSubTool[CITY_MENUTOOL_POS(EDUCATION_SCHOOL, CITYTOOL_GROUP_EDUCATION)]          = TILE_SERVICES_SCHOOL;
	tileFromSubTool[CITY_MENUTOOL_POS(EDUCATION_COLLEGE, CITYTOOL_GROUP_EDUCATION)]         = TILE_SERVICES_COLLEGE;
	tileFromSubTool[CITY_MENUTOOL_POS(EDUCATION_LIBRARY, CITYTOOL_GROUP_EDUCATION)]         = TILE_INFRASTRUCTURE_LIBRARY;
	tileFromSubTool[CITY_MENUTOOL_POS(EDUCATION_MUSEUM, CITYTOOL_GROUP_EDUCATION)]          = TILE_SERVICES_MUSEUM;

	tileFromSubTool[CITY_MENUTOOL_POS(SERVICES_POLICE, CITYTOOL_GROUP_SERVICES)]            = TILE_SERVICES_POLICE;
	tileFromSubTool[CITY_MENUTOOL_POS(SERVICES_FIRESTATION, CITYTOOL_GROUP_SERVICES)]       = TILE_SERVICES_FIRE;
	tileFromSubTool[CITY_MENUTOOL_POS(SERVICES_HOSPITAL, CITYTOOL_GROUP_SERVICES)]          = TILE_SERVICES_HOSPITAL;
	tileFromSubTool[CITY_MENUTOOL_POS(SERVICES_PRISON, CITYTOOL_GROUP_SERVICES)]            = TILE_SERVICES_PRISON;

	tileFromSubTool[CITY_MENUTOOL_POS(PARKS_SMALLPARK, CITYTOOL_GROUP_PARKS)]               = TILE_SMALLPARK;
	tileFromSubTool[CITY_MENUTOOL_POS(PARKS_BIGPARK, CITYTOOL_GROUP_PARKS)]                 = TILE_SERVICES_BIGPARK;
	tileFromSubTool[CITY_MENUTOOL_POS(PARKS_ZOO, CITYTOOL_GROUP_PARKS)]                     = TILE_SERVICES_ZOO;
	tileFromSubTool[CITY_MENUTOOL_POS(PARKS_STADIUM, CITYTOOL_GROUP_PARKS)]                 = TILE_SERVICES_STADIUM;
	tileFromSubTool[CITY_MENUTOOL_POS(PARKS_MARINA, CITYTOOL_GROUP_PARKS)]                  = TILE_INFRASTRUCTURE_MARINA;

	GameMain_String_Cons(&errString);

	pSCApp = &pCSimcityAppThis;

	bFail = FALSE;
	iPos = iSubTool + MAX_CITY_MENUTOOLS * iTool;
	iItemCost = costFromSubTool[iPos];
	if (dwCityFunds < iItemCost && iItemCost) {
		bFail = TRUE;
		goto FAIL;
	}

	iTile = tileFromSubTool[iPos];
	if (!iTile) {
		bFail = TRUE;
		goto FAIL;
	}

	bBadPlacement = FALSE;
	nBuildArea = areaFromSubTool[iPos];
	if (nBuildArea >= AREA_1x1 && nBuildArea <= AREA_4x4) {
		if (nBuildArea == AREA_2x2) {
			if (iX < AREA_2x2_MIN_EDGE || iX > AREA_2x2_MAX_EDGE ||
				iY < AREA_2x2_MIN_EDGE || iY > AREA_2x2_MAX_EDGE)
				bBadPlacement = TRUE;
			if (tilebuild_debug & TILEBUILD_DEBUG_BUILDING)
				ConsoleLog(LOG_DEBUG, "AREA_2x2: iX/iY(%d, %d)\n", iX, iY);
		}
		else if (nBuildArea == AREA_3x3) {
			if (iX < AREA_3x3_MIN_EDGE || iX > AREA_3x3_MAX_EDGE ||
				iY < AREA_3x3_MIN_EDGE || iY > AREA_3x3_MAX_EDGE)
				bBadPlacement = TRUE;
			if (tilebuild_debug & TILEBUILD_DEBUG_BUILDING)
				ConsoleLog(LOG_DEBUG, "AREA_3x3: iX/iY(%d, %d)\n", iX, iY);
		}
		else if (nBuildArea == AREA_4x4) {
			if (iX < AREA_4x4_MIN_EDGE || iX > AREA_4x4_MAX_EDGE ||
				iY < AREA_4x4_MIN_EDGE || iY > AREA_4x4_MAX_EDGE)
				bBadPlacement = TRUE;
			if (tilebuild_debug & TILEBUILD_DEBUG_BUILDING)
				ConsoleLog(LOG_DEBUG, "AREA_4x4: iX/iY(%d, %d)\n", iX, iY);
		}
		else {
			if (iX < MAP_EDGE_MIN || iX > MAP_EDGE_MAX ||
				iY < MAP_EDGE_MIN || iY > MAP_EDGE_MAX)
				bBadPlacement = TRUE;
			if (tilebuild_debug & TILEBUILD_DEBUG_BUILDING)
				ConsoleLog(LOG_DEBUG, "AREA_1x1: iX/iY(%d, %d)\n", iX, iY);
		}
	}
	else {
		bFail = TRUE;
		goto FAIL;
	}

	if (bBadPlacement) {
		bFail = TRUE;
		GameMain_String_LoadStringA(&errString, 105);
		GameMain_AfxMessageBoxStr(errString.m_pchData, 0, 0);
		goto FAIL;
	}

	if (iTile == SPRITE_SMALL_INFRASTRUCTURE_WATERTREATMENT ||
		iTile == TILE_POWERPLANT_COAL ||
		iTile == TILE_POWERPLANT_OIL ||
		iTile == TILE_POWERPLANT_GAS ||
		iTile == TILE_POWERPLANT_NUCLEAR ||
		iTile == TILE_SERVICES_PRISON) {
		nResSelCnt = 0;
		iResAreaNearX = iX - 8;
		iResAreaFarX = iX + nBuildArea + 8;
		iResAreaNearY = iY - 8;
		iResAreaFarY = iY + nBuildArea + 8;
		for (iResAreaCurX = iResAreaNearX; iResAreaCurX < iResAreaFarX; ++iResAreaCurX) {
			for (iResAreaCurY = iResAreaNearY; iResAreaCurY < iResAreaFarY; ++iResAreaCurY) {
				if (iResAreaCurX < GAME_MAP_SIZE && iResAreaCurY < GAME_MAP_SIZE) {
					iZone = XZONReturnZone(iResAreaCurX, iResAreaCurY);
					if (iZone == ZONE_LIGHT_RESIDENTIAL || iZone == ZONE_DENSE_RESIDENTIAL)
						++nResSelCnt;
				}
			}
		}
		if (Game_RandomWordLCGMod(200) < nResSelCnt) {
			Game_SimcityApp_SoundStopActionThingSound(pSCApp, SOUND_BULLDOZER);
			Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_BOOS);
			Game_FailRadioToFileID(403, 106);
			bFail = TRUE;
			goto FAIL;
		}
	}

	if (!L_ItemPlacementCheck(iX, iY, iTile, nBuildArea, false)) {
		bFail = TRUE;
		goto FAIL;
	}

	dwCityFunds -= iItemCost;
	Game_SimcityDoc_UpdateDocumentTitle(pCSimcityDoc);
	switch (iTile) {
		case TILE_SERVICES_HOSPITAL:
			++pBudgetArr[BUDGET_HEALTH].iCurrentCosts;
			break;
		case TILE_SERVICES_POLICE:
			++pBudgetArr[BUDGET_POLICE].iCurrentCosts;
			break;
		case TILE_SERVICES_FIRE:
			++pBudgetArr[BUDGET_FIRE].iCurrentCosts;
			break;
		case TILE_SERVICES_SCHOOL:
			++pBudgetArr[BUDGET_SCHOOL].iCurrentCosts;
			break;
		case TILE_SERVICES_STADIUM:
			if (XTXTGetTextOverlayID(iX, iY)) {
				Game_StadiumSelectTeamDialog_Construct(&stadiumDlg, NULL);
				stadiumDlg.dwSSTDPosX = iX;
				stadiumDlg.dwSSTDPosY = iY;
				Game_GameDialog_DoModal(&stadiumDlg);
				Game_StadiumSelectTeamDialog_Destruct(&stadiumDlg);
			}
			break;
		case TILE_SERVICES_COLLEGE:
			++pBudgetArr[BUDGET_COLLEGE].iCurrentCosts;
			break;
		case TILE_SERVICES_STATUE:
			if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE)
				XBITClearBits(iX, iY, XBIT_POWERABLE);
			break;
		case TILE_INFRASTRUCTURE_WATERPUMP:
			Game_PlacePipesAtCoordinates(iX, iY);
			break;
		case TILE_INFRASTRUCTURE_SUBWAYSTATION:
			Game_PlaceSubwayAtCoordinates(iX, iY);
			Game_PlaceUndergroundTiles(iX, iY, UNDER_TILE_SUBWAYENTRANCE);
			if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE)
				XBITClearBits(iX, iY, XBIT_PIPED);
			pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
			Game_SimcityView_DrawHouse(pSCView);
			UpdateWindow(pSCView->m_hWnd);
			break;
	}

	if (iX >= MAP_EDGE_MIN) {
		// Commented out section is to do with the power/water grid updates slowdown case.
		if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsPowerable(iX, iY)/* && dwCityPopulation < 50000*/)
			Game_SimulationUpdatePowerConsumption();
		if (iX < GAME_MAP_SIZE && iY < GAME_MAP_SIZE && XBITReturnIsPiped(iX, iY)/* && dwCityPopulation < 50000*/)
			Game_SimulationUpdateWaterConsumption();
	}

	Game_SimcityApp_UpdateStatus(pSCApp, FALSE);

	if (tilebuild_debug & TILEBUILD_DEBUG_BUILDING)
		ConsoleLog(LOG_DEBUG, "- 0x%06X -> CityToolPlaceSelectedBuilding(%d, %d, %d, %d): iPos(%u) iTile(%u) [%s] iItemCost(%d) nBuildArea(%u)\n", _ReturnAddress(), iX, iY, iTool, iSubTool, iPos, iTile, szTileNames[iTile], iItemCost, nBuildArea);

FAIL:
	GameMain_String_Dest(&errString);
	return (bFail) ? 0 : 1;
}

static void L_PierCheckStackPush(mapcoord_t x, mapcoord_t y) {
	uint32_t nTileID = GetTileID(x, y);
	if (nTileID) {
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_PIER, TILE_INFRASTRUCTURE_CRANE))
			Game_StackPush(x, y);
	}
}

static void L_RunwayCheckStackPush(mapcoord_t x, mapcoord_t y) {
	uint32_t nTileID = GetTileID(x, y);
	if (nTileID) {
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_RUNWAY, TILE_INFRASTRUCTURE_RUNWAYCROSS))
			Game_StackPush(x, y);
	}
}

// XXX (araxestroy): This function needs some serious comment work.
extern "C" void __stdcall Hook_SimcityView_Demolish(mapcoord_t x, mapcoord_t y, BOOL bExplosion) {
	CSimcityView *pThis;

	__asm mov [pThis], ecx

	CSimcityAppPrimary *pSCApp;
	BYTE *pLockedBits = NULL;
	BYTE *pLockedBaseBits = NULL;
	bool bDoYield;
	mapcoord_t nX, nY;
	int16_t nTileID, nLoopTileID;
	mapcoord_t nCornerX, nCornerY;
	int16_t nArea;
	int16_t nCoordScale, nLandAltScale, nScaleVal;
	int16_t nHighwayTile;
	int16_t nHorzMult, nVertMult;
	int16_t nStoredHorzMult, nStoredVertMult;
	int16_t nSpriteBase, nSpriteID;
	mapcoord_t nExplodeX, nExplodeY, nAltitude;
	mapcoord_t nAreaExplodeX, nAreaExplodeY;
	mapcoord_t nAreaCornerX, nAreaCornerY;
	mapcoord_t nOffsetX, nOffsetY;
	int16_t nRubbleTile;
	WORD nLandAlt;
	BYTE bIsFlipped;
	BYTE bTextOverlay;
	CMFC3XPoint pt;
	coords_w_t tileCoords;

#if 0
	// Debugging and testing.
	if (GetAsyncKeyState(VK_MENU) < 0) {
		GameMain_SimcityView_Demolish(pThis, x, y, bExplosion);
		return;
	}
#endif

	pSCApp = &pCSimcityAppThis;
	pLockedBits = Game_Graphics_LockDIBBits(pThis->SCVGraphics);
	pLockedBaseBits = Game_Graphics_LockDIBBits(pBaseGraphics);
	bDoYield = false;
	nX = x;
	nY = y;
	nTileID = GetTileID(nX, nY);
	if (nTileID >= TILE_TREES1) {
		nCornerX = nX;
		nCornerY = nY;
		nArea = Game_FindCorner(&nCornerX, &nCornerY, nTileID);
		nCoordScale = COORDSCALE_VAL(pThis->wSCVZoomLevel);
		nLandAltScale = LANDALTSCALE_VAL(pThis->wSCVZoomLevel);
		nScaleVal = SCALE_VAL(pThis->wSCVZoomLevel);
		nSpriteBase = SPRITE_BOUNDARY_MULTIPLIER * pThis->wSCVZoomLevel;
		Game_DirtyThing(wDisasterObject);
		if (nArea == 1 && (GET_TILE_RANGE(nTileID, TILE_SUSPENSION_BRIDGE_START_B, TILE_ELEVATED_POWERLINES) ||
			GET_TILE_RANGE(nTileID, TILE_REINFORCED_BRIDGE_PYLON, TILE_REINFORCED_BRIDGE))) {
			// Originally this one may have been undefined
			// until it got further down the chain.
			nHighwayTile = -1;
		HighwayChk:
			if (nArea == 2)
				--nCornerY;
			if (nArea == 1 &&
				nCornerX < GAME_MAP_SIZE &&
				nCornerY < GAME_MAP_SIZE &&
				XBITReturnIsFlipped(nCornerX, nCornerY) ||
				nArea == 2 &&
				(nHighwayTile & 1) == 0) {
				nHorzMult = 1;
				nVertMult = 0;
			}
			else {
				nHorzMult = 0;
				nVertMult = 1;
			}
			nStoredHorzMult = nHorzMult * nArea;
			nStoredVertMult = nVertMult * nArea;
			while (TRUE) {
				nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY);
				if (nArea != 2 || nHighwayTile < 13) {
					if (nArea != 1)
						goto AreaChkOne;
					nLoopTileID = GetTileID(nCornerX, nCornerY);
					if ((nLoopTileID < TILE_SUSPENSION_BRIDGE_START_B || nLoopTileID > TILE_ELEVATED_POWERLINES) &&
						nLoopTileID != TILE_REINFORCED_BRIDGE_PYLON &&
						nLoopTileID != TILE_REINFORCED_BRIDGE)
						break;
				}
				nCornerX -= nStoredHorzMult;
				nCornerY -= nStoredVertMult;
			}
			if (nCornerX >= GAME_MAP_SIZE ||
				nCornerY >= GAME_MAP_SIZE ||
				!XBITReturnIsWater(nCornerX, nCornerY)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						nLandAlt = ALTMReturnLandAltitude(nCornerX, nCornerY) - 1;
						ALTMSetLandAltitude(nCornerX, nCornerY, nLandAlt);
						XBITSetBits(nCornerX, nCornerY, XBIT_WATER);
					}
				}
				Game_SetTerrainTile(nCornerX, nCornerY);
				if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE)
					XBITClearBits(nCornerX, nCornerY, XBIT_FLIPPED);
			}
		AreaChkOne:
			if (nArea == 2 && nTileID != TILE_HIGHWAY_LR && nTileID != TILE_HIGHWAY_TB) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_DirtyTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY + 1);
				Game_DirtyTile(nCornerX + 1, nCornerY + 1);
				Game_SetTerrainTile(nCornerX, nCornerY + 1);
				Game_DirtyTile(nCornerX, nCornerY + 1);
			}
			nCornerX += nStoredHorzMult;
			nCornerY += nStoredVertMult;
			if (bExplosion) {
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
				bDoYield = true;
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			}
			nExplodeX = -1;
			nExplodeY = -1;
			while (true) {
				nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY);
				if (nArea != 2 || nHighwayTile < 13) {
					if (nArea != 1)
						goto AreaChkTwo;
					nLoopTileID = GetTileID(nCornerX, nCornerY);
					if ((nLoopTileID < TILE_SUSPENSION_BRIDGE_START_B || nLoopTileID > TILE_ELEVATED_POWERLINES) &&
						nLoopTileID != TILE_REINFORCED_BRIDGE_PYLON &&
						nLoopTileID != TILE_REINFORCED_BRIDGE)
						break;
				}
				Game_DirtyTile(nCornerX, nCornerY);
				if (nArea == 2 && nHighwayTile < 15) {
					Game_DirtyTile(nCornerX, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY + 1);
					Game_DirtyTile(nCornerX + 1, nCornerY);
				}
				if (bExplosion) {
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
					nExplodeY = iScreenOffSetY + nCoordScale * (nCornerX + nCornerY) -
						nLandAltScale * ALTMReturnWaterLevel(nCornerX, nCornerY) -
						pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nCornerX, nCornerY);
						XBITClearBits(nCornerX, nCornerY, XBIT_FLIPPED);
					}
				}
				if (nArea == 2) {
					if (bExplosion) {
						nAreaExplodeX = nExplodeX + nScaleVal;
						nAreaExplodeY = nExplodeY - nCoordScale;
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nAreaExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
						nAreaExplodeX = nExplodeX + 2 * nScaleVal;
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nExplodeY);
						nAreaExplodeY = nExplodeY + nCoordScale;
						bIsFlipped = rand() & 1;
						Game_DrawProcessObject(nSpriteID, nAreaExplodeX, nAreaExplodeY, bIsFlipped, 0);
						Game_DirtyCloud(nSpriteID, nAreaExplodeX, nAreaExplodeY);
					}
					nAreaCornerY = nCornerY + 1;
					Game_PlaceTile(nCornerX, nAreaCornerY, TILE_CLEAR);
					if (nCornerX < GAME_MAP_SIZE && nAreaCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nCornerX, nAreaCornerY);
						XBITClearBits(nCornerX, nAreaCornerY, XBIT_FLIPPED);
					}
					nAreaCornerX = nCornerX + 1;
					Game_PlaceTile(nAreaCornerX, nAreaCornerY, TILE_CLEAR);
					if (nCornerX >= MAP_EDGE_MIN && nAreaCornerX < GAME_MAP_SIZE && nAreaCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nAreaCornerX, nAreaCornerY);
						XBITClearBits(nAreaCornerX, nAreaCornerY, XBIT_FLIPPED);
					}
					Game_PlaceTile(nAreaCornerX, nCornerY, TILE_CLEAR);
					if (nCornerX >= MAP_EDGE_MIN && nAreaCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						XZONClearCorners(nAreaCornerX, nCornerY);
						XBITClearBits(nAreaCornerX, nCornerY, XBIT_FLIPPED);
					}
				}
				nCornerX += nStoredHorzMult;
				nCornerY += nStoredVertMult;
			}
			if (nCornerX >= GAME_MAP_SIZE ||
				nCornerY >= GAME_MAP_SIZE ||
				!XBITReturnIsWater(nCornerX, nCornerY)) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_PlaceTile(nCornerX, nCornerY, TILE_CLEAR);
				if (nCornerX >= MAP_EDGE_MIN) {
					if (nCornerX < GAME_MAP_SIZE && nCornerY < GAME_MAP_SIZE) {
						nLandAlt = ALTMReturnLandAltitude(nCornerX, nCornerY) - 1;
						ALTMSetLandAltitude(nCornerX, nCornerY, nLandAlt);
						XBITSetBits(nCornerX, nCornerY, XBIT_WATER);
					}
				}
				Game_SetTerrainTile(nCornerX, nCornerY);
			}
		AreaChkTwo:
			if (nArea == 2 && nTileID != TILE_HIGHWAY_LR && nTileID != TILE_HIGHWAY_TB) {
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX, nCornerY);
				Game_DirtyTile(nCornerX, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY);
				Game_DirtyTile(nCornerX + 1, nCornerY);
				Game_SetTerrainTile(nCornerX + 1, nCornerY + 1);
				Game_DirtyTile(nCornerX + 1, nCornerY + 1);
				Game_SetTerrainTile(nCornerX, nCornerY + 1);
				Game_DirtyTile(nCornerX, nCornerY + 1);
			}
			if (!bExplosion)
				goto UpdHouse;
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd)
				Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
			else
				Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
			UpdateWindow(pThis->m_hWnd);
			if (bDoYield)
				goto YieldWnd;
			goto PlaySnd;
		}
		if (nArea == 2) {
			nHighwayTile = Game_GetHighwayTile(nCornerX, nCornerY - 1);
			if (nHighwayTile >= 13)
				goto HighwayChk;
		}
		if (GET_TILE_RANGE(nTileID, TILE_INFRASTRUCTURE_PIER, TILE_INFRASTRUCTURE_CRANE)) {
			if (bExplosion)
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			Game_InitStack(nX, nY);
			while (Game_StackSize()) {
				Game_StackPop(&pt);
				tileCoords.x = (mapcoord_t)pt.x;
				tileCoords.y = (mapcoord_t)pt.y;
				Game_PlaceTile(tileCoords.x, tileCoords.y, TILE_CLEAR);
				if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
					XZONClearCorners(tileCoords.x, tileCoords.y);
					XBITClearBits(tileCoords.x, tileCoords.y, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
				}
				if (bExplosion) {
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE && XBITReturnIsWater(tileCoords.x, tileCoords.y))
						nAltitude = ALTMReturnWaterLevel(tileCoords.x, tileCoords.y);
					else
						nAltitude = ALTMReturnLandAltitude(tileCoords.x, tileCoords.y);
					nExplodeY = iScreenOffSetY + (nCoordScale * (tileCoords.x + tileCoords.y)) - nLandAltScale * nAltitude - pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				if (tileCoords.x > MAP_EDGE_MIN)
					L_PierCheckStackPush(tileCoords.x - 1, tileCoords.y);
				if (tileCoords.x < MAP_EDGE_MAX)
					L_PierCheckStackPush(tileCoords.x + 1, tileCoords.y);
				if (tileCoords.y > MAP_EDGE_MIN)
					L_PierCheckStackPush(tileCoords.x, tileCoords.y - 1);
				// This one here was also tileCoords.x.
				// Most likely a bug, commenting
				// just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_PierCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (!bExplosion)
				goto UpdHouse;
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd) {
			FullRdrw:
				Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
			UpdWnd:
				UpdateWindow(pThis->m_hWnd);
			PlaySnd:
				Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
			YieldWnd:
				Game_YieldToWindows(100);
				goto UpdHouse;
			}
		}
		else {
			if (nTileID != TILE_INFRASTRUCTURE_RUNWAY && nTileID != TILE_INFRASTRUCTURE_RUNWAYCROSS) {
				if (nTileID < TILE_TUNNEL_T || nTileID > TILE_TUNNEL_L) {
					if (bExplosion) {
						nExplodeX = iScreenOffSetX + nScaleVal * (nCornerX - nCornerY);
						if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE && XBITReturnIsWater(nX, nY))
							nAltitude = ALTMReturnWaterLevel(nCornerX, nCornerY);
						else
							nAltitude = ALTMReturnLandAltitude(nCornerX, nCornerY);
						nExplodeY = iScreenOffSetY + (nCoordScale * (nCornerX + nCornerY)) - nLandAltScale * nAltitude;
						if (nTileID >= TILE_ARCOLOGY_PLYMOUTH)
							dirtyRect.top = 0;
						if (nArea > 0) {
							int16_t nVertPos = 0;
							int16_t nAreaPos = nArea;
							do {
								L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
								if (nArea > 0) {
									int16_t nHorzPos = 0;
									int16_t nHorzAreaPos = nArea;
									nAreaExplodeX = nExplodeX;
									do {
										if (nArea > 0) {
											int16_t nAreaExplodeIntX = nAreaExplodeX;
											int16_t nCurrHorzPos = nHorzPos;
											int16_t nVertAreaPos = nArea;
											do {
												nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
												nAreaExplodeY = nCurrHorzPos + nExplodeY - pArrSpriteHeaders[nSpriteID].wHeight - nVertPos;
												bIsFlipped = rand() & 1;
												Game_DrawProcessObject(nSpriteID, nAreaExplodeIntX, nAreaExplodeY, bIsFlipped, 0);
												Game_DirtyCloud(nSpriteID, nAreaExplodeIntX, nAreaExplodeY);
												nCurrHorzPos -= nCoordScale;
												nAreaExplodeIntX += nScaleVal;
												--nVertAreaPos;
											} while (nVertAreaPos);
										}
										nHorzPos += nCoordScale;
										nAreaExplodeX += nScaleVal;
										--nHorzAreaPos;
									} while (nHorzAreaPos);
								}
								Game_FinishProcessObjects();
								if (pThis == (CSimcityView *)&pSomeWnd)
									Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
								else
									Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
								UpdateWindow(pThis->m_hWnd);
								if (!bDoYield) {
									Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_EXPLODE);
									bDoYield = true;
								}
								Game_YieldToWindows(100);
								nVertPos += nCoordScale;
								--nAreaPos;
							} while (nAreaPos);
						}
					}
					bTextOverlay = XTXTGetTextOverlayID(nCornerX, nCornerY);
					if (nTileID == TILE_INFRASTRUCTURE_MAYORSHOUSE)
						Game_SimulationToggleGrantReward(0, 1);
					if (nTileID == TILE_SERVICES_CITYHALL)
						Game_SimulationToggleGrantReward(1, 1);
					if (nTileID == TILE_SERVICES_STATUE)
						Game_SimulationToggleGrantReward(2, 1);
					if (nTileID == TILE_OTHER_BRAUNLLAMADOME)
						Game_SimulationToggleGrantReward(3, 1);
					if (nTileID == TILE_SERVICES_STADIUM &&
						bTextOverlay >= MIN_SIM_TEXT_ENTRIES &&
						bTextOverlay <= MAX_SIM_TEXT_ENTRIES) {
						BYTE bMicrosimEntry = MICROSIMID_ENTRY(bTextOverlay);
						if (GetMicroSimulatorTileID(bMicrosimEntry) == nTileID)
							wStadiumSportsTeams += -1 << GetMicroSimulatorStat2(bMicrosimEntry);
					}
					for (__int16 nPosX = 0; nArea > nPosX; ++nPosX) {
						for (__int16 nPosY = 0; nArea > nPosY; ++nPosY) {
							__int16 nCurrX = nPosX + nCornerX;
							__int16 nCurrY = nCornerY - nPosY;
							if (nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
								nRubbleTile = (GetTerrainTileID(nCurrX, nCurrY)) ? TILE_CLEAR : (rand() & 3) + 1;
								Game_PlaceTile(nCurrX, nCurrY, nRubbleTile);
								if (nCurrX >= MAP_EDGE_MIN && nCurrX < GAME_MAP_SIZE && nCurrY < GAME_MAP_SIZE) {
									XBITClearBits(nCurrX, nCurrY, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
									XZONClearCorners(nCurrX, nCurrY);
								}
								bTextOverlay = XTXTGetTextOverlayID(nCurrX, nCurrY);
								if (bTextOverlay) {
									if (bTextOverlay <= MAX_XTHG_TEXT_ENTRIES || bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
										if (bTextOverlay <= MAX_SIM_TEXT_ENTRIES || bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY)
											XTXTSetTextOverlayID(nCurrX, nCurrY, 0);
										Game_RemoveLabel(bTextOverlay);
										if (bTextOverlay == NGHBR_CONNECTION_TEXT_ENTRY) {
											if (GET_TILE_RANGE(nTileID, TILE_ROAD_LR, TILE_ROAD_LTBR) ||
												GET_TILE_RANGE(nTileID, TILE_TUNNEL_T, TILE_CROSSOVER_ROADTB_RAILLR) ||
												GET_TILE_RANGE(nTileID, TILE_CROSSOVER_HIGHWAYLR_ROADTB, TILE_CROSSOVER_HIGHWAYTB_ROADLR) ||
												GET_TILE_RANGE(nTileID, TILE_ONRAMP_TL, TILE_ONRAMP_BR))
												--wCommerceConnect;
											else
												--wIndustryConnect;
										}
									}
								}
							}
						}
					}
					if (GET_TILE_RANGE(nTileID, TILE_HIGHWAY_HTB, TILE_REINFORCED_BRIDGE) ||
						GET_TILE_RANGE(nTileID, TILE_HIGHWAY_LR, TILE_CROSSOVER_HIGHWAYTB_POWERLR)) {
						Game_SetTerrainTile(nCornerX, nCornerY);
						Game_SetTerrainTile(nCornerX + 1, nCornerY);
						Game_SetTerrainTile(nCornerX + 1, nCornerY - 1);
						Game_SetTerrainTile(nCornerX, nCornerY - 1);
					}
					if (nArea == 1 && nTileID <= TILE_SUBTORAIL_L) {
						if (GetTerrainTileID(nCornerX, nCornerY))
							Game_SetTerrainTile(nCornerX, nCornerY);
					}
					goto UpdHouse;
				}
				nOffsetX = 0;
				nOffsetY = 0;
				switch (nTileID) {
					case TILE_TUNNEL_T:
						nOffsetX = -1;
						nOffsetY = 0;
						break;
					case TILE_TUNNEL_R:
						nOffsetX = 0;
						nOffsetY = -1;
						break;
					case TILE_TUNNEL_B:
						nOffsetX = 1;
						nOffsetY = 0;
						break;
					case TILE_TUNNEL_L:
						nOffsetX = 0;
						nOffsetY = 1;
						break;
					default:
						break;
				}
				Game_DirtyTile(nX, nY);
				nTileID = TILE_CLEAR;
				nRubbleTile = (rand() & 3) + 1;
				Game_PlaceTile(nX, nY, nRubbleTile);
				if (nX >= MAP_EDGE_MIN) {
					if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE) {
						XZONClearCorners(nX, nY);
						ALTMSetTunnelLevels(nX, nY, 0);
					}
				}
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
				nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				bIsFlipped = rand() & 1;
				Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
				Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				while (nTileID < TILE_TUNNEL_T || nTileID > TILE_TUNNEL_L) {
					nX += nOffsetX;
					nY += nOffsetY;
					if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
						ALTMSetTunnelLevels(nX, nY, 0);
					nTileID = GetTileID(nX, nY);
				}
				Game_DirtyTile(nX, nY);
				nRubbleTile = (rand() & 3) + 1;
				Game_PlaceTile(nX, nY, nRubbleTile);
				if (nX < GAME_MAP_SIZE && nY < GAME_MAP_SIZE)
					XZONClearCorners(nX, nY);
				nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
				nExplodeX = iScreenOffSetX + nScaleVal * (nX - nY);
				nExplodeY = iScreenOffSetY + nCoordScale * (nX + nY) - nLandAltScale * ALTMReturnLandAltitude(nX, nY) - pArrSpriteHeaders[nSpriteID].wHeight;
				bIsFlipped = rand() & 1;
				Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
				Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				Game_FinishProcessObjects();
				if (pThis == (CSimcityView *)&pSomeWnd)
					Game_SimcityView_MainWindowUpdate(pThis, NULL, TRUE);
				else
					Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
				UpdateWindow(pThis->m_hWnd);
				if (!bExplosion) {
				UpdHouse:
					mapcoord_t nFirstPosX = nX;
					mapcoord_t nSecondPosX = nX;
					mapcoord_t nAreaPosX = (nX + nArea);
					if (nAreaPosX > nX) {
						do {
							mapcoord_t nCurrPosX = nSecondPosX++;
							Game_DirtyTile(nCurrPosX - 1, nY - 1);
							Game_DirtyTile(nCurrPosX - 2, nY - 2);
							Game_DirtyTile(nCurrPosX - 3, nY - 3);
							Game_DirtyTile(nCurrPosX - 4, nY - 4);
						} while (nSecondPosX < nAreaPosX);
					}
					mapcoord_t nFirstPosY = nY;
					mapcoord_t nSecondPosY = nY;
					mapcoord_t nAreaPosY = (nY + nArea);
					if (nAreaPosY > nY) {
						do {
							mapcoord_t nCurrPosY = nSecondPosY++;
							Game_DirtyTile(nX - 1, nCurrPosY - 1);
							Game_DirtyTile(nX - 2, nCurrPosY - 2);
							Game_DirtyTile(nX - 3, nCurrPosY - 3);
							Game_DirtyTile(nX - 4, nCurrPosY - 4);
						} while (nSecondPosY < nAreaPosY);
					}
					if (nAreaPosX > nX) {
						do {
							for (mapcoord_t nCurrPosY = nFirstPosY; nCurrPosY < nAreaPosY; ++nCurrPosY)
								Game_DirtyTile(nFirstPosX, nCurrPosY);
							++nFirstPosX;
						} while (nFirstPosX < nAreaPosX);
					}
					Game_SimcityView_UpdateHouse(pThis);
					return;
				}
				goto PlaySnd;
			}
			if (bExplosion)
				L_BeginProcessObjects_SC2K1996(pThis->m_hWnd, pLockedBaseBits, pLockedBits, pThis->dwSCVGraphicWidth, pThis->dwSCVGraphicHeight, &pThis->SCVAreaView);
			Game_InitStack(nX, nY);
			while (Game_StackSize()) {
				Game_StackPop(&pt);
				tileCoords.x = (mapcoord_t)pt.x;
				tileCoords.y = (mapcoord_t)pt.y;
				nRubbleTile = (rand() & 3) + 1;
				Game_PlaceTile(tileCoords.x, tileCoords.y, nRubbleTile);
				if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
					XZONClearCorners(tileCoords.x, tileCoords.y);
					XBITClearBits(tileCoords.x, tileCoords.y, XBIT_FLIPPED|XBIT_POWERED|XBIT_POWERABLE);
				}
				if (bExplosion) {
					nSpriteID = (rand() & 3) + nSpriteBase + SPRITE_SMALL_DUSTCLOUD1;
					nExplodeX = iScreenOffSetX + nScaleVal * (tileCoords.x - tileCoords.y);
					nExplodeY = iScreenOffSetY + nCoordScale * (tileCoords.x + tileCoords.y) - nLandAltScale * ALTMReturnLandAltitude(tileCoords.x, tileCoords.y) - pArrSpriteHeaders[nSpriteID].wHeight;
					bIsFlipped = rand() & 1;
					Game_DrawProcessObject(nSpriteID, nExplodeX, nExplodeY, bIsFlipped, 0);
					Game_DirtyCloud(nSpriteID, nExplodeX, nExplodeY);
				}
				if (tileCoords.x > MAP_EDGE_MIN)
					L_RunwayCheckStackPush(tileCoords.x - 1, tileCoords.y);
				if (tileCoords.x < MAP_EDGE_MAX)
					L_RunwayCheckStackPush(tileCoords.x + 1, tileCoords.y);
				if (tileCoords.y > MAP_EDGE_MIN)
					L_RunwayCheckStackPush(tileCoords.x, tileCoords.y - 1);
				// This one here was also tileCoords.x.
				// Most likely a bug, commenting
				// just in case.
				if (tileCoords.y < MAP_EDGE_MAX)
					L_RunwayCheckStackPush(tileCoords.x, tileCoords.y + 1);
			}
			if (!bExplosion)
				goto UpdHouse;
			Game_FinishProcessObjects();
			if (pThis == (CSimcityView *)&pSomeWnd)
				goto FullRdrw;
		}
		Game_SimcityView_MainWindowUpdate(pThis, &dirtyRect, TRUE);
		goto UpdWnd;
	}
}

void InstallTileGrowthOrPlacementHandlingHooks_SC2K1996(void) {
	// Hook into the SimulationGrowthTick function
	// DEPRECATED -- now local function
	SafeVirtualProtect((LPVOID)0x4022FC, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4022FC, Simulation_DoGrowthTick);

	// Hook into the SimulationGrowSpecificZone function
	// DEPRECATED -- now local function
	SafeVirtualProtect((LPVOID)0x4026B2, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4026B2, Simulation_GrowSpecificZone);

	// Hook into the PlacePowerLinesAtCoordinates function
	SafeVirtualProtect((LPVOID)0x402725, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402725, Hook_PlacePowerLinesAtCoordinates);

	// Hook into the ItemPlacementCheck function
	SafeVirtualProtect((LPVOID)0x4027F2, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4027F2, Hook_ItemPlacementCheck);

	// Hook CityToolPlaceSelectedBuilding
	SafeVirtualProtect((LPVOID)0x401005, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x401005, Hook_CityToolPlaceSelectedBuilding);

	// Hook for CSimcityView::Demolish()
	SafeVirtualProtect((LPVOID)0x402211, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x402211, Hook_SimcityView_Demolish);

	// Military base hooks
	InstallMilitaryHooks_SC2K1996();
}
