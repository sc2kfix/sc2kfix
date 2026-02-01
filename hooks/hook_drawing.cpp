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

static int DoWaterfallEdge(__int16 shpWidth, int iX, int iY, __int16 iBottom, __int16 iWaterFallSpriteID, __int16 iDecr) {
	__int16 iTopog;

	if (iX < GAME_MAP_SIZE &&
		iY < GAME_MAP_SIZE &&
		XBITReturnIsWater(iX, iY)) {
		iTopog = ALTMReturnWaterLevel(iX, iY) - ALTMReturnLandAltitude(iX, iY);
		if (iTopog > 0) {
			while (rcDst.top <= iBottom) {
				if (rcDst.bottom > iBottom)
					Game_DrawProcessObject(iWaterFallSpriteID, shpWidth, iBottom, 0, 0);
				iBottom -= iDecr;
				if (--iTopog <= 0)
					return 2;
			}
			return 0;
		}
	}
	return 1;
}

static int DoMapEdge(__int16 shpWidth, int iX, int iY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iWaterFallSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, shpWidth, iBottom, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				return DoWaterfallEdge(shpWidth, iX, iY, iBottom, iWaterFallSpriteID, iDecr);
		}
	}
	return 1;
}

static void DoBedrockEdge(__int16 shpWidth, __int16 iOffSetX, __int16 iOffSetY, __int16 iBottom, __int16 iLandAlt, __int16 iSpriteID, __int16 iDecr) {
	if (iLandAlt > 0) {
		while (rcDst.top <= iBottom) {
			if (rcDst.bottom > iBottom)
				Game_DrawProcessObject(iSpriteID, shpWidth + iOffSetX, iBottom + iOffSetY, 0, 0);
			iBottom -= iDecr;
			if (--iLandAlt <= 0)
				break;
		}
	}
}

static void L_DrawTile_SC2K1996(__int16 shpWidth, __int16 shpHeight, int iX, int iY, int nSizeLevel) {
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
	BYTE iOff;
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
		iBottom = shpHeight - pArrSpriteHeaders[nSprBedrock].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(shpWidth, iX, iY, iBottom, nSprWaterfall, nDecr))
				return;
		}
	}

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = shpHeight - nDecr * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
	}
	else if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						iOff = GetTileCoverage(iTile);
						iSprite = iTile + nSprStart;
						if (iOff >= COVERAGE_1x1) {
							iBottom = shpHeight - pArrSpriteHeaders[nSprBedrock].wHeight;
							iLandAlt = ALTMReturnLandAltitude(iX, iY);
							if (iOff == COVERAGE_1x1) {
								if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
									DoBedrockEdge(shpWidth, BLDOFF_DEFAULT, BLDOFF_DEFAULT, iBottom, iLandAlt, nSprBedrock, nDecr);
							}
							else if (iOff == COVERAGE_2x2) {
								if (iX + COVERAGE_2x2 == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_2x2; i++)
										DoBedrockEdge(shpWidth, coverage2x2OffsetsX[nSizeLevel][AXIS_HORZ][i], coverage2x2OffsetsX[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
								else if (iY == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_2x2; i++)
										DoBedrockEdge(shpWidth, coverage2x2OffsetsY[nSizeLevel][AXIS_HORZ][i], coverage2x2OffsetsY[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
							}
							else if (iOff == COVERAGE_3x3) {
								if (iX + COVERAGE_3x3 == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_3x3; i++)
										DoBedrockEdge(shpWidth, coverage3x3OffsetsX[nSizeLevel][AXIS_HORZ][i], coverage3x3OffsetsX[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
								else if (iY == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_3x3; i++)
										DoBedrockEdge(shpWidth, coverage3x3OffsetsY[nSizeLevel][AXIS_HORZ][i], coverage3x3OffsetsY[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
							}
							else if (iOff == COVERAGE_4x4) {
								if (iX + COVERAGE_4x4 == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_4x4; i++)
										DoBedrockEdge(shpWidth, coverage4x4OffsetsX[nSizeLevel][AXIS_HORZ][i], coverage4x4OffsetsX[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
								else if (iY == MAP_EDGE_MAX) {
									for (__int16 i=0; i<COVERAGE_SIZE_4x4; i++)
										DoBedrockEdge(shpWidth, coverage4x4OffsetsY[nSizeLevel][AXIS_HORZ][i], coverage4x4OffsetsY[nSizeLevel][AXIS_VERT][i], iBottom, iLandAlt, nSprBedrock, nDecr);
								}
							}
						}
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						Game_DrawProcessObject(iSprite, shpWidth, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - nBldOffs, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(nSprPowerInd, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
					iSprite = iZone + nSprWaterTer;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + nSprGreenTile;
				else
					iSprite = iZone + nSprWaterTer;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
			if (!DisplayLayer[LAYER_INFRANATURE]) {
				iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if (XTXTGetTextOverlayID(iX, iY))
					Game_DrawLabel(iX, iY, shpWidth, iTop);
				return;
			}
			if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
				iSprTop = shpHeight - nDecr * iAltTop;
				if (iTerrainTile == TERRAIN_13)
					iSprTop = iTop - nDecr;
				iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
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
				iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
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
						Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
					}
				}
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
					Game_DrawProcessObject(nSprPowerInd, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
				}
			}
			else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
				// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
				iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth + highwayTROffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight - highwayTROffsets[nSizeLevel][AXIS_VERT], 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth + highwayBROffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + nSprStart;
				Game_DrawProcessObject(iSprite, shpWidth + highwayBLOffsets[nSizeLevel][AXIS_HORZ], iTop - pArrSpriteHeaders[iSprite].wHeight + highwayBLOffsets[nSizeLevel][AXIS_VERT], 0, 0);
				// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
				iSprite = iTile + nSprStart;
				iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + nBldOffs;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				iLowTrfThreshold = 28;
				iHeavyTrfThreshold = iLowTrfThreshold * 2;
				if (iTraffic > iLowTrfThreshold) {
					iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
					if (iTraffic > iHeavyTrfThreshold)
						iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
					if (iTrafficSpriteOffset)
						iTrafficSprite = iTrafficSpriteOffset + nSprFireStart;
					Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
				}
			}
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, nSprBedrock, nSprWaterfall, nDecr);
		iSprTop = shpHeight - nDecr * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - nDecr;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + nSprStart;
		else
			iSprite = iZone + nSprWaterTer;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (!DisplayLayer[LAYER_INFRANATURE]) {
			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabel(iX, iY, shpWidth, iTop);
			return;
		}
		iSprite = iTile + nSprStart;
		Game_DrawProcessObject(iSprite, shpWidth, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
			Game_DrawProcessObject(nSprPowerInd, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - nPwrIndOffs, iTop - nPwrIndOffs, 0, 0);
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, shpWidth, iTop);
}

extern "C" void __stdcall Hook_DrawAllLarge() {
	CSimcityAppPrimary *pSCApp;
	CSimcityView *pSCView;
	__int16 iShpWidth, iShpHeight;
	__int16 iScan, iX, iY;

	pSCApp = &pCSimcityAppThis;
	pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		rcDst.left -= 128;
		rcDst.bottom += 220;
		rcDst.top -= 32;

		// Top
		iScan = MAP_EDGE_MIN;
		iX = MAP_EDGE_MIN;
		iY = MAP_EDGE_MIN;
		iShpWidth = iScreenOffSetX;
		iShpHeight = iScreenOffSetY;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth) {
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
			}
			if (iY) {
				++iX;
				--iY;
				iShpWidth += 32;
			}
			else {
				++iScan;
				iX = MAP_EDGE_MIN;
				iY = iScan;
				iShpWidth = iScreenOffSetX - 16 * iScan;
				iShpHeight += 8;
			}
		} while (iScan < GAME_MAP_SIZE);

		// Bottom
		iScan = MAP_EDGE_MIN + 1;
		iX = MAP_EDGE_MIN + 1;
		iY = MAP_EDGE_MAX;
		iShpWidth = iScreenOffSetX - 2016;
		iShpHeight = iScreenOffSetY + 1024;
		do {
			if (rcDst.left < iShpWidth && rcDst.right > iShpWidth) {
				Game_DrawLargeTile(iShpWidth, iShpHeight, iX, iY);
			}
			if (iX == MAP_EDGE_MAX) {
				++iScan;
				iX = iScan;
				iY = MAP_EDGE_MAX;
				iShpWidth = 16 * (iScan - MAP_EDGE_MAX) + iScreenOffSetX;
				iShpHeight += 8;
			}
			else {
				++iX;
				--iY;
				iShpWidth += 32;
			}
		} while (iScan < GAME_MAP_SIZE);
	}
}

extern "C" void __cdecl Hook_DrawSmallTile(__int16 shpWidth, __int16 shpHeight, int iX, int iY) {
#if 1
	L_DrawTile_SC2K1996(shpWidth, shpHeight, iX, iY, SIZE_SMALL);
#else
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
	BYTE iOff;
	BYTE iTraffic, iLowTrfThreshold, iHeavyTrfThreshold;

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = shpHeight - pArrSpriteHeaders[SPRITE_MEDIUM_BEDROCK].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(shpWidth, iX, iY, iBottom, SPRITE_MEDIUM_WATERFALL, DECR_SMALL))
				return;
		}
	}

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = shpHeight - DECR_SMALL * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, SPRITE_MEDIUM_WATERFALL, DECR_SMALL);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_MEDIUM_START;
		else
			iSprite = iZone + SPRITE_MEDIUM_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (XTXTGetTextOverlayID(iX, iY))
			Game_DrawLabel(iX, iY, shpWidth, iTop);
		return;
	}
	if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						iOff = GetTileCoverage(iTile);
						iSprite = iTile + SPRITE_MEDIUM_START;
						if (iOff >= COVERAGE_1x1) {
							iBottom = shpHeight - pArrSpriteHeaders[SPRITE_MEDIUM_BEDROCK].wHeight;
							iLandAlt = ALTMReturnLandAltitude(iX, iY);
							if (iOff == COVERAGE_1x1) {
								if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
									DoBedrockEdge(shpWidth, BLDOFF_DEFAULT,               BLDOFF_DEFAULT,               iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
							}																																  
							else if (iOff == COVERAGE_2x2) {																								  
								if (iX + COVERAGE_2x2 == MAP_EDGE_MAX) {																					  
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_LAST_SMALL,   BLDOFF_2X2_X_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_FIRST_SMALL,  BLDOFF_2X2_X_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}																														  
								else if (iY == MAP_EDGE_MAX) {																							  
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_FIRST_SMALL,  BLDOFF_2X2_Y_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_LAST_SMALL,   BLDOFF_2X2_Y_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}																														  
							}																															  
							else if (iOff == COVERAGE_3x3) {																							  
								if (iX + COVERAGE_3x3 == MAP_EDGE_MAX) {																				  
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_LAST_SMALL,   BLDOFF_3X3_X_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_SECOND_SMALL, BLDOFF_3X3_X_VT_SECOND_SMALL, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_FIRST_SMALL,  BLDOFF_3X3_X_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}																														  
								else if (iY == MAP_EDGE_MAX) {																							  
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_FIRST_SMALL,  BLDOFF_3X3_Y_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_SECOND_SMALL, BLDOFF_3X3_Y_VT_SECOND_SMALL, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_LAST_SMALL,   BLDOFF_3X3_Y_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}																																  
							}																																	  
							else if (iOff == COVERAGE_4x4) {																									  
								if (iX + COVERAGE_4x4 == MAP_EDGE_MAX) {																						  
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_LAST_SMALL,   BLDOFF_4X4_X_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_THIRD_SMALL,  BLDOFF_4X4_X_VT_THIRD_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_SECOND_SMALL, BLDOFF_4X4_X_VT_SECOND_SMALL, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_FIRST_SMALL,  BLDOFF_4X4_X_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}																															  
								else if (iY == MAP_EDGE_MAX) {																								  
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_FIRST_SMALL,  BLDOFF_4X4_Y_VT_FIRST_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_SECOND_SMALL, BLDOFF_4X4_Y_VT_SECOND_SMALL, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_THIRD_SMALL,  BLDOFF_4X4_Y_VT_THIRD_SMALL,  iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_LAST_SMALL,   BLDOFF_4X4_Y_VT_LAST_SMALL,   iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, DECR_SMALL);
								}
							}
						}
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						Game_DrawProcessObject(iSprite, shpWidth, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - BLDOFF_SMALL, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(SPRITE_MEDIUM_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_SMALL, iTop - PWROFF_SMALL, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, SPRITE_MEDIUM_WATERFALL, DECR_SMALL);
					iSprite = iZone + SPRITE_MEDIUM_WATER_R_TERRAIN_TBL;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, SPRITE_MEDIUM_WATERFALL, DECR_SMALL);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + SPRITE_MEDIUM_GREENTILE;
				else
					iSprite = iZone + SPRITE_MEDIUM_WATER_R_TERRAIN_TBL;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, SPRITE_MEDIUM_WATERFALL, DECR_SMALL);
			if (!DisplayLayer[LAYER_INFRANATURE]) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if (XTXTGetTextOverlayID(iX, iY))
					Game_DrawLabel(iX, iY, shpWidth, iTop);
				return;
			}
			if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
				iSprTop = shpHeight - DECR_SMALL * iAltTop;
				if (iTerrainTile == TERRAIN_13)
					iSprTop = iTop - DECR_SMALL;
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
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
				iSprite = iTile + SPRITE_MEDIUM_START;
				iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				if (iSprite < SPRITE_MEDIUM_HIGHWAY_LR || iSprite >= SPRITE_MEDIUM_SUSPENSION_BRIDGE_START_B)
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
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_MEDIUM_FIRE4;
						Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
					}
				}
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
					Game_DrawProcessObject(SPRITE_MEDIUM_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_SMALL, iTop - PWROFF_SMALL, 0, 0);
				}
			}
			else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_TROFF_SMALL, iTop - pArrSpriteHeaders[iSprite].wHeight - HWY_VT_TROFF_SMALL, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BROFF_SMALL, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + SPRITE_MEDIUM_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BLOFF_SMALL, iTop - pArrSpriteHeaders[iSprite].wHeight + HWY_VT_BLOFF_SMALL, 0, 0);
				iSprite = iTile + SPRITE_MEDIUM_START;
				iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + BLDOFF_SMALL;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				iLowTrfThreshold = 28;
				iHeavyTrfThreshold = iLowTrfThreshold * 2;
				if (iTraffic > iLowTrfThreshold) {
					iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
					if (iTraffic > iHeavyTrfThreshold)
						iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
					if (iTrafficSpriteOffset)
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_MEDIUM_FIRE4;
					Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
				}
			}
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_MEDIUM_BEDROCK, SPRITE_MEDIUM_WATERFALL, DECR_SMALL);
		iSprTop = shpHeight - DECR_SMALL * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - DECR_SMALL;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_MEDIUM_START;
		else
			iSprite = iZone + SPRITE_MEDIUM_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (!DisplayLayer[LAYER_INFRANATURE]) {
			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabel(iX, iY, shpWidth, iTop);
			return;
		}
		iSprite = iTile + SPRITE_MEDIUM_START;
		Game_DrawProcessObject(iSprite, shpWidth, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
			Game_DrawProcessObject(SPRITE_MEDIUM_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_SMALL, iTop - PWROFF_SMALL, 0, 0);
			return;
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, shpWidth, iTop);
#endif
}

extern "C" void __cdecl Hook_DrawLargeTile(__int16 shpWidth, __int16 shpHeight, int iX, int iY) {
#if 1
	L_DrawTile_SC2K1996(shpWidth, shpHeight, iX, iY, SIZE_LARGE);
#else
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
	BYTE iOff;
	BYTE iTraffic, iLowTrfThreshold, iHeavyTrfThreshold;

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = shpHeight - pArrSpriteHeaders[SPRITE_LARGE_BEDROCK].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(shpWidth, iX, iY, iBottom, SPRITE_LARGE_WATERFALL, DECR_LARGE))
				return;
		}
	}

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = shpHeight - DECR_LARGE * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
		else
			iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (XTXTGetTextOverlayID(iX, iY))
			Game_DrawLabel(iX, iY, shpWidth, iTop);
		return;
	}
	if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						iOff = GetTileCoverage(iTile);
						iSprite = iTile + SPRITE_LARGE_START;
						if (iOff >= COVERAGE_1x1) {
							iBottom = shpHeight - pArrSpriteHeaders[SPRITE_LARGE_BEDROCK].wHeight;
							iLandAlt = ALTMReturnLandAltitude(iX, iY);
							if (iOff == COVERAGE_1x1) {
								if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
									DoBedrockEdge(shpWidth, BLDOFF_DEFAULT,               BLDOFF_DEFAULT,               iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
							}
							else if (iOff == COVERAGE_2x2) {
								if (iX + COVERAGE_2x2 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_LAST_LARGE,   BLDOFF_2X2_X_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_FIRST_LARGE,  BLDOFF_2X2_X_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_FIRST_LARGE,  BLDOFF_2X2_Y_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_LAST_LARGE,   BLDOFF_2X2_Y_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
							else if (iOff == COVERAGE_3x3) {
								if (iX + COVERAGE_3x3 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_LAST_LARGE,   BLDOFF_3X3_X_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_SECOND_LARGE, BLDOFF_3X3_X_VT_SECOND_LARGE, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_FIRST_LARGE,  BLDOFF_3X3_X_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_FIRST_LARGE,  BLDOFF_3X3_Y_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_SECOND_LARGE, BLDOFF_3X3_Y_VT_SECOND_LARGE, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_LAST_LARGE,   BLDOFF_3X3_Y_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
							else if (iOff == COVERAGE_4x4) {
								if (iX + COVERAGE_4x4 == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_LAST_LARGE,   BLDOFF_4X4_X_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_THIRD_LARGE,  BLDOFF_4X4_X_VT_THIRD_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_SECOND_LARGE, BLDOFF_4X4_X_VT_SECOND_LARGE, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_FIRST_LARGE,  BLDOFF_4X4_X_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
								else if (iY == MAP_EDGE_MAX) {
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_FIRST_LARGE,  BLDOFF_4X4_Y_VT_FIRST_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_SECOND_LARGE, BLDOFF_4X4_Y_VT_SECOND_LARGE, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_THIRD_LARGE,  BLDOFF_4X4_Y_VT_THIRD_LARGE,  iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_LAST_LARGE,   BLDOFF_4X4_Y_VT_LAST_LARGE,   iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, DECR_LARGE);
								}
							}
						}
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						Game_DrawProcessObject(iSprite, shpWidth, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - BLDOFF_LARGE, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_LARGE, iTop - PWROFF_LARGE, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
					iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + SPRITE_LARGE_GREENTILE;
				else
					iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
			if (!DisplayLayer[LAYER_INFRANATURE]) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if (XTXTGetTextOverlayID(iX, iY))
					Game_DrawLabel(iX, iY, shpWidth, iTop);
				return;
			}
			if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
				iSprTop = shpHeight - DECR_LARGE * iAltTop;
				if (iTerrainTile == TERRAIN_13)
					iSprTop = iTop - DECR_LARGE;
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
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
				iSprite = iTile + SPRITE_LARGE_START;
				iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				if (iSprite < SPRITE_LARGE_HIGHWAY_LR || iSprite >= SPRITE_LARGE_SUSPENSION_BRIDGE_START_B)
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
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_LARGE_FIRE4;
						Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
					}
				}
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
					Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_LARGE, iTop - PWROFF_LARGE, 0, 0);
				}
			}
			else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_TROFF_LARGE, iTop - pArrSpriteHeaders[iSprite].wHeight - HWY_VT_TROFF_LARGE, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BROFF_LARGE, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + SPRITE_LARGE_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BLOFF_LARGE, iTop - pArrSpriteHeaders[iSprite].wHeight + HWY_VT_BLOFF_LARGE, 0, 0);
				iSprite = iTile + SPRITE_LARGE_START;
				iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + BLDOFF_LARGE;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				iLowTrfThreshold = 28;
				iHeavyTrfThreshold = iLowTrfThreshold * 2;
				if (iTraffic > iLowTrfThreshold) {
					iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
					if (iTraffic > iHeavyTrfThreshold)
						iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
					if (iTrafficSpriteOffset)
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_LARGE_FIRE4;
					Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
				}
			}
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_LARGE_BEDROCK, SPRITE_LARGE_WATERFALL, DECR_LARGE);
		iSprTop = shpHeight - DECR_LARGE * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - DECR_LARGE;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_LARGE_START;
		else
			iSprite = iZone + SPRITE_LARGE_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (!DisplayLayer[LAYER_INFRANATURE]) {
			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabel(iX, iY, shpWidth, iTop);
			return;
		}
		iSprite = iTile + SPRITE_LARGE_START;
		Game_DrawProcessObject(iSprite, shpWidth, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
			Game_DrawProcessObject(SPRITE_LARGE_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_LARGE, iTop - PWROFF_LARGE, 0, 0);
			return;
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, shpWidth, iTop);
#endif
}

extern "C" void __cdecl Hook_DrawTinyTile(__int16 shpWidth, __int16 shpHeight, int iX, int iY) {
#if 1
	L_DrawTile_SC2K1996(shpWidth, shpHeight, iX, iY, SIZE_TINY);
#else
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
	BYTE iOff;
	BYTE iTraffic, iLowTrfThreshold, iHeavyTrfThreshold;

	iBottom = 0;
	iLandAlt = 0;
	if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX) {
		iBottom = shpHeight - pArrSpriteHeaders[SPRITE_SMALL_BEDROCK].wHeight;
		iLandAlt = ALTMReturnLandAltitude(iX, iY);
		if (!iLandAlt) {
			if (!DoWaterfallEdge(shpWidth, iX, iY, iBottom, SPRITE_SMALL_WATERFALL, DECR_TINY))
				return;
		}
	}

	iTerrainTile = GetTerrainTileID(iX, iY);
	if (iTerrainTile < SUBMERGED_00)
		iAltTop = ALTMReturnLandAltitude(iX, iY);
	else
		iAltTop = ALTMReturnWaterLevel(iX, iY);
	iTop = shpHeight - DECR_TINY * iAltTop;
	iTile = GetTileID(iX, iY);
	if (iTile < TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1 && iTop < rcDst.top)
		return;
	iZone = XZONReturnZone(iX, iY);
	if (iTile == TILE_CLEAR) {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, SPRITE_SMALL_WATERFALL, DECR_TINY);
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_SMALL_START;
		else
			iSprite = iZone + SPRITE_SMALL_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (XTXTGetTextOverlayID(iX, iY))
			Game_DrawLabel(iX, iY, shpWidth, iTop);
		return;
	}
	if (iTile >= TILE_ROAD_LR) {
		if (iTile >= TILE_RESIDENTIAL_1X1_LOWERCLASSHOMES1) {
			if (DisplayLayer[LAYER_BUILDINGS]) {
				if (DisplayLayer[LAYER_ZONES] || !iZone) {
					if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
						iOff = GetTileCoverage(iTile);
						iSprite = iTile + SPRITE_SMALL_START;
						if (iOff >= COVERAGE_1x1) {
							iBottom = shpHeight - pArrSpriteHeaders[SPRITE_SMALL_BEDROCK].wHeight;
							iLandAlt = ALTMReturnLandAltitude(iX, iY);
							if (iOff == COVERAGE_1x1) {
								if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
									DoBedrockEdge(shpWidth, BLDOFF_DEFAULT,               BLDOFF_DEFAULT,             iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
							}																																	
							else if (iOff == COVERAGE_2x2) {																									
								if (iX + COVERAGE_2x2 == MAP_EDGE_MAX) {																						
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_LAST_TINY,   BLDOFF_2X2_X_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_X_HR_FIRST_TINY,  BLDOFF_2X2_X_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}																																
								else if (iY == MAP_EDGE_MAX) {																									
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_FIRST_TINY,  BLDOFF_2X2_Y_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_2X2_Y_HR_LAST_TINY,   BLDOFF_2X2_Y_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}																																
							}																																	
							else if (iOff == COVERAGE_3x3) {																									
								if (iX + COVERAGE_3x3 == MAP_EDGE_MAX) {																						
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_LAST_TINY,   BLDOFF_3X3_X_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_SECOND_TINY, BLDOFF_3X3_X_VT_SECOND_TINY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_X_HR_FIRST_TINY,  BLDOFF_3X3_X_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}																																
								else if (iY == MAP_EDGE_MAX) {																									
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_FIRST_TINY,  BLDOFF_3X3_Y_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_SECOND_TINY, BLDOFF_3X3_Y_VT_SECOND_TINY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_3X3_Y_HR_LAST_TINY,   BLDOFF_3X3_Y_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}																																
							}																																	
							else if (iOff == COVERAGE_4x4) {																									
								if (iX + COVERAGE_4x4 == MAP_EDGE_MAX) {																						
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_LAST_TINY,   BLDOFF_4X4_X_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_THIRD_TINY,  BLDOFF_4X4_X_VT_THIRD_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_SECOND_TINY, BLDOFF_4X4_X_VT_SECOND_TINY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_X_HR_FIRST_TINY,  BLDOFF_4X4_X_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}																																
								else if (iY == MAP_EDGE_MAX) {																									
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_FIRST_TINY,  BLDOFF_4X4_Y_VT_FIRST_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_SECOND_TINY, BLDOFF_4X4_Y_VT_SECOND_TINY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_THIRD_TINY,  BLDOFF_4X4_Y_VT_THIRD_TINY,  iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
									DoBedrockEdge(shpWidth, BLDOFF_4X4_Y_HR_LAST_TINY,   BLDOFF_4X4_Y_VT_LAST_TINY,   iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, DECR_TINY);
								}
							}
						}
						if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
							bIsFlipped = FALSE;
						else
							bIsFlipped = XBITReturnIsFlipped(iX, iY);
						if (!IsEven(wViewRotation))
							bIsFlipped = !bIsFlipped;
						Game_DrawProcessObject(iSprite, shpWidth, (pArrSpriteHeaders[iSprite].wWidth >> 2) - pArrSpriteHeaders[iSprite].wHeight + iTop - BLDOFF_TINY, bIsFlipped, 0);
						if (iX < GAME_MAP_SIZE &&
							iY < GAME_MAP_SIZE &&
							XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
							Game_DrawProcessObject(SPRITE_SMALL_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_TINY, iTop - PWROFF_TINY, 0, 0);
						}
					}
				}
				else {
					if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
						DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, SPRITE_SMALL_WATERFALL, DECR_TINY);
					iSprite = iZone + SPRITE_SMALL_WATER_R_TERRAIN_TBL;
					Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				}
			}
			else {
				if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
					DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, SPRITE_SMALL_WATERFALL, DECR_TINY);
				if (DisplayLayer[LAYER_ZONES] || !iZone)
					iSprite = BuiltUpZones[iZone] + SPRITE_SMALL_GREENTILE;
				else
					iSprite = iZone + SPRITE_SMALL_WATER_R_TERRAIN_TBL;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
			}
		}
		else {
			if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
				DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, SPRITE_SMALL_WATERFALL, DECR_TINY);
			if (!DisplayLayer[LAYER_INFRANATURE]) {
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				if (XTXTGetTextOverlayID(iX, iY))
					Game_DrawLabel(iX, iY, shpWidth, iTop);
				return;
			}
			if (iTile < TILE_HIGHWAY_HTB || iTile >= TILE_SUBTORAIL_T) {
				iSprTop = shpHeight - DECR_TINY * iAltTop;
				if (iTerrainTile == TERRAIN_13)
					iSprTop = iTop - DECR_TINY;
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
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
				iSprite = iTile + SPRITE_SMALL_START;
				iSprTop = iSprTop - pArrSpriteHeaders[iSprite].wHeight;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				if (iSprite < SPRITE_SMALL_HIGHWAY_LR || iSprite >= SPRITE_SMALL_SUSPENSION_BRIDGE_START_B)
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
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_SMALL_FIRE4;
						Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, bIsFlipped);
					}
				}
				if (iX < GAME_MAP_SIZE &&
					iY < GAME_MAP_SIZE &&
					XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
					Game_DrawProcessObject(SPRITE_SMALL_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_TINY, iTop - PWROFF_TINY, 0, 0);
				}
			}
			else if (XZONCornerCheck(iX, iY, wCurrentPositionAngle)) {
				// ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
				iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX, iY - 1)] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_TROFF_TINY, iTop - pArrSpriteHeaders[iSprite].wHeight - HWY_VT_TROFF_TINY, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY - 1)] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BROFF_TINY, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
				iSprite = nXTERTileIDs[GetTerrainTileID(iX + 1, iY)] + SPRITE_SMALL_START;
				Game_DrawProcessObject(iSprite, shpWidth + HWY_HR_BLOFF_TINY, iTop - pArrSpriteHeaders[iSprite].wHeight + HWY_VT_BLOFF_TINY, 0, 0);
				// ^ ---- This block was originally only present in both DrawLargeTile and DrawSmallTile.
				iSprite = iTile + SPRITE_SMALL_START;
				iSprTop = iTop - pArrSpriteHeaders[iSprite].wHeight + BLDOFF_TINY;
				if (iX >= GAME_MAP_SIZE || iY >= GAME_MAP_SIZE)
					bIsFlipped = FALSE;
				else
					bIsFlipped = XBITReturnIsFlipped(iX, iY);
				Game_DrawProcessObject(iSprite, shpWidth, iSprTop, bIsFlipped, 0);
				iTraffic = GetXTRFByteDataWithNormalCoordinates(iX, iY);
				iLowTrfThreshold = 28;
				iHeavyTrfThreshold = iLowTrfThreshold * 2;
				if (iTraffic > iLowTrfThreshold) {
					iTrafficSpriteOffset = trafficSpriteOffsets[iTile];
					if (iTraffic > iHeavyTrfThreshold)
						iTrafficSpriteOffset = trafficSpriteOverlayLevels[iTrafficSpriteOffset];
					if (iTrafficSpriteOffset)
						iTrafficSprite = iTrafficSpriteOffset + SPRITE_SMALL_FIRE4;
					Game_DrawProcessMaskObject(iTrafficSprite, shpWidth, iSprTop + pArrSpriteHeaders[iSprite].wHeight - pArrSpriteHeaders[iTrafficSprite].wHeight, 0);
				}
			}
		}
	}
	else {
		if (iX == MAP_EDGE_MAX || iY == MAP_EDGE_MAX)
			DoMapEdge(shpWidth, iX, iY, iBottom, iLandAlt, SPRITE_SMALL_BEDROCK, SPRITE_SMALL_WATERFALL, DECR_TINY);
		iSprTop = shpHeight - DECR_TINY * iAltTop;
		if (iTerrainTile == TERRAIN_13)
			iSprTop = iTop - DECR_TINY;
		if (iTerrainTile > TERRAIN_00 || !iZone)
			iSprite = nXTERTileIDs[iTerrainTile] + SPRITE_SMALL_START;
		else
			iSprite = iZone + SPRITE_SMALL_WATER_R_TERRAIN_TBL;
		Game_DrawProcessObject(iSprite, shpWidth, iTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (!DisplayLayer[LAYER_INFRANATURE]) {
			if (XTXTGetTextOverlayID(iX, iY))
				Game_DrawLabel(iX, iY, shpWidth, iTop);
			return;
		}
		iSprite = iTile + SPRITE_SMALL_START;
		Game_DrawProcessObject(iSprite, shpWidth, iSprTop - pArrSpriteHeaders[iSprite].wHeight, 0, 0);
		if (iX < GAME_MAP_SIZE &&
			iY < GAME_MAP_SIZE &&
			XBITReturnIsPowerable(iX, iY) && !XBITReturnIsPowered(iX, iY)) {
			Game_DrawProcessObject(SPRITE_SMALL_POWEROUTAGEINDICATOR, shpWidth + (pArrSpriteHeaders[iSprite].wWidth >> 1) - PWROFF_TINY, iTop - PWROFF_TINY, 0, 0);
			return;
		}
	}
	if (XTXTGetTextOverlayID(iX, iY))
		Game_DrawLabel(iX, iY, shpWidth, iTop);
#endif
}

void InstallDrawingHooks_SC2K1996(void) {
	// Hook for DrawAllLarge
	VirtualProtect((LPVOID)0x4017FD, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4017FD, Hook_DrawAllLarge);

	// Hook for DrawSmallTile
	VirtualProtect((LPVOID)0x401E79, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x401E79, Hook_DrawSmallTile);

	// Hook for DrawLargeTile
	VirtualProtect((LPVOID)0x402095, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x402095, Hook_DrawLargeTile);

	// Hook for DrawTinyTile
	VirtualProtect((LPVOID)0x4022D9, 5, PAGE_EXECUTE_READWRITE, &dwDummy);
	NEWJMP((LPVOID)0x4022D9, Hook_DrawTinyTile);

	// This disables the edge-checker for >= 2x2 buildings.
	// *** REMEMBER TO COMMENT OUT AT THE END ***
	//VirtualProtect((LPVOID)0x44056E, 166, PAGE_EXECUTE_READWRITE, &dwDummy);
	//memset((LPVOID)0x44056E, 0x90, 166);
}
