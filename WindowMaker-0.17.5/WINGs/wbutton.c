



#include "WINGsP.h"

typedef struct W_Button {
    W_Class widgetClass;
    WMView *view;

    char *caption;
    int captionHeight;
    
    char *altCaption;
    int altCaptionHeight;
    
    W_Pixmap *image;
    W_Pixmap *altImage;

    void *clientData;
    WMAction *action;
    
    int tag;
    
    int groupIndex;
    
    struct {
	WMButtonType type:4;
	WMImagePosition imagePosition:4;
	WMAlignment alignment:2;
	
	unsigned int selected:1;
	
	unsigned int disabled:1;

	unsigned int bordered:1;

	unsigned int springLoaded:1;
	
	unsigned int pushIn:1;	       /* change relief while pushed */
	
	unsigned int pushLight:1;      /* highlight while pushed */
	
	unsigned int pushChange:1;     /* change caption while pushed */
	    
	unsigned int stateLight:1;     /* state indicated by highlight */
	
	unsigned int stateChange:1;    /* state indicated by caption change */
	
	unsigned int statePush:1;      /* state indicated by relief */
	
	/* */
	unsigned int prevSelected:1;

	unsigned int pushed:1;
	
	unsigned int wasPushed:1;

	unsigned int redrawPending:1;
    } flags;
} Button;



#define DEFAULT_BUTTON_WIDTH	60
#define DEFAULT_BUTTON_HEIGHT	24
#define DEFAULT_BUTTON_ALIGNMENT	WACenter
#define DEFAULT_BUTTON_IS_BORDERED	True


#define DEFAULT_RADIO_WIDTH	100
#define DEFAULT_RADIO_HEIGHT	20
#define DEFAULT_RADIO_ALIGNMENT	WALeft
#define DEFAULT_RADIO_IMAGE_POSITION	WIPLeft
#define DEFAULT_RADIO_TEXT	"Radio"


#define DEFAULT_SWITCH_WIDTH	100
#define DEFAULT_SWITCH_HEIGHT	20
#define DEFAULT_SWITCH_ALIGNMENT	WALeft
#define DEFAULT_SWITCH_IMAGE_POSITION	WIPLeft
#define DEFAULT_SWITCH_TEXT	"Switch"


static void destroyButton(Button *bPtr);
static void paintButton(Button *bPtr);

static void handleEvents(XEvent *event, void *data);
static void handleActionEvents(XEvent *event, void *data);

static void resizeButton();

W_ViewProcedureTable _ButtonViewProcedures = {
    NULL,
	resizeButton,
	NULL,
	NULL
};



WMButton*
WMCreateCustomButton(WMWidget *parent, int behaviourMask)
{
    Button *bPtr;
    
    bPtr = wmalloc(sizeof(Button));
    memset(bPtr, 0, sizeof(Button));

    bPtr->widgetClass = WC_Button;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }

    bPtr->flags.type = 0;

    bPtr->flags.springLoaded = (behaviourMask & WBBSpringLoadedMask)!=0;
    bPtr->flags.pushIn = (behaviourMask & WBBPushInMask)!=0;
    bPtr->flags.pushChange = (behaviourMask & WBBPushChangeMask)!=0;
    bPtr->flags.pushLight = (behaviourMask & WBBPushLightMask)!=0;
    bPtr->flags.stateLight = (behaviourMask & WBBStateLightMask)!=0;
    bPtr->flags.stateChange = (behaviourMask & WBBStateChangeMask)!=0;
    bPtr->flags.statePush = (behaviourMask & WBBStatePushMask)!=0;

    W_ResizeView(bPtr->view, DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT);
    bPtr->flags.alignment = DEFAULT_BUTTON_ALIGNMENT;
    bPtr->flags.bordered = DEFAULT_BUTTON_IS_BORDERED;

    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, bPtr);

    WMCreateEventHandler(bPtr->view, ButtonPressMask|ButtonReleaseMask
			 |EnterWindowMask|LeaveWindowMask, 
			 handleActionEvents, bPtr);

    W_ResizeView(bPtr->view, DEFAULT_BUTTON_WIDTH, DEFAULT_BUTTON_HEIGHT);
    bPtr->flags.alignment = DEFAULT_BUTTON_ALIGNMENT;
    bPtr->flags.bordered = DEFAULT_BUTTON_IS_BORDERED;

    return bPtr;
}



WMButton*
WMCreateButton(WMWidget *parent, WMButtonType type)
{
    W_Screen *scrPtr = W_VIEW(parent)->screen;
    Button *bPtr;
    
    switch (type) {
     case WBTMomentaryPush:
	bPtr = WMCreateCustomButton(parent, WBBSpringLoadedMask
				    |WBBPushInMask|WBBPushLightMask);
	break;

     case WBTMomentaryChange:
	bPtr = WMCreateCustomButton(parent, WBBSpringLoadedMask
				    |WBBPushChangeMask);
	break;
		
     case WBTPushOnPushOff:
	bPtr = WMCreateCustomButton(parent, WBBPushInMask|WBBStatePushMask
				    |WBBStateLightMask);
	break;
	
     case WBTToggle:
	bPtr = WMCreateCustomButton(parent, WBBPushInMask|WBBStateChangeMask
				    |WBBStatePushMask);
	break;

     case WBTOnOff:
	bPtr = WMCreateCustomButton(parent, WBBStateLightMask);
	break;

     case WBTSwitch:
	bPtr = WMCreateCustomButton(parent, WBBStateChangeMask);
	bPtr->flags.bordered = 0;
	bPtr->image = WMRetainPixmap(scrPtr->checkButtonImageOff);
	bPtr->altImage = WMRetainPixmap(scrPtr->checkButtonImageOn);
	break;

     case WBTRadio:
	bPtr = WMCreateCustomButton(parent, WBBStateChangeMask);
	bPtr->flags.bordered = 0;
	bPtr->image = WMRetainPixmap(scrPtr->radioButtonImageOff);
	bPtr->altImage = WMRetainPixmap(scrPtr->radioButtonImageOn);
	break;

     default:
     case WBTMomentaryLight:
	bPtr = WMCreateCustomButton(parent, WBBSpringLoadedMask
				    |WBBPushLightMask);
	bPtr->flags.bordered = 1;
	break;
    }
    
    bPtr->flags.type = type;

    if (type==WBTRadio) {
	W_ResizeView(bPtr->view, DEFAULT_RADIO_WIDTH, DEFAULT_RADIO_HEIGHT);
	WMSetButtonText(bPtr, DEFAULT_RADIO_TEXT);
	bPtr->flags.alignment = DEFAULT_RADIO_ALIGNMENT;
	bPtr->flags.imagePosition = DEFAULT_RADIO_IMAGE_POSITION;
    } else if (type==WBTSwitch) {
	W_ResizeView(bPtr->view, DEFAULT_SWITCH_WIDTH, DEFAULT_SWITCH_HEIGHT);
	WMSetButtonText(bPtr, DEFAULT_SWITCH_TEXT);
	bPtr->flags.alignment = DEFAULT_SWITCH_ALIGNMENT;
	bPtr->flags.imagePosition = DEFAULT_SWITCH_IMAGE_POSITION;
    }
    
    return bPtr;
}


void
WMSetButtonImage(WMButton *bPtr, WMPixmap *image)
{
    if (bPtr->image!=NULL)
	WMReleasePixmap(bPtr->image);
    bPtr->image = WMRetainPixmap(image);
    
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonAltImage(WMButton *bPtr, WMPixmap *image)
{
    if (bPtr->altImage!=NULL)
	WMReleasePixmap(bPtr->altImage);
    bPtr->altImage = WMRetainPixmap(image);
    
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonImagePosition(WMButton *bPtr, WMImagePosition position)
{
    bPtr->flags.imagePosition = position;
    
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonTextAlignment(WMButton *bPtr, WMAlignment alignment)
{
    bPtr->flags.alignment = alignment;
    
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonText(WMButton *bPtr, char *text)
{
    if (bPtr->caption)
	free(bPtr->caption);

    if (text!=NULL) {
	bPtr->caption = wstrdup(text);
	bPtr->captionHeight =
	    W_GetTextHeight(bPtr->view->screen->normalFont, text,
			    bPtr->view->size.width, True);
    } else {
	bPtr->caption = NULL;
	bPtr->captionHeight = 0;
    }
    
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonAltText(WMButton *bPtr, char *text)
{
    if (bPtr->altCaption)
	free(bPtr->altCaption);
    
    if (text!=NULL) {
	bPtr->altCaption = wstrdup(text);
	bPtr->altCaptionHeight =
	    W_GetTextHeight(bPtr->view->screen->normalFont, text,
			    bPtr->view->size.width, True);
    } else {
	bPtr->altCaption = NULL;
	bPtr->altCaptionHeight = 0;
    }
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonSelected(WMButton *bPtr, int isSelected)
{
    bPtr->flags.selected = isSelected;
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


int
WMGetButtonSelected(WMButton *bPtr)
{
    CHECK_CLASS(bPtr, WC_Button);
    
    return bPtr->flags.selected;
}


void
WMSetButtonBordered(WMButton *bPtr, int isBordered)
{
    bPtr->flags.bordered = isBordered;
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}


void
WMSetButtonDisabled(WMButton *bPtr, int isDisabled)
{
    bPtr->flags.disabled = isDisabled;
    
    if (bPtr->view->flags.realized) {
	paintButton(bPtr);
    }
}	
 

void
WMSetButtonTag(WMButton *bPtr, int tag)
{
    bPtr->tag = tag;
}


void
WMPerformButtonClick(WMButton *bPtr)
{
    XEvent event;
    
    CHECK_CLASS(bPtr, WC_Button);
    
    event.type = ButtonPress;
    event.xbutton.button = Button1;
    handleActionEvents(&event, bPtr);
    
    
    
    event.type = ButtonRelease;
    event.xbutton.button = Button1;
    handleActionEvents(&event, bPtr);
}



void
WMSetButtonAction(WMButton *bPtr, WMAction *action, void *clientData)
{
    CHECK_CLASS(bPtr, WC_Button);
    
    bPtr->action = action;
    
    bPtr->clientData = clientData;
}


void
WMGroupButtons(WMButton *bPtr, WMButton *newMember)
{
    CHECK_CLASS(bPtr, WC_Button);
    CHECK_CLASS(newMember, WC_Button);
    
    if (bPtr->groupIndex==0) {
	bPtr->groupIndex = ++bPtr->view->screen->tagIndex;
    }
    newMember->groupIndex = bPtr->groupIndex;
}


static void
resizeButton(WMButton *bPtr, unsigned int width, unsigned int height)
{
    W_ResizeView(bPtr->view, width, height);
    
    if (bPtr->caption) {
	bPtr->captionHeight =
	    W_GetTextHeight(bPtr->view->screen->normalFont, bPtr->caption,
			    bPtr->view->size.width, True);
    } else {
	bPtr->captionHeight = 0;
    }
    if (bPtr->altCaption) {
	bPtr->altCaptionHeight =
	    W_GetTextHeight(bPtr->view->screen->normalFont, bPtr->altCaption,
			    bPtr->view->size.width, True);
    } else {
	bPtr->altCaptionHeight = 0;
    }
}


static void
paintButton(Button *bPtr)
{
    W_Screen *scrPtr = bPtr->view->screen;
    GC gc;
    WMReliefType relief;
    int offset;
    char *caption;
    int captionHeight;
    WMPixmap *image;

    gc = NULL;
    caption = bPtr->caption;
    captionHeight = bPtr->captionHeight;
    image = bPtr->image;
    offset = 0;
    if (bPtr->flags.bordered)
	relief = WRRaised;
    else
	relief = WRFlat;

    if (bPtr->flags.selected) {
	if (bPtr->flags.stateLight)
	    gc = W_GC(scrPtr->white);

	if (bPtr->flags.stateChange) {
	    if (bPtr->altCaption) {
		caption = bPtr->altCaption;
		captionHeight = bPtr->altCaptionHeight;
	    }
	    if (bPtr->altImage)
		image = bPtr->altImage;
	}
	
	if (bPtr->flags.statePush && bPtr->flags.bordered) {
	    relief = WRSunken;
	    offset = 1;
	}
    }
        
    if (bPtr->flags.pushed) {
	if (bPtr->flags.pushIn) {
	    relief = WRSunken;
	    offset = 1;
	}
	if (bPtr->flags.pushLight)
	    gc = W_GC(scrPtr->white);
	
	if (bPtr->flags.pushChange) {
	    if (bPtr->altCaption) {
		caption = bPtr->altCaption;
		captionHeight = bPtr->altCaptionHeight;
	    }
	    if (bPtr->altImage)
		image = bPtr->altImage;
	}
    }
    

    if (bPtr->flags.disabled)
	XSetForeground(scrPtr->display, scrPtr->normalFontGC,
		       W_PIXEL(scrPtr->darkGray));

    W_PaintTextAndImage(bPtr->view, False, scrPtr->normalFontGC,
			scrPtr->normalFont, relief, caption, captionHeight,
			bPtr->flags.alignment, image, 
			bPtr->flags.imagePosition, gc, offset);
    
    if (bPtr->flags.disabled)
	XSetForeground(scrPtr->display, scrPtr->normalFontGC,
		       W_PIXEL(scrPtr->black));
}



static void
handleEvents(XEvent *event, void *data)
{
    Button *bPtr = (Button*)data;

    CHECK_CLASS(data, WC_Button);


    switch (event->type) {
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintButton(bPtr);
	break;
	
     case DestroyNotify:
	destroyButton(bPtr);
	break;
	
     case ClientMessage:
	if (event->xclient.message_type==bPtr->view->screen->internalMessage) {
	    if (event->xclient.data.l[1]==WM_RADIO_PRESS
		&& event->xclient.data.l[2]==bPtr->groupIndex) {
		/* some other button was pushed down */
		if (bPtr->flags.selected 
		    && event->xclient.data.l[0]!=bPtr->view->window) {
		    bPtr->flags.selected = 0;
		    paintButton(bPtr);
		}
	    }
	}
	break;
    }
}


static void
handleActionEvents(XEvent *event, void *data)
{
    Button *bPtr = (Button*)data;
    int doclick = 0, dopaint=0;

    CHECK_CLASS(data, WC_Button);

    if (bPtr->flags.disabled)
	return;
    
    switch (event->type) {
     case EnterNotify:
	bPtr->flags.pushed = bPtr->flags.wasPushed;
	if (bPtr->flags.pushed) {
	    bPtr->flags.selected = !bPtr->flags.prevSelected;
	    dopaint = 1;
	}
	break;

     case LeaveNotify:
	bPtr->flags.wasPushed = bPtr->flags.pushed;
	if (bPtr->flags.pushed) {
	    bPtr->flags.selected = bPtr->flags.prevSelected;
	    dopaint = 1;
	}
	bPtr->flags.pushed = 0;
	break;

     case ButtonPress:
	if (event->xbutton.button == Button1) {
	    if (bPtr->groupIndex>0 && bPtr->flags.selected)
		break;
	    
	    bPtr->flags.wasPushed = 0;
	    bPtr->flags.pushed = 1;
	    bPtr->flags.prevSelected = bPtr->flags.selected;
	    bPtr->flags.selected = !bPtr->flags.selected;
	    dopaint = 1;
	}
	break;
 
     case ButtonRelease:
	if (event->xbutton.button == Button1) {
	    if (bPtr->flags.pushed) {
		doclick = 1;
		dopaint = 1;
		if (bPtr->flags.springLoaded) {
		    bPtr->flags.selected = bPtr->flags.prevSelected;
		}
	    }
	    bPtr->flags.pushed = 0;
	}
	break;	
    }

    if (dopaint)
	paintButton(bPtr);
    
    if (doclick) {
	XEvent ev;

	if (bPtr->action)
	    (*bPtr->action)(bPtr, bPtr->clientData);

	if (bPtr->flags.selected && bPtr->groupIndex>0) {
	    SETUP_INTERNAL_MESSAGE(ev, bPtr->view->screen);
	    ev.xclient.format=32;
	    ev.xclient.data.l[0]=bPtr->view->window;
	    ev.xclient.data.l[1]=WM_RADIO_PRESS;
	    ev.xclient.data.l[2]=bPtr->groupIndex;

	    W_BroadcastMessage(bPtr->view->parent, &ev);
	}
    }
}



static void
destroyButton(Button *bPtr)
{   
    if (bPtr->caption)
	free(bPtr->caption);

    if (bPtr->altCaption)
	free(bPtr->altCaption);
    
    if (bPtr->image)
	WMReleasePixmap(bPtr->image);
    
    if (bPtr->altImage)
	WMReleasePixmap(bPtr->altImage);

    free(bPtr);
}


