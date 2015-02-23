



#include "WINGsP.h"

#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <ctype.h>

#define CURSOR_BLINK_ON_DELAY	600
#define CURSOR_BLINK_OFF_DELAY	300


char *WMTextDidChangeNotification = "WMTextDidChangeNotification";
char *WMTextDidBeginEditingNotification = "WMTextDidBeginEditingNotification";
char *WMTextDidEndEditingNotification = "WMTextDidEndEditingNotification";


typedef struct W_TextField {
    W_Class widgetClass;
    W_View *view;

    struct W_TextField *nextField;     /* next textfield in the chain */
    struct W_TextField *prevField;

    char *text;
    int textLen;		       /* size of text */
    int bufferSize;		       /* memory allocated for text */

    int viewPosition;		       /* position of text being shown */

    int cursorPosition;		       /* position of the insertion cursor */

    short usableWidth;
    short offsetWidth;		       /* offset of text from border */

#if 0
    WMHandlerID	timerID;	       /* for cursor blinking */
#endif
    struct {
	WMAlignment alignment:2;

	unsigned int bordered:1;
	
	unsigned int enabled:1;
	
	unsigned int focused:1;

	unsigned int cursorOn:1;
	
	unsigned int secure:1;	       /* password entry style */

	/**/
	unsigned int notIllegalMovement:1;
    } flags;
} TextField;


#define MIN_TEXT_BUFFER		2
#define TEXT_BUFFER_INCR	8


#define WM_EMACSKEYMASK   ControlMask

#define WM_EMACSKEY_LEFT  XK_b
#define WM_EMACSKEY_RIGHT XK_f
#define WM_EMACSKEY_HOME  XK_a
#define WM_EMACSKEY_END   XK_e
#define WM_EMACSKEY_BS    XK_h
#define WM_EMACSKEY_DEL   XK_d



#define DEFAULT_WIDTH		60
#define DEFAULT_HEIGHT		20
#define DEFAULT_BORDERED	True
#define DEFAULT_ALIGNMENT	WALeft



static void destroyTextField(TextField *tPtr);
static void paintTextField(TextField *tPtr);

static void handleEvents(XEvent *event, void *data);
static void handleTextFieldActionEvents(XEvent *event, void *data);
static void resizeTextField();

struct W_ViewProcedureTable _TextFieldViewProcedures = {
    NULL,
	resizeTextField,
	NULL
};


#define TEXT_WIDTH(tPtr, start)	(WMWidthOfString((tPtr)->view->screen->normalFont, \
				   &((tPtr)->text[(start)]), (tPtr)->textLen - (start) + 1))

#define TEXT_WIDTH2(tPtr, start, end) (WMWidthOfString((tPtr)->view->screen->normalFont, \
				   &((tPtr)->text[(start)]), (end) - (start) + 1))


static void
memmv(char *dest, char *src, int size)
{
    int i;
    
    if (dest > src) {
	for (i=size-1; i>=0; i--) {
	    dest[i] = src[i];
	}
    } else if (dest < src) {
	for (i=0; i<size; i++) {
	    dest[i] = src[i];
	}
    }
}


static int
incrToFit(TextField *tPtr)
{
    int vp = tPtr->viewPosition;

    while (TEXT_WIDTH(tPtr, tPtr->viewPosition) > tPtr->usableWidth) {
	tPtr->viewPosition++;
    }
    return vp!=tPtr->viewPosition;
}

static int
incrToFit2(TextField *tPtr)
{
    int vp = tPtr->viewPosition;
    while (TEXT_WIDTH2(tPtr, tPtr->viewPosition, tPtr->cursorPosition) 
	   >= tPtr->usableWidth)
	tPtr->viewPosition++;

    
    return vp!=tPtr->viewPosition;
}


static void
decrToFit(TextField *tPtr)
{
    while (TEXT_WIDTH(tPtr, tPtr->viewPosition-1) < tPtr->usableWidth
	   && tPtr->viewPosition>0)
	tPtr->viewPosition--;
}

#undef TEXT_WIDTH
#undef TEXT_WIDTH2


WMTextField*
WMCreateTextField(WMWidget *parent)
{
    TextField *tPtr;

    
    tPtr = wmalloc(sizeof(TextField));
    memset(tPtr, 0, sizeof(TextField));

    tPtr->widgetClass = WC_TextField;
    
    tPtr->view = W_CreateView(W_VIEW(parent));
    if (!tPtr->view) {
	free(tPtr);
	return NULL;
    }
    tPtr->view->self = tPtr;
    
    tPtr->view->attribFlags |= CWCursor;
    tPtr->view->attribs.cursor = tPtr->view->screen->textCursor;
    
    W_SetViewBackgroundColor(tPtr->view, tPtr->view->screen->white);
    
    tPtr->text = wmalloc(MIN_TEXT_BUFFER);
    tPtr->text[0] = 0;
    tPtr->textLen = 0;
    tPtr->bufferSize = MIN_TEXT_BUFFER;
    
    tPtr->flags.enabled = 1;
    
    WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
			 |FocusChangeMask, handleEvents, tPtr);

    W_ResizeView(tPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    WMSetTextFieldBordered(tPtr, DEFAULT_BORDERED);
    tPtr->flags.alignment = DEFAULT_ALIGNMENT;
    tPtr->offsetWidth = (tPtr->view->size.height
			 - tPtr->view->screen->normalFont->height)/2;

    WMCreateEventHandler(tPtr->view, EnterWindowMask|LeaveWindowMask
			 |ButtonPressMask|KeyPressMask|Button1MotionMask,
			 handleTextFieldActionEvents, tPtr);

    tPtr->flags.cursorOn = 1;
    
    return tPtr;
}


void
WMInsertTextFieldText(WMTextField *tPtr, char *text, int position)
{
    int len;
 
    CHECK_CLASS(tPtr, WC_TextField);
    
    if (!text)
	return;
    
    len = strlen(text);

    /* check if buffer will hold the text */
    if (len + tPtr->textLen >= tPtr->bufferSize) {
	tPtr->bufferSize = tPtr->textLen + len + TEXT_BUFFER_INCR;
	tPtr->text = realloc(tPtr->text, tPtr->bufferSize);
    }
    
    if (position < 0 || position >= tPtr->textLen) {
	/* append the text at the end */
	strcat(tPtr->text, text);
	
	incrToFit(tPtr);

	tPtr->textLen += len;
	tPtr->cursorPosition += len;
    } else {
	/* insert text at position */ 
	memmv(&(tPtr->text[position+len]), &(tPtr->text[position]),
                tPtr->textLen-position+1);
	
	memcpy(&(tPtr->text[position]), text, len);
	
	tPtr->textLen += len;
	if (position >= tPtr->cursorPosition) {
	    tPtr->cursorPosition += len;
	    incrToFit2(tPtr);
	} else {
	    incrToFit(tPtr);
	}
    }
    
    paintTextField(tPtr);
}


void
WMDeleteTextFieldRange(WMTextField *tPtr, WMRange range)
{    
    CHECK_CLASS(tPtr, WC_TextField);

    if (range.position >= tPtr->textLen)
	return;
    
    if (range.count < 1) {
	if (range.position < 0)
	    range.position = 0;
	tPtr->text[range.position] = 0;
	tPtr->textLen = range.position;
	
	tPtr->cursorPosition = 0;
	tPtr->viewPosition = 0;
    } else {
	if (range.position + range.count > tPtr->textLen)
	    range.count = tPtr->textLen - range.position;
	memmv(&(tPtr->text[range.position]), &(tPtr->text[range.position+range.count]),
                tPtr->textLen - (range.position+range.count) + 1);
	tPtr->textLen -= range.count;
	
	if (tPtr->cursorPosition > range.position)
	    tPtr->cursorPosition -= range.count;

	decrToFit(tPtr);
    }
        
    paintTextField(tPtr);
}



char*
WMGetTextFieldText(WMTextField *tPtr)
{
    CHECK_CLASS(tPtr, WC_TextField);
        
    return wstrdup(tPtr->text);
}


void
WMSetTextFieldText(WMTextField *tPtr, char *text)
{
    if (text==NULL) {
	tPtr->text[0] = 0;
	tPtr->textLen = 0;
    } else {
	tPtr->textLen = strlen(text);
	
	if (tPtr->textLen >= tPtr->bufferSize) {
	    tPtr->bufferSize = tPtr->textLen + TEXT_BUFFER_INCR;
	    tPtr->text = realloc(tPtr->text, tPtr->bufferSize);
	}
	strcpy(tPtr->text, text);
    }
    if (tPtr->textLen < tPtr->cursorPosition)
	tPtr->cursorPosition = tPtr->textLen;
    
    if (tPtr->view->flags.realized)
	paintTextField(tPtr);
}


void
WMSetTextFieldAlignment(WMTextField *tPtr, WMAlignment alignment)
{
    tPtr->flags.alignment = alignment;
    if (alignment!=WALeft) {
	wwarning("only left alignment is supported in textfields");
	return;
    }
	
    if (tPtr->view->flags.realized) {
	paintTextField(tPtr);
    }
}


void
WMSetTextFieldBordered(WMTextField *tPtr, Bool bordered)
{
    tPtr->flags.bordered = bordered;

    if (tPtr->view->flags.realized) {
	paintTextField(tPtr);
    }
}



void
WMSetTextFieldSecure(WMTextField *tPtr, Bool flag)
{
    tPtr->flags.secure = flag;
    
    if (tPtr->view->flags.realized) {
	paintTextField(tPtr);
    }    
}


void
WMSetTextFieldEnabled(WMTextField *tPtr, Bool flag)
{
    tPtr->flags.enabled = flag;
    
    if (tPtr->view->flags.realized) {
	paintTextField(tPtr);
    }
}


static void 
resizeTextField(WMTextField *tPtr, unsigned int width, unsigned int height)
{
    W_ResizeView(tPtr->view, width, height);
    
    tPtr->offsetWidth = (tPtr->view->size.height
			 - tPtr->view->screen->normalFont->height)/2;
    
    tPtr->usableWidth = tPtr->view->size.width - 2*tPtr->offsetWidth;
}


static void
paintCursor(TextField *tPtr)
{
    int cx;
    WMScreen *screen = tPtr->view->screen;
    int textWidth;

    cx = WMWidthOfString(screen->normalFont,
			 &(tPtr->text[tPtr->viewPosition]),
			 tPtr->cursorPosition-tPtr->viewPosition);

    switch (tPtr->flags.alignment) {
     case WARight:
	textWidth = WMWidthOfString(screen->normalFont, tPtr->text, 
				    tPtr->textLen);
	if (textWidth < tPtr->usableWidth)
	    cx += tPtr->offsetWidth + tPtr->usableWidth - textWidth;
	else
	    cx += tPtr->offsetWidth;
	break;
     case WALeft:
	cx += tPtr->offsetWidth;
	break;
     case WACenter:
	textWidth = WMWidthOfString(screen->normalFont, tPtr->text, 
				    tPtr->textLen);
	if (textWidth < tPtr->usableWidth)
	    cx += tPtr->offsetWidth + (tPtr->usableWidth-textWidth)/2;
	else
	    cx += tPtr->offsetWidth;
	break;
    }
    /*
    XDrawRectangle(screen->display, tPtr->view->window, screen->xorGC,
		   cx, tPtr->offsetWidth, 1,
		   tPtr->view->size.height - 2*tPtr->offsetWidth - 1);
     */
    XDrawLine(screen->display, tPtr->view->window, screen->xorGC,
	      cx, tPtr->offsetWidth, cx,
	      tPtr->view->size.height - tPtr->offsetWidth - 1);
}



static void
drawRelief(WMView *view)
{
    WMScreen *scr = view->screen;
    Display *dpy = scr->display;
    GC wgc;
    GC lgc;
    GC dgc;
    int width = view->size.width;
    int height = view->size.height;
    
    wgc = W_GC(scr->white);
    dgc = W_GC(scr->darkGray);
    lgc = W_GC(scr->gray);

    /* top left */
    XDrawLine(dpy, view->window, dgc, 0, 0, width-1, 0);
    XDrawLine(dpy, view->window, dgc, 0, 1, width-2, 1);
    
    XDrawLine(dpy, view->window, dgc, 0, 0, 0, height-2);
    XDrawLine(dpy, view->window, dgc, 1, 0, 1, height-3);
    
    /* bottom right */
    XDrawLine(dpy, view->window, wgc, 0, height-1, width-1, height-1);
    XDrawLine(dpy, view->window, lgc, 1, height-2, width-2, height-2);

    XDrawLine(dpy, view->window, wgc, width-1, 0, width-1, height-1);
    XDrawLine(dpy, view->window, lgc, width-2, 1, width-2, height-3);
}


static void
paintTextField(TextField *tPtr)
{
    W_Screen *screen = tPtr->view->screen;
    W_View *view = tPtr->view;
    int tx, ty, tw, th;
    int bd;
    int totalWidth;

    
    if (!view->flags.realized || !view->flags.mapped)
	return;

    if (!tPtr->flags.bordered) {
	bd = 0;
    } else {
	bd = 2;
    }

    totalWidth = tPtr->view->size.width - 2*bd;

    if (tPtr->textLen > 0) {
    	tw = WMWidthOfString(screen->normalFont, 
			     &(tPtr->text[tPtr->viewPosition]),
			     tPtr->textLen - tPtr->viewPosition);
    
	th = screen->normalFont->height;

	ty = tPtr->offsetWidth;
	switch (tPtr->flags.alignment) {
	 case WALeft:
	    tx = tPtr->offsetWidth;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd+tw, bd,
			   totalWidth-tw, view->size.height-2*bd, 
			   False);
	    break;
	
	 case WACenter:	    
	    tx = tPtr->offsetWidth + (tPtr->usableWidth - tw) / 2;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd, bd,
			   totalWidth, view->size.height-2*bd, False);
	    break;

	 default:
	 case WARight:
	    tx = tPtr->offsetWidth + tPtr->usableWidth - tw;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd, bd,
			   totalWidth-tw, view->size.height-2*bd, False);
	    break;
	}

	if (!tPtr->flags.secure) {
	    if (!tPtr->flags.enabled)
		WMSetColorInGC(screen->darkGray, screen->textFieldGC);

	    WMDrawImageString(screen, view->window, screen->textFieldGC, 
			      screen->normalFont, tx, ty,
			      &(tPtr->text[tPtr->viewPosition]), 
			      tPtr->textLen - tPtr->viewPosition);
	
	    if (!tPtr->flags.enabled)
		WMSetColorInGC(screen->black, screen->textFieldGC);
	}
    } else {
	XClearArea(screen->display, view->window, bd, bd, totalWidth,
		   view->size.height - 2*bd, False);
    }

    /* draw cursor */
    if (tPtr->flags.focused && tPtr->flags.enabled && tPtr->flags.cursorOn) {
	paintCursor(tPtr);
    }
    
    /* draw relief */
    if (tPtr->flags.bordered) {
	drawRelief(view);
    }
}


#if 0
static void
blinkCursor(void *data)
{
    TextField *tPtr = (TextField*)data;
    
    if (tPtr->flags.cursorOn) {
	tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_OFF_DELAY, blinkCursor,
					  data);
    } else {
	tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_ON_DELAY, blinkCursor,
					  data);	
    }
    paintCursor(tPtr);
    tPtr->flags.cursorOn = !tPtr->flags.cursorOn;
}
#endif

static void
handleEvents(XEvent *event, void *data)
{
    TextField *tPtr = (TextField*)data;

    CHECK_CLASS(data, WC_TextField);


    switch (event->type) {
     case FocusIn:
	if (W_FocusedViewOfToplevel(W_TopLevelOfView(tPtr->view))!=tPtr->view)
	    return;
	tPtr->flags.focused = 1;
#if 0
	if (!tPtr->timerID) {
	    tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_ON_DELAY, 
					      blinkCursor, tPtr);
	}
#endif
	paintTextField(tPtr);

	WMPostNotificationName(WMTextDidBeginEditingNotification, tPtr, NULL);

	tPtr->flags.notIllegalMovement = 0;
	break;
	
     case FocusOut:
	tPtr->flags.focused = 0;
#if 0
	if (tPtr->timerID)
	    WMDeleteTimerHandler(tPtr->timerID);
	tPtr->timerID = NULL;
#endif

	paintTextField(tPtr);
	if (!tPtr->flags.notIllegalMovement) {
	    WMPostNotificationName(WMTextDidEndEditingNotification, tPtr,
				   (void*)WMIllegalTextMovement);
	}
	break;
	
     case Expose:
	if (event->xexpose.count!=0)
	    break;
	paintTextField(tPtr);
	break;
	
     case DestroyNotify:
	destroyTextField(tPtr);
	break;
    }
}


static void
handleTextFieldKeyPress(TextField *tPtr, XEvent *event)
{
    char buffer[64];
    KeySym ksym;
    int count, refresh = 0;
    int control_pressed = 0;
    int changed;
    WMScreen *scr = tPtr->view->screen;

    changed = 0;

    if (((XKeyEvent *) event)->state & WM_EMACSKEYMASK) {
	control_pressed = 1;
    }

    count = XLookupString(&event->xkey, buffer, 63, &ksym, NULL);
    buffer[count] = '\0';

    switch (ksym) {
     case XK_Tab:
	if (event->xkey.state & ShiftMask) {
	    if (tPtr->view->prevFocusChain) {
		W_SetFocusOfTopLevel(W_TopLevelOfView(tPtr->view),
				     tPtr->view->prevFocusChain);
		tPtr->flags.notIllegalMovement = 1;
	    }
	    WMPostNotificationName(WMTextDidEndEditingNotification, tPtr,
				   (void*)WMBacktabTextMovement);
	} else {
	    if (tPtr->view->nextFocusChain) {
		W_SetFocusOfTopLevel(W_TopLevelOfView(tPtr->view),
				 tPtr->view->nextFocusChain);
		tPtr->flags.notIllegalMovement = 1;
	    }
	    WMPostNotificationName(WMTextDidEndEditingNotification,
				   tPtr, (void*)WMTabTextMovement);
	}
	break;
	
     case XK_Return:
	WMPostNotificationName(WMTextDidEndEditingNotification, tPtr, 
			       (void*)WMReturnTextMovement);
	break;

     case WM_EMACSKEY_LEFT:
	if (!control_pressed) {
	    goto normal_key;
	}
     case XK_KP_Left:
     case XK_Left:
	if (tPtr->cursorPosition > 0) {
	    paintCursor(tPtr);
	    tPtr->cursorPosition--;
	    if (tPtr->cursorPosition < tPtr->viewPosition) {
		tPtr->viewPosition = tPtr->cursorPosition;
		refresh = 1;
	    } else {
		paintCursor(tPtr);
	    }
	}
	break;

    case WM_EMACSKEY_RIGHT:
      if (!control_pressed) {
	goto normal_key;
      }
    case XK_KP_Right:
     case XK_Right:
	if (tPtr->cursorPosition < tPtr->textLen) {
	    paintCursor(tPtr);
	    tPtr->cursorPosition++;
	    while (WMWidthOfString(scr->normalFont,
				&(tPtr->text[tPtr->viewPosition]),
				tPtr->cursorPosition-tPtr->viewPosition)
		   > tPtr->usableWidth) {
		tPtr->viewPosition++;
		refresh = 1;
	    }
	    if (!refresh)
		paintCursor(tPtr);
	}
	break;
	
    case WM_EMACSKEY_HOME:
      if (!control_pressed) {
	goto normal_key;
      }
    case XK_KP_Home:
     case XK_Home:
	if (tPtr->cursorPosition > 0) {
	    paintCursor(tPtr);
	    tPtr->cursorPosition = 0;
	    if (tPtr->viewPosition > 0) {
		tPtr->viewPosition = 0;
		refresh = 1;
	    } else {
		paintCursor(tPtr);
	    }
	}
	break;
	
    case WM_EMACSKEY_END:
      if (!control_pressed) {
	goto normal_key;
      }
     case XK_KP_End:
     case XK_End:
	if (tPtr->cursorPosition < tPtr->textLen) {
	    paintCursor(tPtr);
	    tPtr->cursorPosition = tPtr->textLen;
	    tPtr->viewPosition = 0;
	    while (WMWidthOfString(scr->normalFont,
				   &(tPtr->text[tPtr->viewPosition]),
				   tPtr->textLen-tPtr->viewPosition)
		   >= tPtr->usableWidth) {
		tPtr->viewPosition++;
		refresh = 1;
	    }
	    if (!refresh)
		paintCursor(tPtr);
	}
	break;
	
     case WM_EMACSKEY_BS:
      if (!control_pressed) {
	goto normal_key;
      }
     case XK_BackSpace:
	if (tPtr->cursorPosition > 0) {
	    WMRange range;
	    changed = 1;
	    range.position = tPtr->cursorPosition-1;
	    range.count = 1;
	    WMDeleteTextFieldRange(tPtr, range);
	}
	break;
	
    case WM_EMACSKEY_DEL:
      if (!control_pressed) {
	goto normal_key;
      }
    case XK_KP_Delete:
     case XK_Delete:
	if (tPtr->cursorPosition < tPtr->textLen) {
	    WMRange range;
	    changed = 1;
	    range.position = tPtr->cursorPosition;
	    range.count = 1;
	    WMDeleteTextFieldRange(tPtr, range);
	}
	break;

    normal_key:
     default:
	if (count > 0 && !iscntrl(buffer[0])) {
	    changed = 1;
	    WMInsertTextFieldText(tPtr, buffer, tPtr->cursorPosition);
	}
    }
    if (refresh) {
	paintTextField(tPtr);
    }
    
    if (changed) {
	WMPostNotificationName(WMTextDidChangeNotification, tPtr, NULL);
    }
}


static int
pointToCursorPosition(TextField *tPtr, int x)
{
    WMFont *font = tPtr->view->screen->normalFont;
    int a, b, mid;
    int tw;

    if (tPtr->flags.bordered)
	x -= 2;

    a = tPtr->viewPosition;
    b = tPtr->viewPosition + tPtr->textLen;
    if (WMWidthOfString(font, &(tPtr->text[tPtr->viewPosition]), 
			tPtr->textLen-tPtr->viewPosition) < x)
	return tPtr->textLen;

    while (a < b && b-a>1) {
	mid = (a+b)/2;
	tw = WMWidthOfString(font, &(tPtr->text[tPtr->viewPosition]), 
			     mid - tPtr->viewPosition);
	if (tw > x)
	    b = mid;
	else if (tw < x)
	    a = mid;
	else
	    return mid;
    }
    return (a+b)/2;
}


static void
handleTextFieldActionEvents(XEvent *event, void *data)
{
    TextField *tPtr = (TextField*)data;
    
    CHECK_CLASS(data, WC_TextField);

    switch (event->type) {
     case KeyPress:
	if (tPtr->flags.enabled)
	    handleTextFieldKeyPress(tPtr, event);
	break;
	
     case MotionNotify:
	if (tPtr->flags.enabled && (event->xmotion.state & Button1Mask)) {
	    tPtr->cursorPosition = pointToCursorPosition(tPtr,
							 event->xmotion.x);
	    paintTextField(tPtr);
	}
	break;
	
     case ButtonPress:
	if (tPtr->flags.enabled && !tPtr->flags.focused) {
	    WMSetFocusToWidget(tPtr);
	    tPtr->cursorPosition = pointToCursorPosition(tPtr, 
							 event->xbutton.x);
	    paintTextField(tPtr);
	} else if (tPtr->flags.focused) {
	    tPtr->cursorPosition = pointToCursorPosition(tPtr, 
							 event->xbutton.x);
	    paintTextField(tPtr);
	}
	if (event->xbutton.button == Button2 && tPtr->flags.enabled) {
	    char *text;
	    
	    text = W_GetTextSelection(tPtr->view->screen, XA_PRIMARY);
	    if (!text) {
		text = W_GetTextSelection(tPtr->view->screen, XA_CUT_BUFFER0);
	    }
	    if (text) {
		WMInsertTextFieldText(tPtr, text, tPtr->cursorPosition);
		XFree(text);
		WMPostNotificationName(WMTextDidChangeNotification, tPtr, 
				       NULL);
	    }
	}
	break;
	
     case ButtonRelease:
	
	break;
    }
}


static void
destroyTextField(TextField *tPtr)
{
#if 0
    if (tPtr->timerID)
	WMDeleteTimerHandler(tPtr->timerID);
#endif
    
    if (tPtr->text)
	free(tPtr->text);

    free(tPtr);
}
