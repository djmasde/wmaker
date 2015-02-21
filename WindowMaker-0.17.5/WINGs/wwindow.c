

#include "WINGsP.h"

#include <X11/Xatom.h>


typedef struct W_Window {
    W_Class widgetClass;
    W_View *view;
    
    struct W_Window *nextPtr;	       /* next in the window list */
    
    char *caption;

    char *wname;

    WMAction *closeAction;
    void *closeData;

    struct {
	unsigned int configured:1;
    } flags;
} _Window;



static void realizeWindow();

struct W_ViewProcedureTable _WindowViewProcedures = {
    NULL,
	NULL,
	NULL,
	realizeWindow
};


#define DEFAULT_WIDTH	400
#define DEFAULT_HEIGHT	180
#define DEFAULT_TITLE	"Untitled"


static void destroyWindow(_Window *win);

static void handleEvents();


WMWindow*
WMCreateWindow(WMScreen *screen, char *name)
{
    _Window *win;
    static int initedApp = 0;

    win = wmalloc(sizeof(_Window));
    memset(win, 0, sizeof(_Window));

    win->widgetClass = WC_Window;

    win->view = W_CreateTopView(screen);
    if (!win->view) {
	free(win);
	return NULL;
    }

    win->wname = wstrdup(name);

    /* add to the window list of the screen (application) */
    win->nextPtr = screen->windowList;
    screen->windowList = win;

    WMCreateEventHandler(win->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask|FocusChangeMask, handleEvents, 
			 win);

    W_ResizeView(win->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    if (!initedApp) {
	W_InitApplication(screen);
	initedApp = 1;
    }

    return win;
}


void
WMSetWindowTitle(WMWindow *win, char *title)
{
    if (win->caption!=NULL)
	free(win->caption);
    if (title!=NULL)
	win->caption = wstrdup(title);
    else
	win->caption = NULL;

    if (win->view->flags.realized) {
	XStoreName(win->view->screen->display, win->view->window, title);
    }
}



void
WMSetWindowCloseAction(WMWindow *win, WMAction *action, void *clientData)
{
    Atom *atoms;
    Atom *newAtoms;
    int count;
    WMScreen *scr = win->view->screen;

    if (win->view->flags.realized) {
	if (action && !win->closeAction) {
	    XGetWMProtocols(scr->display, win->view->window, &atoms, &count);
	    
	    newAtoms = wmalloc((count+1)*sizeof(Atom));
	    memcpy(newAtoms, atoms, count*sizeof(Atom));
	    newAtoms[count++] = scr->deleteWindowAtom;
	    XSetWMProtocols(scr->display, win->view->window, newAtoms, count+1);
	    XFree(atoms);
	    free(newAtoms);
	} else {
	    int i, ncount;
	    
	    XGetWMProtocols(scr->display, win->view->window, &atoms, &count);
	    
	    newAtoms = wmalloc((count-1)*sizeof(Atom));
	    ncount = 0;
	    for (i=0; i < count; i++) {
		if (atoms[i]==scr->deleteWindowAtom) {
		    newAtoms[i] = atoms[i];
		    ncount++;
		}
	    }
	    XSetWMProtocols(scr->display, win->view->window, newAtoms, ncount);
	    XFree(atoms);
	    free(newAtoms);
	}
    }
    win->closeAction = action;
    win->closeData = clientData;    
}


static void 
realizeWindow(WMWindow *win)
{
    XWMHints *hints;
    XClassHint classHint;
    WMScreen *scr = win->view->screen;
    Atom atoms[4];
    int count;
    
    W_RealizeView(win->view);
    
    classHint.res_name = win->wname;
    classHint.res_class = scr->applicationName;
    XSetClassHint(scr->display, win->view->window, &classHint);
    
    if (!scr->aflags.simpleApplication) {
	hints = XAllocWMHints();
	hints->flags = WindowGroupHint;
	hints->window_group = scr->groupLeader;
	XSetWMHints(scr->display, win->view->window, hints);
	XFree(hints);
    }
    
    count = 0;
    if (win->closeAction) {
	atoms[count++] = scr->deleteWindowAtom;
    }
    
    if (count>0)
	XSetWMProtocols(scr->display, win->view->window, atoms, count);
    
    if (win->caption)
	XStoreName(scr->display, win->view->window,  win->caption);
}


void
WMHideWindow(WMWindow *win)
{
    WMUnmapWidget(win);
    XWithdrawWindow(win->view->screen->display, win->view->window,
		    win->view->screen->screen);
}


static void
handleEvents(XEvent *event, void *clientData)
{
    _Window *win = (_Window*)clientData;
    
    
    switch (event->type) {
     case ClientMessage:
	if (event->xclient.message_type == win->view->screen->protocolsAtom
	    && event->xclient.format == 32 
	    && event->xclient.data.l[0]==win->view->screen->deleteWindowAtom) {
	    
	    if (win->closeAction) {
		(*win->closeAction)(win, win->closeData);
	    }
	}
	break;
	
     case DestroyNotify:
	destroyWindow(win);
	break;
    }
}




static void
destroyWindow(_Window *win)
{
    WMScreen *scr = win->view->screen;
    
    if (scr->windowList == win) {
	scr->windowList = scr->windowList->nextPtr;
    } else {
	WMWindow *ptr;
	ptr = scr->windowList;
	
	while (ptr->nextPtr) {
	    if (ptr->nextPtr==win) {
		ptr->nextPtr = ptr->nextPtr->nextPtr;
		break;
	    }
	}
    }

    if (win->caption) {
	free(win->caption);
    }
    
    if (win->wname)
	free(win->wname);

    free(win);
}


