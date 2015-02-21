



#include "WINGsP.h"



typedef struct W_Label {
    W_Class widgetClass;
    W_View *view;
	
    char *caption;
    int captionHeight;

    WMColor *textColor;
    GC gc;			       /* for the text */
    WMFont *font;		       /* if NULL, use default */
    
    W_Pixmap *image;

    struct {
	WMReliefType relief:3;
	WMImagePosition imagePosition:4;
	WMAlignment alignment:2;

	unsigned int redrawPending:1;
    } flags;
} Label;



W_ViewProcedureTable _LabelViewProcedures = {
    NULL,
	NULL,
	NULL,
	NULL
};


#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		14
#define DEFAULT_ALIGNMENT	WALeft
#define DEFAULT_RELIEF		WRFlat
#define DEFAULT_IMAGE_POSITION	WIPNoImage


static void destroyLabel(Label *lPtr);
static void paintLabel(Label *lPtr);


static void handleEvents(XEvent *event, void *data);


WMLabel*
WMCreateLabel(WMWidget *parent)
{
    Label *lPtr;
    
    lPtr = wmalloc(sizeof(Label));
    memset(lPtr, 0, sizeof(Label));

    lPtr->widgetClass = WC_Label;
    
    lPtr->view = W_CreateView(W_VIEW(parent));
    if (!lPtr->view) {
	free(lPtr);
	return NULL;
    }
    
    lPtr->textColor = WMRetainColor(lPtr->view->screen->black);
    
    WMCreateEventHandler(lPtr->view, ExposureMask|StructureNotifyMask,
			 handleEvents, lPtr);

    W_ResizeView(lPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    lPtr->flags.alignment = DEFAULT_ALIGNMENT;
    lPtr->flags.relief = DEFAULT_RELIEF;
    lPtr->flags.imagePosition = DEFAULT_IMAGE_POSITION;

    return lPtr;
}


void
WMSetLabelImage(WMLabel *lPtr, WMPixmap *image)
{
    if (lPtr->image!=NULL) 
	WMReleasePixmap(lPtr->image);
    
    if (image)
	lPtr->image = WMRetainPixmap(image);
    else
	lPtr->image = NULL;

    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}

void
WMSetLabelImagePosition(WMLabel *lPtr, WMImagePosition position)
{
    lPtr->flags.imagePosition = position;
    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}

	
void
WMSetLabelTextAlignment(WMLabel *lPtr, WMAlignment alignment)
{
    lPtr->flags.alignment = alignment;
    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}


void
WMSetLabelRelief(WMLabel *lPtr, WMReliefType relief)
{
    lPtr->flags.relief = relief;
    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}
	
void
WMSetLabelText(WMLabel *lPtr, char *text)
{
    if (lPtr->caption)
	free(lPtr->caption);
    
    if (text!=NULL) {
	lPtr->caption = wstrdup(text);
	lPtr->captionHeight =
	    W_GetTextHeight(lPtr->font ? lPtr->font : lPtr->view->screen->normalFont,
			    lPtr->caption, lPtr->view->size.width, True);
    } else {
	lPtr->caption = NULL;
	lPtr->captionHeight = 0;
    }
    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}


void
WMSetLabelFont(WMLabel *lPtr, WMFont *font)
{
    WMScreen *scrPtr = lPtr->view->screen;
	
    if (lPtr->font!=NULL)
	WMReleaseFont(lPtr->font);
    if (font)
	lPtr->font = WMRetainFont(font);
    else
	lPtr->font = NULL;

    if (lPtr->font!=NULL) {
	if (lPtr->gc==NULL) {
	    XGCValues gcv;
	    gcv.foreground = W_PIXEL(lPtr->textColor);
	    gcv.graphics_exposures = False;
	    lPtr->gc = XCreateGC(scrPtr->display, scrPtr->rootWin, 
				 GCForeground|GCGraphicsExposures, &gcv);
	}
	XSetFont(scrPtr->display, lPtr->gc, lPtr->font->font->fid);
    } else {
	if (lPtr->gc != NULL) 
	    XFreeGC(scrPtr->display, lPtr->gc);
	lPtr->gc = NULL;
    }
    if (lPtr->caption) {
	lPtr->captionHeight =
	    W_GetTextHeight(lPtr->font ? lPtr->font : lPtr->view->screen->normalFont,
			    lPtr->caption, lPtr->view->size.width, True);
    }
    
    if (lPtr->view->flags.realized) {
	paintLabel(lPtr);
    }
}


void
WMSetLabelTextColor(WMLabel *lPtr, WMColor *color)
{
    WMScreen *scrPtr = lPtr->view->screen;
    
    WMReleaseColor(scrPtr, lPtr->textColor);
    lPtr->textColor = WMRetainColor(color);
    
    if (lPtr->gc==NULL) {
	XGCValues gcv;
	/* TODO: Add WMFont to the label struct */
	gcv.foreground = W_PIXEL(color);
	gcv.font = scrPtr->normalFont->font->fid;
	gcv.graphics_exposures = False;
	lPtr->gc = XCreateGC(scrPtr->display, scrPtr->rootWin, 
			     GCForeground|GCFont|GCGraphicsExposures, &gcv);
    } else {
	XSetForeground(scrPtr->display, lPtr->gc, W_PIXEL(color));
    }
}


static void
paintLabel(Label *lPtr)
{
    W_Screen *scrPtr = lPtr->view->screen;

    W_PaintTextAndImage(lPtr->view, True,
			(lPtr->gc!=NULL ? lPtr->gc : scrPtr->normalFontGC),
			(lPtr->font!=NULL ? lPtr->font : scrPtr->normalFont),
			lPtr->flags.relief, lPtr->caption, lPtr->captionHeight,
			lPtr->flags.alignment, lPtr->image, 
			lPtr->flags.imagePosition, NULL, 0);
}



static void
handleEvents(XEvent *event, void *data)
{
    Label *lPtr = (Label*)data;

    CHECK_CLASS(data, WC_Label);


    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintLabel(lPtr);
	break;
	
     case DestroyNotify:
	destroyLabel(lPtr);
	break;
    }
}


static void
destroyLabel(Label *lPtr)
{

    WMReleaseColor(lPtr->view->screen, lPtr->textColor);
    
    if (lPtr->caption)
	free(lPtr->caption);

    if (lPtr->gc)
	XFreeGC(lPtr->view->screen->display, lPtr->gc);
    
    if (lPtr->font)
	WMReleaseFont(lPtr->font);

    if (lPtr->image)
	WMReleasePixmap(lPtr->image);

    free(lPtr);
}
