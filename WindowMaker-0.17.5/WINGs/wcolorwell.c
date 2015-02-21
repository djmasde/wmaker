



#include "WINGsP.h"


typedef struct W_ColorWell {
    W_Class widgetClass;
    WMView *view;
    
    WMView *colorView;

    WMColor *color;

    WMAction *action;
    void *clientData;

    struct {
	unsigned int active:1;
	unsigned int bordered:1;
    } flags;
} ColorWell;


static void destroyColorWell(ColorWell *cPtr);
static void paintColorWell(ColorWell *cPtr);

static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);

static void resizeColorWell();

W_ViewProcedureTable _ColorWellViewProcedures = {
    NULL,
	resizeColorWell,
	NULL,
	NULL
};



#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		30
#define DEFAULT_BORDER_WIDTH	6

#define MIN_WIDTH	16
#define MIN_HEIGHT	8



WMColorWell*
WMCreateColorWell(WMWidget *parent)
{
    ColorWell *cPtr;

    cPtr = wmalloc(sizeof(ColorWell));
    memset(cPtr, 0, sizeof(ColorWell));

    cPtr->widgetClass = WC_ColorWell;
    
    cPtr->view = W_CreateView(W_VIEW(parent));
    if (!cPtr->view) {
	free(cPtr);
	return NULL;
    }
    
    cPtr->colorView = W_CreateView(cPtr->view);
    if (!cPtr->colorView) {
	W_DestroyView(cPtr->view);
	free(cPtr);
	return NULL;
    }

    WMCreateEventHandler(cPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, cPtr);
    
    WMCreateEventHandler(cPtr->colorView, ExposureMask, handleEvents, cPtr);

    WMCreateEventHandler(cPtr->view, ButtonPressMask, 
			 handleActionEvents, cPtr);

    cPtr->colorView->flags.mapWhenRealized = 1;
    
    resizeColorWell(cPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    return cPtr;
}


void
WMSetColorWellColor(WMColorWell *cPtr, WMColor *color)
{
    if (cPtr->color)
	WMReleaseColor(cPtr->view->screen, cPtr->color);
    
    cPtr->color = WMRetainColor(color);
    
    if (cPtr->colorView->flags.realized && cPtr->colorView->flags.mapped)
	paintColorWell(cPtr);
}


WMColor*
WMGetColorWellColor(WMColorWell *cPtr)
{
    return cPtr->color;
}

#define MIN(a,b)	((a) > (b) ? (b) : (a))

static void
resizeColorWell(WMColorWell *cPtr, unsigned int width, unsigned int height)
{
    int bw;
    
    if (width < MIN_WIDTH)
	width = MIN_WIDTH;
    if (height < MIN_HEIGHT)
	height = MIN_HEIGHT;


    bw = MIN(width, height)*2/9;
    
    W_ResizeView(cPtr->view, width, height);
    
    W_ResizeView(cPtr->colorView, width-2*bw, height-2*bw);

    if (cPtr->colorView->pos.x!=bw || cPtr->colorView->pos.y!=bw)
	W_MoveView(cPtr->colorView, bw, bw);
}


static void
paintColorWell(ColorWell *cPtr)
{
    W_Screen *scr = cPtr->view->screen;

    W_DrawRelief(scr, cPtr->view->window, 0, 0, cPtr->view->size.width,
		 cPtr->view->size.height, WRRaised);
    
    W_DrawRelief(scr, cPtr->colorView->window, 0, 0, 
		 cPtr->colorView->size.width, cPtr->colorView->size.height, 
		 WRSunken);

    if (cPtr->color)
	WMPaintColorRectangle(scr, cPtr->color, cPtr->colorView->window,
			      2, 2, cPtr->colorView->size.width-4, 
			      cPtr->colorView->size.height-4);
}



static void
handleEvents(XEvent *event, void *data)
{
    ColorWell *cPtr = (ColorWell*)data;

    CHECK_CLASS(data, WC_ColorWell);


    switch (event->type) {	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintColorWell(cPtr);
	break;
	
     case DestroyNotify:
	destroyColorWell(cPtr);
	break;
	
    }
}


static void
handleActionEvents(XEvent *event, void *data)
{
/*    WMColorWell *cPtr = (ColorWell*)data;*/

    CHECK_CLASS(data, WC_ColorWell);


    switch (event->type) {
     case ButtonPress:
	break;
    }
}



static void
destroyColorWell(ColorWell *cPtr)
{
    if (cPtr->color)
	WMReleaseColor(cPtr->view->screen, cPtr->color);
   
    free(cPtr);
}

