// sc2kfix include/keybindings.h: Key to Action bindings and configuration dialogues header.
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

#pragma once

enum {
	B_KEY_INVALID,

	B_KEY_ESCAPE,

	B_KEY_PAGEUP,
	B_KEY_PAGEDOWN,
	B_KEY_HOME,
	B_KEY_END,

	B_KEY_CURSORLEFT,
	B_KEY_CURSORUP,
	B_KEY_CURSORRIGHT,
	B_KEY_CURSORDOWN,

	B_KEY_INSERT,
	B_KEY_DELETE,

	B_KEY_0,
	B_KEY_1,
	B_KEY_2,
	B_KEY_3,
	B_KEY_4,
	B_KEY_5,
	B_KEY_6,
	B_KEY_7,
	B_KEY_8,
	B_KEY_9,

	B_KEY_A,
	B_KEY_B,
	B_KEY_C,
	B_KEY_D,
	B_KEY_E,
	B_KEY_F,
	B_KEY_G,
	B_KEY_H,
	B_KEY_I,
	B_KEY_J,
	B_KEY_K,
	B_KEY_L,
	B_KEY_M,
	B_KEY_N,
	B_KEY_O,
	B_KEY_P,
	B_KEY_Q,
	B_KEY_R,
	B_KEY_S,
	B_KEY_T,
	B_KEY_U,
	B_KEY_V,
	B_KEY_W,
	B_KEY_X,
	B_KEY_Y,
	B_KEY_Z,

	B_KEY_NUMPAD0,
	B_KEY_NUMPAD1,
	B_KEY_NUMPAD2,
	B_KEY_NUMPAD3,
	B_KEY_NUMPAD4,
	B_KEY_NUMPAD5,
	B_KEY_NUMPAD6,
	B_KEY_NUMPAD7,
	B_KEY_NUMPAD8,
	B_KEY_NUMPAD9,
	B_KEY_NPMULT,
	B_KEY_NPADD,
	B_KEY_NPSEP,
	B_KEY_NPSUB,
	B_KEY_NPDECIMAL,
	B_KEY_NPDIV,

	B_KEY_COLON,
	B_KEY_PLUS,
	B_KEY_COMMA,
	B_KEY_MINUS,
	B_KEY_PERIOD,
	B_KEY_FSLASH,
	B_KEY_TILDE,

	B_KEY_LBRACE,
	B_KEY_BSLASH,
	B_KEY_RBRACE,
	B_KEY_APOSTR,

	B_KEY_MOUSE_MBUTTON,
	B_KEY_MOUSE_RBUTTON,

	B_KEY_MOUSE_WHEELUP,
	B_KEY_MOUSE_WHEELDOWN,

	B_KEY_COUNT
};

#define MAX_BOUND_KEYS 4

#define IS_KEY_NOTSET(x) (x <= B_KEY_INVALID || x >= B_KEY_COUNT)
#define IS_KEY_VALID(x) (x > B_KEY_INVALID && x < B_KEY_COUNT)

enum {
	B_TYPE_GENERAL,
	B_TYPE_VIRTKEY,
	B_TYPE_MOUSEBT
};

enum {
	BIND_ACTION_INVALID,
	BIND_ACTION_VIEWESC,
	BIND_ACTION_SCROLLUP,
	BIND_ACTION_SCROLLDOWN,
	BIND_ACTION_SCROLLLEFT,
	BIND_ACTION_SCROLLRIGHT,
	BIND_ACTION_ZOOMIN,
	BIND_ACTION_ZOOMOUT,
	BIND_ACTION_ROTATECLOCKWISE,
	BIND_ACTION_ROTATEANTICLOCKWISE,
	BIND_ACTION_CENTERTOOL,
	BIND_ACTION_VIEWRIGHTCLICKMENU,

	BIND_ACTION_COUNT
};

#define IS_ACTION_VALID(x) (x > BIND_ACTION_INVALID && x < BIND_ACTION_COUNT)

typedef struct {
	int  nBkey;
	UINT uVirtKey;
	BOOL bImmutable;
	int iKeyType;
	const char *pKeyName;
} bkey_t;

typedef struct {
	int nBaction;
	int nDefVirtKeys[MAX_BOUND_KEYS];
	BOOL bImmutable;
	BOOL bMouseButtonOnly;
	const char *pActname;
} baction_t;

int CharToKey(UINT nChar);
void InitializeDefaultBindings();
void LoadStoredBindings();
void SaveStoredBindings();
void UpdateKeyBindings();
void ClearTempBindings();
void GetKeyBinding_SC2K1996(int nBkey, BOOL bRelease, BOOL bGlobalPtSet = FALSE);
void GetKeyButtonBinding_SC2K1996(int nBkey, BOOL bRelease, POINT *pt);
