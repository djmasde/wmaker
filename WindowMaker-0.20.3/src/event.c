/* event.c- event loop and handling
 * 
 *  Window Maker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */


#include "wconfig.h"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef XDE_DND
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#endif

#ifdef KEEP_XKB_LOCK_STATUS     
#include <X11/XKBlib.h>         
#endif /* KEEP_XKB_LOCK_STATUS */

#include "WindowMaker.h"
#include "window.h"
#include "actions.h"
#include "client.h"
#include "funcs.h"
#include "keybind.h"
#include "application.h"
#include "stacking.h"
#include "defaults.h"
#include "workspace.h"
#include "dock.h"
#include "framewin.h"
#include "properties.h"
#include "balloon.h"


/******** Global Variables **********/
extern XContext wWinContext;

extern Cursor wCursor[WCUR_LAST];

extern WShortKey wKeyBindings[WKBD_LAST];
extern int wScreenCount;
extern Time LastTimestamp;
extern Time LastFocusChange;

extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask

extern Atom _XA_WM_COLORMAP_NOTIFY;

extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;
extern Atom _XA_WINDOWMAKER_WM_FUNCTION;

#ifdef OFFIX_DND
extern Atom _XA_DND_PROTOCOL;
#endif
#ifdef XDE_DND
extern Atom _XA_XDE_REQUEST;
extern Atom _XA_XDE_ENTER;
extern Atom _XA_XDE_LEAVE;
extern Atom _XA_XDE_DATA_AVAILABLE;
extern Atom _XDE_FILETYPE;
extern Atom _XDE_URLTYPE;
#endif


#ifdef SHAPE
extern Bool wShapeSupported;
extern int wShapeEventBase;
#endif

/* special flags */
extern char WProgramState;
extern char WDelayedActionSet;


/************ Local stuff ***********/


static void saveTimestamp(XEvent *event);
static void handleColormapNotify();
static void handleMapNotify(), handleUnmapNotify();
static void handleButtonPress(), handleExpose();
static void handleDestroyNotify();
static void handleConfigureRequest();
static void handleMapRequest();
static void handlePropertyNotify();
static void handleEnterNotify();
static void handleLeaveNotify();
static void handleExtensions();
static void handleClientMessage();
static void handleKeyPress();
static void handleFocusIn();
static void handleMotionNotify();

#ifdef SHAPE
static void handleShapeNotify();
#endif

/* called from the signal handler */
void NotifyDeadProcess(pid_t pid, unsigned char status);

/* real dead process handler */
static void handleDeadProcess(void *foo);


typedef struct DeadProcesses {
    pid_t 		pid;
    unsigned char 	exit_status;
} DeadProcesses;

/* stack of dead processes */
static DeadProcesses deadProcesses[MAX_DEAD_PROCESSES];
static int deadProcessPtr=0;


typedef struct DeathHandler {
    WDeathHandler	*callback;
    pid_t 		pid;
    struct DeathHandler *next;
    void 		*client_data;
} DeathHandler;

static DeathHandler *deathHandler=NULL;



WMagicNumber
wAddDeathHandler(pid_t pid, WDeathHandler *callback, void *cdata)
{
    DeathHandler *handler;
    
    handler = malloc(sizeof(DeathHandler));
    if (!handler)
      return 0;
    
    handler->pid = pid;
    handler->callback = callback;
    handler->client_data = cdata;
    
    handler->next = deathHandler;
        
    deathHandler = handler;

    return handler;
}



void 
wDeleteDeathHandler(WMagicNumber id)
{
    DeathHandler *tmp, *handler=(DeathHandler*)id;

    if (!handler || !deathHandler)
      return;

    tmp = deathHandler;
    if (tmp==handler) {
	deathHandler = handler->next;
	free(handler);
    } else {
	while (tmp->next) {
	    if (tmp->next==handler) {
		tmp->next=handler->next;
		free(handler);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}


void
DispatchEvent(XEvent *event)
{
    int i;

    if (deathHandler)
	handleDeadProcess(NULL);
    
    if (WProgramState==WSTATE_NEED_EXIT) {
	WProgramState = WSTATE_EXITING;

	/* 
	 * WMHandleEvent() can't be called from anything
	 * executed inside here, or we can get in a infinite
	 * recursive loop.
	 */
	for (i=0; i<wScreenCount; i++) {
	    WScreen *scr;
	    scr = wScreenWithNumber(i);
	    if (scr) {
		wScreenSaveState(scr);
	    }
	}
	RestoreDesktop(NULL);
	ExecExitScript();
	/* received SIGTERM */
	Exit(0);
    } else if (WProgramState == WSTATE_NEED_RESTART) {
	WProgramState = WSTATE_RESTARTING;

	for (i=0; i<wScreenCount; i++) {
	    WScreen *scr;
	    scr = wScreenWithNumber(i);
	    if (scr) {
		wScreenSaveState(scr);
	    }
	}
	RestoreDesktop(NULL);
	/* received SIGHUP */
	Restart(NULL);
    }

    /* for the case that all that is wanted to be dispatched is
     * the stuff above */
    if (!event)
	return;
    
    saveTimestamp(event);    
    switch (event->type) {
     case MapRequest:
	handleMapRequest(event);
	break;

     case KeyPress:
	handleKeyPress(event);
	break;

     case MotionNotify:
         handleMotionNotify(event);
         break;

     case ConfigureRequest:
	handleConfigureRequest(event);
	break;

     case DestroyNotify:
	handleDestroyNotify(event->xdestroywindow.window);
	break;
	
     case MapNotify:
	handleMapNotify(event->xmap.window);
	break;
	
     case UnmapNotify:
	handleUnmapNotify(event);
	break;

     case ButtonPress:
	handleButtonPress(event);
	break;
	
     case Expose:
	handleExpose(event);
	break;
	
     case PropertyNotify:
	handlePropertyNotify(event);
	break;

     case EnterNotify:
	handleEnterNotify(event);
	break;

    case LeaveNotify:
        handleLeaveNotify(event);
	break;

     case ClientMessage:
	handleClientMessage(event);
	break;
	
     case ColormapNotify:
	handleColormapNotify(event);
	break;

     case MappingNotify:
	if (event->xmapping.request == MappingKeyboard 
	    || event->xmapping.request == MappingModifier)
	  XRefreshKeyboardMapping(&event->xmapping);
	break;
	
     case FocusIn:
	handleFocusIn(event);
	break;

     default:
	handleExtensions(event);
    }
}


/*
 *----------------------------------------------------------------------
 * EventLoop-
 * 	Processes X and internal events indefinitely.
 * 
 * Returns:
 * 	Never returns
 * 
 * Side effects:
 * 	The LastTimestamp global variable is updated.
 *---------------------------------------------------------------------- 
 */
void
EventLoop()
{
    XEvent event;
   
    for(;;) {
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
}



Bool 
IsDoubleClick(WScreen *scr, XEvent *event)
{
    if ((scr->last_click_time>0) && 
	(event->xbutton.time-scr->last_click_time<=wPreferences.dblclick_time)
	&& (event->xbutton.button == scr->last_click_button)
	&& (event->xbutton.subwindow == scr->last_click_window)) {

	scr->flags.next_click_is_not_double = 1;
	scr->last_click_time = 0;
	scr->last_click_window = None;

	return True;
    }
    return False;
}


void
NotifyDeadProcess(pid_t pid, unsigned char status)
{
    if (deadProcessPtr>=MAX_DEAD_PROCESSES-1) {
	wwarning(_("stack overflow: too many dead processes"));
	return;
    }
    /* stack the process to be handled later,
     * as this is called from the signal handler */
    deadProcesses[deadProcessPtr].pid = pid;
    deadProcesses[deadProcessPtr].exit_status = status;
    deadProcessPtr++;
}


static void
handleDeadProcess(void *foo)
{
    DeathHandler *tmp;
    int i;

    for (i=0; i<deadProcessPtr; i++) {
	wWindowDeleteSavedStatesForPID(deadProcesses[i].pid);
    }

    if (!deathHandler) {
	deadProcessPtr=0;
	return;
    }
	
    /* get the pids on the queue and call handlers */
    while (deadProcessPtr>0) {
	deadProcessPtr--;

	tmp = deathHandler;
	while (tmp) {
	    DeathHandler *t;
	    
	    t = tmp->next;

	    if (tmp->pid == deadProcesses[deadProcessPtr].pid) {
		(*tmp->callback)(tmp->pid, 
				 deadProcesses[deadProcessPtr].exit_status,
				 tmp->client_data);
		wDeleteDeathHandler(tmp);
	    }
	    tmp = t;
	}
    }
}


static void
saveTimestamp(XEvent *event)
{
    LastTimestamp = CurrentTime;

    switch (event->type) {
     case ButtonRelease:
     case ButtonPress:
	LastTimestamp = event->xbutton.time;
	break;
     case KeyPress:
     case KeyRelease:
	LastTimestamp = event->xkey.time;
	break;
     case MotionNotify:
	LastTimestamp = event->xmotion.time;
	break;
     case PropertyNotify:
	LastTimestamp = event->xproperty.time;
	break;
     case EnterNotify:
     case LeaveNotify:
	LastTimestamp = event->xcrossing.time;
	break;
     case SelectionClear:
	LastTimestamp = event->xselectionclear.time;
	break;
     case SelectionRequest:
	LastTimestamp = event->xselectionrequest.time;
	break;
     case SelectionNotify:
	LastTimestamp = event->xselection.time;
	break;
    }
}


static void
handleExtensions(XEvent *event)
{
#ifdef SHAPE
    if (wShapeSupported && event->type == (wShapeEventBase+ShapeNotify)) {
	handleShapeNotify(event);
    }
#endif	
}

static void 
handleMapRequest(XEvent *ev)
{
    WWindow *wwin;
    WScreen *scr = NULL;
    Window window = ev->xmaprequest.window;

#ifdef DEBUG
    printf("got map request for %x\n", (unsigned)window);
#endif

    if ((wwin=wWindowFor(window))) {
	/* deiconify window */
	if (wwin->flags.shaded)
	    wUnshadeWindow(wwin);
	if (wwin->flags.miniaturized) {
	    wDeiconifyWindow(wwin);
	} else if (wwin->flags.hidden) {
            WApplication *wapp = wApplicationOf(wwin->main_window);
            /* go to the last workspace that the user worked on the app */
#ifndef REDUCE_APPICONS
            /* This severely breaks REDUCE_APPICONS.  last_workspace is a neat
             * concept but it needs to be reworked to handle REDUCE_APPICONS -cls
             */
            if (wapp) {
                wWorkspaceChange(wwin->screen_ptr, wapp->last_workspace);
            }
#endif
	    wUnhideApplication(wapp, False, False);
	}
	return;
    }

    scr = wScreenForRootWindow(ev->xmaprequest.parent);

    wwin = wManageWindow(scr, window);

    /* 
     * This is to let the Dock know that the application it launched
     * has already been mapped (eg: it has finished launching). 
     * It is not necessary for normally docked apps, but is needed for
     * apps that were forcedly docked (like with dockit).
     */
    if (scr->last_dock) {
	if (wwin && wwin->main_window!=None && wwin->main_window!=window)
	  wDockTrackWindowLaunch(scr->last_dock, wwin->main_window);
	else
	  wDockTrackWindowLaunch(scr->last_dock, window);
    }

    if (wwin) {
	int state;
	
	if (wwin->wm_hints && (wwin->wm_hints->flags & StateHint))
	    state = wwin->wm_hints->initial_state;
	else
	    state = NormalState;

        if (state==IconicState)
            wwin->flags.miniaturized = 1;

        if (state==WithdrawnState) {
	    wwin->flags.mapped = 0;
	    wClientSetState(wwin, WithdrawnState, None);
	    wUnmanageWindow(wwin, True);
        } else {
            wClientSetState(wwin, NormalState, None);
            if (wwin->flags.shaded) {
                wwin->flags.shaded = 0;
                wwin->flags.skip_next_animation = 1;
                wwin->flags.ignore_next_unmap = 1; /* ??? */
                wShadeWindow(wwin);
            }
            if (wwin->flags.miniaturized) {
                wwin->flags.miniaturized = 0;
                wwin->flags.hidden = 0;
                wwin->flags.skip_next_animation = 1;
                wwin->flags.ignore_next_unmap = 1;
                wIconifyWindow(wwin);
            } else if (wwin->flags.hidden) {
                WApplication *wapp = wApplicationOf(wwin->main_window);
                wwin->flags.hidden = 0;
                wwin->flags.skip_next_animation = 1;
                if (wapp) {
                    wHideApplication(wapp);
                }
                wwin->flags.ignore_next_unmap = 1;
            }
        }
    }
}


static void
handleDestroyNotify(Window window)
{
    WWindow *wwin;
    WApplication *app;
#ifdef DEBUG
    puts("got destroy notify");
#endif	    

    wwin = wWindowFor(window);
    if (wwin) {
	wUnmanageWindow(wwin, False);
    }

    app = wApplicationOf(window);
    if (app) {
	if (window == app->main_window) {
	    app->refcount = 0;
	    wwin = app->main_window_desc->screen_ptr->focused_window;
	    while (wwin) {
		if (wwin->main_window == window) {
		    wwin->main_window = None;
		}
		wwin = wwin->prev;
	    }
	}
	wApplicationDestroy(app);
    }
}



static void 
handleExpose(XEvent *event)
{
    WObjDescriptor *desc;

#ifdef DEBUG    
    puts("got expose");
#endif
    if (event->xexpose.count!=0) {
      return;
    }
    
    if (XFindContext(dpy, event->xexpose.window, wWinContext, 
		     (XPointer *)&desc)==XCNOENT) {
	return;
    }
    
    if (desc->handle_expose) {
	(*desc->handle_expose)(desc, event);
    }
}


/* bindable */
static void
handleButtonPress(XEvent *event)
{
    WObjDescriptor *desc;
    WScreen *scr;

#ifdef DEBUG    
    puts("got button press");
#endif

    scr = wScreenForRootWindow(event->xbutton.root);
    
#ifdef BALLOON_TEXT
    wBalloonHide(scr);
#endif

    if (event->xbutton.window==scr->root_win) {
	if (event->xbutton.button==wPreferences.menu_button) {
	    OpenRootMenu(scr, event->xbutton.x_root,
                         event->xbutton.y_root, False);
	    /* ugly hack */
	    if (scr->root_menu) {
		if (scr->root_menu->brother->flags.mapped)
		  event->xbutton.window = scr->root_menu->brother->frame->core->window;
		else
		  event->xbutton.window = scr->root_menu->frame->core->window;
	    }
	} else if (event->xbutton.button==wPreferences.windowl_button) {
	    
	    OpenSwitchMenu(scr, event->xbutton.x_root,
			   event->xbutton.y_root, False);
	    if (scr->switch_menu) {
		if (scr->switch_menu->brother->flags.mapped)
		  event->xbutton.window = scr->switch_menu->brother->frame->core->window;
		else
		  event->xbutton.window = scr->switch_menu->frame->core->window;
	    }
	} else if (event->xbutton.button==wPreferences.select_button) {
	    
	    wUnselectWindows(scr);
	    wSelectWindows(scr, event);
	}
#ifdef MOUSE_WS_SWITCH
	else if (event->xbutton.button==Button4) {

	    if (scr->current_workspace > 0)
		wWorkspaceChange(scr, scr->current_workspace-1);

	} else if (event->xbutton.button==Button5) {

	    if (scr->current_workspace < scr->workspace_count-1)
		wWorkspaceChange(scr, scr->current_workspace+1);

	}
#endif /* MOUSE_WS_SWITCH */
    }

    if (XFindContext(dpy, event->xbutton.subwindow, wWinContext,
		     (XPointer *)&desc)==XCNOENT) {
	if (XFindContext(dpy, event->xbutton.window, wWinContext,
			 (XPointer *)&desc)==XCNOENT) {
	    return;
	}
    }

    if (desc->parent_type == WCLASS_WINDOW) {
	XSync(dpy, 0);
    
	if (event->xbutton.state & MOD_MASK) {
	    XAllowEvents(dpy, AsyncPointer, CurrentTime);
	}
     
	if (wPreferences.focus_mode == WKF_CLICK) {
	    if (wPreferences.ignore_focus_click) {
		XAllowEvents(dpy, AsyncPointer, CurrentTime);
	    }
	    XAllowEvents(dpy, ReplayPointer, CurrentTime);
	}
	XSync(dpy, 0);
    } else if (desc->parent_type == WCLASS_APPICON
	       || desc->parent_type == WCLASS_MINIWINDOW
	       || desc->parent_type == WCLASS_DOCK_ICON) {
	if (event->xbutton.state & MOD_MASK) {
	    XSync(dpy, 0);
	    XAllowEvents(dpy, AsyncPointer, CurrentTime);
	    XSync(dpy, 0);
	}
    }

    if (desc->handle_mousedown!=NULL) {
	(*desc->handle_mousedown)(desc, event);
    }
    
    /* save double-click information */
    if (scr->flags.next_click_is_not_double) {
	scr->flags.next_click_is_not_double = 0;
    } else {
	scr->last_click_time = event->xbutton.time;
	scr->last_click_button = event->xbutton.button;
	scr->last_click_window = event->xbutton.subwindow;
    }
}


static void
handleMapNotify(Window window)
{
    WWindow *wwin;
    
#ifdef DEBUG
    puts("got map");
#endif
    wwin= wWindowFor(window);
    if (wwin && wwin->client_win==window) {
	if (wwin->flags.ignore_next_unmap) {
	    wwin->flags.ignore_next_unmap=0;	    
	    return;
	}
	if (wwin->flags.miniaturized) {
	    wDeiconifyWindow(wwin);
	} else {
	    XGrabServer(dpy);
	    XSync(dpy,0);
	    XMapWindow(dpy, wwin->client_win);
	    XMapWindow(dpy, wwin->frame->core->window);
	    wwin->flags.mapped=1;
	    wClientSetState(wwin, NormalState, None);
	    XUngrabServer(dpy);
	}
    }
}


static void
handleUnmapNotify(XEvent *event)
{
    WWindow *wwin;
    XEvent ev;
    
#ifdef DEBUG
    puts("got unmap");
#endif
    wwin = wWindowFor(event->xunmap.window);
    if (!wwin || wwin->client_win!=event->xunmap.window)
	return;
    
    if (!wwin->flags.mapped 
	&& wwin->frame->workspace==wwin->screen_ptr->current_workspace
	&& !wwin->flags.miniaturized && !wwin->flags.hidden)
	return;

    if (wwin->flags.ignore_next_unmap) {
	return;
    }
    XGrabServer(dpy);
    XUnmapWindow(dpy, wwin->frame->core->window);
    wwin->flags.mapped = 0;
    XSync(dpy, 0);
    /* check if the window was destroyed */
    if (XCheckTypedWindowEvent(dpy, wwin->client_win, DestroyNotify,&ev)) {
	DispatchEvent(&ev);
    } else {
	Bool reparented = False;

	if (XCheckTypedWindowEvent(dpy, wwin->client_win, ReparentNotify, &ev))
	    reparented = True;

	/* withdraw window */
	wwin->flags.mapped = 0;
	if (!reparented)
	    wClientSetState(wwin, WithdrawnState, None);
	
	/* if the window was reparented, do not reparent it back to the
	 * root window */
	wUnmanageWindow(wwin, !reparented);
    }
    XUngrabServer(dpy);
}


static void
handleConfigureRequest(XEvent *event)
{
    WWindow *wwin;

#ifdef DEBUG
    puts("got configure request");
#endif
    if (!(wwin=wWindowFor(event->xconfigurerequest.window))) {
	/*
	 * Configure request for unmapped window
	 */
	wClientConfigure(NULL, &(event->xconfigurerequest));
    } else {
	wClientConfigure(wwin, &(event->xconfigurerequest));
    }
}


static void
handlePropertyNotify(XEvent *event)
{
    WWindow *wwin;
    WApplication *wapp;
    Window jr;
    int ji;
    unsigned int ju;

#ifdef DEBUG
    puts("got property notify");
#endif
    if ((wwin=wWindowFor(event->xproperty.window))) {
	if (!XGetGeometry(dpy, wwin->client_win, &jr, &ji, &ji,
			  &ju, &ju, &ju, &ju)) {
	    return;
	}
	wClientCheckProperty(wwin, &event->xproperty);
    }
    wapp = wApplicationOf(event->xproperty.window);
    if (wapp) {
	wClientCheckProperty(wapp->main_window_desc, &event->xproperty);
    }
}


static void
handleClientMessage(XEvent *event)
{
    WWindow *wwin;
    WObjDescriptor *desc;
    
#ifdef DEBUG
    puts("got client message");
#endif
    /* handle transition from Normal to Iconic state */
    if (event->xclient.message_type == _XA_WM_CHANGE_STATE
	&& event->xclient.format == 32 
	&& event->xclient.data.l[0] == IconicState) {
	
	wwin = wWindowFor(event->xclient.window);
	if (!wwin) return;
	if (!wwin->flags.miniaturized)
	    wIconifyWindow(wwin);
    } else if (event->xclient.message_type == _XA_WM_COLORMAP_NOTIFY
	       && event->xclient.format == 32) {
	WScreen *scr = wScreenForRootWindow(event->xclient.window);

	if (!scr)
	    return;

	if (event->xclient.data.l[1] == 1) {   /* starting */
	    wColormapAllowClientInstallation(scr, True);
	} else {		       /* stopping */
	    wColormapAllowClientInstallation(scr, False);
	}
    } else if (event->xclient.message_type == _XA_WINDOWMAKER_WM_FUNCTION) {
	WApplication *wapp;
	int done=0;
	wapp = wApplicationOf(event->xclient.window);
	if (wapp) {
	    switch (event->xclient.data.l[0]) {
	     case WMFHideOtherApplications:
		wHideOtherApplications(wapp->main_window_desc);
		done = 1;
		break;

	     case WMFHideApplication:
		wHideApplication(wapp);
		done = 1;
		break;
	    }
	}
	if (!done) {
	    wwin = wWindowFor(event->xclient.window);
	    if (wwin) {
		switch (event->xclient.data.l[0]) {
		 case WMFHideOtherApplications:
		    wHideOtherApplications(wwin);
		    break;

		 case WMFHideApplication:
		    wHideApplication(wApplicationOf(wwin->main_window));
		    break;
		}
	    }
	}
#ifdef XDE_DND
    } else if (event->xclient.message_type==_XA_XDE_DATA_AVAILABLE) {
	GdkEvent gdkev;
	WScreen *scr = wScreenForWindow(event->xclient.window);
	Atom tmpatom;
	int datalenght;
	long tmplong;
	char * tmpstr, * runstr, * freestr, * tofreestr;
	    printf("x\n");
	gdkev.dropdataavailable.u.allflags = event->xclient.data.l[1];
	gdkev.dropdataavailable.timestamp = event->xclient.data.l[4];

	if(gdkev.dropdataavailable.u.flags.isdrop){
		gdkev.dropdataavailable.type = GDK_DROP_DATA_AVAIL;
		gdkev.dropdataavailable.requestor = event->xclient.data.l[0];
		XGetWindowProperty(dpy,gdkev.dropdataavailable.requestor,
				event->xclient.data.l[2],
				0, LONG_MAX -1,
				0, XA_PRIMARY, &tmpatom,
				&datalenght,
				&gdkev.dropdataavailable.data_numbytes,
				&tmplong,
				&tmpstr);
		datalenght=gdkev.dropdataavailable.data_numbytes-1;
		tofreestr=tmpstr;
		runstr=NULL;
                for(;datalenght>0;datalenght-=(strlen(tmpstr)+1),tmpstr=&tmpstr[strlen(tmpstr)+1]){
		freestr=runstr;runstr=wstrappend(runstr,tmpstr);free(freestr);
		freestr=runstr;runstr=wstrappend(runstr," ");free(freestr);
		}
                free(tofreestr);
		scr->xdestring=runstr;
		/* no need to redirect ? */
	        wDockReceiveDNDDrop(scr,event);
		free(runstr);
		scr->xdestring=NULL;
	}

    } else if (event->xclient.message_type==_XA_XDE_LEAVE) {
	    printf("leave\n");
    } else if (event->xclient.message_type==_XA_XDE_ENTER) {
	GdkEvent gdkev;
	XEvent replyev;

	gdkev.dropenter.u.allflags=event->xclient.data.l[1];
	printf("from win %x\n",event->xclient.data.l[0]);
	printf("to win %x\n",event->xclient.window);
        printf("enter %x\n",event->xclient.data.l[1]);
        printf("v %x ",event->xclient.data.l[2]);
        printf("%x ",event->xclient.data.l[3]);
        printf("%x\n",event->xclient.data.l[4]);

	if(event->xclient.data.l[2]==_XDE_FILETYPE ||
	   event->xclient.data.l[3]==_XDE_FILETYPE ||
	   event->xclient.data.l[4]==_XDE_FILETYPE ||
	   event->xclient.data.l[2]==_XDE_URLTYPE ||
	   event->xclient.data.l[3]==_XDE_URLTYPE ||
	   event->xclient.data.l[4]==_XDE_URLTYPE)
        if(gdkev.dropenter.u.flags.sendreply){
		/*reply*/
            replyev.xclient.type = ClientMessage;
            replyev.xclient.window = event->xclient.data.l[0];
            replyev.xclient.format = 32;
            replyev.xclient.message_type = _XA_XDE_REQUEST;
            replyev.xclient.data.l[0] = event->xclient.window;

            gdkev.dragrequest.u.allflags = 0;
            gdkev.dragrequest.u.flags.protocol_version = 0;
            gdkev.dragrequest.u.flags.willaccept = 1;
            gdkev.dragrequest.u.flags.delete_data = 0;

            replyev.xclient.data.l[1] = gdkev.dragrequest.u.allflags;
            replyev.xclient.data.l[2] = replyev.xclient.data.l[3] = 0;
            replyev.xclient.data.l[4] = event->xclient.data.l[2];
            XSendEvent(dpy, replyev.xclient.window, 0, NoEventMask, &replyev);
            XSync(dpy, 0);
        }
#endif /* XDE_DND */
#ifdef OFFIX_DND
    } else if (event->xclient.message_type==_XA_DND_PROTOCOL) {
	WScreen *scr = wScreenForWindow(event->xclient.window);
        if (scr && wDockReceiveDNDDrop(scr,event))
	    goto redirect_message;
#endif /* OFFIX_DND */
    } else {
#ifdef OFFIX_DND
     redirect_message:
#endif
	/*
	 * Non-standard thing, but needed by OffiX DND.
	 * For when the icon frame gets a ClientMessage
	 * that should have gone to the icon_window.
	 */
	if (XFindContext(dpy, event->xbutton.window, wWinContext, 
			 (XPointer *)&desc)!=XCNOENT) {
	    struct WIcon *icon=NULL;

	    if (desc->parent_type == WCLASS_MINIWINDOW) {
		icon = (WIcon*)desc->parent;
	    } else if (desc->parent_type == WCLASS_DOCK_ICON
		     || desc->parent_type == WCLASS_APPICON) {
		icon = ((WAppIcon*)desc->parent)->icon;
	    }
	    if (icon && (wwin=icon->owner)) {
		if (wwin->client_win!=event->xclient.window) {
		    event->xclient.window = wwin->client_win;
		    XSendEvent(dpy, wwin->client_win, False, NoEventMask, 
			       event);
		}
	    }
	}
    }
}


static void
raiseWindow(WScreen *scr)
{
    WWindow *wwin;

    scr->autoRaiseTimer = NULL;

    wwin = wWindowFor(scr->autoRaiseWindow);    
    if (!wwin)
	return;

    if (!wwin->flags.destroyed) {
	wRaiseFrame(wwin->frame->core);
	/* this is needed or a race condition will occur */
	XSync(dpy, False);
    }
}


static void 
handleEnterNotify(XEvent *event)
{
    WWindow *wwin;
    WObjDescriptor *desc = NULL;
    XEvent ev;
    WScreen *scr = wScreenForRootWindow(event->xcrossing.root);


#ifdef DEBUG
    puts("got enter notify");
#endif

    if (XCheckTypedWindowEvent(dpy, event->xcrossing.window, LeaveNotify,
			       &ev)) {
	/* already left the window... */
	saveTimestamp(&ev);
	if (ev.xcrossing.mode==event->xcrossing.mode
	    && ev.xcrossing.detail==event->xcrossing.detail) {
	    return;
	}
    }

    if (XFindContext(dpy, event->xcrossing.window, wWinContext,
                     (XPointer *)&desc)!=XCNOENT) {
        if(desc->handle_enternotify)
            (*desc->handle_enternotify)(desc, event);
    }

    /* enter to window */
    wwin = wWindowFor(event->xcrossing.window);
    if (!wwin) {
	if (wPreferences.focus_mode==WKF_POINTER
	    && event->xcrossing.window==event->xcrossing.root) {
	    wSetFocusTo(scr, NULL);
	}
	if (wPreferences.colormap_mode==WKF_POINTER) {
	    wColormapInstallForWindow(scr, NULL);
	}
	if (scr->autoRaiseTimer 
	    && event->xcrossing.root==event->xcrossing.window) {
	    WMDeleteTimerHandler(scr->autoRaiseTimer);
	    scr->autoRaiseTimer = NULL;
	}
    } else {
	/* set focus if in focus-follows-mouse mode and the event
	 * is for the frame window and window doesn't have focus yet */
	if ((wPreferences.focus_mode==WKF_POINTER
	     || wPreferences.focus_mode==WKF_SLOPPY)
	    && wwin->frame->core->window==event->xcrossing.window
	    && !wwin->flags.focused) {
	    wSetFocusTo(scr, wwin);
	    
	    if (scr->autoRaiseTimer)
		WMDeleteTimerHandler(scr->autoRaiseTimer);
	    scr->autoRaiseTimer = NULL;
	    
	    if (wPreferences.raise_delay && !wwin->window_flags.no_focusable) {
		scr->autoRaiseWindow = wwin->frame->core->window;
		scr->autoRaiseTimer
		    = WMAddTimerHandler(wPreferences.raise_delay,
					(WMCallback*)raiseWindow, scr);
	    }
	}
	/* Install colormap for window, if the colormap installation mode
	 * is colormap_follows_mouse */
	if (wPreferences.colormap_mode==WKF_POINTER) {
	    if (wwin->client_win==event->xcrossing.window)
		wColormapInstallForWindow(scr, wwin);
	    else
		wColormapInstallForWindow(scr, NULL);
	}
    }
    
    /* a little kluge to hide the clip balloon */
    if (!wPreferences.flags.noclip && scr->flags.clip_balloon_mapped) {
	if (!desc) {
	    XUnmapWindow(dpy, scr->clip_balloon);
	    scr->flags.clip_balloon_mapped = 0;
	} else {
	    if (desc->parent_type!=WCLASS_DOCK_ICON
		|| scr->clip_icon != desc->parent) {
		XUnmapWindow(dpy, scr->clip_balloon);
		scr->flags.clip_balloon_mapped = 0;
	    }
	}
    }

    if (event->xcrossing.window == event->xcrossing.root
	&& event->xcrossing.detail == NotifyNormal
	&& event->xcrossing.detail != NotifyInferior
	&& wPreferences.focus_mode != WKF_CLICK) {

	wSetFocusTo(scr, scr->focused_window);
    }

#ifdef BALLOON_TEXT
    wBalloonEnteredObject(scr, desc);
#endif
}


static void
handleLeaveNotify(XEvent *event)
{
    WObjDescriptor *desc = NULL;
    
    if (XFindContext(dpy, event->xcrossing.window, wWinContext, 
		     (XPointer *)&desc)!=XCNOENT) {
        if(desc->handle_leavenotify)
	    (*desc->handle_leavenotify)(desc, event);
    }
    if (event->xcrossing.window == event->xcrossing.root
	&& event->xcrossing.mode == NotifyNormal
	&& event->xcrossing.detail != NotifyInferior
	&& wPreferences.focus_mode != WKF_CLICK) {

	WScreen *scr = wScreenForRootWindow(event->xcrossing.root);

	wSetFocusTo(scr, NULL);
    }
}


#ifdef SHAPE
static void
handleShapeNotify(XEvent *event)
{
    XShapeEvent *shev = (XShapeEvent*)event;
    WWindow *wwin;
    XEvent ev;

#ifdef DEBUG
    puts("got shape notify");
#endif

    while (XCheckTypedWindowEvent(dpy, shev->window, event->type, &ev)) {
	XShapeEvent *sev = (XShapeEvent*)&ev;

	if (sev->kind == ShapeBounding) {
	    if (sev->shaped == shev->shaped) {
		*shev = *sev;
	    } else {
		XPutBackEvent(dpy, &ev);
		break;
	    }
	}
    }

    wwin = wWindowFor(shev->window);
    if (!wwin || shev->kind != ShapeBounding)
      return;

    if (!shev->shaped && wwin->flags.shaped) {

	wwin->flags.shaped = 0;
	wWindowClearShape(wwin);

    } else if (shev->shaped) {

	wwin->flags.shaped = 1;
	wWindowSetShape(wwin);
    }
}
#endif /* SHAPE */


static void 
handleColormapNotify(XEvent *event)
{
    WWindow *wwin;
    WScreen *scr;
    Bool reinstall = False;
    
    wwin = wWindowFor(event->xcolormap.window);
    if (!wwin)
	return;
    
    scr = wwin->screen_ptr;

    do {
	if (wwin) {
	    if (event->xcolormap.new) {
		XWindowAttributes attr;
		
		XGetWindowAttributes(dpy, wwin->client_win, &attr);

		if (wwin == scr->cmap_window && wwin->cmap_window_no == 0)
		    scr->current_colormap = attr.colormap;

		reinstall = True;
	    } else if (event->xcolormap.state == ColormapUninstalled &&
		       scr->current_colormap == event->xcolormap.colormap) {

		/* some bastard app (like XV) removed our colormap */
		/* 
		 * can't enforce or things like xscreensaver wont work
		 * reinstall = True;
		 */
	    } else if (event->xcolormap.state == ColormapInstalled &&
		       scr->current_colormap == event->xcolormap.colormap) {

		/* someone has put our colormap back */
		reinstall = False;
	    }
	}
    } while (XCheckTypedEvent(dpy, ColormapNotify, event)
	     && ((wwin = wWindowFor(event->xcolormap.window)) || 1));

    if (reinstall && scr->current_colormap!=None) {
	if (!scr->flags.colormap_stuff_blocked)
	    XInstallColormap(dpy, scr->current_colormap);
    }
}



static void
handleFocusIn(XEvent *event)
{
    WWindow *wwin;

    /*
     * For applications that like stealing the focus.
     */
    while (XCheckTypedEvent(dpy, FocusIn, event));    
    saveTimestamp(event);
    if (event->xfocus.mode == NotifyUngrab
	|| event->xfocus.mode == NotifyGrab
	|| event->xfocus.detail > NotifyNonlinearVirtual) {
	return;
    }

    wwin = wWindowFor(event->xfocus.window);
    if (wwin && !wwin->flags.focused) {
	if (wwin->flags.mapped)
	    wSetFocusTo(wwin->screen_ptr, wwin);
	else
	    wSetFocusTo(wwin->screen_ptr, NULL);
    } else if (!wwin) {
	WScreen *scr = wScreenForWindow(event->xfocus.window);
	if (scr)
	    wSetFocusTo(scr, NULL);
    }
}


static WWindow*
windowUnderPointer(WScreen *scr)
{
    unsigned int mask;
    int foo;
    Window bar, win;
    
    if (XQueryPointer(dpy, scr->root_win, &bar, &win, &foo, &foo, &foo, &foo,
		      &mask)) 
	return wWindowFor(win);
    return NULL;
}

static void
handleKeyPress(XEvent *event)
{
    WScreen *scr = wScreenForRootWindow(event->xkey.root);
    WWindow *wwin = scr->focused_window;
    int i;
    int modifiers;
    int command=-1;
#ifdef KEEP_XKB_LOCK_STATUS   
    XkbStateRec staterec;     
#endif /*KEEP_XKB_LOCK_STATUS*/

    /* ignore CapsLock */
    modifiers = event->xkey.state & ValidModMask;

    for (i=0; i<WKBD_LAST; i++) {
	if (wKeyBindings[i].keycode==0) 
	    continue;
	
	if (wKeyBindings[i].keycode==event->xkey.keycode
	    && (/*wKeyBindings[i].modifier==0 
		||*/ wKeyBindings[i].modifier==modifiers)) {
	    command = i;
	    break;
	}
    }
    
    if (command < 0) {
	if (!wRootMenuPerformShortcut(event)) {
	    static int dontLoop = 0;
	    
	    if (dontLoop > 10) {
		wwarning("problem with key event processing code");
		return;
	    }
	    dontLoop++;
	    /* if the focused window is an internal window, try redispatching
	     * the event to the managed window, as it can be a WINGs window */
	    if (wwin && wwin->flags.internal_window
		&& wwin->client_leader!=None) {
		/* client_leader contains the WINGs toplevel */
		event->xany.window = wwin->client_leader;
		WMHandleEvent(event);
	    }
	    dontLoop--;
	}
	return;
    }

#define ISMAPPED(w) ((w) && !(w)->flags.miniaturized && ((w)->flags.mapped || (w)->flags.shaded))
#define ISFOCUSED(w) ((w) && (w)->flags.focused)

    switch (command) {
     case WKBD_ROOTMENU:
	OpenRootMenu(scr, event->xkey.x_root, event->xkey.y_root, True);
	break;
     case WKBD_WINDOWMENU:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin))
	    OpenWindowMenu(wwin, wwin->frame_x, 
			   wwin->frame_y+wwin->frame->top_width, True);
	break;
     case WKBD_WINDOWLIST:
	OpenSwitchMenu(scr, event->xkey.x_root, event->xkey.y_root, True);
	break;
     case WKBD_MINIATURIZE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)) {
	    CloseWindowMenu(scr);
	    
	    if (wwin->protocols.MINIATURIZE_WINDOW)
	      	wClientSendProtocol(wwin, _XA_GNUSTEP_WM_MINIATURIZE_WINDOW,
				    event->xbutton.time);
	    else {
		if (!wwin->window_flags.no_miniaturizable)
		    wIconifyWindow(wwin);
	    }
	}
	break;
     case WKBD_HIDE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)) {
	    WApplication *wapp = wApplicationOf(wwin->main_window);
	    CloseWindowMenu(scr);
	    
	    if (wapp && !wapp->main_window_desc->window_flags.no_appicon) {
		wHideApplication(wapp);
	    }
	}
	break;
     case WKBD_MAXIMIZE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin) 
	    && !wwin->window_flags.no_resizable) {
	    CloseWindowMenu(scr);
	    
	    if (wwin->flags.maximized) {
		wUnmaximizeWindow(wwin);
	    } else {
		wMaximizeWindow(wwin, MAX_VERTICAL|MAX_HORIZONTAL);
	    }
	}
	break;
     case WKBD_VMAXIMIZE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)
	    && !wwin->window_flags.no_resizable) {
	    CloseWindowMenu(scr);
	    
	    if (wwin->flags.maximized) {
		wUnmaximizeWindow(wwin);
	    } else {
		wMaximizeWindow(wwin, MAX_VERTICAL);
	    }
	}
	break;
     case WKBD_RAISE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)) {
	    CloseWindowMenu(scr);
	    
	    wRaiseFrame(wwin->frame->core);
	}
	break;
     case WKBD_LOWER:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)) {
	    CloseWindowMenu(scr);

	    wLowerFrame(wwin->frame->core);
	}
	break;
     case WKBD_RAISELOWER:
	/* raise or lower the window under the pointer, not the
	 * focused one 
	 */
	wwin = windowUnderPointer(scr);
	if (wwin)
	    wRaiseLowerFrame(wwin->frame->core);
	break;
     case WKBD_SHADE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)
	    && !wwin->window_flags.no_shadeable) {
	    if (wwin->flags.shaded)
		wUnshadeWindow(wwin);
	    else
		wShadeWindow(wwin);
	}
	break;
     case WKBD_CLOSE:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)
	    && !wwin->window_flags.no_closable) {
	    CloseWindowMenu(scr);
	    if (wwin->protocols.DELETE_WINDOW)
	      wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW,
				  event->xkey.time);
	}
	break;
    case WKBD_SELECT:
	if (ISMAPPED(wwin) && ISFOCUSED(wwin)) {
	    wSelectWindow(wwin, !wwin->flags.selected);
	}
        break;
     case WKBD_FOCUSNEXT:
	wwin = NextFocusWindow(scr);
        if (wwin != NULL) {
            wSetFocusTo(scr, wwin);
            if (wPreferences.circ_raise)
                wRaiseFrame(wwin->frame->core);
        }
	break;
     case WKBD_FOCUSPREV:
	wwin = PrevFocusWindow(scr);
        if (wwin != NULL) {
            wSetFocusTo(scr, wwin);
            if (wPreferences.circ_raise)
                wRaiseFrame(wwin->frame->core);
        }
	break;
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define GOTOWORKS(wk)	case WKBD_WORKSPACE##wk:\
			i = (scr->current_workspace/10)*10 + wk - 1;\
			if (wPreferences.ws_advance || i<scr->workspace_count)\
			    wWorkspaceChange(scr, i);\
			break
#else
#define GOTOWORKS(wk)	case WKBD_WORKSPACE/**/wk:\
			i = (scr->current_workspace/10)*10 + wk - 1;\
			if (wPreferences.ws_advance || i<scr->workspace_count)\
			    wWorkspaceChange(scr, i);\
			break
#endif
    GOTOWORKS(1);
    GOTOWORKS(2);
    GOTOWORKS(3);
    GOTOWORKS(4);
    GOTOWORKS(5);
    GOTOWORKS(6);
    GOTOWORKS(7);
    GOTOWORKS(8);
    GOTOWORKS(9);
    GOTOWORKS(10);
#undef GOTOWORKS
     case WKBD_NEXTWORKSPACE:
	if (scr->current_workspace < scr->workspace_count-1)
            wWorkspaceChange(scr, scr->current_workspace+1);
        else if (scr->current_workspace == scr->workspace_count-1) {
            if (wPreferences.ws_advance &&
                scr->current_workspace < MAX_WORKSPACES-1)
                wWorkspaceChange(scr, scr->current_workspace+1);
            else if (wPreferences.ws_cycle)
                wWorkspaceChange(scr, 0);
        }
	break;
     case WKBD_PREVWORKSPACE:
	if (scr->current_workspace > 0)
	    wWorkspaceChange(scr, scr->current_workspace-1);
        else if (scr->current_workspace==0 && wPreferences.ws_cycle)
            wWorkspaceChange(scr, scr->workspace_count-1);
	break;
     case WKBD_WINDOW1:
     case WKBD_WINDOW2:
     case WKBD_WINDOW3:
     case WKBD_WINDOW4:
        if (scr->shortcutWindow[command-WKBD_WINDOW1]) {
            wMakeWindowVisible(scr->shortcutWindow[command-WKBD_WINDOW1]);
        } else if (wwin && ISMAPPED(wwin) && ISFOCUSED(wwin)) {
            scr->shortcutWindow[command-WKBD_WINDOW1] = wwin;
	    wSelectWindow(wwin, !wwin->flags.selected);
	    XFlush(dpy);
	    wusleep(3000);
	    wSelectWindow(wwin, !wwin->flags.selected);
	    XFlush(dpy);
        }
        break;
     case WKBD_NEXTWSLAYER:
     case WKBD_PREVWSLAYER:
	{
	    int row, column;
	    
	    row = scr->current_workspace/10;
	    column = scr->current_workspace%10;
	    
	    if (command==WKBD_NEXTWSLAYER) {
		if ((row+1)*10 < scr->workspace_count)
		    wWorkspaceChange(scr, column+(row+1)*10);
	    } else {
		if (row > 0)
		    wWorkspaceChange(scr, column+(row-1)*10);
	    }
	}
	break;
    case WKBD_CLIPLOWER:
        if (!wPreferences.flags.noclip)
            wDockLower(scr->workspaces[scr->current_workspace]->clip);
        break;
     case WKBD_CLIPRAISE:
        if (!wPreferences.flags.noclip)
            wDockRaise(scr->workspaces[scr->current_workspace]->clip);
        break;
     case WKBD_CLIPRAISELOWER:
        if (!wPreferences.flags.noclip)
            wDockRaiseLower(scr->workspaces[scr->current_workspace]->clip);
        break;
#ifdef KEEP_XKB_LOCK_STATUS
     case WKBD_TOGGLE:
        if(wPreferences.modelock){
	    XkbGetState(dpy,XkbUseCoreKbd,&staterec);
	    /*toggle*/
	    XkbLockGroup(dpy,XkbUseCoreKbd,
			 wwin->languagemode=staterec.compat_state&32?0:1);
        }
        break;
#endif /* KEEP_XKB_LOCK_STATUS */

    }
}


static void 
handleMotionNotify(XEvent *event)
{
    WMenu *menu;
    WScreen *scr = wScreenForRootWindow(event->xmotion.root);

    if (wPreferences.scrollable_menus) {
        if (event->xmotion.x_root <= 1 ||
            event->xmotion.x_root >= (scr->scr_width - 2) ||
            event->xmotion.y_root <= 1 ||
            event->xmotion.y_root >= (scr->scr_height - 2)) {

#ifdef DEBUG
            puts("pointer at screen edge");
#endif

            menu = wMenuUnderPointer(scr);
            if (menu!=NULL)
                wMenuScroll(menu, event);
        }
    }
}
