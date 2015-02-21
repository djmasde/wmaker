

#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <wraster.h>
#include <X11/Xlib.h>

#define WINGS_H_VERSION  980722

typedef unsigned long WMPixel;



typedef struct {
    unsigned int width;
    unsigned int height;
} WMSize;

typedef struct {
    int x;
    int y;
} WMPoint;

typedef struct {
    WMPoint pos;
    WMSize size;
} WMRect;


#define ClientMessageMask	(1L<<30)


/* button types */
typedef enum {
    /* 0 is reserved for internal use */
	WBTMomentaryPush = 1,
	WBTPushOnPushOff = 2,
	WBTToggle = 3,
	WBTSwitch = 4,
	WBTRadio = 5,
	WBTMomentaryChange = 6,
	WBTOnOff = 7,
	WBTMomentaryLight = 8
} WMButtonType;

/* button behaviour masks */
enum {
    WBBSpringLoadedMask = 	(1 << 0),
	WBBPushInMask = 	(1 << 1),
	WBBPushChangeMask = 	(1 << 2),
	WBBPushLightMask = 	(1 << 3),
	WBBStateLightMask = 	(1 << 5),
	WBBStateChangeMask = 	(1 << 6),
	WBBStatePushMask = 	(1 << 7)
};


/* frame title positions */
typedef enum {
    WTPNoTitle,
	WTPAboveTop,
	WTPAtTop,
	WTPBelowTop,
	WTPAboveBottom,
	WTPAtBottom,
	WTPBelowBottom
} WMTitlePosition;


/* relief types */
typedef enum {
    WRFlat,
	WRSimple,
	WRRaised,
	WRSunken,
	WRGroove,
	WRRidge
} WMReliefType;


/* alignment types */
typedef enum {
    WALeft,
	WACenter,
	WARight
} WMAlignment;


/* image position */
typedef enum {
    WIPNoImage,
    WIPImageOnly,
    WIPLeft,
    WIPRight,
    WIPBelow,
    WIPAbove,
    WIPOverlaps
} WMImagePosition;


/* scroller arrow position */
typedef enum {
    WSAMaxEnd,
	WSAMinEnd,
	WSANone
} WMScrollArrowPosition;

/* scroller parts */
typedef enum {
    WSNoPart,
	WSDecrementPage,
	WSIncrementPage,
	WSDecrementLine,
	WSIncrementLine,
	WSKnob,
	WSKnobSlot
} WMScrollerPart;

/* usable scroller parts */
typedef enum {
    WSUNoParts,
	WSUOnlyArrows,
	WSUAllParts
} WMUsableScrollerParts;

/* orientations for scroller and maybe other stuff */
typedef enum {
    WOVertical,
	WOHorizontal
} WMOrientation;

/* matrix types */
typedef enum {
    WMMRadioMode,
	WMMHighlightMode,
	WMMListMode,
	WMMTrackMode
} WMMatrixTypes;


enum {
    WLDSSelected = (1 << 16),
	WLDSDisabled = (1 << 17),
	WLDSFocused = (1 << 18),
	WLDSIsBranch = (1 << 19)
};

/* alert panel return values */

enum {
    WAPRDefault = 0,
	WAPRAlternate = 1,
	WAPROther = -1,
	WAPRError = -2
};


typedef int W_Class;


#define WC_Window 	0
#define WC_Frame 	1
#define WC_Label 	2
#define WC_Button	3
#define WC_TextField	4
#define WC_Scroller	5
#define WC_ScrollView 	6
#define WC_List		7
#define WC_Browser 	8
#define WC_PopUpButton	9
#define WC_ColorWell	10


/* All widgets must start with the following structure
 * in that order. Used for typecasting to get some generic data */
typedef struct W_WidgetType {
    W_Class widgetClass;
    struct W_View *view;
    
} W_WidgetType;


#define WMWidgetClass(widget)  	(((W_WidgetType*)(widget))->widgetClass)
#define WMWidgetView(widget)   	(((W_WidgetType*)(widget))->view)


typedef struct W_PanelType {
    struct WMWindow *win;
} W_PanelType;

#define W_WINDOW(panel)		(((W_PanelType*)(panel))->win)

/* widgets */

typedef void WMWidget;

typedef struct W_Pixmap WMPixmap;
typedef struct W_Font	WMFont;
typedef struct W_Color	WMColor;

typedef struct W_Screen WMScreen;

typedef struct W_View WMView;

typedef struct W_Window WMWindow;
typedef struct W_Frame WMFrame;
typedef struct W_Button WMButton;
typedef struct W_Label WMLabel;
typedef struct W_TextField WMTextField;
typedef struct W_Scroller WMScroller;
typedef struct W_ScrollView WMScrollView;
typedef struct W_List WMList;
typedef struct W_Browser WMBrowser;
typedef struct W_PopUpButton WMPopUpButton;
typedef struct W_ColorWell WMColorWell;


/* not widgets */
typedef struct W_FilePanel WMFilePanel;
typedef WMFilePanel WMOpenPanel;
typedef WMFilePanel WMSavePanel;


/* item for WMList */
typedef struct WMListItem {
    char *text;
    
    struct WMListItem *nextPtr;
    
    unsigned int uflags:16;	       /* flags for the user */
    unsigned int selected:1;
    unsigned int disabled:1;
    unsigned int isBranch:1;
    unsigned int loaded:1;
} WMListItem;

/* struct for message panel */
typedef struct WMAlertPanel {
    WMWindow *win;		       /* window */
    WMButton *defBtn;		       /* default button */
    WMButton *altBtn;		       /* alternative button */
    WMButton *othBtn;		       /* other button */
    WMLabel *iLbl;		       /* icon label */
    WMLabel *tLbl;		       /* title label */
    WMLabel *mLbl;		       /* message label */
    WMFrame *line;		       /* separator */
    short result;		       /* button that was pushed */
    short done;
    
    KeyCode retKey;
} WMAlertPanel;


typedef struct WMInputPanel {
    WMWindow *win;		       /* window */
    WMButton *defBtn;		       /* default button */
    WMButton *altBtn;		       /* alternative button */
    WMLabel *tLbl;		       /* title label */
    WMLabel *mLbl;		       /* message label */
    WMTextField *text;		       /* text field */
    short result;		       /* button that was pushed */
    short done;
    
    KeyCode retKey;
} WMInputPanel;


typedef void *WMHandlerID;

typedef void WMEventProc(XEvent *event, void *clientData);

typedef void WMEventHook(XEvent *event);

/* self is set to the widget from where the callback is being called and
 * clientData to the data set to with WMSetClientData() */
typedef void WMAction(WMWidget *self, void *clientData);


typedef void WMFreeDataProc(void *data);

typedef void WMListDrawProc(WMList *lPtr, Drawable d, char *text, int state,
			    WMRect *rect);

typedef void WMBrowserFillColumn(WMBrowser *bPtr, int column);

typedef void WMCallback(void *data);


/* ....................................................................... */



WMScreen *WMCreateScreenWithRContext(Display *display, int screen, 
				     RContext *context, char *applicationName,
				     int *argc, char **argv);

WMScreen *WMCreateScreen(Display *display, int screen, char *applicationName,
			 int *argc, char **argv);

WMScreen *WMCreateSimpleApplicationScreen(Display *display, 
					  char *applicationName,
					  int *argc, char **argv);

void WMScreenMainLoop(WMScreen *scr);


RContext *WMScreenRContext(WMScreen *scr);


void WMSetApplicationIconImage(WMScreen *app, WMPixmap *icon);

WMPixmap *WMGetApplicationIconImage(WMScreen *app);

void WMSetFocusToWidget(WMWidget *widget);

WMEventHook *WMHookEventHandler(WMEventHook *handler);

int WMHandleEvent(XEvent *event);

void WMCreateEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

void WMDeleteEventHandler(WMView *view, unsigned long mask,
			  WMEventProc *eventProc, void *clientData);

int WMIsDoubleClick(XEvent *event);

void WMNextEvent(Display *dpy, XEvent *event);

void WMMaskEvent(Display *dpy, long mask, XEvent *event);

WMHandlerID WMAddTimerHandler(int milliseconds, WMCallback *callback, 
			      void *cdata);

void WMDeleteTimerWithClientData(void *cdata);

void WMDeleteTimerHandler(WMHandlerID handlerID);

WMHandlerID WMAddIdleHandler(WMCallback *callback, void *cdata);

void WMDeleteIdleHandler(WMHandlerID handlerID);

/* ....................................................................... */

WMFont *WMCreateFont(WMScreen *scrPtr, char *font_name);

WMFont *WMRetainFont(WMFont *font);

void WMReleaseFont(WMFont *font);

/*
WMFont *WMUserFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMUserFixedPitchFontOfSize(WMScreen *scrPtr, int size);
*/

WMFont *WMSystemFontOfSize(WMScreen *scrPtr, int size);

WMFont *WMBoldSystemFontOfSize(WMScreen *scrPtr, int size);


/* ....................................................................... */

WMPixmap *WMRetainPixmap(WMPixmap *pixmap);

void WMReleasePixmap(WMPixmap *pixmap);

WMPixmap *WMCreatePixmapFromXPixmaps(WMScreen *scrPtr, Pixmap pixmap, 
				     Pixmap mask, int width, int height,
				     int depth);

WMPixmap *WMCreatePixmapFromRImage(WMScreen *scrPtr, RImage *image, 
				   int threshold);

WMSize WMGetPixmapSize(WMPixmap *pixmap);

WMPixmap *WMCreatePixmapFromFile(WMScreen *scrPtr, char *fileName);

WMPixmap *WMCreateBlendedPixmapFromFile(WMScreen *scrPtr, char *fileName,
					RColor *color);
/* ....................................................................... */


WMColor *WMDarkGrayColor(WMScreen *scr);

WMColor *WMGrayColor(WMScreen *scr);

WMColor *WMBlackColor(WMScreen *scr);

WMColor *WMWhiteColor(WMScreen *scr);

GC WMColorGC(WMColor *color);

WMPixel WMColorPixel(WMColor *color);

void WMPaintColorRectangle(WMScreen *scr, WMColor *color, Drawable d, int x, 
			   int y, unsigned int width, unsigned int height);

void WMReleaseColor(WMScreen *scr, WMColor *color);

WMColor *WMRetainColor(WMColor *color);

WMColor *WMCreateRGBColor(WMScreen *scr, unsigned short red, 
			  unsigned short green, unsigned short blue,
			  Bool exact);


/* ....................................................................... */

WMScreen *WMWidgetScreen(WMWidget *w);

void WMUnmapWidget(WMWidget *w);

void WMMapWidget(WMWidget *w);

void WMMoveWidget(WMWidget *w, int x, int y);

void WMResizeWidget(WMWidget *w, unsigned int width, unsigned int height);

void WMSetWidgetBackgroundColor(WMWidget *w, WMColor *color);

void WMMapSubwidgets(WMWidget *w);

void WMRealizeWidget(WMWidget *w);

void WMDestroyWidget(WMWidget *widget);

void WMHangData(WMWidget *widget, void *data);

void *WMGetHangedData(WMWidget *widget);

void WMAddDestroyCallback(WMWidget *w, WMCallback *callback, void *clientData);

unsigned int WMWidgetWidth(WMWidget *w);

unsigned int WMWidgetHeight(WMWidget *w);

Window WMWidgetXID(WMWidget *w);


/* ....................................................................... */

WMWindow *WMCreateWindow(WMScreen *screen, char *name);

void WMSetWindowTitle(WMWindow *wPtr, char *title);

WMScreen *WMWindowApplication(WMWindow *win);

void WMSetWindowCloseAction(WMWindow *win, WMAction *action, void *clientData);

/* ....................................................................... */

void WMSetButtonAction(WMButton *bPtr, WMAction *action, void *clientData);

#define WMCreateCommandButton(parent) \
	WMCreateButton((parent), WBTMomentaryPush)

#define WMCreateRadioButton(parent) \
	WMCreateButton((parent), WBTRadio)

#define WMCreateSwitchButton(parent) \
	WMCreateButton((parent), WBTSwitch)

WMButton *WMCreateButton(WMWidget *parent, WMButtonType type);

WMButton *WMCreateCustomButton(WMWidget *parent, int behaviourMask);

void WMSetButtonImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonAltImage(WMButton *bPtr, WMPixmap *image);

void WMSetButtonImagePosition(WMButton *bPtr, WMImagePosition position);

void WMSetButtonTextAlignment(WMButton *bPtr, WMAlignment alignment);

void WMSetButtonText(WMButton *bPtr, char *text);

void WMSetButtonAltText(WMButton *bPtr, char *text);

void WMSetButtonSelected(WMButton *bPtr, int isSelected);

int WMGetButtonSelected(WMButton *bPtr);

void WMSetButtonBordered(WMButton *bPtr, int isBordered);

void WMSetButtonDisabled(WMButton *bPtr, int isDisabled);

void WMSetButtonTag(WMButton *bPtr, int tag);

void WMGroupButtons(WMButton *bPtr, WMButton *newMember);

void WMPerformButtonClick(WMButton *bPtr);

/* ....................................................................... */

WMLabel *WMCreateLabel(WMWidget *parent);

void WMSetLabelImage(WMLabel *lPtr, WMPixmap *image);

void WMSetLabelImagePosition(WMLabel *lPtr, WMImagePosition position);
	
void WMSetLabelTextAlignment(WMLabel *lPtr, WMAlignment alignment);

void WMSetLabelRelief(WMLabel *lPtr, WMReliefType relief);
	
void WMSetLabelText(WMLabel *lPtr, char *text);

void WMSetLabelFont(WMLabel *lPtr, WMFont *font);

void WMSetLabelTextColor(WMLabel *lPtr, WMColor *color);

/* ....................................................................... */

WMFrame *WMCreateFrame(WMWidget *parent);

void WMSetFrameTitlePosition(WMFrame *fPtr, WMTitlePosition position);

void WMSetFrameRelief(WMFrame *fPtr, WMReliefType relief);

void WMSetFrameTitle(WMFrame *fPtr, char *title);

/* ....................................................................... */

WMTextField *WMCreateTextField(WMWidget *parent);

void WMInsertTextFieldText(WMTextField *tPtr, char *text, int position);

void WMDeleteTextFieldRange(WMTextField *tPtr, int position, int count);

char *WMGetTextFieldText(WMTextField *tPtr);

void WMSetTextFieldText(WMTextField *tPtr, char *text);

void WMSetTextFieldAlignment(WMTextField *tPtr, WMAlignment alignment);

void WMSetTextFieldBordered(WMTextField *tPtr, Bool bordered);

void WMSetTextFieldDisabled(WMTextField *tPtr, Bool disabled);

/* ....................................................................... */

WMScroller *WMCreateScroller(WMWidget *parent);

void WMSetScrollerParameters(WMScroller *sPtr, float floatValue, 
			     float knobProportion);

float WMGetScrollerKnobProportion(WMScroller *sPtr);

float WMGetScrollerFloatValue(WMScroller *sPtr);

WMScrollerPart WMGetScrollerHitPart(WMScroller *sPtr);

void WMSetScrollerAction(WMScroller *sPtr, WMAction *action, void *clientData);

void WMSetScrollerArrowsPosition(WMScroller *sPtr, 
				 WMScrollArrowPosition position);

/* ....................................................................... */

WMList *WMCreateList(WMWidget *parent);

#define WMAddListItem(lPtr, text) WMInsertListItem((lPtr), -1, (text))

WMListItem *WMInsertListItem(WMList *lPtr, int index, char *text);

WMListItem *WMAddSortedListItem(WMList *lPtr, char *text);

char *WMGetListItem(WMList *lPtr, int index);

void WMRemoveListItem(WMList *lPtr, int index);

void WMSelectListItem(WMList *lPtr, int row);

void WMSetListUserDrawProc(WMList *lPtr, WMListDrawProc *proc);

void WMSetListItems(WMList *lPtr, WMListItem *items);

WMListItem *WMGetListItems(WMList *lPtr);

WMListItem *WMGetListSelectedItem(WMList *lPtr);

int WMGetListSelectedItemIndex(WMList *lPtr);

void WMSetListAction(WMList *lPtr, WMAction *action, void *clientData);

void WMSetListDoubleAction(WMList *lPtr, WMAction *action, void *clientData);

void WMClearList(WMList *lPtr);

/* ....................................................................... */

WMBrowser *WMCreateBrowser(WMWidget *parent);

void WMLoadBrowserColumnZero(WMBrowser *bPtr);

int WMAddBrowserColumn(WMBrowser *bPtr);

void WMSetBrowserColumnTitle(WMBrowser *bPtr, int column, char *title);

Bool WMAddBrowserItem(WMBrowser *bPtr, int column, char *text, Bool isBranch);

Bool WMSetBrowserPath(WMBrowser *bPtr, char *path);

char *WMGetBrowserPath(WMBrowser *bPtr);

char *WMGetBrowserPathToColumn(WMBrowser *bPtr, int column);

void WMSetBrowserFillColumnProc(WMBrowser *bPtr, WMBrowserFillColumn *proc);

void WMSetBrowserAction(WMBrowser *bPtr, WMAction *action, void *clientData);

WMListItem *WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column);

/* ....................................................................... */

WMPopUpButton *WMCreatePopUpButton(WMWidget *parent);

void WMSetPopUpButtonAction(WMPopUpButton *sPtr, WMAction *action, 
			    void *clientData);

void WMAddPopUpButtonItem(WMPopUpButton *bPtr, char *title);

void WMInsertPopUpButtonItem(WMPopUpButton *bPtr, int index, char *title);

void WMRemovePopUpButtonItem(WMPopUpButton *bPtr, int index);

void WMSetPopUpButtonItemEnabled(WMPopUpButton *bPtr, int index, Bool flag);

void WMSetPopUpButtonSelectedItem(WMPopUpButton *bPtr, int index);

int WMGetPopUpButtonSelectedItem(WMPopUpButton *bPtr);

/* ....................................................................... */

WMColorWell *WMCreateColorWell(WMWidget *parent);

void WMSetColorWellColor(WMColorWell *cPtr, WMColor *color);

WMColor *WMGetColorWellColor(WMColorWell *cPtr);


/* ...................................................................... */

WMScrollView *WMCreateScrollView(WMWidget *parent);

void WMResizeScrollViewContent(WMScrollView *sPtr, unsigned int width,
			       unsigned int height);

void WMSetScrollViewHasHorizontalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewHasVerticalScroller(WMScrollView *sPtr, Bool flag);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

void WMSetScrollViewRelief(WMScrollView *sPtr, WMReliefType type);

void WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view);

/* ....................................................................... */

int WMRunAlertPanel(WMScreen *app, char *title, char *msg, 
		    char *defaultButton, char *alternateButton, 
		    char *otherButton);

char *WMRunInputPanel(WMScreen *app, char *title, char *msg, 
		      char *defaultText, char *okButton, char *cancelButton);

WMAlertPanel *WMCreateAlertPanel(WMScreen *app, char *title, char *msg, 
				 char *defaultButton, char *alternateButton, 
				 char *otherButton);

WMInputPanel *WMCreateInputPanel(WMScreen *app, char *title, char *msg, 
				 char *defaultText, char *okButton, 
				 char *cancelButton);

void WMDestroyAlertPanel(WMAlertPanel *panel);

void WMDestroyInputPanel(WMInputPanel *panel);

WMOpenPanel *WMCreateOpenPanel(WMScreen *app);

void WMSetFilePanelCanChooseDirectories(WMFilePanel *panel, int flag);

void WMSetFilePanelCanChooseFiles(WMFilePanel *panel, int flag);

void WMSetFilePanelDirectory(WMFilePanel *panel, char *path);

char *WMGetFilePanelFile(WMFilePanel *panel);

void WMDestroyFilePanel(WMFilePanel *panel);

int WMRunModalOpenPanelForDirectory(WMFilePanel *panel, char *path, char *name,
				    char **fileTypes);


#endif
