

#include "WINGsP.h"

#include <X11/Xresource.h>




#define EVENT_MASK  \
	KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask| \
	EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask| \
	VisibilityChangeMask|FocusChangeMask|PropertyChangeMask|\
	SubstructureNotifyMask|SubstructureRedirectMask


static XSetWindowAttributes defAtts= {
    None,                       /* background_pixmap */
    0,                          /* background_pixel */
    CopyFromParent,             /* border_pixmap */
    0,                          /* border_pixel */
    NorthWestGravity,           /* bit_gravity */
    NorthWestGravity,           /* win_gravity */
    NotUseful,                  /* backing_store */
    (unsigned) ~0,              /* backing_planes */
    0,                          /* backing_pixel */
    False,                      /* save_under */
    EVENT_MASK,   	         /* event_mask */
    0,                          /* do_not_propagate_mask */
    False,                      /* override_redirect */
    CopyFromParent,             /* colormap */
    None                        /* cursor */
};



static XContext ViewContext=0;	       /* context for views */



W_View*
W_GetViewForXWindow(Display *display, Window window)
{
    W_View *view;
    
    if (XFindContext(display, window, ViewContext, (XPointer*)&view)==0) {
	return view;
    }
    return NULL;
}



static void
unparentView(W_View *view)
{
    /* remove from parent's children list */
    if (view->parent!=NULL) {
	W_View *ptr;
	
	ptr = view->parent->childrenList;
	if (ptr == view) {
	    view->parent->childrenList = view->nextSister;
	} else {
	    while (ptr!=NULL) {
		if (ptr->nextSister == view) {
		    ptr->nextSister = view->nextSister;
		    break;
		} 
		ptr = ptr->nextSister;
	    }
	}
    }
    view->parent = NULL;
}


static void
adoptChildView(W_View *view, W_View *child)
{
    /* add to children list of parent */
    if (view->childrenList == NULL) {
	view->childrenList = child;
    } else {
	W_View *ptr;
	
	ptr = view->childrenList;
	while (ptr->nextSister!=NULL) 
	    ptr = ptr->nextSister;
	ptr->nextSister = child;
    }
    child->parent = view;
}



static W_View*
createView(W_Screen *screen, W_View *parent)
{
    W_View *view;

    if (ViewContext==0)
	ViewContext = XUniqueContext();
    
    view = wmalloc(sizeof(W_View));
    memset(view, 0, sizeof(W_View));
    
    view->refCount = 1;
    
    view->screen = screen;

    if (parent!=NULL) {
	/* attributes are not valid for root window */
	view->attribFlags = CWEventMask|CWColormap|CWBitGravity;
	view->attribs = defAtts;

	view->attribFlags |= CWBackPixel;
	view->attribs.background_pixel = W_PIXEL(screen->gray);

	adoptChildView(parent, view);
    }
    return view;
}



W_View*
W_CreateView(W_View *parent)
{
    return createView(parent->screen, parent);
}


W_View*
W_CreateRootView(W_Screen *screen)
{
    W_View *view;
    
    view = createView(screen, NULL);

    view->window = screen->rootWin;
    
    view->flags.realized = 1;
    view->flags.mapped = 1;
    view->flags.root = 1;
    
    return view;
}


W_View*
W_CreateTopView(W_Screen *screen)
{
    W_View *view;
    
    view = createView(screen, screen->rootView);
    if (!view)
	return NULL;

    view->flags.topLevel = 1;
    view->attribs.event_mask |= StructureNotifyMask;
    
    return view;
}



void
W_RealizeView(W_View *view)
{
    Window parentWID;
    Display *dpy = view->screen->display;
    W_View *ptr;

    if (view->flags.realized)
	goto realizeChildren;

    if (view->parent && !view->parent->flags.realized) {
	wwarning("trying to realize view with unrealized parent");
	return;
    }

    assert(view->size.width > 0);
    assert(view->size.height > 0);

    parentWID = view->parent->window;

    view->window = XCreateWindow(dpy, parentWID, view->pos.x, view->pos.y,
				 view->size.width, view->size.height, 0, 
				 view->screen->depth, InputOutput,
				 view->screen->visual, view->attribFlags,
				 &view->attribs);
    
    XSaveContext(dpy, view->window, ViewContext, (XPointer)view);

    view->flags.realized = 1;

    if (view->flags.mapWhenRealized) {
	W_MapView(view);
	view->flags.mapWhenRealized = 0;
    }
    
realizeChildren:
    /* realize children */
    ptr = view->childrenList;
    while (ptr!=NULL) {
	W_RealizeView(ptr);
	
	ptr = ptr->nextSister;
    }
}




void
W_ReparentView(W_View *view, W_View *newParent)
{
    int wasMapped;
    Display *dpy = view->screen->display;
    
    assert(!view->flags.topLevel);
    
    wasMapped = view->flags.mapped;
    if (wasMapped)
	W_UnmapView(view);

    unparentView(view);
    adoptChildView(newParent, view);

    if (view->flags.realized) {
	if (newParent->flags.realized) {
	    XReparentWindow(dpy, view->window, newParent->window, 0, 0);
	} else {
	    wwarning("trying to reparent realized view to unrealized parent");
	    return;
	}
    }
    
    
    if (wasMapped)
	W_MapView(view);
}



void
W_MapView(W_View *view)
{
    if (!view->flags.mapped) {

	XMapRaised(view->screen->display, view->window);
	XFlush(view->screen->display);

	view->flags.mapped = 1;
    }
}


/*
 * W_MapSubviews-
 *     maps all children of the current view that where already realized.
 */
void
W_MapSubviews(W_View *view)
{
    XMapSubwindows(view->screen->display, view->window);
    XFlush(view->screen->display);
    
    view = view->childrenList;
    while (view) {
	view->flags.mapped = 1;
	view->flags.mapWhenRealized = 0;
	view = view->nextSister;
    }
}


void
W_UnmapView(W_View *view)
{
    if (!view->flags.mapped)
	return;
  
    XUnmapWindow(view->screen->display, view->window);
    XFlush(view->screen->display);
    
    view->flags.mapped = 0;
}



static void
destroyView(W_View *view)
{
    W_View *ptr;

    if (view->flags.alreadyDead)
	return;
    view->flags.alreadyDead = 1;    

    /* Do not leave focus in a inexisting control */
    if (view->screen->focusedControl == view)
        view->screen->focusedControl = NULL;

    /* destroy children recursively */
    while (view->childrenList!=NULL) {
	ptr = view->childrenList;
	ptr->flags.parentDying = 1;

	W_DestroyView(ptr);

	if (ptr == view->childrenList) {
	    view->childrenList = ptr->nextSister;
	    ptr->parent = NULL;
	}
    }

    if (view->destroyCallback)
	(*view->destroyCallback)(view->destroyData);

    W_CallDestroyHandlers(view);

    if (view->flags.realized) {
	XDeleteContext(view->screen->display, view->window, ViewContext);
    
	/* if parent is being destroyed, it will die naturaly */
	if (!view->flags.parentDying || view->flags.topLevel)
	    XDestroyWindow(view->screen->display, view->window);
    }
    
    /* remove self from parent's children list */
    unparentView(view);

    W_CleanUpEvents(view);
    
    free(view);
}



void
W_DestroyView(W_View *view)
{
    W_ReleaseView(view);
}



void
W_MoveView(W_View *view, int x, int y)
{
    assert(view->flags.root==0);

    if (view->pos.x == x && view->pos.y == y)
	return;
    
    if (view->flags.realized) {
	XMoveWindow(view->screen->display, view->window, x, y);
    }
    view->pos.x = x;
    view->pos.y = y;
}


void
W_ResizeView(W_View *view, unsigned int width, unsigned int height)
{
    assert(width > 0);
    assert(height > 0);
    
    if (view->size.width == width && view->size.height == height)
	return;
    
    if (view->flags.realized) {
	XResizeWindow(view->screen->display, view->window, width, height);
    } 
    view->size.width = width;
    view->size.height = height;
}


void
W_SetViewBackgroundColor(W_View *view, WMColor *color)
{
    view->attribFlags |= CWBackPixel;
    view->attribs.background_pixel = color->color.pixel;
    if (view->flags.realized) {
	XSetWindowBackground(view->screen->display, view->window, 
			     color->color.pixel);
	XClearWindow(view->screen->display, view->window);
    }
}


 
void
W_SetFocusToView(W_View *view)
{
    WMScreen *scr = view->screen;
    XEvent event;
    
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = NotifyDetailNone;
    if (scr->focusedControl) {
	/* simulate FocusOut event */
	event.xfocus.type = FocusOut;
	W_DispatchMessage(scr->focusedControl, &event);
    }
    scr->focusedControl = view;
    /* simulate FocusIn event */
    event.xfocus.type = FocusIn;
    W_DispatchMessage(view, &event);
}


void 
W_BroadcastMessage(W_View *targetParent, XEvent *event)
{
    W_View *target;
    
    target = targetParent->childrenList;
    while (target!=NULL) {
	W_DispatchMessage(target, event);
	
	target = target->nextSister;
    }
}


void 
W_DispatchMessage(W_View *target, XEvent *event)
{
    if (target->window==None)
	return;
    event->xclient.window = target->window;
    XSendEvent(target->screen->display, target->window, False,
	       SubstructureNotifyMask, event);
}



WMView*
W_RetainView(WMView *view)
{
    view->refCount++;
    return view;
}



void
W_ReleaseView(WMView *view)
{
    view->refCount--;
    if (view->refCount < 1) {
	destroyView(view);
    }
}


void
WMAddDestroyCallback(WMWidget *w, WMCallback *callback, void *clientData)
{
    W_VIEW(w)->destroyCallback = callback;
    W_VIEW(w)->destroyData = clientData;
}
