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
	BOOL ShouldDelete;
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

class TBC45XBitmap : public TBC45XGdiObject {

};

class TBC45XBrush : public TBC45XGdiObject {

};

class TBC45XFont : public TBC45XGdiObject {

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

class TBC45XWindowDC : public TBC45XDC {
public:
	HWND hWnd;
};

class TBC45XClientDC : public TBC45XWindowDC {
	
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
    cls (StreamableInit)

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
	typedef BOOL(*TBC45XEqualOperator)(TBC45XGenericTableEntry &, TBC45XEventInfo&);
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
	BOOL      ShouldFree;

public:
	char *lpCmdLine;
	TBC45XStatus   Status;
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

#pragma pack(push, 1)
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
};

class TBC45XDerivedWindowFoot {
public:
	DWORD dwUnknownOne;
	DWORD *__ev_ptr_vftable;
	DWORD *__str_ptr_vftable;
};

class TBC45XWindow : public TBC45XParWindow {
public:
	TBC45XDerivedWindowFoot __clFoot;
};

class TBC45XDerivedWindowHead {
public:
	TBC45XWindow *pWnd;
	DWORD *__ev_vftable;
	DWORD *__str_vftable;
	DWORD *__drvwnd_vftable;
};

class TBC45XDerivedWindowFootNewWnd : public TBC45XDerivedWindowFoot {
	DWORD dwUnknownTwo;
	TBC45XParWindow newWnd;
};

struct TBC45XMargins {
	enum TBC45XUnits {
		Pixels = 0x0,
		LayoutUnits = 0x1,
		BorderUnits = 0x2,
	};
	TBC45XUnits Units;
	__int32 Left : 8;
	__int32 Right : 8;
	__int32 Top : 8;
	__int32 Bottom : 8;
};

class TBC45XGadget;

class TBC45XParGadgetWindow : public TBC45XParWindow {
public:
	TBC45XGadget *Gadgets;
	TBC45XFont *Font;
	TBC45XBrush *BkgndBrush;
	TBC45XGadget *Capture;
	TBC45XGadget *AtMouse;
	TBC45XMargins Margins;
	BYTE NumGadgets;
	BYTE FontHeight;
	BYTE ShrinkWrapWidth;
	BYTE ShrinkWrapHeight;
	BYTE WideAsPossible;
	BYTE DirtyLayout;
	BYTE Direction;
	BYTE HintMode;
};

class TBC45XGadgetWindow : public TBC45XParGadgetWindow {
public:
	TBC45XDerivedWindowFoot __clFoot;
};

class TBC45XParGadget {
public:
	DWORD *__vftable;
	BOOL Clip;
	BOOL WideAsPossible;
	TBC45XGadgetWindow *Window;
	TBC45XRect Bounds;

	enum TBC45XBorderStyle {
		None = 0x0,
		Plain = 0x1,
		Raised = 0x2,
		Recessed = 0x3,
		Embossed = 0x4,
	};

	TBC45XBorderStyle BorderStyle;

	struct TBC45XBorders {
		unsigned __int32 Left : 8;
		unsigned __int32 Right : 8;
		unsigned __int32 Top : 8;
		unsigned __int32 Bottom : 8;
	};

	TBC45XBorders Borders;
	TBC45XMargins Margins;
	BOOL ShrinkWrapWidth;
	BOOL ShrinkWrapHeight;
	BOOL TrackMouse;
	int Id;
	TBC45XGadget *Next;
	BOOL Enabled;
};

class TBC45XGadget : public TBC45XParGadget {

};

class TBC45XCelArray {
public:
	TBC45XBitmap *Bitmap;
	BOOL ShouldDelete;
	TBC45XPoint Offs;
	int NCels;
	TBC45XSize CSize;
};

class TBC45XButtonGadget : public TBC45XParGadget {
public:
	TBC45XResId ResId;
	TBC45XCelArray *CelArray;
	TBC45XPoint BitmapOrigin;
	WORD wTemp;

	enum TBC45XShadowStyle {
		SingleShadow = 0x1,
		DoubleShadow = 0x2,
	};

	enum TBC45XState {
		Up = 0x0,
		Down = 0x1,
		Indeterminate = 0x2,
	};

	enum TBC45XType {
		Command = 0x0,
		Exclusive = 0x1,
		NonExclusive = 0x2,
	};
};

class TBC45XParToolBox : public TBC45XParGadgetWindow {
public:
	int NumRows;
	int NumColumns;
};

class TBC45XToolBox : public TBC45XParToolBox {
public:
	TBC45XDerivedWindowFoot __clFoot;
};

struct TBC45XChildMetrics;
struct TBC45XConstraint;

struct TBC45XVariable {
	int Value;
	TBC45XConstraint *DeterminedBy;
	BOOL Resolved;
};

struct TBC45XFixed {
	int Value;
};

struct TBC45XConstraint {
	TBC45XVariable *Inputs[3];
	TBC45XVariable *Output;
	TBC45XFixed OrderedCombination[4];
	TBC45XConstraint *Next;
};

class TBC45XParLayoutWindow : public TBC45XDerivedWindowHead {
public:
	TBC45XChildMetrics *ChildMetrics;
	TBC45XConstraint *Constraints;
	TBC45XConstraint *Plan;
	TBC45XVariable *Variables;
	BOOL PlanIsDirty;
	int NumChildMetrics;
	int FontHeight;
	DWORD dwTempOne[2];
};

class TBC45XLayoutWindow : public TBC45XParLayoutWindow {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XMenu {
public:
	DWORD *__vftable;
	class TXBC45XMenu : public TXBC45XOwl {
	};
	HMENU Handle;
	BOOL ShouldDelete;
};

class TBC45XMenuDescr : public TBC45XMenu {
public:
	TBC45XModule *Module;
	TBC45XResId Id;

	enum TBC45XGroup {
		FileGroup = 0x0,
		EditGroup = 0x1,
		ContainerGroup = 0x2,
		ObjectGroup = 0x3,
		WindowGroup = 0x4,
		HelpGroup = 0x5,
		NumGroups = 0x6,
	};
	int GroupCount[6];
};

class TBC45XParTinyCaption : public TBC45XDerivedWindowHead {
public:
	TBC45XSize Border;
	TBC45XSize Frame;
	BOOL CloseBox;
	BOOL TCEnabled;
	int CaptionHeight;
	TBC45XFont *CaptionFont;
	unsigned int DownHit;
	BOOL IsPressed;
	BOOL WaitingForSysCmd;
};

class TBC45XTinyCaption : public TBC45XParTinyCaption {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XParFloatingFrame : public TBC45XDerivedWindowHead {
	DWORD dwTempOne[10];
	TBC45XParTinyCaption newTinyCaption;
	TBC45XSize Margin;
	BOOL FloatingPaletteEnabled;
};

class TBC45XFloatingFrame : public TBC45XParFloatingFrame {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XParFrameWindow : public TBC45XDerivedWindowHead {
public:
	BOOL KeyboardHandling;
	HWND HWndRestoreFocus;
	TBC45XWindow *ClientWnd;
	int DocTitleIndex;
	TBC45XModule *MergeModule;
	TBC45XMenuDescr *MenuDescr;
	TBC45XModule *IconModule;
	TBC45XResId IconResId;
	TBC45XPoint MinimizedPos;
};

class TBC45XFrameWindow : public TBC45XParFrameWindow {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XDerivedFrameWindowHead {
public:
	TBC45XFrameWindow *pFrameWnd;
	TBC45XDerivedWindowHead __wndHead;
};

class TBC45XDerivedFrameWindowFoot : public TBC45XDerivedWindowFootNewWnd {
public:
	DWORD dwUnknownThree;
	TBC45XParFrameWindow newFrameWnd;
};

class TBC45XParDecoratedFrame {
public:
	TBC45XFrameWindow *pFrameWnd;
	TBC45XParLayoutWindow newLayoutWnd;
	DWORD *__vftable;
	int TrackMenuSelection;
	DWORD MenuItemId;
	BOOL SettingClient;
};

class TBC45XDecoratedFrame : public TBC45XParDecoratedFrame {
public:
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class TBC45XParDecoratedMDIFrame : public TBC45XDerivedFrameWindowHead {
	TBC45XParDecoratedFrame newDecorFrame;
};

class TBC45XDecoratedMDIFrame : public TBC45XParDecoratedMDIFrame {
public:
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

struct TBC45XDialogAttr {
	char * Name;
	uint32_t    Param;
};

class TBC45XParDialog : public TBC45XDerivedWindowHead {
public:
	TBC45XDialogAttr  Attr;
	BOOL         IsModal;
};

class TBC45XDialog : public TBC45XParDialog {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XParCommonDialog : public TBC45XParDialog {
public:
	TBC45XModule *pModule;
};

class TBC45XCommonDialog : public TBC45XParCommonDialog {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XParOpenSaveDialog : public TBC45XParCommonDialog {
public:
	OPENFILENAME ofn;
	struct TData {
		uint32_t Flags;
		uint32_t Error;
		char *FileName;
		char *Filter;
		char *CustomFilter;
		int FilterIndex;
		char *InitialDir;
		char *DefExt;
		int MaxPath;
	};
	TData *Data;
};

class TBC45XOpenSaveDialog : public TBC45XParOpenSaveDialog {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XFileOpenDialog : public TBC45XOpenSaveDialog {

};

class TBC45XFileSaveDialog : public TBC45XOpenSaveDialog {

};

class TBC45XParControl : public TBC45XParWindow {

};

class TBC45XControl : public TBC45XParControl {
public:
	TBC45XDerivedWindowFoot __clFoot;
};

class TBC45XParListBox : public TBC45XParControl {

};

class TBC45XListBox : public TBC45XParListBox {
public:
	TBC45XDerivedWindowFoot __clFoot;
};

class TBC45XParMDIChild : public TBC45XDerivedFrameWindowHead {

};

class TBC45XMDIChild : public TBC45XParMDIChild {
public:
	TBC45XDerivedFrameWindowFoot __frameWndFoot;
};

class TBC45XParMDIClient : public TBC45XDerivedWindowHead {
public:
	LPCLIENTCREATESTRUCT ClientAttr;
};

class TBC45XMDIClient : public TBC45XParMDIClient {
public:
	TBC45XDerivedWindowFootNewWnd __wndFoot;
};

class TBC45XDocManager;
class TBC45XAppDictionary;

class TBC45XApplication : public TBC45XModule {
public:
	class TXBC45XInvalidMainWindow : public TXBC45XOwl {
	};
	HINSTANCE hPrevInstance;
	int nCmdShow;
	TBC45XDocManager *DocManager;
	TBC45XFrameWindow *MainWindow;
	HACCEL HAccTable;

	class TBC45XAppMutex {
	public:
		char Mutex[sizeof(TBC45XMutex)];
	};

	class TBC45XAppLock {
	public:
		char AppLock[sizeof(TBC45XMutex::Lock)];
	};

	BOOL BreakMessageLoop;
	int MessageLoopResult;
	BC45Xstring CmdLine;
	BOOL BWCCOn;
	TBC45XModule *BWCCModule;
	BOOL Ctl3dOn;
	TBC45XModule *Ctl3dModule;
	BOOL Running;
	TBC45XAppMutex Mutex;
	TBC45XCurrentEvent CurrentEvent;
	int XState;
	BC45Xstring XString;
	size_t XSize;
	TXBC45XBase *XBase;
	TBC45XWindow *CondemnedWindows;
	TBC45XAppDictionary *Dictionary;
};
#pragma pack(pop)
