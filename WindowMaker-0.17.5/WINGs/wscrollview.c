



#include "WINGsP.h"


typedef struct W_ScrollView {
    W_Class widgetClass;
    WMView *view;

    WMView *contentView;
    WMView *viewport;
    
    WMScroller *vScroller;
    WMScroller *hScroller;
    
    struct {
	WMReliefType relief:3;
	unsigned int hasVScroller:1;
	unsigned int hasHScroller:1;
	
    } flags;
    
} ScrollView;




static void destroyScrollView(ScrollView *sPtr);

static void paintScrollView(ScrollView *sPtr);
static void handleEvents(XEvent *event, void *data);

static void resizeScrollView();


W_ViewProcedureTable _ScrollViewViewProcedures = {
    NULL,
	resizeScrollView,
	NULL,
	NULL
};


	      
WMScrollView*
WMCreateScrollView(WMWidget *parent)
{
    ScrollView *sPtr;

    sPtr = wmalloc(sizeof(ScrollView));
    memset(sPtr, 0, sizeof(ScrollView));

    sPtr->widgetClass = WC_ScrollView;
    
    sPtr->view = W_CreateView(W_VIEW(parent));
    if (!sPtr->view) {
	free(sPtr);
	return NULL;
    }
    sPtr->viewport = W_CreateView(sPtr->view);
    if (!sPtr->view) {
	W_DestroyView(sPtr->view);
	free(sPtr);
	return NULL;
    }
    
    sPtr->viewport->flags.mapWhenRealized = 1;

    WMCreateEventHandler(sPtr->view, StructureNotifyMask|ExposureMask,
			 handleEvents, sPtr);

    
    return sPtr;
}



static void
reorganizeInterior(WMScrollView *sPtr)
{
    int hx, hy, hw;
    int vx, vy, vh;
    int cx, cy, cw, ch;
    
    
    cw = hw = sPtr->view->size.width;
    vh = ch = sPtr->view->size.height;

    if (sPtr->flags.relief == WRSimple) {
	cw -= 2;
	ch -= 2;
	cx = 1;
	cy = 1;	
    } else if (sPtr->flags.relief != WRFlat) {
	cw -= 3;
	ch -= 3;
	cx = 2;
	cy = 2;
    } else {
	cx = 0;
	cy = 0;
    }
    
    if (sPtr->flags.hasHScroller) {
	int h = W_VIEW(sPtr->hScroller)->size.height;

	ch -= h;

	if (sPtr->flags.relief == WRSimple) {
	    hx = 0;
	    hy = sPtr->view->size.height - h;
	} else if (sPtr->flags.relief != WRFlat) {
	    hx = 1;
	    hy = sPtr->view->size.height - h - 1;
	    hw -= 2;
	} else {
	    hx = 0;
	    hy = sPtr->view->size.height - h;
	}
    } else {
	/* make compiler shutup */
	hx = 0;
	hy = 0;
    }
    
    if (sPtr->flags.hasVScroller) {
	int w = W_VIEW(sPtr->vScroller)->size.width;
	cw -= w;
	cx += w;
	hx += w - 1;
	hw -= w - 1;

	if (sPtr->flags.relief == WRSimple) {
	    vx = 0;
	    vy = 0;
	} else if (sPtr->flags.relief != WRFlat) {
	    vx = 1;
	    vy = 1;
	    vh -= 2;
	} else {
	    vx = 0;
	    vy = 0;
	}
    } else {
	/* make compiler shutup */
	vx = 0;
	vy = 0;
    }

    W_ResizeView(sPtr->viewport, cw, ch);
    W_MoveView(sPtr->viewport, cx, cy);
    
    if (sPtr->flags.hasHScroller) {
	WMResizeWidget(sPtr->hScroller, hw, 20);
	WMMoveWidget(sPtr->hScroller, hx, hy);
    }
    if (sPtr->flags.hasVScroller) {
	WMResizeWidget(sPtr->vScroller, 20, vh);
	WMMoveWidget(sPtr->vScroller, vx, vy);
    }
}


static void
resizeScrollView(WMScrollView *sPtr, unsigned int width, unsigned int height)
{
    W_ResizeView(sPtr->view, width, height);

    reorganizeInterior(sPtr);
}



void
WMResizeScrollViewContent(WMScrollView *sPtr, unsigned int width, 
			  unsigned int height)
{
    int w, h, x;

    w = width;
    h = height;

    x = 0;
    if (sPtr->flags.relief == WRSimple) {
	w += 2;
	h += 2;
    } else if (sPtr->flags.relief != WRFlat) {
	w += 4;
	h += 4;
	x = 1;
    }
    
    if (sPtr->flags.hasVScroller) {
	w -= W_VIEW(sPtr->hScroller)->size.width;
	WMResizeWidget(sPtr->vScroller, 20, h);
    }    
    if (sPtr->flags.hasHScroller) {
	h -= W_VIEW(sPtr->hScroller)->size.height;
	WMResizeWidget(sPtr->hScroller, w, 20);
	WMMoveWidget(sPtr->hScroller, x, h);
    }

    W_ResizeView(sPtr->view, w, h);

    W_ResizeView(sPtr->viewport, width, height);
}


void
WMSetScrollViewHasHorizontalScroller(WMScrollView *sPtr, Bool flag)
{
    if (flag) {
	if (sPtr->flags.hasHScroller)
	    return;
	sPtr->flags.hasHScroller = 1;
	
	sPtr->hScroller = WMCreateScroller(sPtr);
	/* make it a horiz. scroller */
	WMResizeWidget(sPtr->hScroller, 2, 1);

	reorganizeInterior(sPtr);

	WMMapWidget(sPtr->hScroller);
    } else {
	if (!sPtr->flags.hasHScroller)
	    return;
	
	WMUnmapWidget(sPtr->hScroller);
	WMDestroyWidget(sPtr->hScroller);
	sPtr->hScroller = NULL;
	sPtr->flags.hasHScroller = 0;
	
	reorganizeInterior(sPtr);
    }
}


void
WMSetScrollViewHasVerticalScroller(WMScrollView *sPtr, Bool flag)
{
    if (flag) {
	if (sPtr->flags.hasVScroller)
	    return;
	sPtr->flags.hasVScroller = 1;
	
	sPtr->vScroller = WMCreateScroller(sPtr);
	/* make it a vert. scroller */
	WMResizeWidget(sPtr->vScroller, 1, 2);

	reorganizeInterior(sPtr);
	
	WMMapWidget(sPtr->vScroller);
    } else {
	if (!sPtr->flags.hasVScroller)
	    return;
	sPtr->flags.hasVScroller = 0;
	
	WMUnmapWidget(sPtr->vScroller);
	WMDestroyWidget(sPtr->vScroller);
	sPtr->vScroller = NULL;

	reorganizeInterior(sPtr);
    }
}


void
WMSetScrollViewContentView(WMScrollView *sPtr, WMView *view)
{
    assert(sPtr->contentView == NULL);
    
    sPtr->contentView = view;

    W_ReparentView(sPtr->contentView, sPtr->viewport);
    
    
}


void
WMSetScrollViewRelief(WMScrollView *sPtr, WMReliefType type)
{
    sPtr->flags.relief = type;
    
    if (sPtr->view->flags.mapped)
	paintScrollView(sPtr);
    
    
}


static void
paintScrollView(ScrollView *sPtr)
{
    W_DrawRelief(sPtr->view->screen, sPtr->view->window, 0, 0,
		 sPtr->view->size.width, sPtr->view->size.height,
		 sPtr->flags.relief);
}


static void
handleEvents(XEvent *event, void *data)
{
    ScrollView *sPtr = (ScrollView*)data;

    CHECK_CLASS(data, WC_ScrollView);

    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintScrollView(sPtr);
	break;
	
     case DestroyNotify:
	destroyScrollView(sPtr);
	break;
	
    }
}


static void
destroyScrollView(ScrollView *sPtr)
{
    
   
    free(sPtr);
}

