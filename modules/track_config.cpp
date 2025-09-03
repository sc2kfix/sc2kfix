// sc2kfix modules/track_config.cpp: Track listing and configuration dialogues.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <vector>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define SONGID_STR_LEN 16

#define ListView_InsertItemType(hwndLV, i, tmask) \
{ LV_ITEM _macro_lvi;\
  _macro_lvi.mask = (tmask);\
  _macro_lvi.iItem = (i);\
  _macro_lvi.iSubItem = 0;\
  _macro_lvi.pszText = "";\
  SNDMSG((hwndLV), LVM_INSERTITEM, (WPARAM)(i), (LPARAM)(LV_ITEM *)&_macro_lvi);\
}

#define ListView_InsertItemText(hwndLV, i) ListView_InsertItemType(hwndLV, i, LVIF_TEXT)

#define ListView_InsertColumnEntry(hwndLV, i, text, width, tmask, cfmt) \
{ LV_COLUMN _macro_lvc;\
  _macro_lvc.mask = (tmask);\
  _macro_lvc.fmt = (cfmt);\
  _macro_lvc.iSubItem = (i);\
  _macro_lvc.pszText = (text);\
  _macro_lvc.cx = (width);\
  (int)SNDMSG((hwndLV), LVM_INSERTCOLUMN, (WPARAM)(i), (LPARAM)(LV_COLUMN *)&_macro_lvc);\
}

typedef struct {
	BOOL bMP3;
	int iSongID;
	char szTrackEntry[MAX_PATH + 1];
} trackentry_t;

typedef struct {
	BOOL bMP3;
	char szMusicTracks[MUSIC_TRACKS][MAX_PATH + 1];
} conftracks_t;

BOOL CALLBACK EditMusicTrackDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	const char *pDlgStaticText, *pTrackDirectory;
	char szWndTitle[64+1], szTemp[MAX_PATH + 1];
	HWND hDlgCombo;
	LONG bfdHandle;
	struct _finddata_t	fdi;
	int iCurSel;
	trackentry_t *te;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		te = (trackentry_t *)lParam;

		if (te->bMP3)
			pDlgStaticText = "Select MP3 Music Track";
		else
			pDlgStaticText = "Select MIDI Music Track";

		sprintf_s(szWndTitle, sizeof(szWndTitle), "Edit %s Track - Song ID: %d", ((te->bMP3) ? "MP3" : "MIDI"), te->iSongID);

		SetWindowText(hwndDlg, szWndTitle);
		SetWindowText(GetDlgItem(hwndDlg, IDC_EDITMUSICTRACK_STATIC), pDlgStaticText);

		hDlgCombo = GetDlgItem(hwndDlg, IDC_EDITMUSIC_TRACK_COMBO);
		SetWindowRedraw(hDlgCombo, FALSE);

		ComboBox_ResetContent(hDlgCombo);
		ComboBox_AddString(hDlgCombo, "(Song ID Default)");

		pTrackDirectory = GetGameSoundPath();
		if (pTrackDirectory) {
			sprintf_s(szTemp, sizeof(szTemp), "%s%s", pTrackDirectory, (te->bMP3) ? "*.mp3" : "*.mid");
			bfdHandle = _findfirst(szTemp, &fdi);
			if (bfdHandle != -1L) {
				do
					ComboBox_AddString(hDlgCombo, fdi.name);
				while (_findnext(bfdHandle, &fdi) != -1);
				_findclose(bfdHandle);
			}
		}

		SetWindowRedraw(hDlgCombo, TRUE);
		InvalidateRect(hDlgCombo, NULL, TRUE);

		iCurSel = ComboBox_FindString(hDlgCombo, -1, te->szTrackEntry);
		if (iCurSel < 0)
			iCurSel = 0;

		ComboBox_SetCurSel(hDlgCombo, iCurSel);
		return TRUE;

	case WM_COMMAND:
		te = (trackentry_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgCombo = GetDlgItem(hwndDlg, IDC_EDITMUSIC_TRACK_COMBO);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_EDITMUSIC_TRACK_COMBO:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				memset(szTemp, 0, sizeof(szTemp));
				iCurSel = ComboBox_GetCurSel(hDlgCombo);
				if (iCurSel > 0)
					ComboBox_GetText(hDlgCombo, szTemp, countof(szTemp) - 1);
				strcpy_s(te->szTrackEntry, sizeof(te->szTrackEntry), szTemp);
				return TRUE;
			}
			break;

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

static void DoEditMusicTrack(conftracks_t *cft, HWND hwndDlg, HWND hDlgListView, int iItem, const char *pSongID) {
	trackentry_t te;

	memset(&te, 0, sizeof(te));
	te.bMP3 = cft->bMP3;
	te.iSongID = atoi(pSongID);
	strcpy_s(te.szTrackEntry, sizeof(te.szTrackEntry), cft->szMusicTracks[iItem]);

	if (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_EDITMUSICTRACK), hwndDlg, EditMusicTrackDialogProc, (LPARAM)&te) == TRUE) {

		strcpy_s(cft->szMusicTracks[iItem], sizeof(cft->szMusicTracks[iItem]), te.szTrackEntry);
		ListView_SetItemText(hDlgListView, iItem, 1, cft->szMusicTracks[iItem]);
	}
}

static void InsertTrackListViewRow(HWND hDlgListView, int iRow, int iSongID, const char *pTrackEntry) {
	char szSongID[SONGID_STR_LEN + 1];

	memset(szSongID, 0, sizeof(szSongID));

	sprintf_s(szSongID, sizeof(szSongID), "%d", iSongID);
	ListView_InsertItemText(hDlgListView, iRow);
	ListView_SetItemText(hDlgListView, iRow, 0, szSongID);
	ListView_SetItemText(hDlgListView, iRow, 1, (char *)pTrackEntry);
}

static void EditMusicTrack(conftracks_t *cft, HWND hwndDlg, HWND hDlgListView, int iItem) {
	char szSongID[SONGID_STR_LEN + 1];

	if (iItem < 0)
		return;

	ListView_GetItemText(hDlgListView, iItem, 0, szSongID, countof(szSongID)-1);

	DoEditMusicTrack(cft, hwndDlg, hDlgListView, iItem, szSongID);
}

BOOL CALLBACK ConfMusicTracksDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int iSongID;
	const char *pDlgTitle, *pDlgStaticText;
	HWND hDlgListView;
	unsigned nColumnMask;
	unsigned nColumnFmt;
	int iRow;
	conftracks_t *cft;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		cft = (conftracks_t *)lParam;

		if (cft->bMP3) {
			pDlgTitle = "Configure MP3 Music Tracks";
			pDlgStaticText = "Empty track entries will use the named MP3 track matching the Song ID if present, or the game's default MIDI.";
		}
		else {
			pDlgTitle = "Configure MIDI Music Tracks";
			pDlgStaticText = "Empty track entries will use the game's default.";
		}
		SetWindowText(hwndDlg, pDlgTitle);
		SetWindowText(GetDlgItem(hwndDlg, IDC_CONFMUSICTRACKS_STATIC), pDlgStaticText);

		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFMUSICTRACKS_LIST);
		ListView_SetExtendedListViewStyle(hDlgListView, LVS_EX_FULLROWSELECT);

		nColumnMask = (LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM);
		nColumnFmt = LVCFMT_LEFT;

		ListView_InsertColumnEntry(hDlgListView, 0, "Song ID", 100, nColumnMask, nColumnFmt);
		ListView_InsertColumnEntry(hDlgListView, 1, "Track Entry", 250, nColumnMask, nColumnFmt);

		for (int i = 0; i < MUSIC_TRACKS; i++) {
			iSongID = 10000 + i;
			InsertTrackListViewRow(hDlgListView, i, iSongID, cft->szMusicTracks[i]);
		}

		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		cft = (conftracks_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFMUSICTRACKS_LIST);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_CONFMUSICTRACKS_EDITENTRY:
			iRow = ListView_GetNextItem(hDlgListView, -1, LVNI_ALL | LVNI_SELECTED);

			EditMusicTrack(cft, hwndDlg, hDlgListView, iRow);
			break;
		case IDOK:
			EndDialog(hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		}
		return TRUE;

	case WM_NOTIFY:
		cft = (conftracks_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFMUSICTRACKS_LIST);
		switch (((LPNMHDR)lParam)->code) {
		case NM_DBLCLK:
			if (((LPNMHDR)lParam)->hwndFrom == hDlgListView) {
				NMLISTVIEW *lV = (NMLISTVIEW *)lParam;
				
				EditMusicTrack(cft, hwndDlg, hDlgListView, lV->iItem);
				return TRUE;
			}
			break;
		}
		break;
	}
	return FALSE;
}

BOOL DoConfigureMusicTracks(HWND hwndDlg, BOOL bMP3) {
	conftracks_t cft;
	BOOL bRet;
	char *pMusicTrack;

	memset(&cft, 0, sizeof(conftracks_t));
	cft.bMP3 = bMP3;
	for (int i = 0; i < MUSIC_TRACKS; i++) {
		pMusicTrack = (bMP3) ? szSettingsMP3TrackPath[i] : szSettingsMIDITrackPath[i];
		strcpy_s(cft.szMusicTracks[i], MAX_PATH, pMusicTrack);
	}

	bRet = DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_CONFMUSICTRACKS), hwndDlg, ConfMusicTracksDialogProc, (LPARAM)&cft);
	if (bRet == TRUE) {
		for (int i = 0; i < MUSIC_TRACKS; i++) {
			pMusicTrack = (bMP3) ? szSettingsMP3TrackPath[i] : szSettingsMIDITrackPath[i];
			strcpy_s(pMusicTrack, MAX_PATH, cft.szMusicTracks[i]);
		}
	}

	return bRet;
}
