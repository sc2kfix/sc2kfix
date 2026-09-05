// sc2kfix include/sc2k/sc2k_newengine.h: definitions for locally reimplemented engine functions
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// Things to do when moving things here:
//  - Check against IDA to make sure there's no calls to it outside of our own code
//  - Figure out which prefix it should get (eg. Simulation_, Scenario_, SavedGame_...)
//  - See if we can update the code to use new standards (32-bit ints instead of 8 and 16, Windows
//    types and __intXX types switched out for stdint ones, etc.)
//  - For functions, put a light documentation comment above the declaration for smaller ones and
//    a more comprehensive documentation comment above the declaration for larger ones (eg. any
//    function with modding hooks, stuff that hasn't been completely demystified, complex state
//    machines, etc; use your best judgment)
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

NEWENGINE void Simulation_Orchestrator_ProcessTick(void);

NEWENGINE void Simulation_Growth_StartTick(int iStep, int iSubStep);
NEWENGINE bool Simulation_Growth_GrowSpecificZone(mapcoord_t iX, mapcoord_t iY, uint32_t iTileID, int16_t iZoneType);
NEWENGINE int Simulation_Growth_RunTripGenerator(mapcoord_t x, mapcoord_t y, int16_t nZoneType, int nBuildingPopLevel, int nTripMaxSteps);

NEWENGINE void Save_MakeCityNameFromFileName(const char* lpFileName);
NEWENGINE bool Save_SaveCitySC2X(FILE* fOut);
NEWENGINE bool Save_LoadCitySC2X(FILE* fIn, const char* lpFileName);