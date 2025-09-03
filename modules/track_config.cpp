// sc2kfix modules/track_config.cpp: Track listing and configuration dialogues.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

BOOL CALLBACK ConfMusicTracksDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	const char *pDlgTitle;

	switch (message) {
	case WM_INITDIALOG:
		if (lParam)
			pDlgTitle = "Configure MP3 Music Tracks";
		else
			pDlgTitle = "Configure MIDI Music Tracks";
		SetWindowTextA(hwndDlg, pDlgTitle);

		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:
			EndDialog(hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL DoConfigureMusicTracks(HWND hDlg, BOOL bMP3) {
	
	if (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_CONFMUSICTRACKS), hDlg, ConfMusicTracksDialogProc, (LPARAM)bMP3) == TRUE) {
		ConsoleLog(LOG_DEBUG, "DoConfigureMusicTracks(0x%06X, %d) - EndDialog result: TRUE\n", hDlg, bMP3);
	}

	return TRUE;
}
