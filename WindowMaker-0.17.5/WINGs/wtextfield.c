



#include "WINGsP.h"

#include <X11/keysym.h>

#include <ctype.h>

#define CURSOR_BLINK_ON_DELAY	600
#define CURSOR_BLINK_OFF_DELAY	300


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
	
	unsigned int disabled:1;
	
	unsigned int focused:1;
	
	unsigned int cursorOn:1;
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
	NULL,
	NULL
};


#define TEXT_WIDTH(tPtr, start)	(W_TextWidth((tPtr)->view->screen->normalFont, \
				   &((tPtr)->text[(start)]), (tPtr)->textLen - (start) + 1) \
					+ 2*(tPtr)->offsetWidth)

#define TEXT_WIDTH2(tPtr, start, end) (W_TextWidth((tPtr)->view->screen->normalFont, \
				   &((tPtr)->text[(start)]), (end) - (start) + 1) \
					+ 2*(tPtr)->offsetWidth)


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
    
    while (TEXT_WIDTH(tPtr, tPtr->viewPosition) >= tPtr->usableWidth)
	tPtr->viewPosition++;
    
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
    
    W_SetViewBackgroundColor(tPtr->view, tPtr->view->screen->white);
    
    tPtr->text = wmalloc(MIN_TEXT_BUFFER);
	
    tPtr->text[0] = 0;

    tPtr->textLen = 0;
    tPtr->bufferSize = MIN_TEXT_BUFFER;
    
    WMCreateEventHandler(tPtr->view, ExposureMask|StructureNotifyMask
			 |FocusChangeMask, handleEvents, tPtr);

    W_ResizeView(tPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    WMSetTextFieldBordered(tPtr, DEFAULT_BORDERED);
    tPtr->flags.alignment = DEFAULT_ALIGNMENT;
    tPtr->offsetWidth = (tPtr->view->size.height
			 - tPtr->view->screen->normalFont->height)/2;

    WMCreateEventHandler(tPtr->view, EnterWindowMask|LeaveWindowMask
			 |ButtonPressMask|KeyPressMask,
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
WMDeleteTextFieldRange(WMTextField *tPtr, int position, int count)
{    
    CHECK_CLASS(tPtr, WC_TextField);

    if (position >= tPtr->textLen)
	return;
    
    if (count < 1) {
	if (position < 0)
	    position = 0;
	tPtr->text[position] = 0;
	tPtr->textLen = position;
	
	tPtr->cursorPosition = 0;
	tPtr->viewPosition = 0;
    } else {
	if (position + count > tPtr->textLen)
	    count = tPtr->textLen - position;
	memmv(&(tPtr->text[position]), &(tPtr->text[position+count]),
                tPtr->textLen - (position+count) + 1);	
	tPtr->textLen -= count;
	
	if (tPtr->cursorPosition > position)
	    tPtr->cursorPosition -= count;

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
WMSetTextFieldDisabled(WMTextField *tPtr, Bool disabled)
{
    tPtr->flags.disabled = disabled;
    
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

    cx = W_TextWidth(screen->normalFont, &(tPtr->text[tPtr->viewPosition]),
		     tPtr->cursorPosition-tPtr->viewPosition);

    switch (tPtr->flags.alignment) {
     case WARight:
	textWidth = W_TextWidth(screen->normalFont, tPtr->text, tPtr->textLen);
	if (textWidth < tPtr->usableWidth)
	    cx += tPtr->offsetWidth + tPtr->usableWidth - textWidth;
	else
	    cx += tPtr->offsetWidth;
	break;
     case WALeft:
	cx += tPtr->offsetWidth;
	break;
     case WACenter:
	textWidth = W_TextWidth(screen->normalFont, tPtr->text, tPtr->textLen);
	if (textWidth < tPtr->usableWidth)
	    cx += tPtr->offsetWidth + (tPtr->usableWidth-textWidth)/2;
	else
	    cx += tPtr->offsetWidth;
	break;
    }
    
    XDrawRectangle(screen->display, tPtr->view->window, screen->xorGC,
		   cx, tPtr->offsetWidth, 1,
		   tPtr->view->size.height - 2*tPtr->offsetWidth - 1);
}


static void
paintTextField(TextField *tPtr)
{
    W_Screen *screen = tPtr->view->screen;
    W_View *view = tPtr->view;
    int tx, ty, tw, th;
    int bd;

    
    if (!view->flags.realized || !view->flags.mapped)
	return;

    if (!tPtr->flags.bordered) {
	bd = 0;
    } else {
	bd = 2;
    }

    if (tPtr->textLen > 0) {
    	tw = W_TextWidth(screen->normalFont, &(tPtr->text[tPtr->viewPosition]),
			tPtr->textLen - tPtr->viewPosition);
    
	th = screen->normalFont->height;

	ty = screen->normalFont->y + tPtr->offsetWidth;
	switch (tPtr->flags.alignment) {
	 case WALeft:
	    tx = tPtr->offsetWidth;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd+tw, bd,
			   tPtr->usableWidth-tw, view->size.height-2*bd, 
			   False);
	    break;
	
	 case WACenter:	    
	    tx = tPtr->offsetWidth + (tPtr->usableWidth - tw) / 2;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd, bd,
			   tPtr->usableWidth, view->size.height-2*bd, False);
	    break;

	 default:
	 case WARight:
	    tx = tPtr->offsetWidth + tPtr->usableWidth - tw;
	    if (tw < tPtr->usableWidth)
		XClearArea(screen->display, view->window, bd, bd,
			   tPtr->usableWidth-tw, view->size.height-2*bd, 
			   False);
	    break;
	}
	
	if (tPtr->flags.disabled)
	    XSetForeground(screen->display, screen->normalFontGC,
			   W_PIXEL(screen->darkGray));

	XDrawImageString(screen->display, view->window, screen->normalFontGC, 
		    tx, ty, &(tPtr->text[tPtr->viewPosition]), 
		    tPtr->textLen - tPtr->viewPosition);
	
	if (tPtr->flags.disabled)
	    XSetForeground(screen->display, screen->normalFontGC,
			   W_PIXEL(screen->black));
    } else {
	XClearArea(screen->display, view->window, bd, bd, tPtr->usableWidth,
		   view->size.height - 2*bd, False);
    }

    /* draw cursor */
    if (tPtr->flags.focused && !tPtr->flags.disabled && tPtr->flags.cursorOn) {
	paintCursor(tPtr);
    }
    
    /* draw relief */
    if (tPtr->flags.bordered)
	W_DrawRelief(screen, view->window, 0, 0, view->size.width, 
		     view->size.height, WRSunken);
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
	tPtr->flags.focused = 1;
#if 0
	if (!tPtr->timerID) {
	    tPtr->timerID = WMAddTimerHandler(CURSOR_BLINK_ON_DELAY, 
					      blinkCursor, tPtr);
	}
#endif
	paintTextField(tPtr);
	break;
	
     case FocusOut:
	tPtr->flags.focused = 0;
#if 0
	if (tPtr->timerID)
	    WMDeleteTimerHandler(tPtr->timerID);
	tPtr->timerID = NULL;
#endif

	paintTextField(tPtr);
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


    if (((XKeyEvent *) event)->state & WM_EMACSKEYMASK) {
	control_pressed = 1;
    }

    count = XLookupString(&event->xkey, buffer, 63, &ksym, NULL);
    buffer[count] = '\0';

    switch (ksym) {
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
	    if (incrToFit2(tPtr))
		refresh = 1;
	    else
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
	    if (!incrToFit(tPtr)) {
		refresh = 1;
	    } else {
		paintCursor(tPtr);
	    }
	}
	break;
	
    case WM_EMACSKEY_BS:
      if (!control_pressed) {
	goto normal_key;
      }
     case XK_BackSpace:
	if (tPtr->cursorPosition > 0) {
	    WMDeleteTextFieldRange(tPtr, tPtr->cursorPosition-1, 1);
	}
	break;
	
    case WM_EMACSKEY_DEL:
      if (!control_pressed) {
	goto normal_key;
      }
    case XK_KP_Delete:
     case XK_Delete:
	if (tPtr->cursorPosition < tPtr->textLen) {
	    WMDeleteTextFieldRange(tPtr, tPtr->cursorPosition, 1);
	}
	break;

    normal_key:
     default:
	if (count > 0 && !iscntrl(buffer[0])) {
	    WMInsertTextFieldText(tPtr, buffer, tPtr->cursorPosition);
	}
    }
    if (refresh) {
	paintTextField(tPtr);
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
    if (W_TextWidth(font, &(tPtr->text[tPtr->viewPosition]), 
		    tPtr->textLen-tPtr->viewPosition) < x)
	return tPtr->textLen;

    while (a < b && b-a>1) {
	mid = (a+b)/2;
	tw = W_TextWidth(font, &(tPtr->text[tPtr->viewPosition]), 
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
	if (!tPtr->flags.disabled)
	    handleTextFieldKeyPress(tPtr, event);
	break;
	
     case ButtonPress:
	if (!tPtr->flags.disabled && !tPtr->flags.focused)
	    W_SetFocusToView(tPtr->view);
	tPtr->cursorPosition = pointToCursorPosition(tPtr, 
						     event->xbutton.x);
	paintTextField(tPtr);
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
