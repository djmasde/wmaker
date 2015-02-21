



#include "WINGsP.h"

/* undefine will disable the autoadjusting of the knob dimple to be
 * directly below the cursor 
 * DOES NOT WORK */
#undef STRICT_NEXT_BEHAVIOUR

#define AUTOSCROLL_INITIAL_DELAY 	200

#define AUTOSCROLL_DELAY 		40


typedef struct W_Scroller {
    W_Class widgetClass;
    W_View *view;

    void *clientData;
    WMAction *action;

    float knobProportion;
    float floatValue;

    WMHandlerID timerID;	       /* for continuous scrolling mode */

#ifndef STRICT_NEXT_BEHAVIOUR
    int dragPoint;		       /* point where the knob is being 
					* dragged */
#endif
    struct {
	WMScrollArrowPosition arrowsPosition:3;

	unsigned int horizontal:1;

	WMScrollerPart hitPart:3;

	/* */	
	unsigned int documentFullyVisible:1;   /* document is fully visible */
	
	unsigned int prevSelected:1;
	
	unsigned int pushed:1;
	
	unsigned int incrDown:1;      /* whether increment button is down */

	unsigned int decrDown:1;
	
	unsigned int draggingKnob:1;

	unsigned int configured:1;

	unsigned int redrawPending:1;
    } flags;
} Scroller;



#define DEFAULT_HEIGHT		60
#define DEFAULT_WIDTH		SCROLLER_WIDTH
#define DEFAULT_ARROWS_POSITION	WSAMinEnd



static void destroyScroller(Scroller *sPtr);
static void paintScroller(Scroller *sPtr);

static void resizeScroller();
static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);

static void handleMotion(Scroller *sPtr, int mouseX, int mouseY);


W_ViewProcedureTable _ScrollerViewProcedures = {
    NULL,
	resizeScroller,
	NULL,
	NULL
};



	      
WMScroller*
WMCreateScroller(WMWidget *parent)
{
    Scroller *sPtr;
    
    sPtr = wmalloc(sizeof(Scroller));
    memset(sPtr, 0, sizeof(Scroller));
    
    sPtr->widgetClass = WC_Scroller;

    sPtr->view = W_CreateView(W_VIEW(parent));
    if (!sPtr->view) {
	free(sPtr);
	return NULL;
    }
    
    sPtr->flags.documentFullyVisible = 1;
    
    WMCreateEventHandler(sPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, sPtr);

    resizeScroller(sPtr, DEFAULT_WIDTH, DEFAULT_WIDTH);
    sPtr->flags.arrowsPosition = DEFAULT_ARROWS_POSITION;

    WMCreateEventHandler(sPtr->view, ButtonPressMask|ButtonReleaseMask
			 |EnterWindowMask|LeaveWindowMask|ButtonMotionMask,
			 handleActionEvents, sPtr);
    
    sPtr->flags.hitPart = WSNoPart;

    return sPtr;
}



void
WMSetScrollerArrowsPosition(WMScroller *sPtr, WMScrollArrowPosition position)
{
    sPtr->flags.arrowsPosition = position;
    if (sPtr->view->flags.realized) {
	paintScroller(sPtr);
    }
}


static void
resizeScroller(WMScroller *sPtr, unsigned int width, unsigned int height)
{
    if (width > height) {
	sPtr->flags.horizontal = 1;
	W_ResizeView(sPtr->view, width, SCROLLER_WIDTH);
    } else {
	sPtr->flags.horizontal = 0;
	W_ResizeView(sPtr->view, SCROLLER_WIDTH, height);
    }
    if (sPtr->view->flags.realized) {
	paintScroller(sPtr);
    }
}


void
WMSetScrollerAction(WMScroller *sPtr, WMAction *action, void *clientData)
{
    CHECK_CLASS(sPtr, WC_Scroller);
    
    sPtr->action = action;
    
    sPtr->clientData = clientData;
}


void
WMSetScrollerParameters(WMScroller *sPtr, float floatValue,
			float knobProportion)
{
    CHECK_CLASS(sPtr, WC_Scroller);

    if (floatValue < 0)
	sPtr->floatValue = 0;
    else if (floatValue > 1)
	sPtr->floatValue = 1;
    else
	sPtr->floatValue = floatValue;

    if (knobProportion <= 0) {
	
	sPtr->knobProportion = 0;
	sPtr->flags.documentFullyVisible = 0;
	
    } else if (knobProportion >= 1) {
	
	sPtr->knobProportion = 1;
	sPtr->flags.documentFullyVisible = 1;
	
    } else {
	sPtr->knobProportion = knobProportion;
	sPtr->flags.documentFullyVisible = 0;
    }

    if (sPtr->view->flags.realized)
	paintScroller(sPtr);
}


float
WMGetScrollerKnobProportion(WMScroller *sPtr)
{
    CHECK_CLASS(sPtr, WC_Scroller);
    
    return sPtr->knobProportion;
}


float
WMGetScrollerFloatValue(WMScroller *sPtr)
{
    CHECK_CLASS(sPtr, WC_Scroller);
    
    return sPtr->floatValue;
}


WMScrollerPart
WMGetScrollerHitPart(WMScroller *sPtr)
{
    CHECK_CLASS(sPtr, WC_Scroller);
    
    return sPtr->flags.hitPart;
}


static void
paintArrow(WMScroller *sPtr, Drawable d, int part)
/*
 * part- 0 paints the decrement arrow, 1 the increment arrow
 */
{
    WMView *view = sPtr->view;
    WMScreen *scr = view->screen;
    int ofs, bsize;
    W_Pixmap *arrow;

#ifndef DOUBLE_BUFFER
    GC gc = scr->lightGC;
#endif
    
    bsize = SCROLLER_WIDTH - 4;


    if (part == 0) { /* decrement button */
	if (sPtr->flags.horizontal) {
	    if (sPtr->flags.arrowsPosition == WSAMaxEnd) {
		ofs = view->size.width - 2*(bsize+1) - 1;
	    } else {
		ofs = 2;
	    }
	    if (sPtr->flags.decrDown)
		arrow = scr->hiLeftArrow;
	    else
		arrow = scr->leftArrow;
	    
	} else {
	    if (sPtr->flags.arrowsPosition == WSAMaxEnd) {
		ofs = view->size.height - 2*(bsize+1) - 1;
	    } else {
		ofs = 2;
	    }
	    if (sPtr->flags.decrDown)
		arrow = scr->hiUpArrow;
	    else
		arrow = scr->upArrow;
	}
	    
#ifndef DOUBLE_BUFFER
	if (sPtr->flags.decrDown)
	    gc = W_GC(scr->white);
#endif
    } else { /* increment button */
	if (sPtr->flags.horizontal) {
	    if (sPtr->flags.arrowsPosition == WSAMaxEnd) {
		ofs = view->size.width - bsize+1 - 3;
	    } else {
		ofs = 2 + bsize+1;
	    }
	    if (sPtr->flags.incrDown)
		arrow = scr->hiRightArrow;
	    else
		arrow = scr->rightArrow;
	} else {
	    if (sPtr->flags.arrowsPosition == WSAMaxEnd) {
		ofs = view->size.height - bsize+1 - 3;
	    } else {
		ofs = 2 + bsize+1;
	    }
	    if (sPtr->flags.incrDown)
		arrow = scr->hiDownArrow;
	    else
		arrow = scr->downArrow;
	}
	
#ifndef DOUBLE_BUFFER
	if (sPtr->flags.incrDown)
	    gc = scr->whiteGC;
#endif
    }
    
	
    if (sPtr->flags.horizontal) {
	    
	/* paint button */
#ifndef DOUBLE_BUFFER
	XFillRectangle(scr->display, d, gc,
		       ofs+1, 2+1, bsize+1-3, bsize-3);
#else
	if ((!part&&sPtr->flags.decrDown) || (part&&sPtr->flags.incrDown))
	    XFillRectangle(scr->display, d, W_GC(scr->white),
			   ofs+1, 2+1, bsize+1-3, bsize-3);
#endif /* DOUBLE_BUFFER */
	W_DrawRelief(scr, d, ofs, 2, bsize, bsize, WRRaised);
	
	/* paint arrow */
	XSetClipMask(scr->display, scr->clipGC, arrow->mask);
	XSetClipOrigin(scr->display, scr->clipGC,
		       ofs + (bsize - arrow->width) / 2, 
		       2 + (bsize - arrow->height) / 2);
	
	XCopyArea(scr->display, arrow->pixmap, d, scr->clipGC,
		  0, 0, arrow->width, arrow->height,
		  ofs + (bsize - arrow->width) / 2, 
		  2 + (bsize - arrow->height) / 2);
	
    } else { /* vertical */
	
	/* paint button */
#ifndef DOUBLE_BUFFER
	XFillRectangle(scr->display, d, gc,
		       2+1, ofs+1, bsize-3, bsize+1-3);
#else
	if ((!part&&sPtr->flags.decrDown) || (part&&sPtr->flags.incrDown))
	    XFillRectangle(scr->display, d, W_GC(scr->white),
			   2+1, ofs+1, bsize-3, bsize+1-3);
#endif /* DOUBLE_BUFFER */
	W_DrawRelief(scr, d, 2, ofs, bsize, bsize, WRRaised);
	
	/* paint arrow */
	
	XSetClipMask(scr->display, scr->clipGC, arrow->mask);
	XSetClipOrigin(scr->display, scr->clipGC, 
		       2 + (bsize - arrow->width) / 2, 
		       ofs + (bsize - arrow->height) / 2);
	XCopyArea(scr->display, arrow->pixmap, d, scr->clipGC,
		  0, 0, arrow->width, arrow->height,
		  2 + (bsize - arrow->width) / 2, 
		  ofs + (bsize - arrow->height) / 2);
    }
}    


static int
knobLength(Scroller *sPtr)
{
    int tmp, length;

    
    if (sPtr->flags.horizontal)
	length = sPtr->view->size.width - 4;
    else
	length = sPtr->view->size.height - 4;

    if (sPtr->flags.arrowsPosition==WSAMaxEnd) {
	length -= (SCROLLER_WIDTH - 4 + 1)*2;
    } else if (sPtr->flags.arrowsPosition==WSAMinEnd) {
	length -= (SCROLLER_WIDTH - 4 + 1)*2;
    }
    
    tmp = (int)((float)length * sPtr->knobProportion + 0.5);
    /* keep minimum size */
    if (tmp < SCROLLER_WIDTH-4)
	tmp = SCROLLER_WIDTH-4;
    
    return tmp;
}


static void
paintScroller(Scroller *sPtr)
{
    WMView *view = sPtr->view;
    WMScreen *scr = view->screen;
#ifdef DOUBLE_BUFFER
    Pixmap d;
#else
    Drawable d = view->window;
#endif
    int length, ofs;
    float knobP, knobL;
    
    
#ifdef DOUBLE_BUFFER
    d = XCreatePixmap(scr->display, view->window, view->size.width, 
		      view->size.height, scr->depth);
    XFillRectangle(scr->display, d, W_GC(scr->gray), 0, 0, 
		   view->size.width, view->size.height);
#endif
    
    XDrawRectangle(scr->display, d, W_GC(scr->black), 0, 0,
		   view->size.width-1, view->size.height-1);
#ifndef DOUBLE_BUFFER
    XDrawRectangle(scr->display, d, W_GC(scr->gray), 1, 1,
		   view->size.width-3, view->size.height-3);
#endif

    if (sPtr->flags.horizontal)
	length = view->size.width - 4;
    else
	length = view->size.height - 4;
    
    if (sPtr->flags.documentFullyVisible) {
	XFillRectangle(scr->display, d, scr->stippleGC, 2, 2,
		       view->size.width-4, view->size.height-4);
    } else {
	if (sPtr->flags.arrowsPosition==WSAMaxEnd) {
	    ofs = 0;
	    length -= (SCROLLER_WIDTH - 4 + 1)*2;
	} else if (sPtr->flags.arrowsPosition==WSAMinEnd) {	
	    ofs = (SCROLLER_WIDTH - 4 + 1)*2;
	    length -= (SCROLLER_WIDTH - 4 + 1)*2;
	} else {
	    ofs = 0;
	}
	
	knobL = knobLength(sPtr);

	knobP = sPtr->floatValue * ((float)length - knobL);

	
	if (sPtr->flags.horizontal) {
	    /* before */
	    XFillRectangle(scr->display, d, scr->stippleGC,
			   ofs+2, 2, (int)knobP, view->size.height-4);
	    
	    /* knob */
#ifndef DOUBLE_BUFFER
	    XFillRectangle(scr->display, d, scr->lightGC,
			   ofs+2+(int)knobP+2, 2+2, (int)knobL-4,
			   view->size.height-4-4);
#endif
	    W_DrawRelief(scr, d, ofs+2+(int)knobP, 2, (int)knobL,
			 view->size.height-4, WRRaised);
	    
	    XCopyArea(scr->display, scr->scrollerDimple->pixmap, d, 
		      scr->copyGC, 0, 0,
		      scr->scrollerDimple->width, scr->scrollerDimple->height,
		      ofs+2+knobP+(int)(knobL-scr->scrollerDimple->width-1)/2,
		      (view->size.height-scr->scrollerDimple->height-1)/2);
	    
	    /* after */
	    if ((int)(knobP+knobL) < length)
		XFillRectangle(scr->display, d, scr->stippleGC,
			       ofs+2+(int)(knobP+knobL), 2,
			       length-(int)(knobP+knobL),
			       view->size.height-4);
	} else {
	    /* before */
	    if (knobP>0)
		XFillRectangle(scr->display, d, scr->stippleGC,
			       2, ofs+2, view->size.width-4, (int)knobP);
	    
	    /* knob */
#ifndef DOUBLE_BUFFER
	    XFillRectangle(scr->display, d, scr->lightGC,
			   2+2, ofs+2+(int)knobP+2,
			   view->size.width-4-4, (int)knobL-4);
#endif
	    XCopyArea(scr->display, scr->scrollerDimple->pixmap, d, 
		      scr->copyGC, 0, 0,
		      scr->scrollerDimple->width, scr->scrollerDimple->height,
		      (view->size.width-scr->scrollerDimple->width-1)/2,
		      ofs+2+knobP+(int)(knobL-scr->scrollerDimple->height-1)/2);

	    W_DrawRelief(scr, d, 2, ofs+2+(int)knobP,
			 view->size.width-4, (int)knobL, WRRaised);

	    /* after */
	    if ((int)(knobP+knobL) < length)
		XFillRectangle(scr->display, d, scr->stippleGC,
			       2, ofs+2+(int)(knobP+knobL),
			       view->size.width-4, 
			       length-(int)(knobP+knobL));
	}
	
	paintArrow(sPtr, d, 0);
	paintArrow(sPtr, d, 1);
    }

#ifdef DOUBLE_BUFFER
    XCopyArea(scr->display, d, view->window, scr->copyGC, 0, 0, 
	      view->size.width, view->size.height, 0, 0);
    XFreePixmap(scr->display, d);
#endif
}



static void
handleEvents(XEvent *event, void *data)
{
    Scroller *sPtr = (Scroller*)data;

    CHECK_CLASS(data, WC_Scroller);


    switch (event->type) {
     case Expose:
	if (event->xexpose.count==0)
	    paintScroller(sPtr);
	break;
	
     case DestroyNotify:
	destroyScroller(sPtr);
	break;
    }
}



/*
 * locatePointInScroller-
 *     Return the part of the scroller where the point is located.
 */
static WMScrollerPart
locatePointInScroller(Scroller *sPtr, int x, int y, int alternate)
{
    int width = sPtr->view->size.width;
    int height = sPtr->view->size.height;
    int c, p1, p2, p3, p4, p5, p6;
    int knobL, slotL;


    /* if there is no knob... */
    if (sPtr->flags.documentFullyVisible)
	return WSKnobSlot;
    
    if (sPtr->flags.horizontal)
	c = x;
    else
	c = y;

    /*     p1  p2           p3           p4   p5   p6
     * |   |   |###########|             |#####|   |   |   
     * | < | > |###########|      O      |#####| < | > |
     * |   |   |###########|             |#####|   |   |
     */
    
    if (sPtr->flags.arrowsPosition == WSAMinEnd) {
	p1 = 18;
	p2 = 36;

	if (sPtr->flags.horizontal) {
	    slotL = width - 36;
	    p5 = width;
	} else {
	    slotL = height - 36;
	    p5 = height;
	}
	p6 = p5;
    } else if (sPtr->flags.arrowsPosition == WSAMaxEnd) {
	if (sPtr->flags.horizontal) {
	    slotL = width - 36;
	    p6 = width - 18;
	} else {
	    slotL = height - 36;
	    p6 = height - 18;
	}
	p5 = p6 - 18;
	
	p1 = p2 = 0;
    } else {
	/* no arrows */
	p1 = p2 = 0;

	if (sPtr->flags.horizontal) {
	    slotL = p5 = p6 = width;
	} else {
	    slotL = p5 = p6 = height;
	}
    }
    
    knobL = knobLength(sPtr);
    p3 = p2 + (int)((float)(slotL-knobL) * sPtr->floatValue);
    p4 = p3 + knobL;

    /* uses a mix of the NS and Win ways of doing scroll page */
    if (c <= p1)
	return alternate ? WSDecrementPage : WSDecrementLine;
    else if (c <= p2)
	return alternate ? WSIncrementPage : WSIncrementLine;
    else if (c <= p3)
	return WSDecrementPage;
    else if (c <= p4)
	return WSKnob;
    else if (c <= p5)
	return WSIncrementPage;
    else if (c <= p6)
	return alternate ? WSDecrementPage : WSDecrementLine;
    else
	return alternate ? WSIncrementPage : WSIncrementLine;
}



static void
handlePush(Scroller *sPtr, int pushX, int pushY, int alternate)
{
    WMScrollerPart part;
    int doAction = 0;
    
    part = locatePointInScroller(sPtr, pushX, pushY, alternate);
    
    sPtr->flags.hitPart = part;
    
    switch (part) {
     case WSIncrementLine:
	sPtr->flags.incrDown = 1;
	doAction = 1;
	break;

     case WSIncrementPage:
	doAction = 1;
	break;
	
     case WSDecrementLine:
	sPtr->flags.decrDown = 1;
	doAction = 1;
	break;
	
     case WSDecrementPage:
	doAction = 1;
	break;
	
     case WSKnob:
	sPtr->flags.draggingKnob = 1;
#ifndef STRICT_NEXT_BEHAVIOUR
	if (sPtr->flags.horizontal)
	    sPtr->dragPoint = pushX;
	else
	    sPtr->dragPoint = pushY;

	{
	    int noButtons = (sPtr->flags.arrowsPosition == WSANone);
	    int length, knobP;

	    if (sPtr->flags.horizontal)
		length = sPtr->view->size.width - 4;
	    else
		length = sPtr->view->size.height - 4;

	    if (!noButtons)
		length -= 36;

	    knobP = sPtr->floatValue * (length - knobLength(sPtr));

	    if (sPtr->flags.arrowsPosition == WSAMinEnd)
		sPtr->dragPoint -= 2 + (noButtons ? 0 : 36) + knobP;
	    else 
		sPtr->dragPoint -= 2 + knobP;
	}
#endif /* STRICT_NEXT_BEHAVIOUR */
	handleMotion(sPtr, pushX, pushY);
	break;

     case WSKnobSlot:
     case WSNoPart:
	/* dummy */
	break;
    }
    
    if (doAction && sPtr->action) {
	(*sPtr->action)(sPtr, sPtr->clientData);
    }
}


static float
floatValueForPoint(int slotOfs, int slotLength, int knobLength, int point)
{
    float floatValue = 0;
    float position;

#ifdef STRICT_NEXT_BEHAVIOUR
    if (point < slotOfs + knobLength/2)
	position = slotOfs + knobLength/2;
    else if (point > slotOfs + slotLength - knobLength/2)
	position = slotOfs + slotLength - knobLength/2;
    else
	position = point;
    
    floatValue = (position - (slotOfs+slotLength/2)) / (float)(slotLength-knobLength);
#else
    /* Adjust the last point to lie inside the knob slot */    
    if (point < slotOfs)
	position = slotOfs;
    else if (point > slotOfs + slotLength)
	position = slotOfs + slotLength;
    else
	position = point;

    /* Compute the float value */
    floatValue = (position - slotOfs) / (slotLength - knobLength);
#endif
    
    return floatValue;
}


static void
handleMotion(Scroller *sPtr, int mouseX, int mouseY)
{
    int slotOffset;
    int slotLength;
    int noButtons = (sPtr->flags.arrowsPosition == WSANone);
    
    if (sPtr->flags.arrowsPosition == WSAMinEnd)
	slotOffset = 2 + (noButtons ? 0 : 36);
    else 
	slotOffset = 2;

    if (sPtr->flags.draggingKnob) {
	float newFloatValue;	
#ifdef STRICT_NEXT_BEHAVIOUR
	if (sPtr->flags.horizontal) {
	    slotLength = sPtr->view->size.width-4-(noButtons ? 0 : 36);
	    newFloatValue = floatValueForPoint(slotOffset, slotLength,
				       (int)(slotLength*sPtr->knobProportion),
				       mouseX);
	} else {
	    slotLength = sPtr->view->size.height-4-(noButtons ? 0 : 36);
	    newFloatValue = floatValueForPoint(slotOffset, slotLength,
				       (int)(slotLength*sPtr->knobProportion),
				       mouseY);
	}
#else
	if (sPtr->flags.horizontal) {
	    slotLength = sPtr->view->size.width-4-(noButtons ? 0 : 36);
	    newFloatValue = floatValueForPoint(slotOffset, slotLength,
				       (int)(slotLength*sPtr->knobProportion),
				       mouseX-sPtr->dragPoint);
	} else {
	    slotLength = sPtr->view->size.height-4-(noButtons ? 0 : 36);
	    newFloatValue = floatValueForPoint(slotOffset, slotLength,
				       (int)(slotLength*sPtr->knobProportion),
				       mouseY-sPtr->dragPoint);
	}
#endif /* !STRICT_NEXT_BEHAVIOUR */
	WMSetScrollerParameters(sPtr, newFloatValue, sPtr->knobProportion);
	if (sPtr->action) {
	    (*sPtr->action)(sPtr, sPtr->clientData);
	}
    } else {
	int part;
	
	part = locatePointInScroller(sPtr, mouseX, mouseY, False);

	sPtr->flags.hitPart = part;

	if (part == WSIncrementLine && sPtr->flags.decrDown) {
	    sPtr->flags.decrDown = 0;
	    sPtr->flags.incrDown = 1;
	} else if (part == WSDecrementLine && sPtr->flags.incrDown) {
	    sPtr->flags.incrDown = 0;
	    sPtr->flags.decrDown = 1;
	} else if (part != WSIncrementLine && part != WSDecrementLine) {
	    sPtr->flags.incrDown = 0;
	    sPtr->flags.decrDown = 0;
	}
    } 
}


static void
autoScroll(void *clientData)
{
    Scroller *sPtr = (Scroller*)clientData;
    
    if (sPtr->action) {
	(*sPtr->action)(sPtr, sPtr->clientData);
    }
    sPtr->timerID= WMAddTimerHandler(AUTOSCROLL_DELAY, autoScroll, clientData);
}


static void
handleActionEvents(XEvent *event, void *data)
{
    Scroller *sPtr = (Scroller*)data;
    int id, dd;

    
    /* check if we're really dealing with a scroller, as something
     * might have gone wrong in the event dispatching stuff */
    CHECK_CLASS(sPtr, WC_Scroller);
    
    id = sPtr->flags.incrDown;
    dd = sPtr->flags.decrDown;
    
    switch (event->type) {
     case EnterNotify:
	
	break;
	
     case LeaveNotify:
	if (sPtr->timerID) {
	    WMDeleteTimerHandler(sPtr->timerID);
	    sPtr->timerID = NULL;
	}
	sPtr->flags.incrDown = 0;
	sPtr->flags.decrDown = 0;
	break;
	
     case ButtonPress:
	/* FIXME: change Mod1Mask with something else */
	handlePush(sPtr, event->xbutton.x, event->xbutton.y,
		   (event->xbutton.state & Mod1Mask)
		   ||event->xbutton.button==Button2);
	/* continue scrolling if pushed on the buttons */
	if (sPtr->flags.hitPart == WSIncrementLine
	    || sPtr->flags.hitPart == WSDecrementLine) {
	    sPtr->timerID = WMAddTimerHandler(AUTOSCROLL_INITIAL_DELAY,
					      autoScroll, sPtr);
	}
	break;

     case ButtonRelease:
	if (sPtr->flags.draggingKnob) {
	    if (sPtr->action) {
		(*sPtr->action)(sPtr, sPtr->clientData);
	    }
	}
	if (sPtr->timerID) {
	    WMDeleteTimerHandler(sPtr->timerID);
	    sPtr->timerID = NULL;
	}
	sPtr->flags.incrDown = 0;
	sPtr->flags.decrDown = 0;
	sPtr->flags.draggingKnob = 0;
	break;

     case MotionNotify:
	handleMotion(sPtr, event->xbutton.x, event->xbutton.y);
	if (sPtr->timerID && sPtr->flags.hitPart != WSIncrementLine
	    && sPtr->flags.hitPart != WSDecrementLine) {
	    WMDeleteTimerHandler(sPtr->timerID);
	    sPtr->timerID = NULL;
	}
	break;
    }
    if (id != sPtr->flags.incrDown || dd != sPtr->flags.decrDown)
	paintScroller(sPtr);
}



static void
destroyScroller(Scroller *sPtr)
{
    /* we don't want autoscroll try to scroll a freed widget */
    if (sPtr->timerID) {
	WMDeleteTimerHandler(sPtr->timerID);
    }

    free(sPtr);
}

