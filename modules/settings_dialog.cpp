// sc2kfix modules/settings_dialog.cpp: NEW! settings dialog code and configurator
// (c) 2025-2026 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sc2kfix.h>
#include <keybindings.h>
#include "../resource.h"

#define TAB_COUNT 3

struct {
	HWND hwndTab;
	HWND hwndDisplay;
	RECT rcDisplay;
	DLGTEMPLATE* pDlgResource[TAB_COUNT];
	DLGPROC pDlgProc[TAB_COUNT];
	settings_t* stSettingsChanges;
} stSettingsDialogHeader;

static DLGTEMPLATE* SettingsGetDialogResource(LPCTSTR lpszResName) {
	HRSRC hRsrc = FindResource(hSC2KFixModule, lpszResName, RT_DIALOG);
	if (!hRsrc)
		return NULL;

	HGLOBAL hRes = LoadResource(hSC2KFixModule, hRsrc);
	if (!hRes)
		return NULL;

	return (DLGTEMPLATE*)LockResource(hRes);
}

static void SettingsTabSelectionChanged(HWND hwndDlg) {
	// Get the index of the selected tab.
	int i = TabCtrl_GetCurSel(stSettingsDialogHeader.hwndTab);

	// Destroy the current child dialog box, if any. 
	if (stSettingsDialogHeader.hwndDisplay != NULL)
		DestroyWindow(stSettingsDialogHeader.hwndDisplay);

	// Create the new child dialog box.
	stSettingsDialogHeader.hwndDisplay = CreateDialogIndirect(hSC2KFixModule, (DLGTEMPLATE*)stSettingsDialogHeader.pDlgResource[i], hwndDlg, stSettingsDialogHeader.pDlgProc[i]);
	return;
}

#define GET_CHECKBOX(dest, src) dest = (bool)Button_GetCheck(GetDlgItem(hwndDlg, src))
#define SET_CHECKBOX(src, dest) Button_SetCheck(GetDlgItem(hwndDlg, dest), src.ToBool() ? BST_CHECKED : BST_UNCHECKED)

static BOOL CALLBACK SettingsDialogGeneralTabProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwndParent;
	char szTempRegistrationNameBuffer[64] = { 0 };

	switch (message) {
	case WM_INITDIALOG:
		// Place ourselves in the correct position
		hwndParent = GetParent(hwndDlg);
		SetWindowPos(hwndDlg, NULL, stSettingsDialogHeader.rcDisplay.left, stSettingsDialogHeader.rcDisplay.top,
			(stSettingsDialogHeader.rcDisplay.right - stSettingsDialogHeader.rcDisplay.left),
			(stSettingsDialogHeader.rcDisplay.bottom - stSettingsDialogHeader.rcDisplay.top),
			SWP_SHOWWINDOW);

		Static_GetIcon(GetDlgItem(hwndDlg, IDC_STATIC_TOPSECRET), LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(GetDlgItem(hwndDlg, IDC_STATIC_RELEASEBANNER), WM_SETFONT, (WPARAM)hSystemRegular12, TRUE);
		InvalidateRect(hwndDlg, NULL, TRUE);

		// Create tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS),
			"Resets the file association entries in the registry so that .sc2 and .scn files will automatically open in SimCity 2000.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CONSOLE),
			"sc2kfix has a debugging console that can be activated by passing the -console argument to SimCity 2000's command line. "
			"This setting forces sc2kfix to always start the console along with the game, even if the -console argument is not passed.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES),
			"This setting checks to see if there's a newer release of sc2kfix available when the game starts.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DONT_LOAD_MODS),
			"Enabling this setting forces sc2kfix to skip loading any installed mods on startup.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SKIP_INTRO),
			"Once enabled the introduction videos will be skipped on startup. Only applies if videos have been detected.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");

		// Set fields based on the working JSON
		SetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Mayor Name"].ToString().c_str());
		SetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Company Name"].ToString().c_str());
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"], IDC_SETTINGS_CHECK_CONSOLE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"], IDC_SETTINGS_CHECK_SKIP_INTRO);

		return TRUE;

	case WM_DESTROY:
		// Update the working JSON based on our fields
		if (GetDlgItemText(hwndDlg, IDC_SETTINGS_MAYOR, szTempRegistrationNameBuffer, 63))
			jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Mayor Name"] = szTempRegistrationNameBuffer;
		if (GetDlgItemText(hwndDlg, IDC_SETTINGS_COMPANY, szTempRegistrationNameBuffer, 63))
			jsonSettingsCoreWorkingCopy["SimCity2000"]["Registration"]["Company Name"] = szTempRegistrationNameBuffer;

		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"], IDC_SETTINGS_CHECK_CONSOLE);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"], IDC_SETTINGS_CHECK_CHECK_FOR_UPDATES);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"], IDC_SETTINGS_CHECK_DONT_LOAD_MODS);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"], IDC_SETTINGS_CHECK_SKIP_INTRO);

		return TRUE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_SETTINGS_BUTTON_RESETFILEASSOCIATIONS:
			ResetFileAssociations();
			MessageBox(hwndDlg, ".sc2 and .scn file associations reset!", "It Works!", MB_OK | MB_ICONINFORMATION);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

static BOOL CALLBACK SettingsDialogGameplayTabProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwndParent;
	char szTempRegistrationNameBuffer[64] = { 0 };

	switch (message) {
	case WM_INITDIALOG:
		// Place ourselves in the correct position
		hwndParent = GetParent(hwndDlg);
		SetWindowPos(hwndDlg, NULL, stSettingsDialogHeader.rcDisplay.left, stSettingsDialogHeader.rcDisplay.top,
			(stSettingsDialogHeader.rcDisplay.right - stSettingsDialogHeader.rcDisplay.left),
			(stSettingsDialogHeader.rcDisplay.bottom - stSettingsDialogHeader.rcDisplay.top),
			SWP_SHOWWINDOW);

		// Create tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_STATUS_DIALOG),
			"The DOS and Mac versions of SimCity 2000 used a movable floating dialog to show the current tool, status line, and weather instead of a fixed bar at the bottom of the game window. "
			"Enabling this setting will use the floating status dialog instead of the bottom status bar.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_TITLE_DATE),
			"By default the title bar only displays the month and year. Enabling this setting will display the full in-game date instead.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_NEW_STRINGS),
			"Certain strings in the game have typos, grammatical issues, and/or ambiguous wording. This setting loads corrected strings in memory in place of the affected originals.");

		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_REFRESH_RATE),
			"SimCity 2000 was designed to spend more CPU time on simulation than on rendering by only updating the city's growth when the display moves or on the 24th day of the month. "
			"Enabling this setting allows the game to refresh the city display in real-time instead of batching display updates.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_DARK_UNDGRND),
			"When enabled the underground layer background will be dark.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS),
			"Certain versions of SimCity 2000 had higher quality sounds than the Windows 95 versions. "
			"This setting controls whether or not SimCity 2000 plays higher quality versions of various sounds for which said higher quality versions exist.\n\n"

			"Enabling or disabling this setting takes effect after restarting the game.");

		// Set fields based on the working JSON
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_floating_status"], IDC_SETTINGS_CHECK_STATUS_DIALOG);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"], IDC_SETTINGS_CHECK_TITLE_DATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"], IDC_SETTINGS_CHECK_NEW_STRINGS);

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"], IDC_SETTINGS_CHECK_REFRESH_RATE);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_background"], IDC_SETTINGS_CHECK_DARK_UNDGRND);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);

		return TRUE;

	case WM_DESTROY:
		// Update the working JSON based on our fields
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_floating_status"], IDC_SETTINGS_CHECK_STATUS_DIALOG);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"], IDC_SETTINGS_CHECK_TITLE_DATE);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"], IDC_SETTINGS_CHECK_NEW_STRINGS);

		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"], IDC_SETTINGS_CHECK_REFRESH_RATE);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_background"], IDC_SETTINGS_CHECK_DARK_UNDGRND);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"], IDC_SETTINGS_CHECK_SOUND_REPLACEMENTS);

		return TRUE;
	}

	return FALSE;
}

static BOOL CALLBACK SettingsDialogAudioTabProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwndParent;
	char szFluidSynthSettingPath[MAX_PATH] = { 0 };
	OPENFILENAMEA stOFNFluidSynth = {
		sizeof(OPENFILENAMEA), hwndDlg, NULL,
		"SoundFont2 Files (*.sf2)\0*.sf2\0",
		NULL, NULL, NULL,
		szFluidSynthSettingPath, MAX_PATH - 1,
		NULL, NULL, NULL,
		"Select a FluidSynth SoundFont",
	};

	switch (message) {
	case WM_INITDIALOG:
		// Place ourselves in the correct position
		hwndParent = GetParent(hwndDlg);
		SetWindowPos(hwndDlg, NULL, stSettingsDialogHeader.rcDisplay.left, stSettingsDialogHeader.rcDisplay.top,
			(stSettingsDialogHeader.rcDisplay.right - stSettingsDialogHeader.rcDisplay.left),
			(stSettingsDialogHeader.rcDisplay.bottom - stSettingsDialogHeader.rcDisplay.top),
			SWP_SHOWWINDOW);

		// Set up the music driver combo box
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "None");			// MUSIC_ENGINE_NONE
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "Windows MIDI");	// MUSIC_ENGINE_SEQUENCER
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "FluidSynth");		// MUSIC_ENGINE_FLUIDSYNTH
		ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), "MP3 Playback");	// MUSIC_ENGINE_MP3
		ComboBox_SetMinVisible(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), 4);

		// Create tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT),
			"Selects the music output driver. Uses Windows MIDI as a fallback option.\n\n"
			""
			"None: Disables music playback independent of the per-game music option.\n"
			"Windows MIDI: Uses the native Windows MIDI sequencer.\n"
			"FluidSynth: Uses the FluidSynth software synth, if available (default).\n"
			"MP3 Playback: Uses MP3 files for playback, if available.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT),
			"FluidSynth requires a soundfont for playback that contains the samples and synthesis data required to play back MIDI files. Any SoundFont 2 standard soundfont can be selected. "
			"By default, sc2kfix uses the General MIDI soundfont included with Windows that the MIDI sequencer uses.\n\n"
			""
			"Selecting a new soundfont will reset the music engine and restart the currently playing song.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE),
			"Opens a file browser to select a soundfont for FluidSynth. Not needed for Windows MIDI or MP3 playback drivers.");

		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_BKGDMUSIC),
			"By default, SimCity 2000 stops the currently-playing song when the game window loses focus. This setting continues playing music in the background until the end of the track, "
			"after which a new song will be selected when the game window regains focus.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC),
			"Enabling this setting will result in the next random music selection being played after the current song finishes.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_CHECK_SHUFFLE_MUSIC),
			"By default, SimCity 2000 selects \"random\" music by playing the next track in a looping playlist of songs. "
			"This setting controls whether or not to shuffle the playlist when the game starts and when the end of the playlist is reached.");

		// Set fields based on the working JSON
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MASTER), TBM_SETPOS, TRUE, (int)(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["master_volume"].ToFloat() * 10));
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MASTER), TBM_SETTICFREQ, 1, 0);
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MASTER), TBM_SETRANGE, TRUE, MAKELONG(0, 10));
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MUSIC), TBM_SETPOS, TRUE, (int)(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_volume"].ToFloat() * 10));
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MUSIC), TBM_SETTICFREQ, 1, 0);
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MUSIC), TBM_SETRANGE, TRUE, MAKELONG(0, 10));
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_SOUNDS), TBM_SETPOS, TRUE, (int)(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["sound_volume"].ToFloat() * 10));
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_SOUNDS), TBM_SETTICFREQ, 1, 0);
		SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_SOUNDS), TBM_SETRANGE, TRUE, MAKELONG(0, 10));
		InvalidateRect(hwndDlg, NULL, TRUE);	// Force a redraw so the sliders show up. Friggin' WinAPI.

		ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT), MusicEngineStringToInt(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"].ToString().c_str()));
		SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, GetFileBaseName(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"].ToString().c_str()));

		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"], IDC_SETTINGS_CHECK_BKGDMUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
		SET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

		return TRUE;

	case WM_DESTROY:
		// Update the working JSON based on our fields
		jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["master_volume"] = 0.1f * SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MASTER), TBM_GETPOS, 0, 0);
		jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_volume"] = 0.1f * SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_MUSIC), TBM_GETPOS, 0, 0);
		jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["sound_volume"] = 0.1f * SendMessage(GetDlgItem(hwndDlg, IDC_SLIDER_VOLUME_SOUNDS), TBM_GETPOS, 0, 0);

		jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"] = MusicEngineIntToString(ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SETTINGS_COMBO_MUSICOUTPUT)));

		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"], IDC_SETTINGS_CHECK_BKGDMUSIC);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"], IDC_SETTINGS_CHECK_SHUFFLE_MUSIC);
		GET_CHECKBOX(jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"], IDC_SETTINGS_CHECK_ALWAYSPLAYMUSIC);

		return TRUE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_SETTINGS_BUTTON_SOUNDFONTBROWSE:
			if (GetOpenFileName(&stOFNFluidSynth)) {
				if (mus_debug & 8)
					ConsoleLog(LOG_DEBUG, "CORE: SoundFont setting changed; new soundfont is %s\n", szFluidSynthSettingPath);

				jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"] = szFluidSynthSettingPath;
				SetDlgItemText(hwndDlg, IDC_SETTINGS_FLUIDSYNTH_SOUNDFONT, GetFileBaseName(szFluidSynthSettingPath));
			}
			break;
		case IDC_SETTINGS_BUTTON_CONFMIDTRACKS:
			return DoConfigureMusicTracks(stSettingsDialogHeader.stSettingsChanges, hwndDlg, FALSE);
		case IDC_SETTINGS_BUTTON_CONFMP3TRACKS:
			return DoConfigureMusicTracks(stSettingsDialogHeader.stSettingsChanges, hwndDlg, TRUE);
		}
		return TRUE;
	}

	return FALSE;
}

// Template for future tabs
static BOOL CALLBACK TabDlg(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hwndParent;

	switch (message) {
	case WM_INITDIALOG:
		// Place ourselves in the correct position
		hwndParent = GetParent(hwndDlg);
		SetWindowPos(hwndDlg, NULL, stSettingsDialogHeader.rcDisplay.left, stSettingsDialogHeader.rcDisplay.top,
			(stSettingsDialogHeader.rcDisplay.right - stSettingsDialogHeader.rcDisplay.left),
			(stSettingsDialogHeader.rcDisplay.bottom - stSettingsDialogHeader.rcDisplay.top),
			SWP_SHOWWINDOW);

		// Create tooltips
		
		// Set fields based on the working JSON

		return TRUE;

	case WM_DESTROY:
		// Update the working JSON based on our fields

		return TRUE;

	case WM_COMMAND:
		// switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		// default:
		// 	break;
		// }
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK SettingsDialogContainerProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	std::string strVersionInfo;

	switch (message) {
	case WM_INITDIALOG:
		// Set the dialog box icon
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hSC2KFixModule, MAKEINTRESOURCE(IDI_TOPSECRET)));

		// Create tooltips
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDOK),
			"Saves the currently selected settings and closes the settings dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL),
			"Discards changed settings and closes the settings dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_DEFAULTS),
			"Changes the settings to the default sc2kfix experience but does not save settings or close the dialog.");
		CreateTooltip(hwndDlg, GetDlgItem(hwndDlg, IDC_SETTINGS_BUTTON_VANILLA),
			"Changes the settings to disable all quality of life, interface and gameplay enhancements but does not save settings or close the dialog.");

		// Set the version string.
		strVersionInfo = "sc2kfix ";
		strVersionInfo += szSC2KFixVersion;
		strVersionInfo += " (";
		strVersionInfo += szSC2KFixReleaseTag;
		strVersionInfo += ")";
		SetDlgItemText(hwndDlg, IDC_SETTINGS_STATIC_VERSIONINFO, strVersionInfo.c_str());

		// Create the tab setup
		TCITEM tie;
		RECT rcTab;
		memset(&stSettingsDialogHeader, 0, sizeof(stSettingsDialogHeader));
		stSettingsDialogHeader.hwndTab = GetDlgItem(hwndDlg, IDC_SETTINGS_TAB_CONTAINER);

		// Add tabs to the tab control
		memset(&tie, 0, sizeof(TCITEM));
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;
		tie.pszText = (char*)"General";
		TabCtrl_InsertItem(stSettingsDialogHeader.hwndTab, 0, &tie);
		tie.pszText = (char*)"Gameplay";
		TabCtrl_InsertItem(stSettingsDialogHeader.hwndTab, 1, &tie);
		tie.pszText = (char*)"Audio";
		TabCtrl_InsertItem(stSettingsDialogHeader.hwndTab, 2, &tie);

		// Assign the resources and procedures for the three child tab dialogs
		stSettingsDialogHeader.pDlgResource[0] = SettingsGetDialogResource(MAKEINTRESOURCE(IDD_SETTINGS_GENERAL));
		stSettingsDialogHeader.pDlgResource[1] = SettingsGetDialogResource(MAKEINTRESOURCE(IDD_SETTINGS_GAMEPLAY));
		stSettingsDialogHeader.pDlgResource[2] = SettingsGetDialogResource(MAKEINTRESOURCE(IDD_SETTINGS_AUDIO));
		stSettingsDialogHeader.pDlgProc[0] = SettingsDialogGeneralTabProc;
		stSettingsDialogHeader.pDlgProc[1] = SettingsDialogGameplayTabProc;
		stSettingsDialogHeader.pDlgProc[2] = SettingsDialogAudioTabProc;
		stSettingsDialogHeader.stSettingsChanges = (settings_t*)lParam;

		// Compute the sizing for the tab contents
		//CenterDialogBox(hwndDlg);
		SetRectEmpty(&rcTab);
		GetWindowRect(stSettingsDialogHeader.hwndTab, &rcTab);
		TabCtrl_AdjustRect(stSettingsDialogHeader.hwndTab, TRUE, &rcTab);
		OffsetRect(&rcTab, 8, -16);
		rcTab.right -= 20;
		rcTab.bottom -= 64;
		CopyRect(&stSettingsDialogHeader.rcDisplay, &rcTab);

		// Center the dialog box and select tab 0
		SettingsTabSelectionChanged(hwndDlg);
		//return TRUE;
		break;

	// Close without saving if the dialog is closed via the menu bar close button or Alt+F4
	case WM_CLOSE:
		EndDialog(hwndDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			EndDialog(hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		case IDC_SETTINGS_BUTTON_DEFAULTS:
			// Set all the checkboxes to the defaults.
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"] = false;

			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"] = "fluidsynth";
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"] = "C:\\Windows\\System32\\drivers\\gm.dls";
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["master_volume"] = 1.0;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_volume"] = 1.0;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["sound_volume"] = 1.0;

			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_underground"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_floating_status"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"] = true;

			for (int i = 10000; i <= 10019; i++) {
				jsonSettingsCoreWorkingCopy["sc2kfix"]["music_midi"][std::to_string(i)] = "";
				jsonSettingsCoreWorkingCopy["sc2kfix"]["music_mp3"][std::to_string(i)] = "";
			}

			// Refresh the tab we're on
			SettingsTabSelectionChanged(hwndDlg);
			break;
		case IDC_SETTINGS_BUTTON_VANILLA:
			// Clear all checkboxes except for the update checker.
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["force_console"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["check_for_updates"] = true;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["core"]["skip_mods"] = false;

			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_in_background"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["use_sound_replacements"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["shuffle_music"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"] = "fluidsynth";
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"] = "C:\\Windows\\System32\\drivers\\gm.dls";
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["always_play_music"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["master_volume"] = 1.0;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_volume"] = 1.0;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["sound_volume"] = 1.0;

			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["frequent_updates"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["dark_underground"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["skip_intro"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_new_strings"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["use_floating_status"] = false;
			jsonSettingsCoreWorkingCopy["sc2kfix"]["qol"]["title_calendar"] = false;

			for (int i = 10000; i <= 10019; i++) {
				jsonSettingsCoreWorkingCopy["sc2kfix"]["music_midi"][std::to_string(i)] = "";
				jsonSettingsCoreWorkingCopy["sc2kfix"]["music_mp3"][std::to_string(i)] = "";
			}

			// Refresh the tab we're on
			SettingsTabSelectionChanged(hwndDlg);
			break;
		}
		return TRUE;

	// Handle the tab selection
	case WM_NOTIFY:
		if (((NMHDR*)lParam)->code == TCN_SELCHANGE) {
			SettingsTabSelectionChanged(hwndDlg);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

void ShowNewSettingsDialog(void) {
	settings_t stSettingsChanges;
	memset(&stSettingsChanges, 0, sizeof(settings_t));

	// Copy the active settings structure into the working copy
	jsonSettingsCoreWorkingCopy = jsonSettingsCore;

	// Save the original settings here just prior to saving.
	std::string strOriginalMusicDriver = jsonSettingsCore["sc2kfix"]["audio"]["music_driver"].ToString();
	std::string strOriginalSoundfont = jsonSettingsCore["sc2kfix"]["audio"]["soundfont"].ToString();

	InitializeTempBindings();

	ToggleFloatingStatusDialog(FALSE);

	if (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_SETTINGS_CONTAINER), GameGetRootWindowHandle(), SettingsDialogContainerProc, (LPARAM)&stSettingsChanges) == TRUE) {
		// Copy back all our settings into the active structure
		jsonSettingsCore = jsonSettingsCoreWorkingCopy;

		// Update any keybindings if needed
		if (stSettingsChanges.bKeyBindingsChanged) {
			UpdateKeyBindings();
			SaveJSONBindings(jsonSettingsCore);
		}

		// Save the settings JSON file and update hooks
		SaveJSONSettings();

		// See if we need to reset the music engine.
		if (dwMusicThreadID && (strOriginalMusicDriver != jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["music_driver"].ToString() ||
			strOriginalSoundfont != jsonSettingsCoreWorkingCopy["sc2kfix"]["audio"]["soundfont"].ToString()) ||
			(stSettingsChanges.bActiveMusicDriverTouched && stSettingsChanges.bActiveTrackChanged))
			PostThreadMessage(dwMusicThreadID, WM_MUSIC_RESET, NULL, NULL);
	}

	ToggleFloatingStatusDialog(TRUE);
}