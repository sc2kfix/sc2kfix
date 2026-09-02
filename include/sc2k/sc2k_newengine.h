// sc2kfix include/sc2k/sc2k_newengine.h: definitions for locally reimplemented engine functions
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Things to do when moving things here:
//  - Check against IDA to make sure there's no calls to it outside of our own code
//  - Figure out which prefix it should get (eg. Simulation_, Scenario_, SavedGame_...)
//  - See if we can update the code to use new standards (32-bit ints instead of 8 and 16, Windows
//    types and __intXX types switched out for stdint ones, etc.)
//  - Tag the function with NEWENGINE once all of the above is done so we know it's up to spec

#pragma once

#define NEWENGINE

#include <stdint.h>

// Note: The main global arrays are currently present in sc2k_1996.cpp.
//       They can likely be moved to a more generic area, however since
//       that's the primary target we work with (WinSCURK aside) it
//       seemed to be the most reasonable placement.

extern __int16 wRotateCoordShiftX[VIEWROTATION_COUNT];
extern __int16 wRotateCoordShiftY[VIEWROTATION_COUNT];

extern __int16 wCornerStartBottomLeft[VIEWROTATION_COUNT];
extern __int16 wCornerStartBottomRight[VIEWROTATION_COUNT];
extern __int16 wCornerStartTopLeft[VIEWROTATION_COUNT];
extern __int16 wCornerStartTopRight[VIEWROTATION_COUNT];

extern coords_w_t cornerCoords[VIEWROTATION_COUNT];
extern coords_w_t directionalSteps[VIEWROTATION_COUNT];
extern __int16 advanceX[VIEWROTATION_COUNT];
extern __int16 advanceY[VIEWROTATION_COUNT];

NEWENGINE void Simulation_ProcessTick(void);
NEWENGINE void Simulation_DoGrowthTick(int iStep, int iSubStep);
NEWENGINE bool Simulation_GrowSpecificZone(mapcoord_t iX, mapcoord_t iY, uint32_t iTileID, int16_t iZoneType);
NEWENGINE int Simulation_RunTripGenerator(mapcoord_t x, mapcoord_t y, int16_t nZoneType, int nBuildingPopLevel, int nTripMaxSteps);