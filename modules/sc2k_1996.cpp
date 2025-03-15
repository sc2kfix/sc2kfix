// sc2kfix modules/sc2k_1996.cpp: variables specific to the 1996 Special Edition version
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// DO NOT USE THIS IN ANY OTHER FILE. I MEAN IT.
#define GAMEOFF_IMPL

#include <sc2kfix.h>

const char* szTileNames[256] = {
	"Empty",
	"Rubble 1",
	"Rubble 2",
	"Rubble 3",
	"Rubble 4",
	"Radioactivity",
	"Tree",
	"Couple O' Trees",
	"More Trees",
	"Morer Trees",
	"Even More Trees",
	"Tons O' Trees",
	"Veritable Jungle",
	"Small Park",

	"Power Lines (LR)",
	"Power Lines (TB)",
	"Power Lines (HTB)",
	"Power Lines (LHR)",
	"Power Lines (THB)",
	"Power Lines (HLR)",
	"Power Lines (BR)",
	"Power Lines (BL)",
	"Power Lines (TL)",
	"Power Lines (TR)",
	"Power Lines (RTB)",
	"Power Lines (LBR)",
	"Power Lines (TLB)",
	"Power Lines (LTR)",
	"Power Lines (LTBR)",

	"Road (LR)",
	"Road (TB)",
	"Road (HTB)",
	"Road (LHR)",
	"Road (THB)",
	"Road (HLR)",
	"Road (BR)",
	"Road (BL)",
	"Road (TL)",
	"Road (TR)",
	"Road (RTB)",
	"Road (LBR)",
	"Road (TLB)",
	"Road (LTR)",
	"Road (LTBR)",

	"Rail (LR)",
	"Rail (TB)",
	"Rail (HTB)",
	"Rail (LHR)",
	"Rail (THB)",
	"Rail (HLR)",
	"Rail (BR)",
	"Rail (BL)",
	"Rail (TL)",
	"Rail (TR)",
	"Rail (RTB)",
	"Rail (LBR)",
	"Rail (TLB)",
	"Rail (LTR)",
	"Rail (LTBR)",
	"Rail (HHTB)",
	"Rail (LHHR)",
	"Rail (THHB)",
	"Rail (HHLR)",

	"Tunnel Entrance (Top)",
	"Tunnel Entrance (Right)",
	"Tunnel Entrance (Bottom)",
	"Tunnel Entrance (Left)",

	"Crossover (POWERTB_ROADLR)",
	"Crossover (POWERLR_ROADTB)",
	"Crossover (ROADLR_RAILTB)",
	"Crossover (ROADTB_RAILLR)",
	"Crossover (POWERTB_RAILLR)",
	"Crossover (POWERLR_RAILTB)",

	"Highway (LR)",
	"Highway (TB)",

	"Crossover (HIGHWAYLR_ROADTB)",
	"Crossover (HIGHWAYTB_ROADLR)",
	"Crossover (HIGHWAYLR_RAILTB)",
	"Crossover (HIGHWAYTB_RAILLR)",
	"Crossover (HIGHWAYLR_POWERTB)",
	"Crossover (HIGHWAYTB_POWERLR)",

	"Suspension Bridge (Start-B)",
	"Suspension Bridge (Middle-B)",
	"Suspension Bridge (Center-B)",
	"Suspension Bridge (Middle-T)",
	"Suspension Bridge (End-T)",
	"Raising Bridge (Tower)",
	"Causeway (Pylon)",
	"Raising Bridge (Lowered)",
	"Raising Bridge (Raised)",
	"Rail Bridge (Pylon)",
	"Rail Bridge",
	"Elevated Power Lines",

	"Onramp (TL)",
	"Onramp (TR)",
	"Onramp (BL)",
	"Onramp (BR)",

	"Highway (HTB)",
	"Highway (LHR)",
	"Highway (THB)",
	"Highway (HLR)",
	"Highway (BR)",
	"Highway (BL)",
	"Highway (TL)",
	"Highway (TR)",
	"Highway (LTBR)",

	"Reinforced Bridge (Pylon)",
	"Reinforced Bridge",

	"Subway-to-Rail Junction (Top)",
	"Subway-to-Rail Junction (Right)",
	"Subway-to-Rail Junction (Bottom)",
	"Subway-to-Rail Junction (Left)",

	"Lower-class Homes (1)",
	"Lower-class Homes (2)",
	"Lower-class Homes (3)",
	"Lower-class Homes (4)",
	"Middle-class Homes (1)",
	"Middle-class Homes (2)",
	"Middle-class Homes (3)",
	"Middle-class Homes (4)",
	"Luxury Homes (1)",
	"Luxury Homes (2)",
	"Luxury Homes (3)",
	"Luxury Homes (4)",

	"Gas Station (1)",
	"Bed and Breakfast",
	"Convenience Store",
	"Gas Station (2)",
	"Small Office Building (1)",
	"Small Office Building (2)",
	"Commercial Warehouse (1x1)",
	"Toy Store",

	"Small Warehouse (1)",
	"Chemical Storage",
	"Small Warehouse (2)",
	"Substation",

	"1x1 Construction (1)",
	"1x1 Construction (2)",
	"1x1 Abandoned Building (1)",
	"1x1 Abandoned Building (2)",

	"Small Apartments (1)",
	"Small Apartments (2)",
	"Small Apartments (3)",
	"Medium Apartments (1)",
	"Medium Apartments (2)",
	"Medium Condominums (1)",
	"Medium Condominums (2)",
	"Medium Condominums (3)",

	"Shopping Center",
	"Grocery Store",
	"Medium Office Building (1)",
	"Resort Hotel",
	"Medium Office Building (2)",
	"Office/Retail Building",
	"Medium Office Building (3)",
	"Medium Office Building (4)",
	"Medium Office Building (5)",
	"Medium Office Building (6)",

	"Medium Warehouse",
	"Chemical Processing",
	"Factory (1)",
	"Factory (2)",
	"Factory (3)",
	"Factory (4)",
	"Factory (5)",
	"Factory (6)",

	"2x2 Construction (1)",
	"2x2 Construction (2)",
	"2x2 Construction (3)",
	"2x2 Construction (4)",
	"2x2 Abandoned Building (1)",
	"2x2 Abandoned Building (2)",
	"2x2 Abandoned Building (3)",
	"2x2 Abandoned Building (4)",

	"Large Apartments (1)",
	"Large Apartments (2)",
	"Large Condominiums (1)",
	"Large Condominiums (2)",

	"Office Park",
	"Office Tower (1)",
	"Mini-Mall",
	"Theater Square",
	"Drive-In Theater",
	"Office Tower (2)",
	"Office Tower (3)",
	"Parking Lot",
	"Historic Office Building",
	"Corporate Headquarters",

	"Chemical Processing",
	"Large Factory",
	"Industrial Thingamajig",
	"Medium Factory",
	"Large Warehouse (1)",
	"Large Warehouse (2)",

	"3x3 Construction (1)",
	"3x3 Construction (2)",
	"3x3 Abandoned Building (1)",
	"3x3 Abandoned Building (2)",

	"Hydroelectric Power Plant (1)",
	"Hydroelectric Power Plant (2)",
	"Wind Power Plant",
	"Natural Gas Power Plant",
	"Oil Power Plant",
	"Nuclear Power Plant",
	"Solar Power Plant",
	"Microwave Power Plant",
	"Fusion Power Plant",
	"Coal Power Plant",

	"City Hall",
	"Hospital",
	"Police Station",
	"Fire Station",
	"Museum",
	"Large Park",
	"School",
	"Stadium",
	"Prison",
	"College",
	"Zoo",
	"Statue",

	"Water Pump",
	"Runway",
	"Runway (Cross)",
	"Pier",
	"Crane",
	"Civilan Control Tower",
	"Military Control Tower",
	"Military Warehouse",
	"Port Building (1)",
	"Port Building (2)",
	"Tarmac",
	"F-15B",
	"Military Hangar",
	"Subway Station",
	"Radar",
	"Water Tower",
	"Bus Depot",
	"Railway Station",
	"Parking Lot",
	"Military Parking Lot",
	"Loading Bay",
	"Top Secret",
	"Cargo Yard",
	"Mayor's House",
	"Water Treatment Plant",
	"Library",
	"Large Hangar",
	"Church",
	"Marina",
	"Missile Silo",
	"Desalinization Plant",

	"Plymouth Arcology",
	"Forest Arcology",
	"Darco Arcology",
	"Launch Arcology",

	"Braun-Llama Dome"
};

const char* szUndergroundNames[36] = {
	"None",

	"Subway (LR)",
	"Subway (TB)",
	"Subway (HTB)",
	"Subway (LHR)",
	"Subway (THB)",
	"Subway (HLR)",
	"Subway (BR)",
	"Subway (BL)",
	"Subway (TL)",
	"Subway (TR)",
	"Subway (RTB)",
	"Subway (LBR)",
	"Subway (TLB)",
	"Subway (LTR)",
	"Subway (LTBR)",

	"Pipes (LR)",
	"Pipes (TB)",
	"Pipes (HTB)",
	"Pipes (LHR)",
	"Pipes (THB)",
	"Pipes (HLR)",
	"Pipes (BR)",
	"Pipes (BL)",
	"Pipes (TL)",
	"Pipes (TR)",
	"Pipes (RTB)",
	"Pipes (LBR)",
	"Pipes (TLB)",
	"Pipes (LTR)",
	"Pipes (LTBR)",

	"Crossover (PIPESTB_SUBWAYLR)",
	"Crossover (PIPESLR_SUBWAYTB)",
	"Unknown",
	"Missile Silo",
	"Subway Entrance"
};

int PlaceRoadsAlongPath(int x1, int y1, int x2, int y2) {
	int retval = 0;
	WORD wOldToolGroup = wMaybeActiveToolGroup;
	wMaybeActiveToolGroup = TOOL_GROUP_ROADS;
	if (Game_MaybeCheckViablePlacementPath(x1, y1, x2, y2)) {
		__int16 x = x1;
		__int16 y = y1;
		*(DWORD*)0x4C7D58 = 0;
		*(DWORD*)0x4C7D84 = 0;

		do {
			(*(DWORD*)0x4C7D84)++;
			if (dwMapXTER[x]->iTileID[y] < 0x30)
				if (*(((DWORD*)0x4E7B28) + (dwMapXTER[x]->iTileID[y] & 0xF)))
					(*(DWORD*)0x4C7D58)++;
			Game_PlaceRoadAtCoordinates(x, y);
		} while (Game_MaybeRoadViabilityAlongPath(&x, &y));
		Game_CDocument_UpdateAllViews(pCDocumentMainWindow, NULL, 2, NULL);
		retval = 1;
	} else
		retval = 0;
	wMaybeActiveToolGroup = wOldToolGroup;
	return retval;
}