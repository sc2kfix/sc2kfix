// sc2kfix include/bc45xhelp.h: helper classes/structs/macros/functions for manipulating Borland 4.5x state
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE: ONLY MINIMUM CASES ARE INCLUDED HERE FOR SPECIFIC INSTANCES!

#pragma once

class TBC45XPoint : public tagPOINT {

};

class TBC45XRect : public tagRECT {

};

struct TBC45XGdiBase {
	HANDLE Handle;
	bool ShouldDelete;
};

class TBC45XGdiObject : public TBC45XGdiBase {

};

class TBC45XPalette : public TBC45XGdiObject {

};

class TBC45XDC : public TBC45XGdiBase {
	DWORD *__vftable;
	HBRUSH OrgBrush;
	HPEN OrgPen;
	HFONT OrgFont;
	HPALETTE OrgPalette;
	HBRUSH OrgTextBrush;
};
