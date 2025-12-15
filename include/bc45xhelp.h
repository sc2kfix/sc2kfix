// sc2kfix include/bc45xhelp.h: helper classes/structs/macros/functions for manipulating Borland 4.5x state
// (c) 2025 sc2kfix project (https://sc2kfix.net) - released under the MIT license

// NOTE: These classes have been entered in-order for there to be
// visibility concerning contained variables, they also contain some
// visible padding due to downstream alignment cases.
//
// These AREN'T supposed to be indicative of a native compilation.

#pragma once

class TBC45XPoint : public tagPOINT {

};

class TBC45XRect : public tagRECT {

};

class TBC45XSize : public tagSIZE {

};

class TBC45XReference {
public:
	unsigned __int16 Refs;
};

#pragma pack(push, 1)
class TBC45XStringRef : public TBC45XReference {
public:
	char *array;
	size_t nchars;
	size_t capacity;
	unsigned int flags;
};
#pragma pack(pop)

class BC45Xstring {
public:
	TBC45XStringRef *p;
};

class BC45Xxmsg {
public:
	BC45Xstring *str;
};

class BC45Xxalloc : public BC45Xxmsg {
public:
	size_t siz;
};

class TXBC45XBase : public BC45Xxmsg {
public:
	DWORD *__vftable;
};

class TXBC45XOwl : public TXBC45XBase {
public:
	unsigned int ResId;
};

class TXBC45XAuto : public TXBC45XBase {
public:
	enum TBC45XError {
		xNoError,
		xConversionFailure,
		xNotIDispatch,
		xForeignIDispatch,
		xTypeMismatch,
		xNoArgSymbol,
		xParameterMissing,
		xNoDefaultValue,
		xValidateFailure,
		xExecutionFailure,
		xErrorStatus,
	};
	TBC45XError ErrorCode;
};

class TBC45XGdiBase {
public:
	class TBC45XXGdi : public TXBC45XOwl {

	};
	HANDLE Handle;
	bool ShouldDelete;
};

class TBC45XDib : public TBC45XGdiBase {
public:
	enum {
		MapFace      = 0x01,  // Or these together to control colors to map
		MapText      = 0x02,  // to current SysColor values 
		MapShadow    = 0x04,
		MapHighlight = 0x08,
		MapFrame     = 0x10
	};

	BITMAPINFO *Info;
	char *Bits;
	int NumClrs;
	int W;
	int H;
	uint16_t Mode;
	uint8_t IsCore;
	uint8_t IsResHandle;
};

class TBC45XGdiObject : public TBC45XGdiBase {

};

class TBC45XPalette : public TBC45XGdiObject {

};

class TBC45XDC : public TBC45XGdiBase {
public:
	DWORD *__vftable;
	HBRUSH OrgBrush;
	HPEN OrgPen;
	HFONT OrgFont;
	HPALETTE OrgPalette;
	HBRUSH OrgTextBrush;
};

enum StreamableInit { streamableInit };

class TBC45XStreamableBase {
	DWORD *__vftable;
};

class TBC45XStreamable : public TBC45XStreamableBase {

};

class TBC45XStreamer {
public:
	DWORD *__vftable;
	TBC45XStreamableBase *object;
};

class TBC45XOldStreamer : public TBC45XStreamer{

};

class TBC45XNewStreamer : public TBC45XStreamer{

};

#define DECLARE_STREAMER( cls, ver )                           \
public:                                                        \
    class BC45XStreamer : public TBC45XNewStreamer {           \
        public:                                                \
            cls *object;                                       \
        };

#define DECLARE_STREAMABLE_CTOR( cls )                         \
public:                                                        \
    cls ( StreamableInit )

#define DECLARE_STREAMABLE( cls, ver )                         \
    DECLARE_STREAMER( cls, ver );                              \
    DECLARE_STREAMABLE_CTOR( cls )

class BC45XGENERIC;

typedef void (BC45XGENERIC::*TAnyPMF)();

typedef int32_t (*TBC45XAnyDispatcher)(BC45XGENERIC&, TAnyPMF, unsigned int, int32_t);

template <class T> class TBC45XResponseTableEntry;
typedef TBC45XResponseTableEntry<BC45XGENERIC>  TBC45XGenericTableEntry;

class TBC45XEventHandler {
public:
	DWORD *__vftable;
	class TBC45XEventInfo {
	public:
		const unsigned int          Msg;
		const unsigned int          Id;
		BC45XGENERIC*               Object;
		TBC45XGenericTableEntry *Entry;
	};
	typedef bool(*TBC45XEqualOperator)(TBC45XGenericTableEntry &, TBC45XEventInfo&);
};

template <class T> class TBC45XResponseTableEntry {
public:
	typedef void (T::*PMF)();

	union {
		unsigned int          Msg;
		unsigned int          NotifyCode;
	};
	unsigned int         Id;
	TBC45XAnyDispatcher  Dispatcher;
	PMF             Pmf;
};

#define DECLARE_RESPONSE_TABLE(cls)                                \
  private:                                                         \
    static TBC45XResponseTableEntry< cls >         __entries[];    \
    typedef TBC45XResponseTableEntry< cls >::PMF   TMyPMF;         \
    typedef cls                                    TMyClass;

class TBC45XStatus {
public:
	int StatusCode;
};

class TBC45XModule : public TBC45XEventHandler, public TBC45XStreamableBase {
public:
	class TXBC45XInvalidModule : public TXBC45XOwl {

	};

protected:    
	char  *Name;
	HINSTANCE HInstance;

private:    
	bool      ShouldFree;

public:
	char *lpCmdLine;
	TBC45XStatus   Status;

	DECLARE_STREAMABLE(TBC45XModule, 1);
};

class TBC45XMutex {
public:
	enum { NoLimit = -1 };
	typedef HANDLE TBC45XHandle;

	class Lock {
	public:
		const TBC45XMutex *MutexObj;

	};

	TBC45XHandle Handle;
};

class TBC45XApplication;
class TBC45XWindow;
class TBC45XScroller;

struct TBC45XCurrentEvent {
	TBC45XWindow  *Win;
	unsigned int  Message;
	WPARAM        WParam;
	LPARAM        LParam;
};

class TBC45XCommandEnabler {
public:
	const unsigned int  Id;

	const HWND  HWndReceiver;

	enum {WasHandled = 1, NonSender = 2};

	unsigned int        Handled;
};

class TBC45XResId {
public:
	const char *Id;
};

struct TBC45XWindowAttr {
	uint32_t     Style;
	uint32_t     ExStyle;
	int          X, Y, W, H;
	TBC45XResId  Menu;        // Menu resource id
	int          Id;          // Child identifier
	char         *Param;
	TBC45XResId  AccelTable;  // Accelerator table resource id
};

#pragma pack(push, 2)
class TBC45XParWindow : public TBC45XEventHandler,
	public TBC45XStreamableBase {
public:
	DWORD *__vftable;
	class TXBC45XWindow : public TXBC45XOwl {
	public:
		TBC45XWindow*      Window;
	};

	TBC45XStatus      Status;
	HWND              HWindow;  // handle to associated MS-Windows window
	char              *Title;
	TBC45XWindow      *Parent;
	TBC45XWindowAttr  Attr;
	WNDPROC           DefaultProc;
	TBC45XScroller    *Scroller;

	void         *TransferBuffer;
	HACCEL       hAccel;
	TBC45XModule *CursorModule;
	TBC45XResId  CursorResId;
	HCURSOR      HCursor;
	uint32_t     BkgndColor;

	WNDPROC            Thunk;        // Thunk that load 'this' into registers
	TBC45XApplication  *Application;  // Application that this window belongs to
	TBC45XModule       *Module;       // default module used for getting resources
	uint32_t           Flags;
	uint16_t           ZOrder;
	TBC45XWindow       *ChildList;
	TBC45XWindow       *SiblingList;
	uint32_t           UniqueId;

	DECLARE_RESPONSE_TABLE(TBC45XParWindow);
	DECLARE_STREAMABLE(TBC45XParWindow, 3);
};

class TBC45XDerivedWindowFoot {
public:
	DWORD dwUnknownOne;
	DWORD *__ev_ptr_vftable;
	DWORD *__str_ptr_vftable;
};

class TBC45XWindow : public TBC45XParWindow {
public:
	TBC45XDerivedWindowFoot wndFoot;
};

class TBC45XDerivedWindowHead {
public:
	TBC45XWindow *pWnd;
	DWORD *__ev_vftable;
	DWORD *__str_vftable;
	DWORD *__drvwnd_vftable;
};
#pragma pack(pop)

struct TBC45XDialogAttr {
	char * Name;
	uint32_t    Param;
};

#pragma pack(push, 2)
class TBC45XParDialog : public TBC45XDerivedWindowHead {
public:
	TBC45XDialogAttr  Attr;
	BOOL         IsModal;

	DECLARE_RESPONSE_TABLE(TBC45XParDialog);
	DECLARE_STREAMABLE(TBC45XParDialog, 1);
};

class TBC45XDialog : public TBC45XParDialog {
public:
	TBC45XDerivedWindowFoot wndFoot;
	DWORD dwUnknownTwo;
	TBC45XParWindow newWnd;
};

class TBC45XParControl : public TBC45XParWindow {
	DECLARE_RESPONSE_TABLE(TBC45XParControl);
	DECLARE_STREAMABLE(TBC45XParControl, 1);
};

class TBC45XControl : public TBC45XParControl {
public:
	TBC45XDerivedWindowFoot wndFoot;
};

class TBC45XParListBox : public TBC45XParControl {
	DECLARE_STREAMABLE(TBC45XParListBox, 1);
};

class TBC45XListBox : public TBC45XParListBox {
public:
	TBC45XDerivedWindowFoot wndFoot;
};
#pragma pack(pop)
