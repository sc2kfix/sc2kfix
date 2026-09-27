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

typedef struct {
	int nType;
	int nIndex;
	bool bGeneralClose;
	RECT textRect;
} helpDlg_t;

// Once it is open we really don't want multiple invocations...
static bool bHelpOpen = false;

static const char *GetHelpString_GeneralHelp() {
	return "Hold down the SHIFT key while clicking the mouse on any button or area.  This will bring up this help dialog with a detailed description of the tool or display.";
}

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
		case CITYTOOL_BUTTON_NATURE:
			pStr = "- LANDSCAPE\n\n"
				"Trees - This tool adds trees to the terrain.\n\n"
				"Water - This will put down small streams and decorative ponds. It can be used to create waterfalls for the hydroelectric power plant.";
			break;
		case CITYTOOL_BUTTON_DISPATCH:
			pStr = "- DISPATCH\n\n"
				"This tool is only available during emergencies. It allows you to direct your police and firefighters to suppress problems.\n\n"
				"Dispatch Police - Police are useful for suppressing riots and resisting floods.\n\n"
				"Dispatch Firefighters - Firefighters suppress fire as well as resist floods and toxic clouds.\n\n"
				"Dispatch Military - Military units may be available to your city after they have constructed a base. Military units are highly trained and are effective against a variety of disasters.";
			break;
		case CITYTOOL_BUTTON_POWER:
			pStr = "- POWER\n\n"
				"Power Lines - Build these from your power plants to your zoned areas so they can start to build. These can cross roads and rails only at right angles. There is a slight transmission loss of power through these lines, so try to minimize the distance they have to traverse.\n\n"
				"Power Plants - This will bring up a list of the currently available power plants that you may build. This list will grow as time passes and technology progresses.";
			break;
		case CITYTOOL_BUTTON_WATER:
			pStr = "- WATER\n\n"
				"Pipes - These transmit water and carry away sewage. You need powered water pumps to generate water.\n\n"
				"Water Pump - Powered pumps will generate water for your city. The amount they produce is increased by placing them next to standing water. It also has a seasonal variance depending on rainfall.\n\n"
				"Water Tower - Water towers store water to combat seasonal variation.\n\n"
				"Treatment - Adding a treatment plant reduces your city - wide pollution levels.\n\n"
				"Desalinization - Water pumps will not pump sea water. A desalinization plant will take sea water and produce pure water for your city.";
			break;
		case CITYTOOL_BUTTON_REWARDS:
			pStr = "- CITY BONUS\n\n"
				"As your city grows in size, the city council will vote to reward you.\n\n"
				"Mayor's House - You will be given a residence at 2,000 population.\n\n"
				"City Hall - The council will vote to build a City Hall at 10,000 population.\n\n"
				"Statue - This will occur after the City Hall, but before Arcologies or the other rewards.";
			break;
		case CITYTOOL_BUTTON_ROAD:
			pStr = "- ROADS\n\n"
				"Road - This is your primary method of transport. Roads connect zones, allowing them to grow.\n\n"
				"Highway - Highways are faster and more efficient than roads. Commuters will move from road to onramps, then to highways, back to onramps, then on to roads again. Without the roads and on-ramps, highways are useless.\n\n"
				"Tunnel - Tunnels dig through a mountain rather than going over it. One advantage to tunnels is that zones can be built over tunneled areas, improving land usage.\n\n"
				"Onramp - These are necessary for highways to function. They can only be placed at a highway/road juncture.\n\n"
				"Bus Depot - Depots provide rapid, low traffic transport. Up to half of your city's commuters will use these depots if they are well placed.";
			break;
		case CITYTOOL_BUTTON_RAIL:
			pStr = "- RAIL\n\n"
				"Rail - This transport method is efficient and traffic free. You must carefully place your rail depots, or else the rail will go unused.\n\n"
				"Subway - These are underground railways. They operate like rail, except that subways need subway stations to function.\n\n"
				"Rail Depot - This is where commuters enter and exit the rail system.\n\n"
				"Sub Station - This is where commuters enter and exit the subway system.\n\n"
				"Sub<-->Rail - This allows you to connect your above-ground rail with your below-ground subway. You must place this next to an existing rail line.";
			break;
		case CITYTOOL_BUTTON_PORTS:
			pStr = "- PORTS\n\n"
				"Seaport - This provides vital external transport for your city's industries. It will not be necessary until your city hits 10,000 people or so.\n\n"
				"Airport - This provides inter-city transport for your city's commerce. It will not be needed until your city hits 15,000 people or so.";
			break;
		case CITYTOOL_BUTTON_RESIDENTIAL:
			pStr = "- RESIDENTIAL ZONING\n\n"
				"Residential zones are where the people live. Low density zoning will only allow single family homes in an area. High density zoning will allow homes as well as high-rise apartments and condominiums.";
			break;
		case CITYTOOL_BUTTON_COMMERCIAL:
			pStr = "- COMMERCIAL ZONING\n\n"
				"Commercial areas provide services to your local population. These include grocery stores, motels, entertainment, maintenance, and more. High density zoning includes banking, real estate, and financial services.";
			break;
		case CITYTOOL_BUTTON_INDUSTRIAL:
			pStr = "- INDUSTRIAL ZONING\n\n"
				"Industry is the backbone of your city. Initially, new residents are moving in to work with your industry. Meanwhile, industry is growing to meet external demands.";
			break;
		case CITYTOOL_BUTTON_EDUCATION:
			pStr = "- EDUCATION ZONES\n\n"
				"These special zones increase the \"EQ\" or education quotient of your residents over time. The EQ of your city will influence many factors including crime, productivity, and which industries prosper. Each type of zone affects different age groups and has a maintenance cost associated with it.\n\n"
				"School - This represents primary and secondary education (from kindergarten to 12th grade). This will increase the EQ of the 5+ to 20+-year-olds in your city. Of all education zones, these should be the most numerous in your city.\n\n"
				"College - This represents higher education - universities, junior colleges and vocational schools. This zone increases the EQ of the 15+ to 25+-year-olds primarily, and the older residents as well, though to a lesser degree.\n\n"
				"Library - This increases the EQ for all ages but to a lesser degree than the schools and colleges.\n\n"
				"Museum - Like the library this zone increases the EQ for all ages, but the effect is more and so is the cost.";
			break;
		case CITYTOOL_BUTTON_SERVICES:
			pStr = "- HEALTH AND SAFETY ZONES\n\n"
				"These zones include essential city services to protect your residents.\n\n"
				"Police - Police stations help manage the crime in your city.\n\n"
				"Fire Station - Fire stations attempt to prevent and extinguish any fires in your city. They also help during any sort of emergency.\n\n"
				"Hospital - This will have a beneficial effect on the health of your citizens.\n\n"
				"Prison - This will improve police performance if there is a lot of crime in your city.";
			break;
		case CITYTOOL_BUTTON_PARKS:
			pStr = "- RECREATION ZONES\n\n"
				"These special zones have a positive effect on residential growth and generally make your city a nicer place to live.\n\n"
				"Parks - Small and big parks both have a positive effect on local land values.\n\n"
				"Zoo - Zoos improve your city's desirability for residents, and improve its tourist value.\n\n"
				"Stadium - Your citizens are more enthusiastic and loyal if they have a local team to rally behind.\n\n"
				"Marina - These can only be placed down by the water.";
			break;
		case CITYTOOL_BUTTON_SIGNS:
			pStr = "- PLACE SIGN\n\n"
				"This tool is used to place signs (labels) in your city. To use it just click on the city location where you want it placed and then enter the text for it. These can be used to name streets, subdivisions, lakes, etc. Or you may use this for jotting down notes to yourself about future plans for each area.\n"
				"These signs can be toggled on and off with the layer control button near the bottom of the City toolbar. To erase a sign, click on the base with the sign tool to open the record, then hit the 'Delete' button (the button is only visible while modifying an existing player-created sign).";
			break;
		case CITYTOOL_BUTTON_QUERY:
			pStr = "- QUERY TOOL\n\n"
				"This tool will give you detailed information on anything in your city. Most areas only tell you about land value, local traffic, power and water supply. Special buildings such as fire departments, zoos, museums, et al., will have a specific micro-simulation you can examine.";
			break;
		case CITYTOOL_BUTTON_CENTERINGTOOL:
			pStr = "- CENTER DISPLAY\n\n"
				"This is the centering tool. It is used to scroll around your city. When you click in the window the scene will re-center on the place you clicked. If you click near the center of the window and hold both the 'Alt' key and mouse button down, you can then smoothly scroll around by moving the mouse to adjust direction.";
			break;
		case CITYTOOL_BUTTON_ZOOMOUT:
			pStr = "- ZOOM OUT\n\n"
				"There are four scales your city can be viewed at. This button allows you to increase the scale of your display. The tiles grow smaller and the area displayed grows.";
			break;
		case CITYTOOL_BUTTON_ZOOMIN:
			pStr = "- ZOOM IN\n\n"
				"There are four scales your city can be viewed at. This button allows you to decrease the scale of your display. The tiles grow larger and the area displayed shrinks.";
			break;
		case CITYTOOL_BUTTON_ROTATEANTICLOCKWISE:
			pStr = "- ROTATE COUNTER-CLOCKWISE\n\n"
				"Each time you click this button the scene in the window will rotate 90 degrees counter-clockwise.";
			break;
		case CITYTOOL_BUTTON_ROTATECLOCKWISE:
			pStr = "- ROTATE CLOCKWISE\n\n"
				"Each time you click this button the scene in the window will rotate 90 degrees clockwise.";
			break;
		case CITYTOOL_BUTTON_CITYMAP:
			pStr = "- MAP WINDOW\n\n"
				"This button brings up the map window.\n\n"
				"The map window shows a small, overhead view of your city and contains tabs that allow you to select and view information such as 'Police Power', 'Crime Rate' and 'Pollution'.";
			break;
		case CITYTOOL_BUTTON_CITYPOPULATION:
			pStr = "- POPULATION WINDOW\n\n"
				"This button brings up the population window.\n\n"
				"This window is used to follow trends in your population. It will show the education, population and health for each age-group in your city. The technical name for this kind of display is a cohort-population graph.";
			break;
		case CITYTOOL_BUTTON_CITYNEIGHBOURS:
			pStr = "- NEIGHBORS WINDOW\n\n"
				"This button brings up the neighbors window.\n\n"
				"This shows the names and size of your regional neighbors, allowing you to compare your perfomance to your neighbors'. It also shows the size of the nation your city occupies.";
			break;
		case CITYTOOL_BUTTON_CITYGRAPHS:
			pStr = "- GRAPHS WINDOW\n\n"
				"This button brings up the graphs window.\n\n"
				"These charts show both short and long-term evolution of population, traffic, pollution, crime, land value, health, education, power & water use, plus some national characteristics.";
			break;
		case CITYTOOL_BUTTON_CITYINDUSTRY:
			pStr = "- INDUSTRY WINDOW\n\n"
				"This button brings up the industry window.\n\n"
				"This window shows information about the 11 different industry groups. For each group you can display the national demand, your local tax rate and the ratio of local industries.";
			break;
		case CITYTOOL_BUTTON_BUDGET:
			pStr = "- BUDGET WINDOW\n\n"
				"This allows you to examine your city's budget. You will be able to adjust your income and expenses, and hopefully have a positive cash flow.";
			break;
		case CITYTOOL_BUTTON_DISPLAYBUILDINGS:
			pStr = "- BUILDING LAYER\n\n"
				"This button will flatten your buildings, allowing you to examine your roads, wires and other infrastructure. You will recognize the building types by their color: green - residential, blue - commercial, yellow - industrial, orange - city structures, grey - port structures. In under-view, the zone colors will be shown as outlines instead of colored-in squares.";
			break;
		case CITYTOOL_BUTTON_DISPLAYSIGNS:
			pStr = "- SIGN LAYER\n\n"
				"This will hide your city's names and labels. See the 'PLACE SIGN' tool above.";
			break;
		case CITYTOOL_BUTTON_DISPLAYINFRA:
			pStr = "- ROAD/TREE LAYER\n\n"
				"This will turn off the display of your roads, rail, wires, trees and other non-building structures.";
			break;
		case CITYTOOL_BUTTON_DISPLAYZONES:
			pStr = "- ZONE LAYER\n\n"
				"This button will allow you to examine your zones. In normal mode, this will hide all your zone buildings. In the under-view, this will put down colored tiles indicating the zone.";
			break;
		case CITYTOOL_BUTTON_DISPLAYUNDERGROUND:
			pStr = "- UNDER-VIEW LAYER\n\n"
				"This will transform your city display to a stick-figure outline of the terrain. Pipes and subways will become visible, while all surface items will be hidden. The other layer buttons are still available and allow you to further customize the view.";
			break;
		case CITYTOOL_BUTTON_HELP:
			pStr = GetHelpString_GeneralHelp();
			break;
		case CITYTOOL_BUTTON_RCI:
			pStr = "- ZONE DEMAND\n\n"
				"These colored bars show you the current demand for each type of zone in your city.\n\n"
				"If the bar is up in the \"+\" area then your city needs more of that type of zone.\n\n"
				"The letters represent \"R\"esidential, \"C\"ommercial and \"I\"ndustrial.";
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString_MapToolBar(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case MAPTOOL_BUTTON_TOGGLEOCEAN:
			pStr = "- COAST SELECT\n\n"
				"This button will add a coastline to the next map generated.";
			break;
		case MAPTOOL_BUTTON_TOGGLERIVER:
			pStr = "- RIVER SELECT\n\n"
				"This button will add a river to the next map generated.";
			break;
		case MAPTOOL_BUTTON_TERRAINHILLS:
			pStr = "- HILL SLIDER\n\n"
				"This slider adjusts how hilly the generated terrain will be.";
			break;
		case MAPTOOL_BUTTON_TERRAINWATER:
			pStr = "- WATER SLIDER\n\n"
				"This slider adjusts how wet the generated terrain will be. It will adjust both the sea level on the map as well as the number of streams and lakes.";
			break;
		case MAPTOOL_BUTTON_TERRAINTREES:
			pStr = "- TREE SLIDER\n\n"
				"This slider adjusts the number of trees on the generated terrain.";
			break;
		case MAPTOOL_BUTTON_MAKE:
			pStr = "- MAKE NEW MAP\n\n"
				"This button will generate a new map. The new map will be based on the settings of the two buttons and three sliders above.";
			break;
		case MAPTOOL_BUTTON_RAISETERRAIN:
			pStr = "- RAISE TERRAIN\n\n"
				"This tool will raise the altitude when you click on the terrain, thereby creating hills.";
			break;
		case MAPTOOL_BUTTON_LOWERTERRAIN:
			pStr = "- LOWER TERRAIN\n\n"
				"This tool will lower the altitude when you click on the terrain, thereby creating valleys.";
			break;
		case MAPTOOL_BUTTON_STRETCHTERRAIN:
			pStr = "- STRETCH TERRAIN\n\n"
				"This tool will allow you to stretch the terrain up or down. Click on the tile you wish to change and slowly move the mouse up or down while holding the button.";
			break;
		case MAPTOOL_BUTTON_LEVELTERRAIN:
			pStr = "- LEVEL TERRAIN\n\n"
				"This tool will level terrain and remove trees. Click on the tile level you wish to extend and move the mouse in the direction you want while holding down the mouse button.";
			break;
		case MAPTOOL_BUTTON_INCREASEWATERLEVEL:
			pStr = "- RAISE SEA LEVEL\n\n"
				"Each time you press this button the sea level across the entire map will be raised one level.";
			break;
		case MAPTOOL_BUTTON_DECREASEWATERLEVEL:
			pStr = "- LOWER SEA LEVEL\n\n"
				"Each time you press this button the sea level across the entire map will be lowered one level.";
			break;
		case MAPTOOL_BUTTON_WATER:
			pStr = "- PLACE WATER\n\n"
				"This tool places tiles of water, thereby allowing you to create larger bodies of water like lakes and streams.";
			break;
		case MAPTOOL_BUTTON_STREAM:
			pStr = "- PLACE STREAM\n\n"
				"This tool creates streams. Click where you want the stream to start and it will flow downhill from that point.";
			break;
		case MAPTOOL_BUTTON_TREES:
			pStr = "- PLACE TREE\n\n"
				"This tool adds trees to the terrain. Holding down the SHIFT key while using this tool will remove trees.";
			break;
		case MAPTOOL_BUTTON_FOREST:
			pStr = "- PLACE FOREST\n\n"
				"This tool will add a forested area to the terrain. Holding down the SHIFT key while using this tool will remove trees.";
			break;
		case MAPTOOL_BUTTON_ZOOMOUT:
			pStr = "- ZOOM OUT\n\n"
				"There are four scales your city can be viewed at. This button allows you to increase the scale of your display. The tiles grow smaller and the area displayed grows.";
			break;
		case MAPTOOL_BUTTON_ZOOMIN:
			pStr = "- ZOOM IN\n\n"
				"There are three scales your city can be viewed at. This button allows you to decrease the scale of your display. The tiles grow larger and the area displayed shrinks.";
			break;
		case MAPTOOL_BUTTON_ROTATEANTICLOCKWISE:
			pStr = "- ROTATE COUNTER-CLOCKWISE\n\n"
				"Each time you click this button the scene in the window will rotate 90 degrees counter-clockwise.";
			break;
		case MAPTOOL_BUTTON_ROTATECLOCKWISE:
			pStr = "- ROTATE CLOCKWISE\n\n"
				"Each time you click this button the scene in the window will rotate 90 degrees clockwise.";
			break;
		case MAPTOOL_BUTTON_CENTERINGTOOL:
			pStr = "- CENTER DISPLAY\n\n"
				"This is the centering tool. It is used to scroll around your city. When you click in the window the scene will re-center on the place you clicked. If you click near the center of the window and hold both the 'Alt' key and mouse button down, you can then smoothly scroll around by moving the mouse to adjust direction.";
			break;
		case MAPTOOL_BUTTON_DONE:
			pStr = "- DONE\n\n"
				"When you are finished editing the terrain this button will bring you into the game. Make sure you are finished because you cannot return to the map-editing mode once the game has started.";
			break;
		case MAPTOOL_BUTTON_HELP:
			pStr = GetHelpString_GeneralHelp();
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString_Budget(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case SC2K_DIALOG_BUDGET_EDIT_NAMEYEARMONTH:
			pStr = "This is the status region of the budget. It gives your city name, the current year and month. When the budget window automatically opens each January, it will also contain an hourglass that slowly empties unless you click in the window. When the sands run out, SimCity 2000 assumes that the mayor is out to lunch and resumes the simulation.";
			break;
		case IDOK:
			pStr = "Click the DONE button when you are finished adjusting your city budget.";
			break;
		case SC2K_DIALOG_BUDGET_HELP:
			pStr = GetHelpString_GeneralHelp();
			break;
		case SC2K_DIALOG_BUDGET_LBL_PROPTAX:
			pStr = "These are your property taxes. The tax rate shown to the right is an average of the rates levied against your three zoning types (Residential, Commercial, Industrial). If you wish to alter taxes on a specific zone, then click on the book to the right.\n\n"
				"The arrows to the right of the percent box will increase or decrease your tax rate by one percentage point. The minimum tax is 0% and the maximum is 20%. A tax rate of 7% is average. Low taxes attract citizens and businesses. High taxes will drive them out, but earn you more.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_ORDINANCES:
			pStr = "This is a summary of the costs and revenues of city programs and ordinances. To examine which programs are in effect (or to implement an ordinance) click on the book symbol to the right, which brings up the Ordinances Window.\n\n"
				"If you see money in this column, but have never implemented an ordinance, you might want to check the Ordinances Window. If you have been doing very well, the city counselors sometimes take it into their heads to begin beneficial programs using public funds.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_BINDPAYMNT:
			pStr = "Bonds are moneys loaned to you by the public which you must repay at a given interest rate. The rate on these bonds is based on the prime rate plus a percentage based on your city value. The best rate is prime+1%. Given that the prime rate changes from year to year, it is possible to have an outstanding bond with a rate that is lower than the current prime.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_POLICE:
			pStr = "This is your police department funding. Police departments reduce crime and stop riots. With less than 100% funding, the police will be less efficient, but the annual cost will decrease accordingly. A single police station costs $100 for one full year of operation.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_FIRE:
			pStr = "This is your fire department funding. Fire departments prevent fires and clean up toxic spills. With less than 100% funding, your firefighters will be less efficient, but the annual cost will decrease accordingly. A single fire station costs $100 for one full year of operation.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_HEALTH:
			pStr = "This is your hospital funding. Hospitals receive matching funds from the state and federal government, but your money helps it to operate. Try to maintain 100% funding over several decades, and your average Life Expectancy (LE) will improve. Hospitals cost $50 for one full year of operation.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_EDUCATION:
			pStr = "This is your educational funding. Without public schools, education is based on a verbal lore handed from parent to child. Schools add to this Educational Quotient (EQ) and Colleges double EQ for 15-25 year olds. Maintain 100% funding for several decades in order to improve the city EQ. Schools cost $25 per year, Colleges cost $100 per year.";
			break;
		case SC2K_DIALOG_BUDGET_LBL_TRANSIT:
			pStr = "Your city transit costs are broken down into six categories: Road, Highway, Bridge, Rail, Subway, Tunnel. When your maintenance is less than 100%, these vital city services start to decay. Roads, rails, and highways turn to rubble. Sections of subway collapse. Bridges and tunnels become unstable.\n\n"
				"You might reduce some of these expenses for a year or two during a financial crisis, but extended cutbacks will strangle your city.";
			break;
		case SC2K_DIALOG_BUDGET_EDIT_PROPTAX:
		case SC2K_DIALOG_BUDGET_EDIT_POLICE:
		case SC2K_DIALOG_BUDGET_EDIT_FIRE:
		case SC2K_DIALOG_BUDGET_EDIT_HEALTH:
		case SC2K_DIALOG_BUDGET_EDIT_EDUCATION:
		case SC2K_DIALOG_BUDGET_EDIT_TRANSIT:
			pStr = "These are the 'percent' areas. It shows either your current funding level or your current tax rate.\n\n"
				"For most budget items, the percent is a number from zero to one hundred. One hundred indicates full funding for that category. Less than full funding has a detrimental effect on the given department: Fire and police service smaller areas, hospitals help fewer people, transit cutbacks allow roads to decay, and education cutbacks result in a lowered EQ.\n\n"
				"The percent next to Property Taxes indicates the average tax rate for all zones. A higher rate returns more revenues but inhibits growth. A lower rate encourages growth but returns less money. Sometimes a lower rate can bring in more funds because the city grows larger.";
			break;
		case SC2K_DIALOG_BUDGET_SCROLLBAR_PROPTAX:
		case SC2K_DIALOG_BUDGET_SCROLLBAR_POLICE:
		case SC2K_DIALOG_BUDGET_SCROLLBAR_FIRE:
		case SC2K_DIALOG_BUDGET_SCROLLBAR_EDUCATION:
		case SC2K_DIALOG_BUDGET_SCROLLBAR_TRANSIT:
		case SC2K_DIALOG_BUDGET_SCROLLBAR_HEALTH:
			pStr = "The arrows next to the percent boxes adjust your city's revenues and expenses. Up arrows increase the percentage and down arrows decrease it. When the percentage is adjusted, only the 'Estimated' costs will change. The 'Year to Date' column shows funds that have already been paid or collected.\n\n"
				"The 'Property Taxes' are your main source of revenue. The arrows alter the tax rate by +/-1%. The minimum is zero and the maximum is twenty.\n\n"
				"The other categories are funding rates for various city services. 100% funding means you are paying full price and receiving full service. The up and down arrows adjust the funding by +/-5%. The minimum is zero and the maximum is one hundred.";
			break;
		case SC2K_DIALOG_BUDGET_EDIT_TDE_PROPTAX:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_ORDINANCES:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_BONDPAYMNT:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_POLICE:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_FIRE:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_HEALTH:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_EDUCATION:
		case SC2K_DIALOG_BUDGET_EDIT_TDE_TRANSIT:
			pStr = "This column shows the Year To Date revenues and expenses. This is the money that has accumulated during the current budget year. It will not be added or subtracted from current funds until the year's end. If the number in this column baffles you, open the book to the right for detailed information.\n\n"
				"Example: A police department costs $100 per year to maintain. If it were built in September, then it would only cost $33 for the year (one-third of a year). If it were set to 50% funding in July, it would cost $75 for the year ($50 for the first half, $25 for the second).";
			break;
		case SC2K_DIALOG_BUDGET_EDIT_YEE_PROPTAX:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_ORDINANCES:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_BONDPAYMNT:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_POLICE:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_FIRE:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_HEALTH:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_EDUCATION:
		case SC2K_DIALOG_BUDGET_EDIT_YEE_TRANSIT:
			pStr = "This shows the estimated total revenues and expenses for the current or next budget year. The number incorporates the year to date amount and makes an estimate based on the current city status. For example, if there were two police departments, the estimated cost would be $200 for the next year. This number is subject to change, however. If a police department were built or destroyed, or the funding levels changed in mid-year, then the actual year end amount would be different from the original estimate.\n\n"
				"Examine the books to the right for more details.";
			break;
		case SC2K_DIALOG_BUDGET_BTN_PROPTAX:
		case SC2K_DIALOG_BUDGET_BTN_ORDINANCES:
		case SC2K_DIALOG_BUDGET_BTN_BONDPAYMNT:
		case SC2K_DIALOG_BUDGET_BTN_POLICE:
		case SC2K_DIALOG_BUDGET_BTN_FIRE:
		case SC2K_DIALOG_BUDGET_BTN_HEALTH:
		case SC2K_DIALOG_BUDGET_BTN_EDUCATION:
		case SC2K_DIALOG_BUDGET_BTN_TRANSIT:
			pStr = "The books give you detailed information and control for each of the budget categories. Clicking on a specific book icon will open a window for that item.";
			break;
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_PROPTAX:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_ORDINANCES:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_BONDPAYMNT:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_POLICE:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_FIRE:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_HEALTH:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_EDUCATION:
		case SC2K_DIALOG_BUDGET_BTN_ADVISOR_TRANSIT:
			pStr = "The question balloons ask each of the eight budget commisioners for advice on how to operate the budget. Remember that the commisioners each has his or her own priorities and they may give you conflicting advice.";
			break;
		case SC2K_DIALOG_BUDGET_STATIC_SUMMARY:
			pStr = "This area shows the cash left over from the previous budget period, the amount of money accumulated this year, and the estimated total funds for the full year. Estimated and accrued funds are not added into current funds until the year's end.\n\n"
				"Sometimes the totals for To Date Expenses and Year End Estimate do not add up. This is due to each category having fractional amounts that aren't shown. For example, if Property Taxes earn 4.5 dollars and Ordinances gather 3.5 dollars, they will be displayed as 4 and 3 respectively, but the total is still 8.";
			break;
		case SC2K_DIALOG_BUDGET_STATIC_EOYFUNDS:
			pStr = "This shows what your total is with either accrued or estimated funds added in.\n\n"
				"During a year end, the accrued column shows what your funds will be in the upcoming year. At other times, the Estimated column shows what you will probably earn during the current year.";
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString_Ordinances(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case ORDINANCE_OPT_SALES_TAX:
			pStr = "A 1% Sales Tax will earn the city additional revenues each year, but may inhibit local commerce.";
			break;
		case ORDINANCE_OPT_INCOME_TAX:
			pStr = "The 1% Income Tax will bring additional funds to the city coffers, but may induce city residents to move away.";
			break;
		case ORDINANCE_OPT_LEGALIZED_GAMBLING:
			pStr = "Legalized Gambling will increase your tourist trade and act as an incentive to commerce. It also earns the city money as permits are sold. Unfortunately, it tends to attract a criminal element.";
			break;
		case ORDINANCE_OPT_PARKING_FINES:
			pStr = "Parking Fines will earn your city revenues and encourage city residents to use mass transit. It does, however, make people think twice before moving to your city.";
			break;
		case ORDINANCE_OPT_VOLUNTEER_FIRE_DEPARTMENT:
			pStr = "A Volunteer Fire Dept. is much cheaper than a regular fire department. For a small monthly investment, Fire Protection is increased across the city.";
			break;
		case ORDINANCE_OPT_PUBLIC_SMOKING_BAN:
			pStr = "For a small monthly fee, you can enforce a Public Smoking Ban. This will marginally increase the Life Expectancy of your citizens due to reduced second-hand smoke inhalation.";
			break;
		case ORDINANCE_OPT_FREE_CLINICS:
			pStr = "Free Clinics cost a fair sum to maintain, but they significantly increase the Life Expectancy (LE) of your citizens by helping low-income families.";
			break;
		case ORDINANCE_OPT_JUNIOR_SPORTS:
			pStr = "Junior Sports programs cost a moderate sum of money, but promote healthy habits that can last throughout the young citizen's lives.";
			break;
		case ORDINANCE_OPT_PRO_READING_CAMPAIGN:
			pStr = "A Pro Reading Campaign is an inexpensive advertising blitz that increases the Educational Quotient of your citizens.";
			break;
		case ORDINANCE_OPT_ANTI_DRUG_CAMPAIGN:
			pStr = "An Anti-Drug Campaign is an inexpensive advertising blitz that reduces crime.";
			break;
		case ORDINANCE_OPT_CPR_TRAINING:
			pStr = "CPR Training programs are an inexpensive way to increase your citizens' Life Expectancy (LE).";
			break;
		case ORDINANCE_OPT_NEIGHBORHOOD_WATCH:
			pStr = "A Neighborhood Watch is an all-volunteer program. For a small investment, police protection across the city is increased.";
			break;
		case ORDINANCE_OPT_TOURIST_ADVERTISING:
			pStr = "Tourist Advertising is an expensive, nationwide campaign to attract tourism. It will increase local commerce.";
			break;
		case ORDINANCE_OPT_BUSINESS_ADVERTISING:
			pStr = "Business Advertising is an expensive, nationwide campaign to increase business contacts. This improves local industry.";
			break;
		case ORDINANCE_OPT_CITY_BEAUTIFICATION:
			pStr = "The City Beautification program is fairly expensive, but it improves city-wide land values and attracts tourists.";
			break;
		case ORDINANCE_OPT_ANNUAL_CARNIVAL:
			pStr = "The Annual Carnival is a moderately expensive way to increase your tourist trade and local commerce.";
			break;
		case ORDINANCE_OPT_ENERGY_CONSERVATION:
			pStr = "The Energy Conservation program is an advertising campaign that reduces energy use. It effectively increases the number of citizens your power plants can supply.";
			break;
		case ORDINANCE_OPT_NUCLEAR_FREE_ZONE:
			pStr = "Declaring your city a Nuclear Free Zone costs nothing. It improves the desirability of your city to residents, but marginally decreases desirability for industry.\n\n"
				"This will NOT stop the military from building missile silos or basing nuclear weapons near your city IF you give them permission to build a base.";
			break;
		case ORDINANCE_OPT_HOMELESS_SHELTER:
			pStr = "Homeless Shelters increase the number of laborers available to local industry and marginally improves downtown land values.";
			break;
		case ORDINANCE_OPT_POLLUTION_CONTROLS:
			pStr = "Pollution Controls will significantly reduce pollution emissions from local industry. Unfortunately, this will drive away some industries.";
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString_Population(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case SC2K_DIALOG_POPULATION_RADIO_POPULATION:
			pStr = "This graph shows the percentage of citizens in each age group. The population graph bars between 20 and 55 are considered your potential working-class citizens.";
			break;
		case SC2K_DIALOG_POPULATION_RADIO_HEALTH:
			pStr = "This graph shows the health of citizens in each age group. The average health of your working-class citizens is expressed as their Life Expectancy (LE).";
			break;
		case SC2K_DIALOG_POPULATION_RADIO_EDUCATION:
			pStr = "This graph shows the education level or \"EQ\" of citizens in each age group. Higher EQ levels attract higher-tech industries to your city. Your citizens' EQ is affected by the presence of schools, colleges, libraries, and museums.";
			break;
		default:
			break;
	}
	return pStr;
}

// The CityMap case is a combination of tab indices and control IDs.
static const char *GetHelpString_CityMap(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case CITYMAPDLG_STRUCTURESZONES:
			pStr = "Click here to be able to select STRUCTURES or ZONES from the ListBox below.\n\n"
				"In the STRUCTURES display, shades of brown indicate altitude; green areas are trees; blue areas are water.\n\n"
				"In the ZONES display, green indicates residential zones, blue indicates commercial zones, and yellow indicates industrial zones.";
			break;
		case CITYMAPDLG_ROADRAILTRAFFIC:
			pStr = "Click here to be able to select ROADS, RAIL or TRAFFIC from the ListBox below.\n\n"
				"ROADS shows your roads as white areas.\n\n"
				"RAIL shows your rail lines as white areas.\n\n"
				"TRAFFIC indicates traffic density with shades of gray. Dark gray is dense traffic, white is light traffic.";
			break;
		case CITYMAPDLG_POWERGRID:
			pStr = "Click here to see your power grid. White indicates wires, yellow indicates powered areas, and red indicates unpowered areas.";
			break;
		case CITYMAPDLG_WATERSUPPLY:
			pStr = "Click here to examine your water/supply. Yellow indicates the areas receiving water. Red indicates the areas that are unwatered.";
			break;
		case CITYMAPDLG_POPDENSITYROG:
			pStr = "Click here to be able to select the density or rate of growth of your city's growth from the ListBox below.\n\n"
				"In the population density display, dark gray indicates dense population and white indicates sparse population.\n\n"
				"In the rate of growth display, green indicates positive growth and red indicates negative growth.";
			break;
		case CITYMAPDLG_POLICECRIME:
			pStr = "Click here to be able to select the CRIME RATE, POLICE POWER, or POLICE DEPTS from the ListBox below.\n\n"
				"CRIME RATE indicates levels of crime with shades of gray. Dark gray is the worst, white is the best.\n\n"
				"POLICE POWER shows the areas protected by police departments.\n\n"
				"POLICE DEPTS shows the location of police stations.";
			break;
		case CITYMAPDLG_POLLUTION:
			pStr = "Click here to see the pollution in your city. Dark gray is the worst pollution, white is the least.";
			break;
		case CITYMAPDLG_LANDVALUE:
			pStr = "Click here to show land values in your city. Dark gray indicates the valuable land. Light gray indicates less valuable land.\n\n"
				"Unshaded areas have not been zoned by the city and have not had their value assessed.";
			break;
		case CITYMAPDLG_FIREEDUCATION:
			pStr = "Click here to be able to select the FIRE POWER, FIRE DEPTS, SCHOOLS or COLLEGES from the ListBox below.\n\n"
				"FIRE POWER shows the areas protected by fire departments.\n\n"
				"FIRE DEPTS, SCHOOLS, COLLEGES show the locations of each of these items.";
			break;
		case SC2K_DIALOG_CITYMAP_STATIC_MAPAREA:
			pStr = "This is the Map window. Shift-Click on the tabs above to determine what can be displayed here (and the button below to indicate the effect in the main edit window).\n\n"
				"The selected entry in the ListBox below will dictate what is currently displayed.\n\n"
				"There is also a tilted box outlined in the window. This indicates the area viewed in the City window. The \"T\" on the box shows the top of the city window.";
			break;
		case SC2K_DIALOG_CITYMAP_BTN_SHOWCITYINWINDOW:
			pStr = "While this button is depressed the data you are currently viewing will also appear in the main edit window. This is useful for pinpointing areas of high crime, heavy traffic, etc.";
			break;
		default:
			break;
	}
	return pStr;
}

// The Industry case is a combination of hit indices and control IDs.
static const char *GetHelpString_Industry(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case IND_STEELMINING:
			pStr = "Steel/Mining is a heavy industry that prefers dense zoning. It is a major source of employment. It causes a lot of pollution, and can be stifled by enacting the Pollution Controls ordinance.";
			break;
		case IND_TEXTILES:
			pStr = "Textiles is a fairly heavy industry that can employ thousands in a large city. It contributes to both air and water pollution, and can be hampered by enacting the Pollution Controls ordinance.";
			break;
		case IND_PETROCHEMICAL:
			pStr = "Petrochemicals is a high-tech industry that requires a city with a high Education Quotient to be properly staffed. It is a serious polluter, and can be chased away by enacting the Pollution Controls ordinance.";
			break;
		case IND_FOOD:
			pStr = "Food is a fairly stable industry that can exist in both high and low-density areas. It doesn't require a high Education Quotient, and doesn't pollute (much).";
			break;
		case IND_CONSTRUCTION:
			pStr = "The construction industry isn't a big employer. Though spurred by local growth, it depends primarily on temporary, out of town workers.";
			break;
		case IND_AUTOMATIVE:
			pStr = "The Automotive industry is a big employer. It requires a work force with a high Education Quotient. It has the potential to be a bad polluter, and is negatively affected by enacting the Pollution Controls ordinance.";
			break;
		case IND_AEROSPACE:
			pStr = "Aerospace is a heavy, dense industry that can employ many citizens. It requires a work force with a very high Education Quotient.";
			break;
		case IND_FINANCE:
			pStr = "Finance is a medium to low-density industry that requires an educated work force.";
			break;
		case IND_MEDIA:
			pStr = "Media is a medium to low-density industry that requires an educated work force.";
			break;
		case IND_ELECTRONICS:
			pStr = "Electronics is a potentially huge employer that will continue to grow for many decades to come. It is a mild polluter, and requires a very well-educated work force.";
			break;
		case IND_TOURISM:
			pStr = "Tourism is a very mild polluter (mostly litter), that can exist in high or low-density areas. It thrives in cities with lots of tourist attractions (zoos, marinas, etc.) and scenic beauty. A high crime rate can ruin tourism.";
			break;
		case SC2K_DIALOG_INDUSTRY_RADIO_RATIOS:
			pStr = "This button will graph the relative amount of each industry type that currently exists within your city. This ratio will influence employment, industrial growth, educational needs, and pollution within your city.";
			break;
		case SC2K_DIALOG_INDUSTRY_RADIO_TAXRATES:
			pStr = "This graph allows you to set different tax rates for each industry type by dragging the particular industry's blue bar to the left or right. This is useful if you want to promote or retard certain industries. For instance if you think the automobile is about to be the next big thing, try lowering the tax rate for that industry to spur growth. On the other hand, if the pollution from the steel mills is getting you down, increase their tax rate to run them out of town.\n\n"
				"You can adjust all the industries at the same time by holding the ALT key as you drag the bars.";
			break;
		case SC2K_DIALOG_INDUSTRY_RADIO_DEMAND:
			pStr = "This graph will show you the national demand for each of the industry groups. This demand is what fuels the growth of your city. The demands here are based on a model of the national economy. The newspaper is your primary source of information on the national economy; look there for hints about which industries you should be promoting.";
			break;
		default:
			break;
	}
	return pStr;
}

static const char *GetHelpString_Graphs(int nIndex) {
	const char *pStr = NULL;
	switch (nIndex) {
		case SC2K_DIALOG_GRAPH_RADIO_RANGEONEYEAR:
			pStr = "This button displays the graph data for a one-year period. The time chart shows the months displayed. The current month is farthest to the right.";
			break;
		case SC2K_DIALOG_GRAPH_RADIO_RANGETENYEARS:
			pStr = "This button displays the graph data for a ten-year period. The time chart shows the years displayed. The current year is farthest to the right.";
			break;
		case SC2K_DIALOG_GRAPH_RADIO_RANGEHUNDREDYEARS:
			pStr = "This button displays the graph data for a hundred-year period. The time chart shows the decades displayed. The current decade is farthest to the right.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCITYSIZE:
			pStr = "City Size is your city's total population. It is the sum of commercial laborers, industrial laborers, and their families. \"Residents, \" \"Commerce, \" and \"Industry\" are all scaled according to \"City Size.\"";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTRESIDENTS:
			pStr = "Residents are the children, parents and home-spouses of the work force. It is scaled against the \"City Size.\"";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCOMMERCE:
			pStr = "Commerce indicates the number of laborers working for local services. Their jobs produce for the local market only. It is scaled against the \"City Size.\"";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTINDUSTRY:
			pStr = "Industry are the laborers working for local industry. Their jobs produce for the external markets. It is scaled against the \"City Size.\"";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTTRAFFIC:
			pStr = "Traffic measures the average cars per minute of all city roads, highways and bridges.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOLLUTION:
			pStr = "Pollution indicates the amount of air pollution. It is measured in PPM (parts per million).";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTLANDVALUE:
			pStr = "Value is an average of your city block value. It is measured in thousands of dollars per lot.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTCRIME:
			pStr = "Crime is an average measure of criminal activity. It is measured in annual incidents per thousand citizens.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTPOWERPERCENT:
			pStr = "Power% indicates your power surplus. It is measured as an INVERSE percent of usage. The percentage indicates how much power is left.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTWATERPERCENT:
			pStr = "Water% indicates your water surplus. It is measured as an INVERSE percent of usage. The percentage indicates how much pumped water goes unused.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTHEALTH:
			pStr = "Health is an average measure of your citizens' health. It is measured in Life Expectancy (LE), which is the number of years a laborer is expected to live.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTEDUCATION:
			pStr = "Education is a measure of your citizens' educational level. It is measured using an Educational Quotient (EQ). The average national citizen has an EQ of 100.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTUNEMPLOYMENT:
			pStr = "The unemp. graph shows the percentage of people in your city who want to find work but cannot.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTGNP:
			pStr = "GNP is the Gross National Product of the SimNation your city is in. It is a general measure of external demand for your industrial production.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTNATIONALPOP:
			pStr = "\"Nat'l Pop\" is your national population. It has little relevance to your city except for how it influences GNP, the Fed Rate and the current industrial demands, as indicated in the Industry window.";
			break;
		case SC2K_DIALOG_GRAPH_CHECKBOX_OPTFEDRATE:
			pStr = "The \"Fed Rate\" is the percentage paid for bonds by the national government. The rate your city pays for bonds is closely related to this number. Fed Rate is influenced by both GNP and Nat'l Pop. You cannot control this, but by watching GNP and Nat'l Pop., you might predict it.";
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
		case HELPTYPE_MAPTOOLBAR:
			pStr = GetHelpString_MapToolBar(nIndex);
			break;
		case HELPTYPE_FLOATINGSTATUS:
			if (nIndex == 1)
				pStr = "This is the big red arrow. By clicking here, you can cycle through the disasters afflicting your city.";
			else
				pStr = "The Status Window shows the currently selected tool and its cost. It also has an iconic display for the weather. The second line of the window shows messages, warnings, and recommendations.\n\n"
					"In the Emergency Mode, the weather icon changes into a big red arrow. By clicking here, you can cycle through the disasters afflicting your city.";
			break;
		case HELPTYPE_BUDGET:
			pStr = GetHelpString_Budget(nIndex);
			break;
		case HELPTYPE_ORDINANCES:
			pStr = GetHelpString_Ordinances(nIndex);
			break;
		case HELPTYPE_POPULATION:
			pStr = GetHelpString_Population(nIndex);
			break;
		case HELPTYPE_CITYMAP:
			pStr = GetHelpString_CityMap(nIndex);
			break;
		case HELPTYPE_INDUSTRY:
			pStr = GetHelpString_Industry(nIndex);
			break;
		case HELPTYPE_GRAPHS:
			pStr = GetHelpString_Graphs(nIndex);
			break;
		case HELPTYPE_NEIGHBOURS:
			pStr = "The neighbor window displays your city's population along with the population of its neighboring cities and the total population of SimNation. Use this window to compare your city with the cities you compete with for people and other resources, and to see just how big a part of the whole nation you are (or aren't).";
			break;
		default:
			if (nIndex == 1)
				pStr = "- Using Help:\n\nGo to Help -> Information";
			else
				pStr = "- Welcome to SimCity 2000!\n\n"
				"INFORMATION\n\n"
				"Help 'tips' are available in the following areas by holding down 'Shift' and clicking your mouse over a given control or section:\n\n"
				" - City ToolBar - Controls and RCI 'widget'\n\n"
				" - Map ToolBar - Controls\n\n"
				" - Status Floating - General area (also during the disaster state the 'Red Arrow' button)\n\n"
				" - Budget Main Dialog - Controls, labels and various fields\n\n"
				" - Ordinances Dialog - Checkbox controls\n\n"
				" - Population Dialog - Buttons\n\n"
				" - CityMap Dialog - Tabs, button at the bottom and minimap view\n\n"
				" - Industry Dialog - Labels, icons, bars and buttons\n\n"
				" - Graphs Dialog - Buttons\n\n"
				" - Neighbors Dialog - Entire map area";
			break;
	}
	return pStr;
}

BOOL CALLBACK ConfHelpDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	helpDlg_t *hlpD;
	const char *pStr;
	bool bInitialized;
	HDC hDC;
	int nDlgFrameCX, nDlgFrameCY;
	HFONT hOldFont;
	HWND hwndItem;
	RECT butRect, r;
	int nHeight, nY;
	PAINTSTRUCT ps;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		hlpD = (helpDlg_t *)lParam;

		bInitialized = false;

		pStr = GetHelpString(hlpD->nType, hlpD->nIndex);
		if (pStr) {
			hDC = GetDC(hwndDlg);
			if (hDC) {
				hOldFont = SelectFont(hDC, hFontMSSansSerifBold8);
				GetClientRect(hwndDlg, &hlpD->textRect);
				DrawTextA(hDC, pStr, strlen(pStr), &hlpD->textRect, DT_WORDBREAK | DT_CALCRECT);
				nDlgFrameCX = GetSystemMetrics(SM_CXDLGFRAME);
				if (hlpD->nType == HELPTYPE_GENERAL)
					nDlgFrameCX *= 2;
				nDlgFrameCY = GetSystemMetrics(SM_CYDLGFRAME);
				hwndItem = GetDlgItem(hwndDlg, IDOK);
				ShowWindow(hwndItem, SW_SHOW);
				GetClientRect(hwndItem, &butRect);
				CopyRect(&r, &hlpD->textRect);
				nHeight = butRect.bottom + 2 * nDlgFrameCY + 44 - butRect.top + 24;
				InflateRect(&r, 2 * nDlgFrameCX + 44, nHeight);
				SetWindowPos(hwndDlg, HWND_TOP, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOMOVE);
				nY = butRect.bottom - butRect.top + 32;
				OffsetRect(&hlpD->textRect, 20, nY);
				CopyRect(&r, &hlpD->textRect);
				InflateRect(&r, 15, 15);
				SelectFont(hDC, hOldFont);
				ReleaseDC(hwndDlg, hDC);

				bInitialized = true;
			}
		}
		if (bInitialized)
			CenterDialogBox(hwndDlg);
		else
			EndDialog(hwndDlg, FALSE);
		return TRUE;

	case WM_PAINT:
		hlpD = (helpDlg_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		BeginPaint(hwndDlg, &ps);
		SetBkMode(ps.hdc, TRANSPARENT);
		hOldFont = SelectFont(ps.hdc, hFontMSSansSerifBold8);
		pStr = GetHelpString(hlpD->nType, hlpD->nIndex);
		if (pStr)
			DrawTextA(ps.hdc, pStr, strlen(pStr), &hlpD->textRect, DT_WORDBREAK);
		SelectFont(ps.hdc, hOldFont);
		EndPaint(hwndDlg, &ps);
		return FALSE;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_KEYDOWN:
		hlpD = (helpDlg_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		if (hlpD->bGeneralClose)
			EndDialog(hwndDlg, TRUE);
		return TRUE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwndDlg, TRUE);
			break;
		}
		return TRUE;
	}
	return FALSE;
}

// Needed context:
// 'nType' - Check out the enum containing the 'HELPTYPE' entries.
// 'nIndex' - this can be:
// - For the City/Map Toolbars it is the button position index (not the control index).
// - For the Floating Status Widget it is ignored.
// - For all other dialogues most of the time it'll be their defined Ctrl IDs (unless stated otherwise).
//
// 'bFromMain' should be set to true only when DisplayItemHelp() is called from a modeless
// dialogue, the map/city toolbars, the Help menu of when F1 is pressed while the View
// window is active and focused; otherwise if a modal dialogue is open, set it to false.
// This is to ensure correct disable/enable behaviour when it comes to the floating status
// bar - a requirement at present until a method of better integration becomes available.
void DisplayItemHelp(HWND hWnd, int nType, int nIndex, bool bFromMain) {
	helpDlg_t hlpD;

	if (bHelpOpen)
		return;

	if (nType >= HELPTYPE_COUNT) {
		L_MessageBoxA(hWnd, "Invalid 'Help' type.", gamePrimaryKey, MB_ICONERROR);
		return;
	}

	memset(&hlpD, 0, sizeof(hlpD));
	hlpD.nType = nType;
	hlpD.nIndex = nIndex;
	hlpD.bGeneralClose = (nType == HELPTYPE_GENERAL) ? false : true;

	bHelpOpen = true;
	if (bFromMain)
		ToggleFloatingStatusDialog(FALSE);

	ConsoleLog(LOG_DEBUG, "DisplayItemHelp(%d, %d)\n", hlpD.nType, hlpD.nIndex);
	DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_HELPDISPLAY), hWnd, ConfHelpDialogProc, (LPARAM)&hlpD);

	if (bFromMain)
		ToggleFloatingStatusDialog(TRUE);
	bHelpOpen = false;
}

extern "C" void __stdcall Hook_WinApp_WinHelpA(unsigned int dwData, unsigned int nCmd) {
	CMFC3XWinApp *pThis;

	__asm mov [pThis], ecx

	ConsoleLog(LOG_DEBUG, "0x%06X -> CWinApp::WinHelpA(%u, %u)\n", _ReturnAddress(), dwData, nCmd);
	// Goes nowhere, does nothing. Will never do anything.
}

extern "C" void __stdcall Hook_WinApp_OnHelpIndex() {
	CMFC3XWinApp *pThis;

	__asm mov [pThis], ecx

	HWND hWnd;

	ConsoleLog(LOG_DEBUG, "0x%06X -> CWinApp::OnHelpIndex()\n", _ReturnAddress());

	hWnd = GetActiveWindow();
	DisplayItemHelp(hWnd, HELPTYPE_GENERAL, 0, true);
}

extern "C" void __stdcall Hook_WinApp_OnHelpUsing() {
	CMFC3XWinApp *pThis;

	__asm mov [pThis], ecx

	HWND hWnd;

	ConsoleLog(LOG_DEBUG, "0x%06X -> CWinApp::OnHelpUsing()\n", _ReturnAddress());

	hWnd = GetActiveWindow();
	DisplayItemHelp(hWnd, HELPTYPE_GENERAL, 1, true);
}

extern "C" LRESULT __stdcall Hook_FrameWnd_OnCommandHelp(WPARAM wParam, LPARAM lParam) {
	CMFC3XFrameWnd *pThis;

	__asm mov [pThis], ecx

	ConsoleLog(LOG_DEBUG, "0x%06X -> CFrameWnd::OnCommandHelp(%u, %u)\n", _ReturnAddress(), wParam, lParam);
	// Goes nowhere, does nothing. Will never do anything.
	return 1;
}

extern "C" LRESULT __stdcall Hook_Dialog_OnCommandHelp(WPARAM wParam, LPARAM lParam) {
	CMFC3XDialog *pThis;

	__asm mov [pThis], ecx

	ConsoleLog(LOG_DEBUG, "0x%06X -> CDialog::OnCommandHelp(%u, %u)\n", _ReturnAddress(), wParam, lParam);
	// Goes nowhere, does nothing. Will never do anything.
	return 1;
}

extern "C" LRESULT __stdcall Hook_GameDialog_OnCommandHelp(WPARAM wParam, LPARAM lParam) {
	CGameDialog *pThis;

	__asm mov [pThis], ecx

	ConsoleLog(LOG_DEBUG, "0x%06X -> CGameDialog::OnCommandHelp(%u, %u)\n", _ReturnAddress(), wParam, lParam);
	// Goes nowhere, does nothing. Will never do anything.
	return 1;
}

extern "C" void __stdcall Hook_SimcityApp_WinHelpA() {
	CSimcityAppPrimary *pThis;

	__asm mov [pThis], ecx

	CSimcityView *pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pThis);
	CMainFrame *pMainFrm = (CMainFrame *)pThis->m_pMainWnd;
	HWND hWnd = GetActiveWindow();

	if (pMainFrm && pSCView && pSCView->bSCVViewActive &&
		(hWnd == pMainFrm->m_hWnd || 
		hWnd == pMainFrm->dwMFStatusControlBar.m_hWnd ||
		hWnd == pMainFrm->dwMFCityToolBar.m_hWnd ||
		hWnd == pMainFrm->dwMFMapToolBar.m_hWnd)) {
		ConsoleLog(LOG_DEBUG, "0x%06X -> CSimcityApp::WinHelpA()\n", _ReturnAddress());
		DisplayItemHelp(pMainFrm->m_hWnd, HELPTYPE_GENERAL, 0, true);
	}
}

void InstallHelpHooks_SC2K1996(void) {
	// Nullify
	SafeVirtualProtect((LPVOID)0x4A194E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4A194E, Hook_WinApp_WinHelpA);

	// Direct accordingly
	SafeVirtualProtect((LPVOID)0x4B1622, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4B1622, Hook_WinApp_OnHelpIndex);

	// Direct accordingly
	SafeVirtualProtect((LPVOID)0x4B162E, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4B162E, Hook_WinApp_OnHelpUsing);

	// Nullify
	SafeVirtualProtect((LPVOID)0x4B8F8F, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4B8F8F, Hook_FrameWnd_OnCommandHelp);

	// Nullify
	SafeVirtualProtect((LPVOID)0x4A733A, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4A733A, Hook_Dialog_OnCommandHelp);

	// Nullify
	SafeVirtualProtect((LPVOID)0x4013E8, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4013E8, Hook_GameDialog_OnCommandHelp);

	// Direct under specific circumstances.
	SafeVirtualProtect((LPVOID)0x4015AA, 5, PAGE_EXECUTE_READWRITE);
	NEWJMP((LPVOID)0x4015AA, Hook_SimcityApp_WinHelpA);
}
