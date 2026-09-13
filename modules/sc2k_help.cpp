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
	bool bOkayButton;
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
		case MAPTOOL_BUTTON_RAISETERRAIN:
			break;
		case MAPTOOL_BUTTON_TOGGLEOCEAN:
			pStr = "- COAST SELECT\n\n"
				"This button will add a coastline to the next map generated.";
			break;
			// TXT1201 next
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
		case IDC_BUDGET_HELP:
			pStr = GetHelpString_GeneralHelp();
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
		case HELPTYPE_BUDGET:
			pStr = GetHelpString_Budget(nIndex);
			break;
		default:
			if (nIndex == 1)
				pStr = "- Using Help:\n\nGo to Help -> Information";
			else
				pStr = "- Welcome to SimCity 2000!\n\nINFORMATION\n\n1\n\n2\n\n3\n\n4\n\n5\n\n6\n\n7\n\n8\n\n9";
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
				nDlgFrameCY = GetSystemMetrics(SM_CYDLGFRAME);
				hwndItem = GetDlgItem(hwndDlg, IDOK);
				ShowWindow(hwndItem, (hlpD->bOkayButton) ? SW_SHOW : SW_HIDE);
				GetClientRect(hwndItem, &butRect);
				CopyRect(&r, &hlpD->textRect);
				nHeight = (hlpD->bOkayButton) ? butRect.bottom + 2 * nDlgFrameCY + 44 - butRect.top + 24 : 2 * nDlgFrameCY + 44;
				InflateRect(&r, 2 * nDlgFrameCX + 44, nHeight);
				SetWindowPos(hwndDlg, HWND_TOP, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOMOVE);
				nY = (hlpD->bOkayButton) ? butRect.bottom - butRect.top + 32 : 20;
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
	case WM_KEYDOWN:
		hlpD = (helpDlg_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		if (!hlpD->bOkayButton)
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
// - For all other dialogues it'll be their defined Ctrl IDs.
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
	hlpD.bOkayButton = (nType == HELPTYPE_GENERAL) ? true : false;

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
	HWND hWnd = GetActiveWindow();

	if (pThis->m_pMainWnd && hWnd == pThis->m_pMainWnd->m_hWnd && pSCView && pSCView->bSCVViewActive) {
		ConsoleLog(LOG_DEBUG, "0x%06X -> CSimcityApp::WinHelpA()\n", _ReturnAddress());
		DisplayItemHelp(hWnd, HELPTYPE_GENERAL, 0, false);
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
