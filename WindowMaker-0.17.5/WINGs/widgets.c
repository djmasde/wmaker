

#include "WINGsP.h"

#include <X11/Xutil.h>



/********** data ************/


#define CHECK_BUTTON_ON_WIDTH 	16
#define CHECK_BUTTON_ON_HEIGHT 	16

static char *CHECK_BUTTON_ON[] = {
"               %",
" .............%#",
" ........... .%#",
" .......... #.%#",
" ......... #%.%#",
" ........ #%..%#",
" ... #.. #%...%#",
" ... #% #%....%#",
" ... % #%.....%#",
" ...  #%......%#",
" ... #%.......%#",
" ...#%........%#",
" .............%#",
" .............%#",
" %%%%%%%%%%%%%%#",
"%###############"};

#define CHECK_BUTTON_OFF_WIDTH 	16
#define CHECK_BUTTON_OFF_HEIGHT	16

static char *CHECK_BUTTON_OFF[] = {
"               %",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" .............%#",
" %%%%%%%%%%%%%%#",
"%###############"};

#define RADIO_BUTTON_ON_WIDTH 	15
#define RADIO_BUTTON_ON_HEIGHT	15
static char *RADIO_BUTTON_ON[] = {
".....%%%%%.....",
"...%%#####%%...",
"..%##.....%.%..",
".%#%..    .....",
".%#.        ...",
"%#..        .. ",
"%#.          . ",
"%#.          . ",
"%#.          . ",
"%#.          . ",
".%%.        . .",
".%..        . .",
"..%...    .. ..",
".... .....  ...",
".....     .....",
};

#define RADIO_BUTTON_OFF_WIDTH 	15
#define RADIO_BUTTON_OFF_HEIGHT	15
static char *RADIO_BUTTON_OFF[] = {
".....%%%%%.....",
"...%%#####%%...",
"..%##.......%..",
".%#%...........",
".%#............",
"%#............ ",
"%#............ ",
"%#............ ",
"%#............ ",
"%#............ ",
".%%.......... .",
".%........... .",
"..%......... ..",
".... .....  ...",
".....     .....",
};


static char *BUTTON_ARROW[] = {
"...............",
"....##....#### ",
"...#.%....#... ",
"..#..%#####... ",
".#............ ",
"#............. ",
".#............ ",
"..#..          ",
"...#. .........",
"....# ........."
};

#define BUTTON_ARROW_WIDTH	15
#define BUTTON_ARROW_HEIGHT	10


static char *BUTTON_ARROW2[] = {
"               ",
"    ##    ####.",
"   # %    #   .",
"  #  %#####   .",
" #            .",
"#             .",
" #            .",
"  #  ..........",
"   # .         ",
"    #.         "
};

#define BUTTON_ARROW2_WIDTH	15
#define BUTTON_ARROW2_HEIGHT	10


static char *SCROLLER_DIMPLE[] = {
".%###.",
"%#%%%%",
"#%%...",
"#%..  ",
"#%.   ",
".%.  ."
};

#define SCROLLER_DIMPLE_WIDTH   6
#define SCROLLER_DIMPLE_HEIGHT  6


static char *SCROLLER_ARROW_UP[] = {
"....%....",
"....#....",
"...%#%...",
"...###...",
"..%###%..",
"..#####..",
".%#####%.",
".#######.",
"%#######%"
};

static char *HI_SCROLLER_ARROW_UP[] = {
"    %    ",
"    %    ",
"   %%%   ",
"   %%%   ",
"  %%%%%  ",
"  %%%%%  ",
" %%%%%%% ",
" %%%%%%% ",
"%%%%%%%%%"
};

#define SCROLLER_ARROW_UP_WIDTH   9
#define SCROLLER_ARROW_UP_HEIGHT  9


static char *SCROLLER_ARROW_DOWN[] = {
"%#######%",
".#######.",
".%#####%.",
"..#####..",
"..%###%..",
"...###...",
"...%#%...",
"....#....",
"....%...."
};

static char *HI_SCROLLER_ARROW_DOWN[] = {
"%%%%%%%%%",
" %%%%%%% ",
" %%%%%%% ",
"  %%%%%  ",
"  %%%%%  ",
"   %%%   ",
"   %%%   ",
"    %    ",
"    %    "
};

#define SCROLLER_ARROW_DOWN_WIDTH   9
#define SCROLLER_ARROW_DOWN_HEIGHT  9



static char *SCROLLER_ARROW_LEFT[] = {
"........%",
"......%##",
"....%####",
"..%######",
"%########",
"..%######",
"....%####",
"......%##",
"........%"
};

static char *HI_SCROLLER_ARROW_LEFT[] = {
"        %",
"      %%%",
"    %%%%%",
"  %%%%%%%",
"%%%%%%%%%",
"  %%%%%%%",
"    %%%%%",
"      %%%",
"        %"
};

#define SCROLLER_ARROW_LEFT_WIDTH   9
#define SCROLLER_ARROW_LEFT_HEIGHT  9


static char *SCROLLER_ARROW_RIGHT[] = {
"%........",
"##%......",
"####%....",
"######%..",
"########%",
"######%..",
"####%....",
"##%......",
"%........"
};

static char *HI_SCROLLER_ARROW_RIGHT[] = {
"%        ",
"%%%      ",
"%%%%%    ",
"%%%%%%%  ",
"%%%%%%%%%",
"%%%%%%%  ",
"%%%%%    ",
"%%%      ",
"%        "
};

#define SCROLLER_ARROW_RIGHT_WIDTH   9
#define SCROLLER_ARROW_RIGHT_HEIGHT  9


static char *POPUP_INDICATOR[] = {
"        #==",
" ......%#==",
" ......%#%%",
" ......%#%%",
" %%%%%%%#%%",
"#########%%",
"==%%%%%%%%%",
"==%%%%%%%%%"
};

#define POPUP_INDICATOR_WIDTH	11
#define POPUP_INDICATOR_HEIGHT 	8


#define STIPPLE_WIDTH 8
#define STIPPLE_HEIGHT 8
static unsigned char STIPPLE_BITS[] = {
   0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};



extern W_ViewProcedureTable _WindowViewProcedures;
extern W_ViewProcedureTable _FrameViewProcedures;
extern W_ViewProcedureTable _LabelViewProcedures;
extern W_ViewProcedureTable _ButtonViewProcedures;
extern W_ViewProcedureTable _TextFieldViewProcedures;
extern W_ViewProcedureTable _ScrollerViewProcedures;
extern W_ViewProcedureTable _ScrollViewProcedures;
extern W_ViewProcedureTable _ListViewProcedures;
extern W_ViewProcedureTable _BrowserViewProcedures;
extern W_ViewProcedureTable _PopUpButtonViewProcedures;
extern W_ViewProcedureTable _ColorWellViewProcedures;
extern W_ViewProcedureTable _ScrollViewViewProcedures;

/*
 * All widget classes defined must have an entry here.
 */
W_ViewProcedureTable *procedureTables[16];


/*****  end data  ******/



static void
initProcedureTable()
{
    static int inited = 0;
    
    if (inited)
	return;
    inited = 1;

    procedureTables[WC_Window] = &_WindowViewProcedures;
    procedureTables[WC_Frame] = &_FrameViewProcedures;
    procedureTables[WC_Label] = &_LabelViewProcedures;
    procedureTables[WC_Button] = &_ButtonViewProcedures;
    procedureTables[WC_TextField] = &_TextFieldViewProcedures;
    procedureTables[WC_Scroller] = &_ScrollerViewProcedures;
    procedureTables[WC_List] = &_ListViewProcedures;
    procedureTables[WC_Browser] = &_BrowserViewProcedures;
    procedureTables[WC_PopUpButton] = &_PopUpButtonViewProcedures;
    procedureTables[WC_ColorWell] = &_ColorWellViewProcedures;
    procedureTables[WC_ScrollView] = &_ScrollViewViewProcedures;
}


static void
renderPixmap(W_Screen *screen, Pixmap d, Pixmap mask, char **data, 
	     int width, int height)
{
    int x, y;
    GC whiteGC = W_GC(screen->white);
    GC blackGC = W_GC(screen->black);
    GC lightGC = W_GC(screen->gray);
    GC darkGC = W_GC(screen->darkGray);


    if (mask)
	XSetForeground(screen->display, screen->monoGC,
		       W_PIXEL(screen->black));
	
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    switch (data[y][x]) {		
	     case ' ':
	     case 'w':
		XDrawPoint(screen->display, d, whiteGC, x, y);
		break;

	     case '=':
		if (mask)
		    XDrawPoint(screen->display, mask, screen->monoGC, x, y);
		       
	     case '.':
	     case 'l':
		XDrawPoint(screen->display, d, lightGC, x, y);
		break;
		
	     case '%':
	     case 'd':
		XDrawPoint(screen->display, d, darkGC, x, y);
		break;
		
	     case '#':
	     case 'b':
	     default:
		XDrawPoint(screen->display, d, blackGC, x, y);
		break;
	    }
	}
    }
}



static WMPixmap*
makePixmap(W_Screen *sPtr, char **data, int width, int height, int masked)
{
    Pixmap pixmap, mask = None;
    
    pixmap = XCreatePixmap(sPtr->display, sPtr->rootWin, width, height, 
			   sPtr->depth);
    
    if (masked) {
	mask = XCreatePixmap(sPtr->display, sPtr->rootWin, width, height, 1);
	XSetForeground(sPtr->display, sPtr->monoGC, W_PIXEL(sPtr->white));
	XFillRectangle(sPtr->display, mask, sPtr->monoGC, 0, 0, width, height);
    }

    renderPixmap(sPtr, pixmap, mask, data, width, height);

    return WMCreatePixmapFromXPixmaps(sPtr, pixmap, mask, width, height,
				      sPtr->depth);
}

#ifdef USE_TIFF
#define WINGS_IMAGES_FILE  RESOURCE_PATH"/Images.tiff"
#else
#define WINGS_IMAGES_FILE  RESOURCE_PATH"/Images.xpm"
#endif

static Bool
loadPixmaps(WMScreen *scr)
{
    RImage *image, *tmp;
    Pixmap pixmap;
    RColor gray;
    
    image = RLoadImage(scr->rcontext, WINGS_IMAGES_FILE, 0);
    if (!image) {
	wwarning("WINGs: could not load widget images file");
	return False;
    }
    /* make it have a gray background */
    gray.red = 0xae;
    gray.green = 0xaa;
    gray.blue = 0xae;
    RCombineImageWithColor(image, &gray);
    tmp = RGetSubImage(image, 0, 0, 24, 24);
    if (!RConvertImage(scr->rcontext, tmp, &pixmap)) {
	scr->homeIcon = NULL;
    } else {
	scr->homeIcon = WMCreatePixmapFromXPixmaps(scr, pixmap, None, 24, 24,
						   scr->depth);
    }
    RDestroyImage(tmp);
    
    RDestroyImage(image);
    return True;
}



WMScreen*
WMCreateSimpleApplicationScreen(Display *display, char *applicationName,
				int *argc, char **argv)
{
    WMScreen *scr;
    
    scr = WMCreateScreen(display, DefaultScreen(display), 
			 applicationName, argc, argv);
    
    scr->aflags.hasAppIcon = 0;
    scr->aflags.simpleApplication = 1;
    
    return scr;
}



WMScreen*
WMCreateScreen(Display *display, int screen, char *applicationName,
	       int *argc, char **argv)

{
    return WMCreateScreenWithRContext(display, screen,
				      RCreateContext(display, screen, NULL),
				      applicationName, argc, argv);
}


WMScreen*
WMCreateScreenWithRContext(Display *display, int screen, RContext *context,
			   char *applicationName, int *argc, char **argv)
{
    W_Screen *scrPtr;
    XGCValues gcv;
    Pixmap stipple;

    initProcedureTable();
    
    scrPtr = malloc(sizeof(W_Screen));
    if (!scrPtr)
	return NULL;
    memset(scrPtr, 0, sizeof(W_Screen));
    
    scrPtr->aflags.hasAppIcon = 1;
    
    scrPtr->display = display;
    scrPtr->screen = screen;
    scrPtr->rcontext = context;

    if (argc && argv) {
	int i;

	scrPtr->argc = *argc;
	scrPtr->argv = wmalloc((scrPtr->argc+1)*sizeof(char*));
	for (i=0; i<scrPtr->argc; i++) {
	    scrPtr->argv[i] = wstrdup(argv[i]);
	}
	scrPtr->argv[i] = NULL;
    }
    
    scrPtr->applicationName = wstrdup(applicationName);

    scrPtr->depth = DefaultDepth(display, screen);

    scrPtr->visual = DefaultVisual(display, screen);
    scrPtr->lastEventTime = 0;

    scrPtr->colormap = DefaultColormap(display, screen);

    scrPtr->rootWin = RootWindow(display, screen);


    /* initially allocate some colors */
    WMWhiteColor(scrPtr);
    WMBlackColor(scrPtr);
    WMGrayColor(scrPtr);
    WMDarkGrayColor(scrPtr);
    
    gcv.graphics_exposures = False;
    
    gcv.function = GXxor;
    gcv.foreground = W_PIXEL(scrPtr->white);
    scrPtr->xorGC = XCreateGC(display, scrPtr->rootWin, GCFunction
			      |GCGraphicsExposures|GCForeground, &gcv);

    gcv.function = GXcopy;
    scrPtr->copyGC = XCreateGC(display, scrPtr->rootWin, GCFunction
			       |GCGraphicsExposures, &gcv);

    scrPtr->clipGC = XCreateGC(display, scrPtr->rootWin, GCFunction
			       |GCGraphicsExposures, &gcv);

    
    stipple = XCreateBitmapFromData(display, scrPtr->rootWin, 
				    STIPPLE_BITS, STIPPLE_WIDTH, STIPPLE_HEIGHT);
    gcv.foreground = W_PIXEL(scrPtr->darkGray);
    gcv.background = W_PIXEL(scrPtr->gray);
    gcv.fill_style = FillStippled;
    gcv.stipple = stipple;
    scrPtr->stippleGC = XCreateGC(display, scrPtr->rootWin, 
				  GCForeground|GCBackground|GCStipple
				  |GCFillStyle|GCGraphicsExposures, &gcv);
    
    /* we need a 1bpp drawable for the monoGC, so borrow this one */
    scrPtr->monoGC = XCreateGC(display, stipple, 0, NULL);

    XFreePixmap(display, stipple);
    
    
    scrPtr->normalFont = WMSystemFontOfSize(scrPtr, 12);
    gcv.font = W_FONTID(scrPtr->normalFont);
    gcv.foreground = W_PIXEL(scrPtr->black);
    gcv.background = W_PIXEL(scrPtr->white);
    scrPtr->normalFontGC = XCreateGC(display, scrPtr->rootWin, GCFont
				     |GCForeground|GCGraphicsExposures
				     |GCBackground, &gcv);

    scrPtr->boldFont = WMBoldSystemFontOfSize(scrPtr, 12);
    gcv.font = W_FONTID(scrPtr->boldFont);
    gcv.foreground = W_PIXEL(scrPtr->black);
    scrPtr->boldFontGC = XCreateGC(display, scrPtr->rootWin, GCFont
				   |GCForeground|GCGraphicsExposures, &gcv);

    scrPtr->checkButtonImageOn = makePixmap(scrPtr, CHECK_BUTTON_ON,
					    CHECK_BUTTON_ON_WIDTH,
					    CHECK_BUTTON_ON_HEIGHT, False);

    scrPtr->checkButtonImageOff = makePixmap(scrPtr, CHECK_BUTTON_OFF,
					    CHECK_BUTTON_OFF_WIDTH,
					    CHECK_BUTTON_OFF_HEIGHT, False);

    scrPtr->radioButtonImageOn = makePixmap(scrPtr, RADIO_BUTTON_ON,
					    RADIO_BUTTON_ON_WIDTH,
					    RADIO_BUTTON_ON_HEIGHT, False);

    scrPtr->radioButtonImageOff = makePixmap(scrPtr, RADIO_BUTTON_OFF,
					    RADIO_BUTTON_OFF_WIDTH,
					    RADIO_BUTTON_OFF_HEIGHT, False);

    scrPtr->buttonArrow = makePixmap(scrPtr, BUTTON_ARROW, 
				     BUTTON_ARROW_WIDTH, BUTTON_ARROW_HEIGHT, 
				     False);

    scrPtr->pushedButtonArrow = makePixmap(scrPtr, BUTTON_ARROW2,
			           BUTTON_ARROW2_WIDTH, BUTTON_ARROW2_HEIGHT, 
				   False);


    scrPtr->scrollerDimple = makePixmap(scrPtr, SCROLLER_DIMPLE,
					SCROLLER_DIMPLE_WIDTH,
					SCROLLER_DIMPLE_HEIGHT, False);


    scrPtr->upArrow = makePixmap(scrPtr, SCROLLER_ARROW_UP,
				 SCROLLER_ARROW_UP_WIDTH,
				 SCROLLER_ARROW_UP_HEIGHT, True);

    scrPtr->downArrow = makePixmap(scrPtr, SCROLLER_ARROW_DOWN,
				   SCROLLER_ARROW_DOWN_WIDTH,
				   SCROLLER_ARROW_DOWN_HEIGHT, True);

    scrPtr->leftArrow = makePixmap(scrPtr, SCROLLER_ARROW_LEFT,
				   SCROLLER_ARROW_LEFT_WIDTH,
				   SCROLLER_ARROW_LEFT_HEIGHT, True);

    scrPtr->rightArrow = makePixmap(scrPtr, SCROLLER_ARROW_RIGHT,
				    SCROLLER_ARROW_RIGHT_WIDTH,
				    SCROLLER_ARROW_RIGHT_HEIGHT, True);

    scrPtr->hiUpArrow = makePixmap(scrPtr, HI_SCROLLER_ARROW_UP,
				 SCROLLER_ARROW_UP_WIDTH,
				 SCROLLER_ARROW_UP_HEIGHT, True);

    scrPtr->hiDownArrow = makePixmap(scrPtr, HI_SCROLLER_ARROW_DOWN,
				   SCROLLER_ARROW_DOWN_WIDTH,
				   SCROLLER_ARROW_DOWN_HEIGHT, True);

    scrPtr->hiLeftArrow = makePixmap(scrPtr, HI_SCROLLER_ARROW_LEFT,
				   SCROLLER_ARROW_LEFT_WIDTH,
				   SCROLLER_ARROW_LEFT_HEIGHT, True);

    scrPtr->hiRightArrow = makePixmap(scrPtr, HI_SCROLLER_ARROW_RIGHT,
				    SCROLLER_ARROW_RIGHT_WIDTH,
				    SCROLLER_ARROW_RIGHT_HEIGHT, True);

    scrPtr->popUpIndicator = makePixmap(scrPtr, POPUP_INDICATOR,
					POPUP_INDICATOR_WIDTH,
					POPUP_INDICATOR_HEIGHT, True);

    loadPixmaps(scrPtr);
    
    scrPtr->internalMessage = XInternAtom(display, "_WINGS_MESSAGE", False);

    scrPtr->attribsAtom = XInternAtom(display, "_GNUSTEP_WM_ATTR", False);
    
    scrPtr->deleteWindowAtom = XInternAtom(display, "WM_DELETE_WINDOW", False);

    scrPtr->protocolsAtom = XInternAtom(display, "WM_PROTOCOLS", False);

    scrPtr->rootView = W_CreateRootView(scrPtr);

    
    
    return scrPtr;
}


void 
WMHangData(WMWidget *widget, void *data)
{
    W_VIEW(widget)->hangedData = data;
}


void*
WMGetHangedData(WMWidget *widget)
{
    return W_VIEW(widget)->hangedData;
}



void
WMDestroyWidget(WMWidget *widget)
{
    W_DestroyView(W_VIEW(widget));
}


void
WMSetFocusToWidget(WMWidget *widget)
{
    W_SetFocusToView(W_VIEW(widget));
}


/*
 * WMRealizeWidget-
 * 	Realizes the widget and all it's children.
 * 
 */
void
WMRealizeWidget(WMWidget *w)
{
    if (procedureTables[W_CLASS(w)]->realize)
	(*procedureTables[W_CLASS(w)]->realize)(w);
    else
	W_RealizeView(W_VIEW(w));
}

void
WMMapWidget(WMWidget *w)
{        
    if (!W_VIEW(w)->flags.realized) {
	W_VIEW(w)->flags.mapWhenRealized = 1;
    } else {
	W_MapView(W_VIEW(w));
    }
}


static void
makeChildrenAutomap(W_View *view)
{
    view = view->childrenList;
    
    while (view) {
	view->flags.mapWhenRealized = 1;
	makeChildrenAutomap(view);
	
	view = view->nextSister;
    }
}


void
WMMapSubwidgets(WMWidget *w)
{   
    /* make sure that subwidgets created after the parent was realized
     * are mapped too */
    if (!W_VIEW(w)->flags.realized) {
	makeChildrenAutomap(W_VIEW(w));
    } else {
	W_MapSubviews(W_VIEW(w));
    }
}

void
WMUnmapWidget(WMWidget *w)
{   
    W_UnmapView(W_VIEW(w));
}


void
WMSetWidgetBackgroundColor(WMWidget *w, WMColor *color)
{
    if (procedureTables[W_CLASS(w)]->setBackgroundColor)
	(*procedureTables[W_CLASS(w)]->setBackgroundColor)(w, color);
    else
	W_SetViewBackgroundColor(W_VIEW(w), color);
}

void
WMMoveWidget(WMWidget *w, int x, int y)
{
    if (procedureTables[W_CLASS(w)]->move)
	(*procedureTables[W_CLASS(w)]->move)(w, x, y);
    else
	W_MoveView(W_VIEW(w), x, y);
}

void
WMResizeWidget(WMWidget *w, unsigned int width, unsigned int height)
{
    if (procedureTables[W_CLASS(w)]->resize)
	(*procedureTables[W_CLASS(w)]->resize)(w, width, height);
    else
	W_ResizeView(W_VIEW(w), width, height);
}



RContext*
WMScreenRContext(WMScreen *scr)
{
    return scr->rcontext;
}



unsigned int 
WMWidgetWidth(WMWidget *w)
{
    return W_VIEW(w)->size.width;
}


unsigned int
WMWidgetHeight(WMWidget *w)
{
    return W_VIEW(w)->size.height;
}


Window
WMWidgetXID(WMWidget *w)
{
    return W_VIEW(w)->window;
}


WMScreen*
WMWidgetScreen(WMWidget *w)
{
    return W_VIEW(w)->screen;
}



void
WMScreenMainLoop(WMScreen *scr)
{
    XEvent event;
	
    while (1) {
	WMNextEvent(scr->display, &event);
	WMHandleEvent(&event);
    }
}
