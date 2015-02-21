/* event.c- event loop and handling
 * 
 *  WindowMaker window manager
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


/******** Global Variables **********/
extern XContext wWinContext;

extern Cursor wCursor[WCUR_LAST];

extern WShortKey wKeyBindings[WKBD_LAST];

extern WScreen *wScreen;

extern Time LastTimestamp;
extern Time LastFocusChange;

extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask

extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WINDOWMAKER_WM_MINIATURIZE_WINDOW;
extern Atom _XA_WINDOWMAKER_WM_FUNCTION;

#ifdef OFFIX_DND
extern Atom _XA_DND_PROTOCOL;
#endif

#ifdef SHAPE
extern int ShapeEventBase;
#endif

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
static void handleFocusOut();
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

static WWindowState *windowState=NULL;




WMagicNumber
wAddWindowSavedState(char *instance, char *class, char *command,
                     pid_t pid, WSavedState *state)
{
    WWindowState *wstate;

    wstate = malloc(sizeof(WWindowState));
    if (!wstate)
        return 0;

    memset(wstate, 0, sizeof(WWindowState));
    wstate->pid = pid;
    if (instance)
        wstate->instance = wstrdup(instance);
    if (class)
        wstate->class = wstrdup(class);
    if (command)
        wstate->command = wstrdup(command);
    wstate->state = state;

    wstate->next = windowState;
    windowState = wstate;

#ifdef DEBUG
    printf("Added WindowState with ID %p, for %s.%s : \"%s\"\n", wstate, instance,
           class, command);
#endif

    return wstate;
}


#define SAME(x, y) (((x) && (y) && !strcmp((x), (y))) || (!(x) && !(y)))


WMagicNumber
wGetWindowSavedState(Window win)
{
    char *instance, *class, *command=NULL;
    WWindowState *wstate = windowState;
    char **argv;
    int argc;

    if (!wstate)
        return NULL;

    if (XGetCommand(dpy, win, &argv, &argc) && argc>0) {
        command = FlattenStringList(argv, argc);
        XFreeStringList(argv);
    }
    if (!command)
        return NULL;

    if (PropGetWMClass(win, &class, &instance)) {
        while (wstate) {
            if (SAME(instance, wstate->instance) &&
                SAME(class, wstate->class) &&
                SAME(command, wstate->command)) {
                break;
            }
            wstate = wstate->next;
        }
    } else {
        wstate = NULL;
    }

#ifdef DEBUG
    printf("Read WindowState with ID %p, for %s.%s : \"%s\"\n", wstate, instance,
           class, command);
#endif

    if (command) free(command);
    if (instance) XFree(instance);
    if (class) XFree(class);

    return wstate;
}


void 
wDeleteWindowSavedState(WMagicNumber id)
{
    WWindowState *tmp, *wstate=(WWindowState*)id;

    if (!wstate || !windowState)
        return;

    tmp = windowState;
    if (tmp==wstate) {
        windowState = wstate->next;
#ifdef DEBUG
        printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
               wstate, wstate->instance, wstate->class, wstate->command);
#endif
        if (wstate->instance) free(wstate->instance);
        if (wstate->class)    free(wstate->class);
        if (wstate->command)  free(wstate->command);
        free(wstate->state);
        free(wstate);
    } else {
	while (tmp->next) {
	    if (tmp->next==wstate) {
		tmp->next=wstate->next;
#ifdef DEBUG
                printf("Deleted WindowState with ID %p, for %s.%s : \"%s\"\n",
                       wstate, wstate->instance, wstate->class, wstate->command);
#endif
                if (wstate->instance) free(wstate->instance);
                if (wstate->class)    free(wstate->class);
                if (wstate->command)  free(wstate->command);
                free(wstate->state);
                free(wstate);
		break;
	    }
	    tmp = tmp->next;
	}
    }
}


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
    if (deathHandler)
	handleDeadProcess(NULL);
    
    if (wScreen->flags.exit_asap) {
	/* received SIGTERM */
	wScreenSaveState(wScreen);
	
	RestoreDesktop(wScreen);
	exit(0);
    } else if (wScreen->flags.restart_asap) {
	/* received SIGHUP */
	wScreenSaveState(wScreen);

	RestoreDesktop(wScreen);
	Restart(NULL);
    }
    
    saveTimestamp(event);    
    switch (event->type) {
     case MapRequest:
	handleMapRequest(event->xmaprequest.window);
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
	
     case FocusOut:
	handleFocusOut(event);
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
    WWindowState *wins;

    int tmpPtr = deadProcessPtr;

    if (windowState) {
        while (tmpPtr>0) {
            tmpPtr--;

            wins = windowState;
            while (wins) {
                WWindowState *t;

                t = wins->next;

                if (wins->pid == deadProcesses[tmpPtr].pid) {
                    wDeleteWindowSavedState(wins);
                }
                wins = t;
            }
        }
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
    if (event->type == (ShapeEventBase+ShapeNotify)) {
	handleShapeNotify(event);
    }
#endif	
}

static void 
handleMapRequest(Window window)
{
    WWindow *wwin;

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

    wwin = wManageWindow(wScreen, window);

    /* 
     * This is to let the Dock know that the application it launched
     * has already been mapped (eg: it has finished launching). 
     * It is not necessary for normally docked apps, but is needed for
     * apps that were forcedly docked (like with dockit).
     */
    if (wScreen->last_dock) {
	if (wwin && wwin->main_window!=None && wwin->main_window!=window)
	  wDockTrackWindowLaunch(wScreen->last_dock, wwin->main_window);
	else
	  wDockTrackWindowLaunch(wScreen->last_dock, window);
    }

    if (wwin) {
	int state;
#if 0
	if (wwin->window_flags.start_withdrawn) {
	    wwin->window_flags.start_withdrawn = 0;
	    state = WithdrawnState;
	} else
#endif
	{
	    if (wwin->wm_hints && (wwin->wm_hints->flags & StateHint))
	      state = wwin->wm_hints->initial_state;
	    else
	      state = NormalState;
        }

        if (state==IconicState)
            wwin->flags.miniaturized = 1;

        if (state==WithdrawnState) {
            wwin->flags.mapped = 0;
            wClientSetState(wwin, WithdrawnState, None);
            XSelectInput(dpy, wwin->client_win, NoEventMask);
            XRemoveFromSaveSet(dpy, wwin->client_win);
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
#if 0
        switch (state) {	    
	 case WithdrawnState:
	    wwin->flags.mapped = 0;
	    wClientSetState(wwin, WithdrawnState, None);
	    XSelectInput(dpy, wwin->client_win, NoEventMask);
	    XRemoveFromSaveSet(dpy, wwin->client_win);
	    wUnmanageWindow(wwin, True);
	    break;
	    
	 case IconicState:
            if (!wwin->flags.miniaturized) {
                wwin->flags.ignore_next_unmap=1;
                wwin->flags.skip_next_animation=1;
                wIconifyWindow(wwin);
            }
	    break;
/*
	 case DontCareState:
	 case NormalState:
 */
	 default:
	    wClientSetState(wwin, NormalState, None);
	    break;
        }
#endif
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
      
#ifdef DEBUG    
    puts("got button press");
#endif
    if (event->xbutton.window==wScreen->root_win) {
	int menuButton, selectButton;
	
	if (wPreferences.swap_menu_buttons) {
	    menuButton = Button1;
	    selectButton = Button3;
	} else {
	    menuButton = Button3;
	    selectButton = Button1;
	}
	if (event->xbutton.button==menuButton) {
	    OpenRootMenu(wScreen, event->xbutton.x_root,
                         event->xbutton.y_root, False);
	    /* ugly hack */
	    if (wScreen->root_menu) {
		if (wScreen->root_menu->brother->flags.mapped)
		  event->xbutton.window = wScreen->root_menu->brother->frame->core->window;
		else
		  event->xbutton.window = wScreen->root_menu->frame->core->window;
	    }
	} else if (event->xbutton.button==selectButton) {
	    
	    wUnselectWindows();
	    wSelectWindows(wScreen, event);
	    
	} else if (event->xbutton.button==Button2) {
	    
	    OpenSwitchMenu(wScreen, event->xbutton.x_root,
			   event->xbutton.y_root, False);
	    if (wScreen->switch_menu) {
		if (wScreen->switch_menu->brother->flags.mapped)
		  event->xbutton.window = wScreen->switch_menu->brother->frame->core->window;
		else
		  event->xbutton.window = wScreen->switch_menu->frame->core->window;
	    }
	}
    }
    if (XFindContext(dpy, event->xbutton.window, wWinContext, 
		 (XPointer *)&desc)==XCNOENT) {
	return;
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
	XSync(dpy, 0);
	XAllowEvents(dpy, AsyncPointer, CurrentTime);
	XSync(dpy, 0);
    }

    if (desc->handle_mousedown!=NULL) {
	(*desc->handle_mousedown)(desc, event);
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
    XSync(dpy, 0);
    /* check if the window was destroyed */
    if (XCheckTypedWindowEvent(dpy, wwin->client_win, DestroyNotify,&ev)) {
	DispatchEvent(&ev);
    } else {
	/* withdraw window */
	wwin->flags.mapped = 0;
	XSelectInput(dpy, wwin->client_win, NoEventMask);
	XRemoveFromSaveSet(dpy, wwin->client_win);
	wClientSetState(wwin, WithdrawnState, None);
	wUnmanageWindow(wwin, True);
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
#ifdef OFFIX_DND
    } else if (event->xclient.message_type==_XA_DND_PROTOCOL) {
        if (wDockReceiveDNDDrop(wScreen, event))
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
handleEnterNotify(XEvent *event)
{
    WWindow *wwin;
    static WMagicNumber *prev_window=NULL;
    WObjDescriptor *desc;

#ifdef DEBUG
    puts("got enter notify");
#endif

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
	    wSetFocusTo(wScreen, NULL);
	}
	if (wPreferences.colormap_mode==WKF_POINTER) {
	    InstallWindowColormaps(NULL);
	}
	if (prev_window && event->xcrossing.root==event->xcrossing.window) {
	    WMDeleteTimerHandler(prev_window);
	    prev_window = NULL;
	}

	return;
    }
    /* Install colormap for window, if the colormap installation mode
     * is colormap_follows_mouse */
    if (wPreferences.colormap_mode==WKF_POINTER) {
	if (wwin->client_win==event->xcrossing.window)
	    InstallWindowColormaps(wwin);
	else
	    InstallWindowColormaps(NULL);
    }

    /* set focus if in focus-follows-mouse mode and the event
     * is for the frame window and window doesn't have focus yet */
    if ((wPreferences.focus_mode==WKF_POINTER
	 || wPreferences.focus_mode==WKF_SLOPPY)
	&& wwin->frame->core->window==event->xcrossing.window
	&& !wwin->flags.focused) {
	WMDeleteTimerHandler(prev_window);
	prev_window = NULL;

	wSetFocusTo(wwin->screen_ptr, wwin);
	if (wPreferences.raise_delay && wwin->window_flags.focusable) {
	    prev_window = WMAddTimerHandler(wPreferences.raise_delay,
					   (WMCallback*)wRaiseFrame,
					   wwin->frame->core);
	}
    }
}


static void
handleLeaveNotify(XEvent *event)
{
    WObjDescriptor *desc;

    if (XFindContext(dpy, event->xcrossing.window, wWinContext, 
		     (XPointer *)&desc)!=XCNOENT) {
        if(desc->handle_leavenotify)
	    (*desc->handle_leavenotify)(desc, event);
        
    }
}


#ifdef SHAPE
static void
handleShapeNotify(XEvent *event)
{
    XShapeEvent *shev = (XShapeEvent*)event;
    WWindow *wwin;
    
#ifdef DEBUG
    puts("got shape notify");
#endif
    wwin = wWindowFor(event->xany.window);
    if (!wwin || shev->kind != ShapeBounding)
      return;
    
    wwin->flags.shaped = shev->shaped;
    wWindowSetShape(wwin);
}
#endif /* SHAPE */


extern HandleColormapNotify();

static void 
handleColormapNotify(XEvent *event)
{
    WWindow *wwin;
    
    wwin = wWindowFor(event->xcolormap.window);
    HandleColormapNotify(wwin, event);
}


static void
handleFocusIn(XEvent *event)
{
#if 0
    WWindow *wwin;

    if (event->xfocus.mode != NotifyNormal
	&& event->xfocus.mode != NotifyWhileGrabbed)
	return;
    
    wwin = wWindowFor(event->xfocus.window);
    if (wwin) {
    }
#endif
}

static void
handleFocusOut(XEvent *event)
{
#if 0
    WWindow *wwin;

    if (event->xfocus.mode != NotifyNormal
	&& event->xfocus.mode != NotifyWhileGrabbed)
	return;

    wwin = wWindowFor(event->xfocus.window);
    if (wwin && wwin->flags.focused && !wwin->flags.shaded) {
	Window win;
	int foo;

	XGetInputFocus(dpy, &win, &foo);
	
	if (win!=wwin->client_win)
	    wWindowUnfocus(wwin);
    }
#endif
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
    WWindow *wwin = wScreen->focused_window;
    int i;
    int modifiers;
    int command=-1;

    /* ignore CapsLock */
    modifiers = event->xkey.state & ~IGNORE_MOD_MASK;
    
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
	wRootMenuPerformShortcut(event);
	return;
    }

#define ISMAPPED(w) ((w) && !(w)->flags.miniaturized && ((w)->flags.mapped || (w)->flags.shaded))

    switch (command) {
     case WKBD_ROOTMENU:
	OpenRootMenu(wScreen, event->xkey.x_root, event->xkey.y_root, True);
	break;
     case WKBD_WINDOWMENU:
	if (ISMAPPED(wwin))
	    OpenWindowMenu(wwin, wwin->frame_x, 
			   wwin->frame_y+wwin->frame->top_width, True);
	break;
     case WKBD_WINDOWLIST:
	OpenSwitchMenu(wScreen, event->xkey.x_root, event->xkey.y_root, True);
	break;
     case WKBD_MINIATURIZE:
	if (ISMAPPED(wwin)) {
	    CloseWindowMenu(wScreen);
	    
	    if (wwin->protocols.MINIATURIZE_WINDOW)
	      	wClientSendProtocol(wwin, _XA_WINDOWMAKER_WM_MINIATURIZE_WINDOW,
				    event->xbutton.time);
	    else {
		if (wwin->window_flags.miniaturizable)
		    wIconifyWindow(wwin);
	    }
	}
	break;
     case WKBD_HIDE:
	if (ISMAPPED(wwin)) {
	    CloseWindowMenu(wScreen);
	    
	    if (wwin->main_window && wwin->window_flags.application) {
		wHideApplication(wApplicationOf(wwin->main_window));
	    }
	}
	break;
     case WKBD_MAXIMIZE:
	if (ISMAPPED(wwin) && wwin->window_flags.resizable) {
	    CloseWindowMenu(wScreen);
	    
	    if (wwin->flags.maximized) {
		wUnmaximizeWindow(wwin);
	    } else {
		wMaximizeWindow(wwin, MAX_VERTICAL|MAX_HORIZONTAL);
	    }
	}
	break;
     case WKBD_VMAXIMIZE:
	if (ISMAPPED(wwin) && wwin->window_flags.resizable) {
	    CloseWindowMenu(wScreen);
	    
	    if (wwin->flags.maximized) {
		wUnmaximizeWindow(wwin);
	    } else {
		wMaximizeWindow(wwin, MAX_VERTICAL);
	    }
	}
	break;
     case WKBD_RAISE:
	if (ISMAPPED(wwin)) {
	    CloseWindowMenu(wScreen);
	    
	    wRaiseFrame(wwin->frame->core);
	}
	break;
     case WKBD_LOWER:
	if (ISMAPPED(wwin)) {
	    CloseWindowMenu(wScreen);

	    wLowerFrame(wwin->frame->core);
	}
	break;
     case WKBD_RAISELOWER:
	/* raise or lower the window under the pointer, not the
	 * focused one 
	 */
	wwin = windowUnderPointer(wScreen);
	if (wwin)
	    wRaiseLowerFrame(wwin->frame->core);
	break;
     case WKBD_SHADE:
	if (ISMAPPED(wwin) &&wwin->window_flags.shadeable) {
	    if (wwin->flags.shaded)
		wUnshadeWindow(wwin);
	    else
		wShadeWindow(wwin);
	}
	break;
     case WKBD_CLOSE:
	if (ISMAPPED(wwin) && wwin->window_flags.closable) {
	    CloseWindowMenu(wScreen);
	    if (wwin->protocols.DELETE_WINDOW)
	      wClientSendProtocol(wwin, _XA_WM_DELETE_WINDOW,
				  event->xkey.time);
	}
	break;
     case WKBD_FOCUSNEXT:
	wwin = NextFocusWindow(wScreen);
        if (wwin != NULL) {
            wSetFocusTo(wScreen, wwin);
            if (wPreferences.circ_raise)
                wRaiseFrame(wwin->frame->core);
        }
	break;
     case WKBD_FOCUSPREV:
	wwin = PrevFocusWindow(wScreen);
        if (wwin != NULL) {
            wSetFocusTo(wScreen, wwin);
            if (wPreferences.circ_raise)
                wRaiseFrame(wwin->frame->core);
        }
	break;
    case WKBD_WORKSPACE1:
	wWorkspaceChange(wScreen, 0);
	break;
     case WKBD_WORKSPACE2:
	wWorkspaceChange(wScreen, 1);
	break;
     case WKBD_WORKSPACE3:
	wWorkspaceChange(wScreen, 2);
	break;
     case WKBD_WORKSPACE4:
	wWorkspaceChange(wScreen, 3);
	break;
     case WKBD_WORKSPACE5:
	wWorkspaceChange(wScreen, 4);
	break;
     case WKBD_WORKSPACE6:
	wWorkspaceChange(wScreen, 5);
	break;
     case WKBD_WORKSPACE7:
	wWorkspaceChange(wScreen, 6);
	break;
     case WKBD_WORKSPACE8:
	wWorkspaceChange(wScreen, 7);
	break;
     case WKBD_WORKSPACE9:
	wWorkspaceChange(wScreen, 8);
	break;
     case WKBD_WORKSPACE10:
	wWorkspaceChange(wScreen, 9);
	break;
     case WKBD_NEXTWORKSPACE:
	if (wScreen->current_workspace < wScreen->workspace_count-1)
            wWorkspaceChange(wScreen, wScreen->current_workspace+1);
        else if (wScreen->current_workspace == wScreen->workspace_count-1) {
            if (wPreferences.ws_advance &&
                wScreen->current_workspace < MAX_WORKSPACES-1)
                wWorkspaceChange(wScreen, wScreen->current_workspace+1);
            else if (wPreferences.ws_cycle)
                wWorkspaceChange(wScreen, 0);
        }
	break;
     case WKBD_PREVWORKSPACE:
	if (wScreen->current_workspace > 0)
	    wWorkspaceChange(wScreen, wScreen->current_workspace-1);
        else if (wScreen->current_workspace==0 && wPreferences.ws_cycle)
            wWorkspaceChange(wScreen, wScreen->workspace_count-1);
	break;
	
     case WKBD_NEXTWSLAYER:
     case WKBD_PREVWSLAYER:
	{
	    int row, column;
	    
	    row = wScreen->current_workspace/10;
	    column = wScreen->current_workspace%10;
	    
	    if (command==WKBD_NEXTWSLAYER) {
		if ((row+1)*10 < wScreen->workspace_count)
		    wWorkspaceChange(wScreen, column+(row+1)*10);
	    } else {
		if (row > 0)
		    wWorkspaceChange(wScreen, column+(row-1)*10);
	    }
	}
	break;
    case WKBD_CLIPLOWER:
        if (!wPreferences.flags.noclip)
            wDockLower(wScreen->workspaces[wScreen->current_workspace]->clip);
        break;
     case WKBD_CLIPRAISE:
        if (!wPreferences.flags.noclip)
            wDockRaise(wScreen->workspaces[wScreen->current_workspace]->clip);
        break;
     case WKBD_CLIPRAISELOWER:
        if (!wPreferences.flags.noclip)
            wDockRaiseLower(wScreen->workspaces[wScreen->current_workspace]->clip);
        break;
    }
}


static void handleMotionNotify(XEvent *event)
{
    WMenu *menu;

    if (wPreferences.scrollable_menus) {
        if (event->xmotion.x_root <= 1 ||
            event->xmotion.x_root >= (wScreen->scr_width - 2) ||
            event->xmotion.y_root <= 1 ||
            event->xmotion.y_root >= (wScreen->scr_height - 2)) {

#ifdef DEBUG
            puts("pointer at screen edge");
#endif

            menu = wMenuUnderPointer(wScreen);
            if (menu!=NULL)
                wMenuScroll(menu, event);
        }
    }
}
