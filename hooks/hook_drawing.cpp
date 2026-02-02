// sc2kfix hooks/hook_drawing.cpp: map drawing hooks
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#pragma intrinsic(_ReturnAddress)

enum {
	SIZE_LARGE,
	SIZE_SMALL,
	SIZE_TINY,

	SIZE_LEVELS
};

enum {
	AXIS_HORZ,
	AXIS_VERT,

	AXIS_COUNT
};

// Decrement values
#define DECR_LARGE 12
#define DECR_SMALL (DECR_LARGE / 2)
#define DECR_TINY (DECR_SMALL / 2)

// Building offsets
#define BLDOFF_LARGE 8
#define BLDOFF_SMALL (BLDOFF_LARGE / 2)
#define BLDOFF_TINY (BLDOFF_SMALL / 2)

// Power outage indicator offsets
#define PWROFF_LARGE 16
#define PWROFF_SMALL (PWROFF_LARGE / 2)
#define PWROFF_TINY (PWROFF_SMALL / 2)

// Building terrain edge offsets
// NOTE: Ordering for X is descending and Y is ascending
#define BLDOFF_DEFAULT 0

// 2x2
// X
#define BLDOFF_2X2_X_HR_LAST_LARGE 32
#define BLDOFF_2X2_X_VT_LAST_LARGE BLDOFF_DEFAULT

#define BLDOFF_2X2_X_HR_FIRST_LARGE 16
#define BLDOFF_2X2_X_VT_FIRST_LARGE 8

#define BLDOFF_2X2_X_HR_LAST_SMALL (BLDOFF_2X2_X_HR_LAST_LARGE / 2)
#define BLDOFF_2X2_X_VT_LAST_SMALL BLDOFF_2X2_X_VT_LAST_LARGE

#define BLDOFF_2X2_X_HR_FIRST_SMALL (BLDOFF_2X2_X_HR_FIRST_LARGE / 2)
#define BLDOFF_2X2_X_VT_FIRST_SMALL (BLDOFF_2X2_X_VT_FIRST_LARGE / 2)

#define BLDOFF_2X2_X_HR_LAST_TINY (BLDOFF_2X2_X_HR_LAST_SMALL / 2)
#define BLDOFF_2X2_X_VT_LAST_TINY BLDOFF_2X2_X_VT_LAST_SMALL

#define BLDOFF_2X2_X_HR_FIRST_TINY (BLDOFF_2X2_X_HR_FIRST_SMALL / 2)
#define BLDOFF_2X2_X_VT_FIRST_TINY (BLDOFF_2X2_X_VT_FIRST_SMALL / 2)

// Y
#define BLDOFF_2X2_Y_HR_FIRST_LARGE BLDOFF_DEFAULT
#define BLDOFF_2X2_Y_VT_FIRST_LARGE BLDOFF_DEFAULT

#define BLDOFF_2X2_Y_HR_LAST_LARGE 16
#define BLDOFF_2X2_Y_VT_LAST_LARGE 8

#define BLDOFF_2X2_Y_HR_FIRST_SMALL BLDOFF_2X2_Y_HR_FIRST_LARGE
#define BLDOFF_2X2_Y_VT_FIRST_SMALL BLDOFF_2X2_Y_VT_FIRST_LARGE

#define BLDOFF_2X2_Y_HR_LAST_SMALL (BLDOFF_2X2_Y_HR_LAST_LARGE / 2)
#define BLDOFF_2X2_Y_VT_LAST_SMALL (BLDOFF_2X2_Y_VT_LAST_LARGE / 2)

#define BLDOFF_2X2_Y_HR_FIRST_TINY BLDOFF_2X2_Y_HR_FIRST_SMALL
#define BLDOFF_2X2_Y_VT_FIRST_TINY BLDOFF_2X2_Y_VT_FIRST_SMALL

#define BLDOFF_2X2_Y_HR_LAST_TINY (BLDOFF_2X2_Y_HR_LAST_SMALL / 2)
#define BLDOFF_2X2_Y_VT_LAST_TINY (BLDOFF_2X2_Y_VT_LAST_SMALL / 2)

// 3x3
// X
#define BLDOFF_3X3_X_HR_LAST_LARGE 64
#define BLDOFF_3X3_X_VT_LAST_LARGE BLDOFF_DEFAULT

#define BLDOFF_3X3_X_HR_SECOND_LARGE 48
#define BLDOFF_3X3_X_VT_SECOND_LARGE 8

#define BLDOFF_3X3_X_HR_FIRST_LARGE 32
#define BLDOFF_3X3_X_VT_FIRST_LARGE 16

#define BLDOFF_3X3_X_HR_LAST_SMALL (BLDOFF_3X3_X_HR_LAST_LARGE / 2)
#define BLDOFF_3X3_X_VT_LAST_SMALL BLDOFF_3X3_X_VT_LAST_LARGE

#define BLDOFF_3X3_X_HR_SECOND_SMALL (BLDOFF_3X3_X_HR_SECOND_LARGE / 2)
#define BLDOFF_3X3_X_VT_SECOND_SMALL (BLDOFF_3X3_X_VT_SECOND_LARGE / 2)

#define BLDOFF_3X3_X_HR_FIRST_SMALL (BLDOFF_3X3_X_HR_FIRST_LARGE / 2)
#define BLDOFF_3X3_X_VT_FIRST_SMALL (BLDOFF_3X3_X_VT_FIRST_LARGE / 2)

#define BLDOFF_3X3_X_HR_LAST_TINY (BLDOFF_3X3_X_HR_LAST_SMALL / 2)
#define BLDOFF_3X3_X_VT_LAST_TINY BLDOFF_3X3_X_VT_LAST_SMALL

#define BLDOFF_3X3_X_HR_SECOND_TINY (BLDOFF_3X3_X_HR_SECOND_SMALL / 2)
#define BLDOFF_3X3_X_VT_SECOND_TINY (BLDOFF_3X3_X_VT_SECOND_SMALL / 2)

#define BLDOFF_3X3_X_HR_FIRST_TINY (BLDOFF_3X3_X_HR_FIRST_SMALL / 2)
#define BLDOFF_3X3_X_VT_FIRST_TINY (BLDOFF_3X3_X_VT_FIRST_SMALL / 2)

// Y
#define BLDOFF_3X3_Y_HR_FIRST_LARGE BLDOFF_DEFAULT
#define BLDOFF_3X3_Y_VT_FIRST_LARGE BLDOFF_DEFAULT

#define BLDOFF_3X3_Y_HR_SECOND_LARGE BLDOFF_2X2_Y_HR_LAST_LARGE
#define BLDOFF_3X3_Y_VT_SECOND_LARGE BLDOFF_2X2_Y_VT_LAST_LARGE

#define BLDOFF_3X3_Y_HR_LAST_LARGE 32
#define BLDOFF_3X3_Y_VT_LAST_LARGE 16

#define BLDOFF_3X3_Y_HR_FIRST_SMALL BLDOFF_3X3_Y_HR_FIRST_LARGE
#define BLDOFF_3X3_Y_VT_FIRST_SMALL BLDOFF_3X3_Y_VT_FIRST_LARGE

#define BLDOFF_3X3_Y_HR_SECOND_SMALL (BLDOFF_3X3_Y_HR_SECOND_LARGE / 2)
#define BLDOFF_3X3_Y_VT_SECOND_SMALL (BLDOFF_3X3_Y_VT_SECOND_LARGE / 2)

#define BLDOFF_3X3_Y_HR_LAST_SMALL (BLDOFF_3X3_Y_HR_LAST_LARGE / 2)
#define BLDOFF_3X3_Y_VT_LAST_SMALL (BLDOFF_3X3_Y_VT_LAST_LARGE / 2)

#define BLDOFF_3X3_Y_HR_FIRST_TINY BLDOFF_3X3_Y_HR_FIRST_SMALL
#define BLDOFF_3X3_Y_VT_FIRST_TINY BLDOFF_3X3_Y_VT_FIRST_SMALL

#define BLDOFF_3X3_Y_HR_SECOND_TINY (BLDOFF_3X3_Y_HR_SECOND_SMALL / 2)
#define BLDOFF_3X3_Y_VT_SECOND_TINY (BLDOFF_3X3_Y_VT_SECOND_SMALL / 2)

#define BLDOFF_3X3_Y_HR_LAST_TINY (BLDOFF_3X3_Y_HR_LAST_SMALL / 2)
#define BLDOFF_3X3_Y_VT_LAST_TINY (BLDOFF_3X3_Y_VT_LAST_SMALL / 2)

// 4x4
// X
#define BLDOFF_4X4_X_HR_LAST_LARGE 96
#define BLDOFF_4X4_X_VT_LAST_LARGE BLDOFF_DEFAULT

#define BLDOFF_4X4_X_HR_THIRD_LARGE 80
#define BLDOFF_4X4_X_VT_THIRD_LARGE 8

#define BLDOFF_4X4_X_HR_SECOND_LARGE 64
#define BLDOFF_4X4_X_VT_SECOND_LARGE 16

#define BLDOFF_4X4_X_HR_FIRST_LARGE 48
#define BLDOFF_4X4_X_VT_FIRST_LARGE 24

#define BLDOFF_4X4_X_HR_LAST_SMALL (BLDOFF_4X4_X_HR_LAST_LARGE / 2)
#define BLDOFF_4X4_X_VT_LAST_SMALL BLDOFF_4X4_X_VT_LAST_LARGE

#define BLDOFF_4X4_X_HR_THIRD_SMALL (BLDOFF_4X4_X_HR_THIRD_LARGE / 2)
#define BLDOFF_4X4_X_VT_THIRD_SMALL (BLDOFF_4X4_X_VT_THIRD_LARGE / 2)

#define BLDOFF_4X4_X_HR_SECOND_SMALL (BLDOFF_4X4_X_HR_SECOND_LARGE / 2)
#define BLDOFF_4X4_X_VT_SECOND_SMALL (BLDOFF_4X4_X_VT_SECOND_LARGE / 2)

#define BLDOFF_4X4_X_HR_FIRST_SMALL (BLDOFF_4X4_X_HR_FIRST_LARGE / 2)
#define BLDOFF_4X4_X_VT_FIRST_SMALL (BLDOFF_4X4_X_VT_FIRST_LARGE / 2)

#define BLDOFF_4X4_X_HR_LAST_TINY (BLDOFF_4X4_X_HR_LAST_SMALL / 2)
#define BLDOFF_4X4_X_VT_LAST_TINY BLDOFF_4X4_X_VT_LAST_SMALL

#define BLDOFF_4X4_X_HR_THIRD_TINY (BLDOFF_4X4_X_HR_THIRD_SMALL / 2)
#define BLDOFF_4X4_X_VT_THIRD_TINY (BLDOFF_4X4_X_VT_THIRD_SMALL / 2)

#define BLDOFF_4X4_X_HR_SECOND_TINY (BLDOFF_4X4_X_HR_SECOND_SMALL / 2)
#define BLDOFF_4X4_X_VT_SECOND_TINY (BLDOFF_4X4_X_VT_SECOND_SMALL / 2)

#define BLDOFF_4X4_X_HR_FIRST_TINY (BLDOFF_4X4_X_HR_FIRST_SMALL / 2)
#define BLDOFF_4X4_X_VT_FIRST_TINY (BLDOFF_4X4_X_VT_FIRST_SMALL / 2)

// Y
#define BLDOFF_4X4_Y_HR_FIRST_LARGE BLDOFF_DEFAULT
#define BLDOFF_4X4_Y_VT_FIRST_LARGE BLDOFF_DEFAULT

#define BLDOFF_4X4_Y_HR_SECOND_LARGE BLDOFF_2X2_Y_HR_LAST_LARGE
#define BLDOFF_4X4_Y_VT_SECOND_LARGE BLDOFF_2X2_Y_VT_LAST_LARGE

#define BLDOFF_4X4_Y_HR_THIRD_LARGE BLDOFF_3X3_Y_HR_LAST_LARGE
#define BLDOFF_4X4_Y_VT_THIRD_LARGE BLDOFF_3X3_Y_VT_LAST_LARGE

#define BLDOFF_4X4_Y_HR_LAST_LARGE 48
#define BLDOFF_4X4_Y_VT_LAST_LARGE 24

#define BLDOFF_4X4_Y_HR_FIRST_SMALL BLDOFF_4X4_Y_HR_FIRST_LARGE
#define BLDOFF_4X4_Y_VT_FIRST_SMALL BLDOFF_4X4_Y_VT_FIRST_LARGE

#define BLDOFF_4X4_Y_HR_SECOND_SMALL (BLDOFF_4X4_Y_HR_SECOND_LARGE / 2)
#define BLDOFF_4X4_Y_VT_SECOND_SMALL (BLDOFF_4X4_Y_VT_SECOND_LARGE / 2)

#define BLDOFF_4X4_Y_HR_THIRD_SMALL (BLDOFF_4X4_Y_HR_THIRD_LARGE / 2)
#define BLDOFF_4X4_Y_VT_THIRD_SMALL (BLDOFF_4X4_Y_VT_THIRD_LARGE / 2)

#define BLDOFF_4X4_Y_HR_LAST_SMALL (BLDOFF_4X4_Y_HR_LAST_LARGE / 2)
#define BLDOFF_4X4_Y_VT_LAST_SMALL (BLDOFF_4X4_Y_VT_LAST_LARGE / 2)

#define BLDOFF_4X4_Y_HR_FIRST_TINY BLDOFF_4X4_Y_HR_FIRST_SMALL
#define BLDOFF_4X4_Y_VT_FIRST_TINY BLDOFF_4X4_Y_VT_FIRST_SMALL

#define BLDOFF_4X4_Y_HR_SECOND_TINY (BLDOFF_4X4_Y_HR_SECOND_SMALL / 2)
#define BLDOFF_4X4_Y_VT_SECOND_TINY (BLDOFF_4X4_Y_VT_SECOND_SMALL / 2)

#define BLDOFF_4X4_Y_HR_THIRD_TINY (BLDOFF_4X4_Y_HR_THIRD_SMALL / 2)
#define BLDOFF_4X4_Y_VT_THIRD_TINY (BLDOFF_4X4_Y_VT_THIRD_SMALL / 2)

#define BLDOFF_4X4_Y_HR_LAST_TINY (BLDOFF_4X4_Y_HR_LAST_SMALL / 2)
#define BLDOFF_4X4_Y_VT_LAST_TINY (BLDOFF_4X4_Y_VT_LAST_SMALL / 2)

// Raising bridge
#define RAISE_THRESHOLD 8

// Highway tile terrain offsets.
#define HWY_HR_TROFF_LARGE 16
#define HWY_VT_TROFF_LARGE 8

#define HWY_HR_BROFF_LARGE 32

#define HWY_HR_BLOFF_LARGE HWY_HR_TROFF_LARGE
#define HWY_VT_BLOFF_LARGE HWY_VT_TROFF_LARGE

#define HWY_HR_TROFF_SMALL (HWY_HR_TROFF_LARGE / 2)
#define HWY_VT_TROFF_SMALL (HWY_VT_TROFF_LARGE / 2)

#define HWY_HR_BROFF_SMALL (HWY_HR_BROFF_LARGE / 2)

#define HWY_HR_BLOFF_SMALL HWY_HR_TROFF_SMALL
#define HWY_VT_BLOFF_SMALL HWY_VT_TROFF_SMALL

#define HWY_HR_TROFF_TINY (HWY_HR_TROFF_SMALL / 2)
#define HWY_VT_TROFF_TINY (HWY_VT_TROFF_SMALL / 2)

#define HWY_HR_BROFF_TINY (HWY_HR_BROFF_SMALL / 2)

#define HWY_HR_BLOFF_TINY HWY_HR_TROFF_TINY
#define HWY_VT_BLOFF_TINY HWY_VT_TROFF_TINY

#define MDRAWING_DEBUG_OTHER 1

#define MDRAWING_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef MDRAWING_DEBUG
#define MDRAWING_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT mdrawing_debug = MDRAWING_DEBUG;

static DWORD dwDummy;

static __int16 drawDecr[SIZE_LEVELS] = {
	DECR_LARGE,
	DECR_SMALL,
	DECR_TINY
};

static __int16 buildOffset[SIZE_LEVELS] = {
	BLDOFF_LARGE,
	BLDOFF_SMALL,
	BLDOFF_TINY
};

static __int16 pwrIndOffset[SIZE_LEVELS] = {
	PWROFF_LARGE,
	PWROFF_SMALL,
	PWROFF_TINY
};

static __int16 spriteBedrockSize[SIZE_LEVELS] = {
	SPRITE_LARGE_BEDROCK,
	SPRITE_MEDIUM_BEDROCK,
	SPRITE_SMALL_BEDROCK
};

static __int16 spriteWaterfallSize[SIZE_LEVELS] = {
	SPRITE_LARGE_WATERFALL,
	SPRITE_MEDIUM_WATERFALL,
	SPRITE_SMALL_WATERFALL
};

static __int16 spriteStartOffset[SIZE_LEVELS] = {
	SPRITE_LARGE_START,
	SPRITE_MEDIUM_START,
	SPRITE_SMALL_START
};

static __int16 spritePowerIndSize[SIZE_LEVELS] = {
	SPRITE_LARGE_POWEROUTAGEINDICATOR,
	SPRITE_MEDIUM_POWEROUTAGEINDICATOR,
	SPRITE_SMALL_POWEROUTAGEINDICATOR
};

static __int16 spriteWaterTerOffset[SIZE_LEVELS] = {
	SPRITE_LARGE_WATER_R_TERRAIN_TBL,
	SPRITE_MEDIUM_WATER_R_TERRAIN_TBL,
	SPRITE_SMALL_WATER_R_TERRAIN_TBL
};

static __int16 spriteGreenTileOffset[SIZE_LEVELS] = {
	SPRITE_LARGE_GREENTILE,
	SPRITE_MEDIUM_GREENTILE,
	SPRITE_SMALL_GREENTILE
};

static __int16 spriteHighwayOffset[SIZE_LEVELS] = {
	SPRITE_LARGE_HIGHWAY_LR,
	SPRITE_MEDIUM_HIGHWAY_LR,
	SPRITE_SMALL_HIGHWAY_LR
};

static __int16 spriteSuspBridgeOffset[SIZE_LEVELS] = {
	SPRITE_LARGE_SUSPENSION_BRIDGE_START_B,
	SPRITE_MEDIUM_SUSPENSION_BRIDGE_START_B,
	SPRITE_SMALL_SUSPENSION_BRIDGE_START_B
};

static __int16 spriteFire4Offset[SIZE_LEVELS] = {
	SPRITE_LARGE_FIRE4,
	SPRITE_MEDIUM_FIRE4,
	SPRITE_SMALL_FIRE4
};

static __int16 coverage2x2OffsetsX[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_2x2] = {
	{
		{ BLDOFF_2X2_X_HR_LAST_LARGE, BLDOFF_2X2_X_HR_FIRST_LARGE },
		{ BLDOFF_2X2_X_VT_LAST_LARGE, BLDOFF_2X2_X_VT_FIRST_LARGE }
	},
	{
		{ BLDOFF_2X2_X_HR_LAST_SMALL, BLDOFF_2X2_X_HR_FIRST_SMALL },
		{ BLDOFF_2X2_X_VT_LAST_SMALL, BLDOFF_2X2_X_VT_FIRST_SMALL }
	},
	{
		{ BLDOFF_2X2_X_HR_LAST_TINY,  BLDOFF_2X2_X_HR_FIRST_TINY  },
		{ BLDOFF_2X2_X_VT_LAST_TINY,  BLDOFF_2X2_X_VT_FIRST_TINY  }
	}
};

static __int16 coverage3x3OffsetsX[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_3x3] = {
	{
		{ BLDOFF_3X3_X_HR_LAST_LARGE, BLDOFF_3X3_X_HR_SECOND_LARGE, BLDOFF_3X3_X_HR_FIRST_LARGE },
		{ BLDOFF_3X3_X_VT_LAST_LARGE, BLDOFF_3X3_X_VT_SECOND_LARGE, BLDOFF_3X3_X_VT_FIRST_LARGE }
	},
	{
		{ BLDOFF_3X3_X_HR_LAST_SMALL, BLDOFF_3X3_X_HR_SECOND_SMALL, BLDOFF_3X3_X_HR_FIRST_SMALL },
		{ BLDOFF_3X3_X_VT_LAST_SMALL, BLDOFF_3X3_X_VT_SECOND_SMALL, BLDOFF_3X3_X_VT_FIRST_SMALL }
	},
	{
		{ BLDOFF_3X3_X_HR_LAST_TINY,  BLDOFF_3X3_X_HR_SECOND_TINY,  BLDOFF_3X3_X_HR_FIRST_TINY  },
		{ BLDOFF_3X3_X_VT_LAST_TINY,  BLDOFF_3X3_X_VT_SECOND_TINY,  BLDOFF_3X3_X_VT_FIRST_TINY  }
	}
};

static __int16 coverage4x4OffsetsX[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_4x4] = {
	{
		{ BLDOFF_4X4_X_HR_LAST_LARGE, BLDOFF_4X4_X_HR_THIRD_LARGE, BLDOFF_4X4_X_HR_SECOND_LARGE, BLDOFF_4X4_X_HR_FIRST_LARGE },
		{ BLDOFF_4X4_X_VT_LAST_LARGE, BLDOFF_4X4_X_VT_THIRD_LARGE, BLDOFF_4X4_X_VT_SECOND_LARGE, BLDOFF_4X4_X_VT_FIRST_LARGE }
	},
	{
		{ BLDOFF_4X4_X_HR_LAST_SMALL, BLDOFF_4X4_X_HR_THIRD_SMALL, BLDOFF_4X4_X_HR_SECOND_SMALL, BLDOFF_4X4_X_HR_FIRST_SMALL },
		{ BLDOFF_4X4_X_VT_LAST_SMALL, BLDOFF_4X4_X_VT_THIRD_SMALL, BLDOFF_4X4_X_VT_SECOND_SMALL, BLDOFF_4X4_X_VT_FIRST_SMALL }
	},
	{
		{ BLDOFF_4X4_X_HR_LAST_TINY,  BLDOFF_4X4_X_HR_THIRD_TINY,  BLDOFF_4X4_X_HR_SECOND_TINY,  BLDOFF_4X4_X_HR_FIRST_TINY  },
		{ BLDOFF_4X4_X_VT_LAST_TINY,  BLDOFF_4X4_X_VT_THIRD_TINY,  BLDOFF_4X4_X_VT_SECOND_TINY,  BLDOFF_4X4_X_VT_FIRST_TINY  }
	}
};

static __int16 coverage2x2OffsetsY[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_2x2] = {
	{
		{ BLDOFF_2X2_Y_HR_FIRST_LARGE, BLDOFF_2X2_Y_HR_LAST_LARGE },
		{ BLDOFF_2X2_Y_VT_FIRST_LARGE, BLDOFF_2X2_Y_VT_LAST_LARGE }
	},
	{
		{ BLDOFF_2X2_Y_HR_FIRST_SMALL, BLDOFF_2X2_Y_HR_LAST_SMALL },
		{ BLDOFF_2X2_Y_VT_FIRST_SMALL, BLDOFF_2X2_Y_VT_LAST_SMALL }
	},
	{
		{ BLDOFF_2X2_Y_HR_FIRST_TINY,  BLDOFF_2X2_Y_HR_LAST_TINY  },
		{ BLDOFF_2X2_Y_VT_FIRST_TINY,  BLDOFF_2X2_Y_VT_LAST_TINY  }
	}
};

static __int16 coverage3x3OffsetsY[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_3x3] = {
	{
		{ BLDOFF_3X3_Y_HR_FIRST_LARGE, BLDOFF_3X3_Y_HR_SECOND_LARGE, BLDOFF_3X3_Y_HR_LAST_LARGE },
		{ BLDOFF_3X3_Y_VT_FIRST_LARGE, BLDOFF_3X3_Y_VT_SECOND_LARGE, BLDOFF_3X3_Y_VT_LAST_LARGE }
	},
	{
		{ BLDOFF_3X3_Y_HR_FIRST_SMALL, BLDOFF_3X3_Y_HR_SECOND_SMALL, BLDOFF_3X3_Y_HR_LAST_SMALL },
		{ BLDOFF_3X3_Y_VT_FIRST_SMALL, BLDOFF_3X3_Y_VT_SECOND_SMALL, BLDOFF_3X3_Y_VT_LAST_SMALL }
	},
	{
		{ BLDOFF_3X3_Y_HR_FIRST_TINY,  BLDOFF_3X3_Y_HR_SECOND_TINY,  BLDOFF_3X3_Y_HR_LAST_TINY  },
		{ BLDOFF_3X3_Y_VT_FIRST_TINY,  BLDOFF_3X3_Y_VT_SECOND_TINY,  BLDOFF_3X3_Y_VT_LAST_TINY  }
	}
};

static __int16 coverage4x4OffsetsY[SIZE_LEVELS][AXIS_COUNT][COVERAGE_SIZE_4x4] = {
	{
		{ BLDOFF_4X4_Y_HR_FIRST_LARGE, BLDOFF_4X4_Y_HR_SECOND_LARGE, BLDOFF_4X4_Y_HR_THIRD_LARGE, BLDOFF_4X4_Y_HR_LAST_LARGE },
		{ BLDOFF_4X4_Y_VT_FIRST_LARGE, BLDOFF_4X4_Y_VT_SECOND_LARGE, BLDOFF_4X4_Y_VT_THIRD_LARGE, BLDOFF_4X4_Y_VT_LAST_LARGE }
	},
	{
		{ BLDOFF_4X4_Y_HR_FIRST_SMALL, BLDOFF_4X4_Y_HR_SECOND_SMALL, BLDOFF_4X4_Y_HR_THIRD_SMALL, BLDOFF_4X4_Y_HR_LAST_SMALL },
		{ BLDOFF_4X4_Y_VT_FIRST_SMALL, BLDOFF_4X4_Y_VT_SECOND_SMALL, BLDOFF_4X4_Y_VT_THIRD_SMALL, BLDOFF_4X4_Y_VT_LAST_SMALL }
	},
	{
		{ BLDOFF_4X4_Y_HR_FIRST_TINY,  BLDOFF_4X4_Y_HR_SECOND_TINY,  BLDOFF_4X4_Y_HR_THIRD_TINY,  BLDOFF_4X4_Y_HR_LAST_TINY  },
		{ BLDOFF_4X4_Y_VT_FIRST_TINY,  BLDOFF_4X4_Y_VT_SECOND_TINY,  BLDOFF_4X4_Y_VT_THIRD_TINY,  BLDOFF_4X4_Y_VT_LAST_TINY  }
	}
};

static __int16 highwayTROffsets[SIZE_LEVELS][AXIS_COUNT] = {
	{ HWY_HR_TROFF_LARGE, HWY_VT_TROFF_LARGE },
	{ HWY_HR_TROFF_SMALL, HWY_VT_TROFF_SMALL },
	{ HWY_HR_TROFF_TINY,  HWY_VT_TROFF_TINY  }
};

static __int16 highwayBROffsets[SIZE_LEVELS][AXIS_COUNT] = {
	{ HWY_HR_BROFF_LARGE, 0 },
	{ HWY_HR_BROFF_SMALL, 0 },
	{ HWY_HR_BROFF_TINY,  0 }
};

static __int16 highwayBLOffsets[SIZE_LEVELS][AXIS_COUNT] = {
	{ HWY_HR_BLOFF_LARGE, HWY_VT_BLOFF_LARGE },
	{ HWY_HR_BLOFF_SMALL, HWY_VT_BLOFF_SMALL },
	{ HWY_HR_BLOFF_TINY,  HWY_VT_BLOFF_TINY  }
};

static int DoWaterfallEdge(__int16 iMapOffSetX, int iX, int iY, __int16 iBottom, __int16 iWaterFallSpriteID, __int16 iDecr) {
	__int16 iTopog;

	if (iX < GAME_MAP_SIZE &&
		iY < GAME_MAP_SIZE &&
		XBITReturnIsWater(iX, iY)) {
		iTopog = ALTMReturnWaterLevel(iX, iY) - ALTMReturnLandAltitude(iX, iY);
		if (iTopog > 0) {
			while (rcDst.top <= iBottom) {
				if (rcDst.bottom > iBottom)
					Game_DrawProcessObject(iWaterFallSpriteID, iMapOffSetX, iBottom, 0, 0);
				iBottom -= iDecr;
				if (--iTopog <= 0)
					return 2;
			}
			return 0;
		}
	}
	return 1;
}

static int DoMapEdge(__int16 iMapOffSetX, int iX, int iY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iWaterFallSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, iMapOffSetX, iBottom, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				return DoWaterfallEdge(iMapOffSetX, iX, iY, iBottom, iWaterFallSpriteID, iDecr);
		}
	}
	return 1;
}

static void DoBedrockEdge(__int16 iMapOffSetX, __int16 iOffSetX, __int16 iOffSetY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom + iOffSetY) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, iMapOffSetX + iOffSetX, iBottom + iOffSetY, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				break;
		}
	}
}

static __int16 getCoverageOffsetsX(__int16 nSizeLevel, __int16 nAxis, __int16 nPos, __int16 nCoverage) {
	if (nCoverage == COVERAGE_2x2)
		return coverage2x2OffsetsX[nSizeLevel][nAxis][nPos];
	else if (nCoverage == COVERAGE_3x3)
		return coverage3x3OffsetsX[nSizeLevel][nAxis][nPos];
	else if (nCoverage == COVERAGE_4x4)
		return coverage4x4OffsetsX[nSizeLevel][nAxis][nPos];
	return 0;
}

static __int16 getCoverageOffsetsY(__int16 nSizeLevel, __int16 nAxis, __int16 nPos, __int16 nCoverage) {
	if (nCoverage == COVERAGE_2x2)
		return coverage2x2OffsetsY[nSizeLevel][nAxis][nPos];
	else if (nCoverage == COVERAGE_3x3)
		return coverage3x3OffsetsY[nSizeLevel][nAxis][nPos];
	else if (nCoverage == COVERAGE_4x4)
		return coverage4x4OffsetsY[nSizeLevel][nAxis][nPos];
	return 0;
}

static __int16 getCoverageOffsets(__int16 nSizeLevel, __int16 nAxis, __int16 nPos, __int16 nCoverage, BOOL bX) {
	if (bX)
		return getCoverageOffsetsX(nSizeLevel, nAxis, nPos, nCoverage);
	else
		return getCoverageOffsetsY(nSizeLevel, nAxis, nPos, nCoverage);
}

static void DoCoverageMapEdgeFill(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY, __int16 nSprBedrock, __int16 nDecr, __int16 nSizeLevel, BYTE iTile) {
	__int16 iBottom, iLandAlt;
	__int16 iCoverageSize;
	BYTE iCoverage;

	iCoverage = GetTileCoverage(iTile);
	if (iCoverage >= COVERAGE_1x1 && iCoverage < COVERAGE_COUNT) {
		iCoverageSize = iCoverage + 1;
		iBottom = iMapOffSetY - pArrSpriteHeaders[nSprBedrock].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (iCoverage == COVERAGE_1x1) {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoBedrockEdge(iMapOffSetX, BLDOFF_DEFAULT, BLDOFF_DEFAULT, iBottom, iLandAlt, nSprBedrock, nDecr);
		}
		else {
			if (iX + iCoverage == MAP_EDGE_MAX) {
				for (__int16 i = 0; i < iCoverageSize; i++)
					DoBedrockEdge(iMapOffSetX, getCoverageOffsets(nSizeLevel, AXIS_HORZ, i, iCoverage, TRUE), getCoverageOffsets(nSizeLevel, AXIS_VERT, i, iCoverage, TRUE), iBottom, iLandAlt, nSprBedrock, nDecr);
			}
			else if (iY == MAP_EDGE_MAX) {
				for (__int16 i = 0; i < iCoverageSize; i++)
					DoBedrockEdge(iMapOffSetX, getCoverageOffsets(nSizeLevel, AXIS_HORZ, i, iCoverage, FALSE), getCoverageOffsets(nSizeLevel, AXIS_VERT, i, iCoverage, FALSE), iBottom, iLandAlt, nSprBedrock, nDecr);
			}
		}
	}
}

static void L_DrawTile_SC2K1996(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY, int nSizeLevel) {
	__int16 nDecr;
	__int16 nBldOffs, nPwrIndOffs;
	__int16 nSprBedrock, nSprWaterfall;
	__int16 nSprStart, nSprWaterTer;
	__int16 nSprGreenTile, nSprFireStart;
	__int16 nSprPowerInd;
	__int16 nSprHighway, nSprSuspBridge;

	__int16 iBottom;
	__int16 iTop;
	__int16 iSprTop;
	__int16 iAltTop;
	__int16 iLandAlt;
	__int16 iSprite;
	__int16 iTrafficSprite, iTrafficSpriteOffset;
	__int16 iThing;
	BOOL bIsFlipped;
	BYTE iTerrainTile;
	BYTE iTile;
	BYTE iZone;
	BYTE iTraffic, iLowTrfThreshold, iHeavyTrfThreshold;

	nDecr = drawDecr[nSizeLevel];
	nBldOffs = buildOffset[nSizeLevel];
	nPwrIndOffs = pwrIndOffset[nSizeLevel];
	nSprBedrock = spriteBedrockSize[nSizeLevel];
	nSprWaterfall = spriteWaterfallSize[nSizeLevel];
	nSprStart = spriteStartOffset[nSizeLevel];
	nSprWaterTer = spriteWaterTerOffset[nSizeLevel];
	nSprGreenTile = spriteGreenTileOffset[nSizeLevel];
	nSprFireStart = spriteFire4Offset[nSizeLevel];
	nSprPowerInd = spritePowerIndSize[nSizeLevel];
	nSprHighway = spriteHighwayOffset[nSizeLevel];
	nSprSuspBridge = spriteSuspBridgeOffset[nSizeLevel];

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = iMapOffSetY - pArrSpriteHeaders[nSprBedrock].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(iMapOffSetX, iX, iY, iBottom, nSprWaterfall, nDecr))
				return;
		}
	}

	// Note: Originally there was a general 'DoMapEdge' processing
	// call here, however it has now been moved so the calls only
	// occur where they're needed for greater control - such as
	// when an alternative call is required in order to avoid
	// map-edge bleed on >= 2x2 buildings.

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = iMapOffSetY - nDecr * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top) {
		// Add this here otherwise as soon as the map starts to go off screen
		// (most noticeable in large mode) tile blocks will disappear.
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
		return;
	}
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
	}
	else if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						// The following call is to account for >= 1x1 buildings and avoid the map-edge
						// bedrock-bleed that was previously occurring on >= 2x2 buildings.
						DoCoverageMapEdgeFill(iMapOffSetX, iMapOffSetY, iX, iY, nSprBedrock, nDecr, nSizeLevel, iTile);
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						iSprite = iTile + nSprStart;
						Game_DrawProcessObject(iSprite, iMapOffSetX, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - nBldOffs, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
					iSprite = iZone + nSprWaterTer;
					Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + nSprGreenTile;
				else
					iSprite = iZone + nSprWaterTer;
				Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
			iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
			if (DisplayLayer[LAYER_INFRANATURE]) {
				if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
					Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					if (iTile == TILE_RAISING_BRIDGE_LOWERED) {
						if (wActiveShips) {
							for (iThing = MIN_THING_IDX; iThing < MAX_THING_COUNT; ++iThing) {
								if (XTHGGetType(iThing) == XTHG_CARGO_SHIP)
									break;
							}
							if (iThing != MAX_THING_COUNT) {
								map_XTHG_t *pThing = GetXTHG(iThing);

								if (pThing) {
									__int16 iDestDist = Game_GetDestDistance(iX, iY, pThing->iX, pThing->iY);
									if (iDestDist < RAISE_THRESHOLD)
										iTile = TILE_RAISING_BRIDGE_RAISED;
								}
							}
						}
					}
					// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					iSprite = iTile + nSprStart;
					iSprTop = iMapOffSetY - nDecr * iAltTop;
					if (iTerrainTile == TERRAIN_13)
						iSprTop = iTop - nDecr;
					iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
					if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
						bIsFlipped = FALSE;
					else
						bIsFlipped = XBITReturnIsFlipped(iX, iY);
					Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop, bIsFlipped, 0);
					iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
					if (iSprite < nSprHighway || iSprite >= nSprSuspBridge)
						iLowTrfThreshold = 85;
					else
						iLowTrfThreshold = 28;
					iHeavyTrfThreshold = iLowTrfThreshold * 2;
					if (iTraffic > iLowTrfThreshold) {
						iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
						if (iTrafficSpriteOffset) {
							if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
								bIsFlipped = FALSE;
							else
								bIsFlipped = XBITReturnIsFlipped(iX, iY);
							if (iTrafficSpriteOffset == 11) {
								if (!IsEven(iX))
									iTrafficSpriteOffset = 12;
							}
							else if (iTrafficSpriteOffset == 12) {
								bIsFlipped = TRUE;
								if (!IsEven(iY))
									iTrafficSpriteOffset = 11;
							}
							if (iTraffic > iHeavyTrfThreshold)
								iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
							iTrafficSprite = iTrafficSpriteOffset + nSprFireStart;
							Game_DrawProcessMaskObject(iTrafficSprite, iMapOffSetX, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
						}
					}
					if (iX < GAME_MAP_SIZE &&
						iY < GAME_MAP_SIZE &&
						XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
						Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
					}
				}
				else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
					// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + nSprStart;
					Game_DrawProcessObject(iSprite, iMapOffSetX + highwayTROffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight - highwayTROffsets[nSizeLevel][AXIS_VERT], 0, 0);
					iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + nSprStart;
					Game_DrawProcessObject(iSprite, iMapOffSetX + highwayBROffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
					iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + nSprStart;
					Game_DrawProcessObject(iSprite, iMapOffSetX + highwayBLOffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight + highwayBLOffsets[nSizeLevel][AXIS_VERT], 0, 0);
					// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
					iSprite = iTile + nSprStart;
					iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + nBldOffs;
					if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
						bIsFlipped = FALSE;
					else
						bIsFlipped = XBITReturnIsFlipped(iX, iY);
					Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop, bIsFlipped, 0);
					iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
					iLowTrfThreshold = 28;
					iHeavyTrfThreshold = iLowTrfThreshold * 2;
					if (iTraffic > iLowTrfThreshold) {
						iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
						if (iTraffic > iHeavyTrfThreshold)
							iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
						if (iTrafficSpriteOffset)
							iTrafficSprite = iTrafficSpriteOffset + nSprFireStart;
						Game_DrawProcessMaskObject(iTrafficSprite, iMapOffSetX, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
					}
				}
			}
			else
				Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(iMapOffSetX, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
		iSprTop = iMapOffSetY - nDecr * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - nDecr;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, iMapOffSetX, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (DisplayLayer[LAYER_INFRANATURE]) {
			iSprite = iTile + nSprStart;
			Game_DrawProcessObject(iSprite, iMapOffSetX, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			if (iX < GAME_MAP_SIZE &&
				iY < GAME_MAP_SIZE &&
				XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
				Game_DrawProcessObject(nSprPowerInd, iMapOffSetX + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
			}
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, iMapOffSetX, iTop);
}

extern "C" void __stdcall Hook_DrawAllLarge() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iMapOffSetX, iMapOffSetY;
	__int16 iScan, iX, iY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		rcDst.left -= 128;
		rcDst.bottom += 220;
		rcDst.top -= 32;

		// Top half
		iScan = MAP_EDGE_MIN;
		iX = MAP_EDGE_MIN;
		iY = MAP_EDGE_MIN;
		iMapOffSetX = iScreenOffSetX;
		iMapOffSetY = iScreenOffSetY;
		do {
			if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
				Game_DrawLargeTile(iMapOffSetX, iMapOffSetY, iX, iY);
			}
			if (iY) {
				++iX;
				--iY;
				iMapOffSetX += 32;
			}
			else {
				++iScan;
				iX = MAP_EDGE_MIN;
				iY = iScan;
				iMapOffSetX = iScreenOffSetX - 16 * iScan;
				iMapOffSetY += 8;
			}
		} while (iScan < GAME_MAP_SIZE);

		// Bottom half
		iScan = MAP_EDGE_MIN + 1;
		iX = MAP_EDGE_MIN + 1;
		iY = MAP_EDGE_MAX;
		iMapOffSetX = iScreenOffSetX - 2016;
		iMapOffSetY = iScreenOffSetY + 1024;
		do {
			if (rcDst.left < iMapOffSetX && rcDst.right > iMapOffSetX) {
				Game_DrawLargeTile(iMapOffSetX, iMapOffSetY, iX, iY);
			}
			if (iX == MAP_EDGE_MAX) {
				++iScan;
				iX = iScan;
				iY = MAP_EDGE_MAX;
				iMapOffSetX = 16 * (iScan - MAP_EDGE_MAX) + iScreenOffSetX;
				iMapOffSetY += 8;
			}
			else {
				++iX;
				--iY;
				iMapOffSetX += 32;
			}
		} while (iScan < GAME_MAP_SIZE);
	}
}

extern "C" void __cdecl Hook_DrawLargeTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY, SIZE_LARGE);
}

extern "C" void __cdecl Hook_DrawSmallTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY, SIZE_SMALL);
}

extern "C" void __cdecl Hook_DrawTinyTile(__int16 iMapOffSetX, __int16 iMapOffSetY, int iX, int iY) {
	L_DrawTile_SC2K1996(iMapOffSetX, iMapOffSetY, iX, iY, SIZE_TINY);
}

extern "C" void __cdecl Hook_DrawUnderTile(__int16 iX, __int16 iY) {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
		if (pSCView) {

		}
	}
}

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	VirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawLargeTile
	VirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);

	// Hook for DrawSmallTile
	VirtualProtect((LPVOID)0x401E79, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401E79, Hook_DrawSmallTile);

	// Hook for DrawTinyTile
	VirtualProtect((LPVOID)0x4022D9, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4022D9, Hook_DrawTinyTile);

	// Hook for DrawUnderTile
	//VirtualProtect((LPVOID)0x402D9C, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	//NEWJMP((LPVOID)0x402D9C, Hook_DrawUnderTile);

	// This disables the edge-checker for >= 2x2 buildings.
	// *** REMEMBER TO COMMENT OUT AT THE END ***
	//VirtualProtect((LPVOID)0x44056E, 166, PAGE_EXECUTE_READWRITE, &dwDummy);
	//memset((LPVOID)0x44056E, 0x90, 166);

	UpdateDrawingHooks_SC2K1996();
}

void UpdateDrawingHooks_SC2K1996(void) {
	COLORREF undgrndBkgnd;

	if (bSettingsDarkUndergroundBkgnd)
		undgrndBkgnd = PALETTERGB(0, 0, 0);       // Black
	else 
		undgrndBkgnd = PALETTERGB(192, 192, 192); // Default

	// Set via InitializeDataColorsFonts() first (on program load).
	VirtualProtect((LPVOID)0x42C008, 4, PAGE_EXECUTE_READWRITE, &dwDummy);
	*(COLORREF *)0x42C008 = undgrndBkgnd;

	// Set to the actual variable second (during runtime).
	colGameBackgndUnder = undgrndBkgnd;
}
