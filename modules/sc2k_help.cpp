// sc2kfix modules/sc2k_help.cpp: Help
// (c) 2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

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

// add calls here specific to the type rather than them all ending up in a large function.
static const char *GetHelpString_CityToolBar(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case CITYTOOL_BUTTON_BULLDOZER:
			pStr = "- BULLDOZER\n\n"
				"Demolish/Clear - This will destroy buildings, roads, trees, and decorative water, and will remove rubble.\n\n"
				"Level Terrain - This tool will level terrain to the same altitude as the first location you click on. It will also clear terrain by removing trees, roads, powerlines, and buildings.\n\n"
				"Raise Terrain - This raises the terrain.\n\n"
				"Lower Terrain - This lowers the terrain.\n\n"
				"De-zone - This will remove the zone from an area.";
			break;
		case CITYTOOL_BUTTON_RCI:
			pStr = "- ZONE DEMAND\n\n"
				"These colored bars show you the current demand for each type of zone in your city. If the bar is up in the \"+\" area then your city needs more of that type of zone. The letters represent \"R\"esidential, \"C\"ommercial and \"I\"ndustrial.";
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString(int nType, int nIndex) {
	const char *pStr = NULL;
	switch (nType) {
		case HELPTYPE_CITYTOOLBAR:
			pStr = GetHelpString_CityToolBar(nIndex);
			break;
		default:
			break;
	}
	return pStr;
}

// Needed context:
// nIndex - this can be:
// - For the City/Map Toolbars it is the button position index (not the control index).
// - For the Floating Status Widget it is ignored.
// - For all other dialogues it'll be their defined Ctrl IDs.

void DisplayItemHelp(HWND hWnd, int nType, int nIndex) {
	const char *pHelpStr;
	std::string str;

	pHelpStr = GetHelpString(nType, nIndex);
	if (pHelpStr) {
		str = string_format("Item Help: nType(%d), nIndex(%d) [%s]\n", nType, nIndex, pHelpStr);
		L_MessageBoxA(hWnd, str.c_str(), gamePrimaryKey, 0);
	}
}
