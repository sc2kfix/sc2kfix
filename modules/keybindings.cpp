// sc2kfix modules/keybindings.cpp: Key to Action bindings and configuration dialogues.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#undef UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <list>
#include <map>
#include <string>

#include <sc2kfix.h>
#include "../resource.h"

#define KEYBINDS_DEBUG_OTHER 1

#define KEYBINDS_DEBUG DEBUG_FLAGS_NONE

#ifdef DEBUGALL
#undef KEYBINDS_DEBUG
#define KEYBINDS_DEBUG DEBUG_FLAGS_EVERYTHING
#endif

UINT keybinds_debug = KEYBINDS_DEBUG;

// NOTE: To avoid headaches, make sure the order matches that
// of its enum.
static bkey_t progkeys[B_KEY_COUNT] = {
	{ B_KEY_INVALID,         0x00,          TRUE,  B_TYPE_GENERAL, "<None>"            },

	{ B_KEY_ESCAPE,          VK_ESCAPE,     TRUE,  B_TYPE_VIRTKEY, "Esc"               },
																					   
	{ B_KEY_PAGEUP,          VK_PRIOR,      FALSE, B_TYPE_VIRTKEY, "PageUp"            },
	{ B_KEY_PAGEDOWN,        VK_NEXT,       FALSE, B_TYPE_VIRTKEY, "PageDown"          },
	{ B_KEY_HOME,            VK_HOME,       FALSE, B_TYPE_VIRTKEY, "Home"              },
	{ B_KEY_END,             VK_END,        FALSE, B_TYPE_VIRTKEY, "End"               },
																					   
	{ B_KEY_CURSORLEFT,      VK_LEFT,       FALSE, B_TYPE_VIRTKEY, "CursorLeft"        },
	{ B_KEY_CURSORUP,        VK_UP,         FALSE, B_TYPE_VIRTKEY, "CursorUp"          },
	{ B_KEY_CURSORRIGHT,     VK_RIGHT,      FALSE, B_TYPE_VIRTKEY, "CursorRight"       },
	{ B_KEY_CURSORDOWN,      VK_DOWN,       FALSE, B_TYPE_VIRTKEY, "CursorDown"        },
																					   
	{ B_KEY_INSERT,          VK_INSERT,     FALSE, B_TYPE_VIRTKEY, "Insert"            },
	{ B_KEY_DELETE,          VK_DELETE,     FALSE, B_TYPE_VIRTKEY, "Delete"            },
																					   
	{ B_KEY_0,               0x30,          FALSE, B_TYPE_VIRTKEY, "0"                 },
	{ B_KEY_1,               0x31,          FALSE, B_TYPE_VIRTKEY, "1"                 },
	{ B_KEY_2,               0x32,          FALSE, B_TYPE_VIRTKEY, "2"                 },
	{ B_KEY_3,               0x33,          FALSE, B_TYPE_VIRTKEY, "3"                 },
	{ B_KEY_4,               0x34,          FALSE, B_TYPE_VIRTKEY, "4"                 },
	{ B_KEY_5,               0x35,          FALSE, B_TYPE_VIRTKEY, "5"                 },
	{ B_KEY_6,               0x36,          FALSE, B_TYPE_VIRTKEY, "6"                 },
	{ B_KEY_7,               0x37,          FALSE, B_TYPE_VIRTKEY, "7"                 },
	{ B_KEY_8,               0x38,          FALSE, B_TYPE_VIRTKEY, "8"                 },
	{ B_KEY_9,               0x39,          FALSE, B_TYPE_VIRTKEY, "9"                 },
																					   
	{ B_KEY_A,               0x41,          FALSE, B_TYPE_VIRTKEY, "A"                 },
	{ B_KEY_B,               0x42,          FALSE, B_TYPE_VIRTKEY, "B"                 },
	{ B_KEY_C,               0x43,          FALSE, B_TYPE_VIRTKEY, "C"                 },
	{ B_KEY_D,               0x44,          FALSE, B_TYPE_VIRTKEY, "D"                 },
	{ B_KEY_E,               0x45,          FALSE, B_TYPE_VIRTKEY, "E"                 },
	{ B_KEY_F,               0x46,          FALSE, B_TYPE_VIRTKEY, "F"                 },
	{ B_KEY_G,               0x47,          FALSE, B_TYPE_VIRTKEY, "G"                 },
	{ B_KEY_H,               0x48,          FALSE, B_TYPE_VIRTKEY, "H"                 },
	{ B_KEY_I,               0x49,          FALSE, B_TYPE_VIRTKEY, "I"                 },
	{ B_KEY_J,               0x4A,          FALSE, B_TYPE_VIRTKEY, "J"                 },
	{ B_KEY_K,               0x4B,          FALSE, B_TYPE_VIRTKEY, "K"                 },
	{ B_KEY_L,               0x4C,          FALSE, B_TYPE_VIRTKEY, "L"                 },
	{ B_KEY_M,               0x4D,          FALSE, B_TYPE_VIRTKEY, "M"                 },
	{ B_KEY_N,               0x4E,          FALSE, B_TYPE_VIRTKEY, "N"                 },
	{ B_KEY_O,               0x4F,          FALSE, B_TYPE_VIRTKEY, "O"                 },
	{ B_KEY_P,               0x50,          FALSE, B_TYPE_VIRTKEY, "P"                 },
	{ B_KEY_Q,               0x51,          FALSE, B_TYPE_VIRTKEY, "Q"                 },
	{ B_KEY_R,               0x52,          FALSE, B_TYPE_VIRTKEY, "R"                 },
	{ B_KEY_S,               0x53,          FALSE, B_TYPE_VIRTKEY, "S"                 },
	{ B_KEY_T,               0x54,          FALSE, B_TYPE_VIRTKEY, "T"                 },
	{ B_KEY_U,               0x55,          FALSE, B_TYPE_VIRTKEY, "U"                 },
	{ B_KEY_V,               0x56,          FALSE, B_TYPE_VIRTKEY, "V"                 },
	{ B_KEY_W,               0x57,          FALSE, B_TYPE_VIRTKEY, "W"                 },
	{ B_KEY_X,               0x58,          FALSE, B_TYPE_VIRTKEY, "X"                 },
	{ B_KEY_Y,               0x59,          FALSE, B_TYPE_VIRTKEY, "Y"                 },
	{ B_KEY_Z,               0x5A,          FALSE, B_TYPE_VIRTKEY, "Z"                 },
																					   
	{ B_KEY_NUMPAD0,         VK_NUMPAD0,    FALSE, B_TYPE_VIRTKEY, "Numpad0"           },
	{ B_KEY_NUMPAD1,         VK_NUMPAD1,    FALSE, B_TYPE_VIRTKEY, "Numpad1"           },
	{ B_KEY_NUMPAD2,         VK_NUMPAD2,    FALSE, B_TYPE_VIRTKEY, "Numpad2"           },
	{ B_KEY_NUMPAD3,         VK_NUMPAD3,    FALSE, B_TYPE_VIRTKEY, "Numpad3"           },
	{ B_KEY_NUMPAD4,         VK_NUMPAD4,    FALSE, B_TYPE_VIRTKEY, "Numpad4"           },
	{ B_KEY_NUMPAD5,         VK_NUMPAD5,    FALSE, B_TYPE_VIRTKEY, "Numpad5"           },
	{ B_KEY_NUMPAD6,         VK_NUMPAD6,    FALSE, B_TYPE_VIRTKEY, "Numpad6"           },
	{ B_KEY_NUMPAD7,         VK_NUMPAD7,    FALSE, B_TYPE_VIRTKEY, "Numpad7"           },
	{ B_KEY_NUMPAD8,         VK_NUMPAD8,    FALSE, B_TYPE_VIRTKEY, "Numpad8"           },
	{ B_KEY_NUMPAD9,         VK_NUMPAD9,    FALSE, B_TYPE_VIRTKEY, "Numpad9"           },
	{ B_KEY_NPMULT,          VK_MULTIPLY,   FALSE, B_TYPE_VIRTKEY, "NumpadMultiply"    },
	{ B_KEY_NPADD,           VK_ADD,        FALSE, B_TYPE_VIRTKEY, "NumpadAdd"         },
	{ B_KEY_NPSEP,           VK_SEPARATOR,  FALSE, B_TYPE_VIRTKEY, "NumpadSeparator"   },
	{ B_KEY_NPSUB,           VK_SUBTRACT,   FALSE, B_TYPE_VIRTKEY, "NumpadSubtract"    },
	{ B_KEY_NPDECIMAL,       VK_DECIMAL,    FALSE, B_TYPE_VIRTKEY, "NumpadDecimal"     },
	{ B_KEY_NPDIV,           VK_DIVIDE,     FALSE, B_TYPE_VIRTKEY, "NumpadDivide"      },
																					   
	{ B_KEY_COLON,           VK_OEM_1,      FALSE, B_TYPE_VIRTKEY, "Colon"             },
	{ B_KEY_PLUS,            VK_OEM_PLUS,   FALSE, B_TYPE_VIRTKEY, "Plus"              },
	{ B_KEY_COMMA,           VK_OEM_COMMA,  FALSE, B_TYPE_VIRTKEY, "Comma"             },
	{ B_KEY_MINUS,           VK_OEM_MINUS,  FALSE, B_TYPE_VIRTKEY, "Minus"             },
	{ B_KEY_PERIOD,          VK_OEM_PERIOD, FALSE, B_TYPE_VIRTKEY, "Period"            },
	{ B_KEY_FSLASH,          VK_OEM_2,      FALSE, B_TYPE_VIRTKEY, "ForwardSlash"      },
	{ B_KEY_TILDE,           VK_OEM_3,      FALSE, B_TYPE_VIRTKEY, "Tilde"             },
																					   
	{ B_KEY_LBRACE,          VK_OEM_4,      FALSE, B_TYPE_VIRTKEY, "LeftBrace"         },
	{ B_KEY_BSLASH,          VK_OEM_5,      FALSE, B_TYPE_VIRTKEY, "BackSlash"         },
	{ B_KEY_RBRACE,          VK_OEM_6,      FALSE, B_TYPE_VIRTKEY, "RightBrace"        },
	{ B_KEY_APOSTR,          VK_OEM_7,      FALSE, B_TYPE_VIRTKEY, "Apostrophe"        },

	{ B_KEY_MOUSE_MBUTTON,   0x00,          FALSE, B_TYPE_MOUSEBT, "MouseMiddleButton" },
	{ B_KEY_MOUSE_RBUTTON,   0x00,          FALSE, B_TYPE_MOUSEBT, "MouseRightButton"  },

	{ B_KEY_MOUSE_WHEELUP,   0x00,          FALSE, B_TYPE_GENERAL, "MouseWheelUp"      },
	{ B_KEY_MOUSE_WHEELDOWN, 0x00,          FALSE, B_TYPE_GENERAL, "MouseWheelDown"    },
};

static bkey_t *GetKeyFromEntry(int nBkey) {
	for (int i = 0; i < B_KEY_COUNT; i++) {
		bkey_t *prog_key = &progkeys[i];
		if (prog_key) {
			if (prog_key->nBkey == nBkey)
				return prog_key;
		}
	}
	return NULL;
}

static bkey_t *GetKeyFromName(const char *pKeyName) {
	for (int i = 0; i < B_KEY_COUNT; i++) {
		bkey_t *prog_key = &progkeys[i];
		if (prog_key) {
			if (_stricmp(prog_key->pKeyName, pKeyName) == 0)
				return prog_key;
		}
	}
	return NULL;
}

int CharToKey(UINT nChar) {
	for (int i = 0; i < B_KEY_COUNT; i++) {
		bkey_t *key_entry = &progkeys[i];
		if (key_entry && key_entry->iKeyType == B_TYPE_VIRTKEY) {
			if (key_entry->uVirtKey == nChar) {
				//ConsoleLog(LOG_DEBUG, "CharToKey(0x%06X): (%d) (0x%06X) [%s]\n", nChar, key_entry->nBkey, key_entry->uVirtKey, key_entry->pKeyName);
				return key_entry->nBkey;
			}
		}
	}
	//ConsoleLog(LOG_DEBUG, "CharToKey(0x%06X): 0\n", nChar);
	return 0;
}

// NOTE: To avoid headaches, make sure the order matches that
// of its enum.
static baction_t prog_acts[BIND_ACTION_COUNT] = {
	{ BIND_ACTION_INVALID,             { B_KEY_INVALID },                 TRUE,  FALSE, "ActionInvalid"             },
	{ BIND_ACTION_VIEWESC,             { B_KEY_ESCAPE },                  TRUE,  FALSE, "ActionEscape"              },
	{ BIND_ACTION_SCROLLUP,            { B_KEY_CURSORUP },                FALSE, FALSE, "ActionScrollUp"            },
	{ BIND_ACTION_SCROLLDOWN,          { B_KEY_CURSORDOWN },              FALSE, FALSE, "ActionScrollDown"          },
	{ BIND_ACTION_SCROLLLEFT,          { B_KEY_CURSORLEFT },              FALSE, FALSE, "ActionScrollLeft"          },
	{ BIND_ACTION_SCROLLRIGHT,         { B_KEY_CURSORRIGHT },             FALSE, FALSE, "ActionScrollRight"         },
	{ BIND_ACTION_ZOOMIN,              { B_KEY_HOME, B_KEY_NUMPAD7 },     FALSE, FALSE, "ActionZoomIn"              },
	{ BIND_ACTION_ZOOMOUT,             { B_KEY_END, B_KEY_NUMPAD1 },      FALSE, FALSE, "ActionZoomOut"             },
	{ BIND_ACTION_ROTATECLOCKWISE,     { B_KEY_PAGEDOWN, B_KEY_NUMPAD3 }, FALSE, FALSE, "ActionRotateClockwise"     },
	{ BIND_ACTION_ROTATEANTICLOCKWISE, { B_KEY_DELETE, B_KEY_NPDECIMAL }, FALSE, FALSE, "ActionRotateAntiClockwise" },
	{ BIND_ACTION_CENTERTOOL,          { B_KEY_MOUSE_MBUTTON },           FALSE, TRUE,  "ActionCenterTool"          },
	{ BIND_ACTION_VIEWRIGHTCLICKMENU,  { B_KEY_MOUSE_RBUTTON },           FALSE, TRUE,  "ActionViewRightClickMenu"  }
};

static std::vector<baction_t> defBindings;

static baction_t *GetActionFromKey(baction_t *prog_act, int nBkey) {
	for (int nKeyPos = 0; nKeyPos < MAX_BOUND_KEYS; nKeyPos++) {
		if (IS_KEY_VALID(prog_act->nDefVirtKeys[nKeyPos])) {
			if (prog_act->nDefVirtKeys[nKeyPos] == nBkey)
				return prog_act;
		}
	}
	return NULL;
}

static const char *GetDefaultAction(int nBkey) {
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		baction_t *prog_act = GetActionFromKey(&prog_acts[i], nBkey);
		if (prog_act)
			return prog_act->pActname;
	}
	return "";
}

static const char *GetCurrentAction(std::vector<baction_t> &targBindings, int nBkey) {
	for (unsigned i = 0; i < targBindings.size(); i++) {
		baction_t *prog_act = GetActionFromKey(&targBindings[i], nBkey);
		if (prog_act && !prog_act->bImmutable)
			return prog_act->pActname;
	}
	return "";
}

static baction_t *GetActionFromName(std::vector<baction_t> &targBindings, const char *pActionName) {
	for (unsigned i = 0; i < targBindings.size(); i++) {
		baction_t *prog_act = &targBindings[i];
		if (prog_act) {
			if (_stricmp(prog_act->pActname, pActionName) == 0)
				return prog_act;
		}
	}
	return NULL;
}

void InitializeDefaultBindings() {
	defBindings.clear();
	for (int i = 0; i < BIND_ACTION_COUNT; i++) {
		defBindings.push_back(prog_acts[i]);
	}
}

static BOOL IsKeyAndActionValid(std::vector<baction_t> &targBindings, bkey_t *key_entry, const char *pDefAction, const char *pActionName) {
	baction_t *prog_act = GetActionFromName(targBindings, pActionName);
	if (prog_act) {
		if (!prog_act->bImmutable) {
			if (!prog_act->bMouseButtonOnly)
				return TRUE;
			else {
				if (key_entry->iKeyType == B_TYPE_MOUSEBT)
					return TRUE;
			}
		}
		if (keybinds_debug & KEYBINDS_DEBUG_OTHER) {
			ConsoleLog(LOG_DEBUG, "Invalid Key and Action Combination: Key[%s], Action[%s], Immutable(%c), MouseButtonOnly(%c)\n",
				key_entry->pKeyName, prog_act->pActname, ((prog_act->bImmutable) ? 'Y' : 'N'), ((prog_act->bMouseButtonOnly) ? 'Y' : 'N'));
		}
	}
	else
		return (strlen(pDefAction) > 0) ? TRUE : FALSE;
	return FALSE;
}

static void ShiftActionKeyArray(baction_t *prog_act) {
	for (int nKeyPos = 0; nKeyPos < MAX_BOUND_KEYS; nKeyPos++) {
		if (nKeyPos > 0) {
			if (IS_KEY_NOTSET(prog_act->nDefVirtKeys[nKeyPos - 1])) {
				if (IS_KEY_VALID(prog_act->nDefVirtKeys[nKeyPos])) {
					bkey_t *pBkey = GetKeyFromEntry(prog_act->nDefVirtKeys[nKeyPos]);
					if (pBkey) {
						if (keybinds_debug & KEYBINDS_DEBUG_OTHER)
							ConsoleLog(LOG_DEBUG, "Action '%s' Key '%s' pos adjusted %d -> %d\n", prog_act->pActname, pBkey->pKeyName, nKeyPos, nKeyPos-1);
						prog_act->nDefVirtKeys[nKeyPos - 1] = prog_act->nDefVirtKeys[nKeyPos];
						prog_act->nDefVirtKeys[nKeyPos] = B_KEY_INVALID;
					}
				}
			}
		}
	}
}

static BOOL CheckOrRemoveForAssignedKeyAndAction(std::vector<baction_t> &targBindings, bkey_t *key_entry, const char *pActionName) {
	BOOL bModified = FALSE;

	for (unsigned i = 0; i < targBindings.size(); i++) {
		baction_t *prog_act = &targBindings[i];
		if (prog_act && !prog_act->bImmutable) {
			if (_stricmp(prog_act->pActname, pActionName) != 0) {
				for (int nKeyPos = 0; nKeyPos < MAX_BOUND_KEYS; nKeyPos++) {
					if (IS_KEY_VALID(prog_act->nDefVirtKeys[nKeyPos])) {
						if (prog_act->nDefVirtKeys[nKeyPos] == key_entry->nBkey) {
							if (keybinds_debug & KEYBINDS_DEBUG_OTHER) {
								if (strlen(pActionName) > 0)
									ConsoleLog(LOG_DEBUG, "Key '%s' already assigned to Action '%s' - unsetting.\n", key_entry->pKeyName, prog_act->pActname);
								else
									ConsoleLog(LOG_DEBUG, "Key '%s' assignment removed from Action '%s'\n", key_entry->pKeyName, prog_act->pActname);
							}
							prog_act->nDefVirtKeys[nKeyPos] = B_KEY_INVALID;
							ShiftActionKeyArray(prog_act);
							bModified = TRUE;
						}
					}
				}
			}
		}
	}

	return bModified;
}

static BOOL SetKeyToAction(std::vector<baction_t> &targBindings, bkey_t *key_entry, const char *pActionName) {
	// With the following call it'll check to see whether there's a conflicting
	// key <-> action case, or if the action is empty.. it'll remove that
	// assignment from the key in question.
	BOOL bRet = CheckOrRemoveForAssignedKeyAndAction(targBindings, key_entry, pActionName);

	baction_t *prog_act = GetActionFromName(targBindings, pActionName);
	int nKeyFreePos = -1;

	if (prog_act && !prog_act->bImmutable) {
		for (int nKeyPos = 0; nKeyPos < MAX_BOUND_KEYS; nKeyPos++) {
			if (IS_KEY_NOTSET(prog_act->nDefVirtKeys[nKeyPos])) {
				if (nKeyFreePos == -1)
					nKeyFreePos = nKeyPos;
			}
			else {
				if (prog_act->nDefVirtKeys[nKeyPos] == key_entry->nBkey) {
					//ConsoleLog(LOG_DEBUG, "Key '%s' already assigned to action '%s' at key slot position %d\n", key_entry->pKeyName, prog_act->pActname, nKeyPos);
					return bRet;
				}
			}
		}
		if (nKeyFreePos >= 0) {
			prog_act->nDefVirtKeys[nKeyFreePos] = key_entry->nBkey;
			if (keybinds_debug & KEYBINDS_DEBUG_OTHER)
				ConsoleLog(LOG_DEBUG, "Key '%s' set to action '%s' at key slot position %d\n", key_entry->pKeyName, prog_act->pActname, nKeyFreePos);
			return TRUE;
		}
		else {
			if (keybinds_debug & KEYBINDS_DEBUG_OTHER)
				ConsoleLog(LOG_DEBUG, "Key '%s' couldn't be set to action '%s' - All key slots are full (%d).\n", key_entry->pKeyName, prog_act->pActname, MAX_BOUND_KEYS);
		}
	}
	return bRet;
}

void LoadStoredBindings() {
	const char *ini_file = GetIniPath();
	const char *section = "sc2kfix.Bindings";
	char szActionName[64 + 1];

	for (int i = 0; i < B_KEY_COUNT; i++) {
		memset(szActionName, 0, sizeof(szActionName));

		bkey_t *key_entry = &progkeys[i];
		if (key_entry && !key_entry->bImmutable) {
			const char *pDefAction = GetDefaultAction(key_entry->nBkey);
			GetPrivateProfileStringA(section, key_entry->pKeyName, pDefAction, szActionName, sizeof(szActionName)-1, ini_file);
			if (!IsKeyAndActionValid(defBindings, key_entry, pDefAction, szActionName))
				continue;
			SetKeyToAction(defBindings, key_entry, szActionName);
		}
	}
}

void SaveStoredBindings() {
	const char *ini_file = GetIniPath();
	const char *section = "sc2kfix.Bindings";

	for (int i = 0; i < B_KEY_COUNT; i++) {
		bkey_t *key_entry = &progkeys[i];
		if (key_entry && !key_entry->bImmutable) {
			const char *pCurrentAction = GetCurrentAction(defBindings, key_entry->nBkey);
			WritePrivateProfileStringA(section, key_entry->pKeyName, pCurrentAction, ini_file);
		}
	}
}

static int GetBinding(int nBkey) {
	for (unsigned i = 0; i < defBindings.size(); i++) {
		baction_t *prog_act = GetActionFromKey(&defBindings[i], nBkey);
		if (prog_act)
			return prog_act->nBaction;
	}
	return BIND_ACTION_INVALID;
}

// Config Area

static std::vector<baction_t> tempBindings;

typedef struct {
	HWND hWndParListView;
	int iItem;
	bkey_t *pKey;
	const char *pActionName;
	BOOL bButtonOnly;
} keybindentry_t;

typedef struct {
	BOOL bKeyBindingsChanged;
} keybinds_t;

void UpdateKeyBindings() {
	int nOldDefVirtKeys[MAX_BOUND_KEYS];

	for (unsigned i = 0; i < tempBindings.size(); i++) {
		baction_t *temp_prog_act = &tempBindings[i];
		baction_t *prog_act = &defBindings[i];
		if (temp_prog_act && !temp_prog_act->bImmutable && prog_act && !prog_act->bImmutable) {
			if (temp_prog_act->nBaction == prog_act->nBaction) {
				memcpy(nOldDefVirtKeys, prog_act->nDefVirtKeys, sizeof(nOldDefVirtKeys));
				memcpy(prog_act->nDefVirtKeys, temp_prog_act->nDefVirtKeys, sizeof(prog_act->nDefVirtKeys));
				if (keybinds_debug & KEYBINDS_DEBUG_OTHER) {
					for (int nKeyPos = 0; nKeyPos < MAX_BOUND_KEYS; nKeyPos++) {
						bkey_t *old_prog_key = GetKeyFromEntry(nOldDefVirtKeys[nKeyPos]);
						bkey_t *new_prog_key = GetKeyFromEntry(prog_act->nDefVirtKeys[nKeyPos]);
						if ((old_prog_key->nBkey != new_prog_key->nBkey) || (old_prog_key->nBkey != B_KEY_INVALID && new_prog_key->nBkey != B_KEY_INVALID))
							ConsoleLog(LOG_DEBUG, "Verify: '%s': Key (%d) '%s' %s '%s'\n", prog_act->pActname, nKeyPos, old_prog_key->pKeyName, ((_stricmp(old_prog_key->pKeyName, new_prog_key->pKeyName) == 0) ? "==" : "->"), new_prog_key->pKeyName);
					}
				}
			}
		}
	}
}

void InitializeTempBindings() {
	tempBindings.clear();
	for (unsigned i = 0; i < defBindings.size(); i++) {
		tempBindings.push_back(defBindings[i]);
	}
}

static BOOL ActionAvailableKeySlot(HWND hWndDlgListView, int iItem, const char *pActionName) {
	int nKeySlots = 0;
	char szTemp[64 + 1];

	if (strlen(pActionName) <= 0)
		return TRUE;

	for (int i = 0; i < ListView_GetItemCount(hWndDlgListView); i++) {
		memset(szTemp, 0, sizeof(szTemp));
		if (iItem != i) {
			ListView_GetItemText(hWndDlgListView, i, 1, szTemp, countof(szTemp) - 1);
			if (_stricmp(szTemp, pActionName) == 0)
				nKeySlots++;
		}
	}
	return (nKeySlots >= MAX_BOUND_KEYS) ? FALSE : TRUE;
}

static void UpdateTempKeyBindings(HWND hWndDlgListView) {
	char szKeyName[64 + 1], szActionName[64 + 1];

	for (int i = 0; i < ListView_GetItemCount(hWndDlgListView); i++) {
		memset(szKeyName, 0, sizeof(szKeyName));
		memset(szActionName, 0, sizeof(szActionName));

		ListView_GetItemText(hWndDlgListView, i, 0, szKeyName, countof(szKeyName) - 1);
		ListView_GetItemText(hWndDlgListView, i, 1, szActionName, countof(szActionName) - 1);

		bkey_t *pKey = GetKeyFromName(szKeyName);
		if (pKey && !pKey->bImmutable) {
			SetKeyToAction(tempBindings, pKey, szActionName);
		}
	}
}

//// Entry Selection

BOOL CALLBACK EditKeyBindingDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	const char *pDlgStaticText;
	char szWndTitle[64+1], szTemp[64 + 1];
	HWND hDlgCombo;
	int iCurSel;
	keybindentry_t *kbe;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		kbe = (keybindentry_t *)lParam;

		pDlgStaticText = "Select Action";

		sprintf_s(szWndTitle, sizeof(szWndTitle), "Edit '%s' Action", kbe->pKey->pKeyName);

		SetWindowText(hwndDlg, szWndTitle);
		SetWindowText(GetDlgItem(hwndDlg, IDC_EDITCONFIGENTRY_STATIC), pDlgStaticText);

		hDlgCombo = GetDlgItem(hwndDlg, IDC_EDITCONFIGENTRY_COMBO);
		SetWindowRedraw(hDlgCombo, FALSE);

		ComboBox_ResetContent(hDlgCombo);
		ComboBox_AddString(hDlgCombo, "<None>");

		for (int i = 0; i < BIND_ACTION_COUNT; i++) {
			baction_t *pAction = &prog_acts[i];
			if (pAction && !pAction->bImmutable) {
				if (kbe->bButtonOnly) {
					if (!pAction->bMouseButtonOnly)
						continue;
				}
				else {
					if (pAction->bMouseButtonOnly)
						continue;
				}
				if (strlen(pAction->pActname) > 0)
					ComboBox_AddString(hDlgCombo, pAction->pActname);
			}
		}

		SetWindowRedraw(hDlgCombo, TRUE);
		InvalidateRect(hDlgCombo, NULL, TRUE);

		iCurSel = ComboBox_FindString(hDlgCombo, -1, kbe->pActionName);
		if (iCurSel < 0)
			iCurSel = 0;

		ComboBox_SetCurSel(hDlgCombo, iCurSel);
		return TRUE;

	case WM_COMMAND:
		kbe = (keybindentry_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgCombo = GetDlgItem(hwndDlg, IDC_EDITCONFIGENTRY_COMBO);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_EDITCONFIGENTRY_COMBO:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
				memset(szTemp, 0, sizeof(szTemp));
				iCurSel = ComboBox_GetCurSel(hDlgCombo);
				if (iCurSel > 0) {
					ComboBox_GetText(hDlgCombo, szTemp, countof(szTemp) - 1);
					baction_t *pAction = GetActionFromName(tempBindings, szTemp);
					if (pAction && !pAction->bImmutable)
						kbe->pActionName = pAction->pActname;
					else
						kbe->pActionName = "";
				}
				else
					kbe->pActionName = "";
				return TRUE;
			}
			break;

		case IDOK:
			if (!ActionAvailableKeySlot(kbe->hWndParListView, kbe->iItem, kbe->pActionName)) {
				char szError[512 + 1];

				sprintf_s(szError, sizeof(szError) - 1, "Cannot set action '%s' on key '%s': All %d key slots are in use.", kbe->pActionName, kbe->pKey->pKeyName, MAX_BOUND_KEYS);
				MessageBox(hwndDlg, szError, "Assignment Error", MB_ICONERROR);
				break;
			}
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

static void DoEditKeyBinding(keybinds_t *kbs, HWND hwndDlg, HWND hDlgListView, int iItem, const char *pKeyName, const char *pActionName) {
	keybindentry_t kbe;
	bkey_t *prog_key;
	baction_t *prog_act;

	prog_key = GetKeyFromName(pKeyName);
	if (!prog_key)
		return;
	if (prog_key->bImmutable)
		return;
	prog_act = GetActionFromName(tempBindings, pActionName);

	memset(&kbe, 0, sizeof(keybindentry_t));
	kbe.hWndParListView = hDlgListView;
	kbe.iItem = iItem;
	kbe.pKey = prog_key;
	kbe.pActionName = (prog_act) ? prog_act->pActname : "";
	kbe.bButtonOnly = (prog_key->iKeyType == B_TYPE_MOUSEBT) ? TRUE : FALSE;

	if (DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_EDITCONFIGENTRY), hwndDlg, EditKeyBindingDialogProc, (LPARAM)&kbe) == TRUE) {
		if (_stricmp(pActionName, kbe.pActionName) != 0) {
			ListView_SetItemText(hDlgListView, iItem, 1, (char *)kbe.pActionName);
			if (!kbs->bKeyBindingsChanged)
				kbs->bKeyBindingsChanged = TRUE;
		}
	}
}

//// Overall

static WNDPROC OldListViewWndProc;

static void EditKeyBinding(keybinds_t *kbs, HWND hwndDlg, HWND hDlgListView, int iItem) {
	char szKeyName[64 + 1], szActionName[64 + 1];

	memset(szKeyName, 0, sizeof(szKeyName));
	memset(szActionName, 0, sizeof(szActionName));

	if (iItem < 0)
		return;

	ListView_GetItemText(hDlgListView, iItem, 0, szKeyName, countof(szKeyName)-1);
	ListView_GetItemText(hDlgListView, iItem, 1, szActionName, countof(szActionName)-1);

	DoEditKeyBinding(kbs, hwndDlg, hDlgListView, iItem, szKeyName, szActionName);
}

static void InsertKeyBindingViewRow(HWND hDlgListView, int iRow, const char *pKeyName, const char *pActionName) {
	ListView_InsertItemText(hDlgListView, iRow);
	ListView_SetItemText(hDlgListView, iRow, 0, (char *)pKeyName);
	ListView_SetItemText(hDlgListView, iRow, 1, (char *)pActionName);
}

static void PopulateKeyBindingList(HWND hDlgListView, BOOL bDefaults = FALSE) {
	int nIdx = 0;

	ListView_DeleteAllItems(hDlgListView);
	for (int i = 0; i < B_KEY_COUNT; i++) {
		bkey_t *prog_key = &progkeys[i];
		if (prog_key && !prog_key->bImmutable) {
			const char *pActionName = (bDefaults) ? GetDefaultAction(prog_key->nBkey) : GetCurrentAction(tempBindings, prog_key->nBkey);
			InsertKeyBindingViewRow(hDlgListView, nIdx, prog_key->pKeyName, pActionName);
			nIdx++;
		}
	}
}

LRESULT CALLBACK NewListViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_KEYDOWN:
		if (wParam != VK_BACK && wParam != VK_F12)
			break;
		HWND hWndParent;
		keybinds_t *kbs;

		hWndParent = GetParent(hWnd);
		kbs = (keybinds_t *)GetWindowLong(hWndParent, GWL_USERDATA);

		if (wParam == VK_BACK) {
			int iRow;
			char szKeyName[64 + 1];
			bkey_t *pKey;

			memset(szKeyName, 0, sizeof(szKeyName));

			iRow = ListView_GetNextItem(hWnd, -1, LVNI_ALL | LVNI_SELECTED);
			if (iRow >= 0) {
				ListView_GetItemText(hWnd, iRow, 0, szKeyName, countof(szKeyName)-1);
				pKey = GetKeyFromName(szKeyName);
				if (pKey && !pKey->bImmutable) {
					ListView_SetItemText(hWnd, iRow, 1, (char *)"");

					kbs->bKeyBindingsChanged = TRUE;
				}
			}
		}
		else if (wParam == VK_F12) {
			if (MessageBoxA(hWndParent, "WARNING: You are about to reset all key -> action binds back to their default state; are you quite sure that you want to proceed?", "Caution", MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
				PopulateKeyBindingList(hWnd, TRUE);

				kbs->bKeyBindingsChanged = TRUE;
			}
		}
		return FALSE;
	default:
		break;
	}
	return CallWindowProc(OldListViewWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK ConfKeyBindingsDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	int iRow;
	const char *pDlgTitle, *pDlgStaticText;
	HWND hDlgListView;
	unsigned nColumnMask;
	unsigned nColumnFmt;
	keybinds_t *kbs;

	switch (message) {
	case WM_INITDIALOG:
		SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
		kbs = (keybinds_t *)lParam;

		pDlgTitle = "Configure Key Bindings";
		pDlgStaticText = "To quickly unset a key, select an entry and press 'Backspace'\nTo reset all bindings back to their default state, click the list and press 'F12'";

		SetWindowText(hwndDlg, pDlgTitle);
		SetWindowText(GetDlgItem(hwndDlg, IDC_CONFIGSECTION_STATIC), pDlgStaticText);

		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFIGSECTION_LIST);

		OldListViewWndProc = SubclassWindow(hDlgListView, (WNDPROC)NewListViewWndProc);
		ListView_SetExtendedListViewStyle(hDlgListView, LVS_EX_FULLROWSELECT);

		nColumnMask = (LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM);
		nColumnFmt = LVCFMT_LEFT;

		ListView_InsertColumnEntry(hDlgListView, 0, "Key", 150, nColumnMask, nColumnFmt);
		ListView_InsertColumnEntry(hDlgListView, 1, "Action", 250, nColumnMask, nColumnFmt);

		PopulateKeyBindingList(hDlgListView, FALSE);

		CenterDialogBox(hwndDlg);
		return TRUE;

	case WM_COMMAND:
		kbs = (keybinds_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFIGSECTION_LIST);
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDC_CONFIGSECTION_EDITENTRY:
			iRow = ListView_GetNextItem(hDlgListView, -1, LVNI_ALL | LVNI_SELECTED);

			EditKeyBinding(kbs, hwndDlg, hDlgListView, iRow);
			break;

		case IDOK:
			if (kbs->bKeyBindingsChanged) {
				UpdateTempKeyBindings(hDlgListView);
			}
			EndDialog(hwndDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hwndDlg, FALSE);
			break;
		}
		return TRUE;

	case WM_NOTIFY:
		kbs = (keybinds_t *)GetWindowLong(hwndDlg, GWL_USERDATA);
		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFIGSECTION_LIST);
		switch (((LPNMHDR)lParam)->code) {
		case NM_DBLCLK:
			if (((LPNMHDR)lParam)->hwndFrom == hDlgListView) {
				NMLISTVIEW *lV = (NMLISTVIEW *)lParam;

				EditKeyBinding(kbs, hwndDlg, hDlgListView, lV->iItem);
				return TRUE;
			}
			break;
		}
		break;

	case WM_DESTROY:
		hDlgListView = GetDlgItem(hwndDlg, IDC_CONFIGSECTION_LIST);
		if (hDlgListView && OldListViewWndProc && (WNDPROC)GetWindowLong(hDlgListView, GWL_WNDPROC) == NewListViewWndProc)
			SubclassWindow(hDlgListView, OldListViewWndProc);
		break;
	}
	return FALSE;
}

BOOL DoConfigureKeyBindings(settings_t *st, HWND hwndDlg) {
	keybinds_t kbs;
	BOOL bRet;

	memset(&kbs, 0, sizeof(keybinds_t));
	kbs.bKeyBindingsChanged = (st->bKeyBindingsChanged) ? TRUE : FALSE;

	bRet = DialogBoxParamA(hSC2KFixModule, MAKEINTRESOURCE(IDD_CONFIGSECTION), hwndDlg, ConfKeyBindingsDialogProc, (LPARAM)&kbs);
	if (bRet) {
		if (kbs.bKeyBindingsChanged)
			st->bKeyBindingsChanged = TRUE;
	}

	return bRet;
}

// SC2K1996-specific calls.

static void DoCenterTool_SC2K1996(CSimcityAppPrimary *pSCApp, UINT nFlags, POINT *pt) {
	__int16 wTileCoords = 0;
	coords_w_t tileCoords;
	CSimcityView *pSCView = NULL;

	if (pSCApp)
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	if (pSCView) {
		wTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt->x, (__int16)pt->y);
		if (wTileCoords >= 0) {
			tileCoords.x = LOBYTE(wTileCoords);
			tileCoords.y = HIBYTE(wTileCoords);
			if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
				if (nFlags & MK_CONTROL)
					;
				else if (nFlags & MK_SHIFT)
					;
				else if (GetAsyncKeyState(VK_MENU) < 0) {
					// useful for tests
				}
				else {
					Game_SimcityApp_SoundPlaySound(pSCApp, SOUND_CLICK);
					Game_SimcityView_DoCenterOnPoint(pSCView);
				}
			}
		}
	}
}

static void DoBindAction_SC2K1996(int nAction, BOOL bRelease) {
	CSimcityAppPrimary *pSCApp = NULL;
	CSimcityView *pSCView = NULL;
	CMainFrame *pMainFrame = NULL;
	POINT pt;

	pSCApp = &pCSimcityAppThis;
	if (pSCApp) {
		pMainFrame = reinterpret_cast<CMainFrame *>(pSCApp->m_pMainWnd);
		pSCView = Game_SimcityApp_PointerToCSimcityViewClass(pSCApp);
	}
	if (nAction > BIND_ACTION_INVALID) {
		switch (nAction) {
			case BIND_ACTION_VIEWESC:
				if (!bRelease) {
					if (pSCView) {
#if 0
						// This one here is an interesting case.
						// While examining the 1996 SE and
						// Interactive Demo versions, both Msg
						// and wParam always point back to the
						// address of the CSimcityWnd vftable,
						// they're then passed to SendMessageA().
						UINT Msg;
						unsigned __int16 wParam;
						CSimcityWnd *pSCWnd;

						pSCWnd = &pMainFrame->dwMFCSimcityWnd;

						Msg = *(DWORD *)&pSCWnd[0];
						wParam = *(WORD *)&pSCWnd[0];

						SendMessageA(pSCView->m_hWnd, Msg, wParam, 0);
#else
						// Let's have the 'Esc' key reset
						// the cheat input state.
						//
						// This will likely be here by
						// default; as for other future
						// actions, they can be considered
						// over time.
						ResetCheatInput_SC2K1996();
#endif
					}
				}
				break;
			case BIND_ACTION_SCROLLUP:
				if (!bRelease) {
					if (pSCView)
						SendMessageA(pSCView->m_hWnd, WM_VSCROLL, SB_LINEUP, NULL);
				}
				break;
			case BIND_ACTION_SCROLLDOWN:
				if (!bRelease) {
					if (pSCView)
						SendMessageA(pSCView->m_hWnd, WM_VSCROLL, SB_LINEDOWN, NULL);
				}
				break;
			case BIND_ACTION_SCROLLLEFT:
				if (!bRelease) {
					if (pSCView)
						SendMessageA(pSCView->m_hWnd, WM_HSCROLL, SB_LINELEFT, NULL);
				}
				break;
			case BIND_ACTION_SCROLLRIGHT:
				if (!bRelease) {
					if (pSCView)
						SendMessageA(pSCView->m_hWnd, WM_HSCROLL, SB_LINERIGHT, NULL);
				}
				break;
			case BIND_ACTION_ZOOMIN:
				if (!bRelease) {
					if (pSCView) {
						if (wCityMode)
							Game_CityToolBar_SetSelection(&pMainFrame->dwMFCityToolBar, CITYTOOL_BUTTON_ZOOMIN, 0);
						else
							Game_MapToolBar_SetSelection(&pMainFrame->dwMFMapToolBar, MAPTOOL_BUTTON_ZOOMIN, 0, 0);
					}
				}
				break;
			case BIND_ACTION_ZOOMOUT:
				if (!bRelease) {
					if (pSCView) {
						if (wCityMode)
							Game_CityToolBar_SetSelection(&pMainFrame->dwMFCityToolBar, CITYTOOL_BUTTON_ZOOMOUT, 0);
						else
							Game_MapToolBar_SetSelection(&pMainFrame->dwMFMapToolBar, MAPTOOL_BUTTON_ZOOMOUT, 0, 0);
					}
				}
				break;
			case BIND_ACTION_ROTATECLOCKWISE:
				if (!bRelease) {
					if (pSCView) {
						Game_SimcityView_RotateClockwise(pSCView);
						UpdateWindow(pSCView->m_hWnd);
					}
				}
				break;
			case BIND_ACTION_ROTATEANTICLOCKWISE:
				if (!bRelease) {
					if (pSCView) {
						Game_SimcityView_RotateAntiClockwise(pSCView);
						UpdateWindow(pSCView->m_hWnd);
					}
				}
				break;
			case BIND_ACTION_CENTERTOOL:
				if (!bRelease) {
					if (pSCView) {
						UINT nFlags;

						nFlags = 0;
						if (GetAsyncKeyState(VK_CONTROL) < 0)
							nFlags |= MK_CONTROL;
						if (GetAsyncKeyState(VK_SHIFT) < 0)
							nFlags |= MK_SHIFT;

						pt.x = gameViewPt.x;
						pt.y = gameViewPt.y;
						DoCenterTool_SC2K1996(pSCApp, nFlags, &pt);
					}
				}
				break;
			case BIND_ACTION_VIEWRIGHTCLICKMENU:
				if (!bRelease) {
					if (pSCView) {
						__int16 wTileCoords = 0;
						coords_w_t tileCoords;
						HMENU hMenu, hSubMenu;

						Game_SimcityView_TileHighlightUpdate(pSCView);
						pt.x = gameViewPt.x;
						pt.y = gameViewPt.y;
						wTileCoords = Game_GetTileCoordsFromScreenCoords((__int16)pt.x, (__int16)pt.y);
						if (wTileCoords >= 0) {
							tileCoords.x = LOBYTE(wTileCoords);
							tileCoords.y = HIBYTE(wTileCoords);
							if (tileCoords.x < GAME_MAP_SIZE && tileCoords.y < GAME_MAP_SIZE) {
								hMenu = LoadMenuA(game_AfxCoreState.m_hCurrentResourceHandle, (LPCSTR)243);
								ClientToScreen(pSCView->m_hWnd, &pt);
								hSubMenu = GetSubMenu(hMenu, 0);
								TrackPopupMenu(hSubMenu, 0, pt.x, pt.y, 0, pMainFrame->m_hWnd, 0);
								Game_SimcityView_MaintainCursor(pSCView);
								pSCView->dwSCVRightClickMenuOpen = TRUE;
							}
						}
					}
				}
				break;
		}
	}
}

// The 'bRelease' case is for certain special instances
// where the bindaction will also need to be registered
// in the "Release" (Up) handle for a finishing call.

void GetKeyBinding_SC2K1996(int nBkey, BOOL bRelease, BOOL bGlobalPtSet) {
	if (IS_KEY_VALID(nBkey)) {
		int nBindAction = GetBinding(nBkey);
		if (IS_ACTION_VALID(nBindAction))
			DoBindAction_SC2K1996(nBindAction, bRelease);
	}
}

// This one is used for mouse button calls (or any others that happen to
// pass the POINT variable). The right-click SimcityView menu needs
// gameViewPt to be set.

void GetKeyButtonBinding_SC2K1996(int nBkey, BOOL bRelease, POINT *pt) {
	if (IS_KEY_VALID(nBkey)) {
		gameViewPt.x = pt->x;
		gameViewPt.y = pt->y;
		GetKeyBinding_SC2K1996(nBkey, bRelease, TRUE);
	}
}
