/*
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
#include <signal.h>
#include <sys/wait.h>
#ifdef __FreeBSD__
#include <sys/signal.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>

#include "WindowMaker.h"
#include "GNUstep.h"
#ifdef MWM_HINTS
# include "motif.h"
#endif
#include "texture.h"
#include "screen.h"
#include "window.h"
#include "actions.h"
#include "client.h"
#include "funcs.h"
#include "dock.h"
#include "workspace.h"
#include "keybind.h"
#include "framewin.h"
#include "session.h"

#include "xutil.h"

#ifdef WMSOUND
#include "wmsound.h"
#endif


#ifdef SYS_SIGLIST_DECLARED
extern const char * const sys_siglist[];
#endif
/****** Global Variables ******/

extern WPreferences wPreferences;

extern WShortKey wKeyBindings[WKBD_LAST];

/* currently only single screen support */
extern WScreen *wScreen;

/* contexts */
extern XContext wWinContext;
extern XContext wAppWinContext;
extern XContext wStackContext;

/* atoms */
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_TAKE_FOCUS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_SAVE_YOURSELF;
extern Atom _XA_WM_CLIENT_LEADER;
extern Atom _XA_WM_COLORMAP_WINDOWS;

extern Atom _XA_GNUSTEP_WM_ATTR;

extern Atom _XA_WINDOWMAKER_MENU;
extern Atom _XA_WINDOWMAKER_WM_PROTOCOLS;
extern Atom _XA_WINDOWMAKER_STATE;
extern Atom _XA_WINDOWMAKER_WM_FUNCTION;

extern Atom _XA_WINDOWMAKER_WM_MINIATURIZE_WINDOW;
extern Atom _XA_GNUSTEP_WM_RESIZEBAR;

#ifdef OFFIX_DND
extern Atom _XA_DND_PROTOCOL;
extern Atom _XA_DND_SELECTION;
#endif


/* cursors */
extern Cursor wCursor[WCUR_LAST];

static void manageAllWindows();

extern void NotifyDeadProcess(pid_t pid, unsigned char status);



static int 
catchXError(Display *dpy, XErrorEvent *error)
{
    char buffer[MAXLINE];
    
    /* ignore some errors */
    if (error->resourceid != None 
	&& ((error->error_code == BadDrawable 
	     && error->request_code == X_GetGeometry)
	    || (error->error_code == BadMatch
		&& (error->request_code == X_SetInputFocus))
	    || (error->error_code == BadWindow)
	    /*
		&& (error->request_code == X_GetWindowAttributes
		    || error->request_code == X_SetInputFocus
		    || error->request_code == X_ChangeWindowAttributes
		    || error->request_code == X_GetProperty
		    || error->request_code == X_ChangeProperty
		    || error->request_code == X_QueryTree
		    || error->request_code == X_GrabButton
		    || error->request_code == X_UngrabButton
		    || error->request_code == X_SendEvent
		    || error->request_code == X_ConfigureWindow))
	     */
	    || (error->request_code == X_InstallColormap))) {
#ifndef DEBUG
	return 0;
#else
	printf("got X error %x %x %x\n", error->request_code,
	       error->error_code, (unsigned)error->resourceid);
	return 0;
#endif
    }
    FormatXError(dpy, error, buffer, MAXLINE); 
    wwarning(_("internal X error: %s\n"), buffer);
    return -1;
}


/*
 *---------------------------------------------------------------------- 
 * handleXIO-
 * 	Handle X shutdowns and other stuff. 
 *---------------------------------------------------------------------- 
 */
static int
handleXIO(Display *dpy)
{
    exit(0);
}


/*
 *----------------------------------------------------------------------
 * handleSig--
 * 	general signal handler. Exits the program gently.
 *---------------------------------------------------------------------- 
 */
static RETSIGTYPE
handleSig(int sig)
{
#ifndef NO_EMERGENCY_AUTORESTART
    char *argv[2];
    
    argv[1] = NULL;
#endif
    
    /* 
     * No functions that potentially do Xlib calls should be called from
     * here. Xlib calls are not atomic so, so the logical integrity of
     * Xlib is not guaranteed if a Xlib call is made from a signal handler.
     */
    if (sig == SIGHUP) {
#ifdef SYS_SIGLIST_DECLARED
        wwarning(_("got signal %i (%s) - restarting\n"), sig, sys_siglist[sig]);
#else
        wwarning(_("got signal %i - restarting\n"), sig);
#endif
        if (wScreen) {
	    wScreen->flags.restart_asap = 1;
	    return;
        }
    } else if (sig==SIGTERM) {
	printf(_("%s: Received signal SIGTERM. Exiting..."), ProgName);
	
	if (wScreen) {
	    wScreen->flags.exit_asap = 1;
            return;
	}
    }

#ifdef SYS_SIGLIST_DECLARED
    wfatal(_("got signal %i (%s)\n"), sig, sys_siglist[sig]);
#else
    wfatal(_("got signal %i\n"), sig);
#endif
    
    
#ifndef NO_EMERGENCY_AUTORESTART
    if (sig==SIGSEGV || sig==SIGFPE || sig==SIGBUS) {
    	/* restart another window manager so that the X session doesn't
	 * go to space */

    	wwarning(_("trying to start alternative window manager..."));
    	if (dpy)
	    XCloseDisplay(dpy);
    	dpy = NULL;
    
    	argv[0] = FALLBACK_WINDOWMANAGER;
    	execvp(FALLBACK_WINDOWMANAGER, argv);

    	argv[0] = "fvwm";
    	execvp("fvwm", argv);

    	argv[0] = "twm";
    	execvp("twm", argv);
    }
#endif /* !NO_EMERGENCY_AUTORESTART */
    
    wAbort(sig==SIGSEGV);
}


static RETSIGTYPE
ignoreSig(int signal)
{
    return;
}


static RETSIGTYPE
buryChild(int foo)
{
    pid_t pid;
    int status;
    
    /* R.I.P. */
    pid = waitpid(-1, &status, WNOHANG);
    if (pid>0) {
	NotifyDeadProcess(pid, WEXITSTATUS(status));
    }
}


static int
getWorkspaceState(Window root, WWorkspaceState **state)            
{
    Atom type_ret;
    int fmt_ret;    
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;

    if (XGetWindowProperty(dpy, root, _XA_WINDOWMAKER_STATE, 0,
                           sizeof(WWorkspaceState),
                           True, _XA_WINDOWMAKER_STATE,
                           &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
                           (unsigned char **)state)!=Success)
      return 0;
    if (type_ret==_XA_WINDOWMAKER_STATE)
      return 1;
    else
      return 0;
}


       
/*
 *---------------------------------------------------------- 
 * StartUp--
 * 	starts the window manager and setup global data.
 * Called from main() at startup.
 * 
 * Side effects:
 * global data declared in main.c is initialized
 *----------------------------------------------------------
 */
void 
StartUp()
{
    WWorkspaceState *ws_state;
    struct sigaction sig_action;
    int i;
    
    memset(&wKeyBindings, 0, sizeof(wKeyBindings));

    wWinContext = XUniqueContext();
    wAppWinContext = XUniqueContext();
    wStackContext = XUniqueContext();
    _XA_WM_STATE = XInternAtom(dpy, "WM_STATE", False);
    _XA_WM_CHANGE_STATE = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    _XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    _XA_WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    _XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    _XA_WM_SAVE_YOURSELF = XInternAtom(dpy, "WM_SAVE_YOURSELF", False);    
    _XA_WM_CLIENT_LEADER = XInternAtom(dpy, "WM_CLIENT_LEADER", False);
    _XA_WM_COLORMAP_WINDOWS = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    
    _XA_GNUSTEP_WM_ATTR = XInternAtom(dpy, GNUSTEP_WM_ATTR_NAME, False);
#ifdef MWM_HINTS
    _XA_MOTIF_WM_HINTS = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
#endif

    _XA_WINDOWMAKER_MENU = XInternAtom(dpy, "_WINDOWMAKER_MENU", False);
    _XA_WINDOWMAKER_STATE = XInternAtom(dpy, "_WINDOWMAKER_STATE", False);

    _XA_WINDOWMAKER_WM_PROTOCOLS =
      XInternAtom(dpy, "_WINDOWMAKER_WM_PROTOCOLS", False);

    _XA_WINDOWMAKER_WM_MINIATURIZE_WINDOW = 
      XInternAtom(dpy, WINDOWMAKER_WM_MINIATURIZE_WINDOW, False);
    
    _XA_GNUSTEP_WM_RESIZEBAR =
      XInternAtom(dpy, GNUSTEP_WM_RESIZEBAR, False);

    _XA_WINDOWMAKER_WM_FUNCTION = XInternAtom(dpy, "_WINDOWMAKER_WM_FUNCTION",
					  False);


#ifdef OFFIX_DND
    _XA_DND_SELECTION = XInternAtom(dpy, "DndSelection", False);
    _XA_DND_PROTOCOL = XInternAtom(dpy, "DndProtocol", False);
#endif

    /* cursors */
    wCursor[WCUR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);    
    wCursor[WCUR_ARROW] = XCreateFontCursor(dpy, XC_top_left_arrow);
    wCursor[WCUR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
    wCursor[WCUR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
    wCursor[WCUR_WAIT] = XCreateFontCursor(dpy, XC_watch);
    wCursor[WCUR_QUESTION] = XCreateFontCursor(dpy, XC_question_arrow);
    wCursor[WCUR_TEXT]     = XCreateFontCursor(dpy, XC_xterm); /* odd name???*/
    
    /* emergency exit... */
    signal(SIGINT, handleSig);
    signal(SIGTERM, handleSig);
    signal(SIGQUIT, handleSig);
    signal(SIGSEGV, handleSig);
    signal(SIGBUS, handleSig);
    signal(SIGFPE, handleSig);
    signal(SIGHUP, handleSig);

    /* handle X shutdowns a such */
    XSetIOErrorHandler(handleXIO);
    
    /* handle dead children */
    sig_action.sa_handler = buryChild;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_NOCLDSTOP|SA_RESTART;
    sigaction(SIGCHLD, &sig_action, NULL);

    /* ignore dead pipe */
    sig_action.sa_handler = ignoreSig;
    sigemptyset(&sig_action.sa_mask);
    sigaction(SIGPIPE, &sig_action, NULL);

    /* set hook for out event dispatcher in WINGs event dispatcher */
    WMHookEventHandler(DispatchEvent);

    /* Sound init */
#ifdef WMSOUND
    wSoundInit(dpy);
#endif

    XSetErrorHandler((XErrorHandler)catchXError);

    /* right now, only works for the default screen */
    wScreen = wScreenInit(DefaultScreen(dpy));

    /* restore workspace state */
    if (!getWorkspaceState(wScreen->root_win, &ws_state)) {
        ws_state = NULL;
    }

    wScreenRestoreState(wScreen);

    /* manage all windows that were already 
     * here before us */
    if (!wPreferences.flags.nodock && wScreen->dock)
        wScreen->last_dock = wScreen->dock;

    manageAllWindows(wScreen);

    /* restore saved menus */
    wMenuRestoreState(wScreen);

    /* If we're not restarting restore session */
    if (ws_state == NULL)
        wSessionRestoreState(wScreen);


    /* auto-launch apps */
    if (!wPreferences.flags.nodock && wScreen->dock) {
        wScreen->last_dock = wScreen->dock;
	wDockDoAutoLaunch(wScreen->dock, 0);
    }
    /* auto-launch apps in clip */
    if (!wPreferences.flags.noclip) {
        for(i=0; i<wScreen->workspace_count; i++) {
            if (wScreen->workspaces[i]->clip) {
                wScreen->last_dock = wScreen->workspaces[i]->clip;
                wDockDoAutoLaunch(wScreen->workspaces[i]->clip, i);
            }
        }
    }

    /* go to workspace where we were before restart */
    if (ws_state) { 
	wWorkspaceForceChange(wScreen, ws_state->workspace);
        XFree(ws_state);
    } else {
        wSessionRestoreLastWorkspace(wScreen);
    }
}



static int
getState(Window window)
{
    Atom type;
    int form;
    unsigned long nitems, bytes_rem;
    unsigned char *data;
    long ret;

    if (XGetWindowProperty(dpy, window, _XA_WM_STATE, 0, 3, False,
			   _XA_WM_STATE, &type,&form,&nitems,&bytes_rem,
			   &data)==Success) {
	if (data != NULL) {
	    ret = *(long*)data;
	    free(data);
	    return ret;
	}
    }
    return -1;
}


/*
 *-----------------------------------------------------------------------
 * manageAllWindows--
 * 	Manages all windows in the screen.
 * 
 * Notes:
 * 	Called when the wm is being started.
 *	No events can be processed while the windows are being
 * reparented/managed. 
 *----------------------------------------------------------------------- 
 */
static void
manageAllWindows(WScreen *scr)
{
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    XWindowAttributes wattribs;
    unsigned int i, j;
    int state;
    WWindow *wwin;
    XWMHints *wmhints;
    
    XGrabServer(dpy);
    XQueryTree(dpy, scr->root_win, &root, &parent, &children, &nchildren);

    scr->flags.startup = 1;

    /* first remove all icon windows */
    for (i=0; i<nchildren; i++) {
	if (children[i]==None) 
	  continue;

    	wmhints = XGetWMHints(dpy, children[i]);
	if (wmhints && (wmhints->flags & IconWindowHint)) {
	    for (j = 0; j < nchildren; j++)  {
		if (children[j] == wmhints->icon_window) {
		    XFree(wmhints);
		    wmhints = NULL;
		    children[j] = None;
		    break;
		}
	    }
	}
	if (wmhints) {
	    XFree(wmhints);
	}
    }
    /* map all windows without OverrideRedirect */
    for (i=0; i<nchildren; i++) {
	if (children[i]==None) 
	  continue;

	XGetWindowAttributes(dpy, children[i], &wattribs);

	state = getState(children[i]);
	if (!wattribs.override_redirect 
	    && (state>=0 || wattribs.map_state!=IsUnmapped)) {
	    XUnmapWindow(dpy, children[i]);
	    
	    if (state==WithdrawnState) {
		/* move the window far away so that it doesn't flash */
		XMoveWindow(dpy, children[i], scr->scr_width+10,
			    scr->scr_height+10);
	    }

	    wwin = wManageWindow(scr, children[i]);
	    if (wwin) {
		if (state==WithdrawnState) {
		    wwin->flags.mapped = 0;
		    wClientSetState(wwin, WithdrawnState, None);
		    XSelectInput(dpy, wwin->client_win, NoEventMask);
		    XRemoveFromSaveSet(dpy, wwin->client_win);
		    wUnmanageWindow(wwin, True);
                } else {
                    /* apply states got from WSavedState */
                    /* shaded + minimized is not restored correctly */
                    if (wwin->flags.shaded) {
			wwin->flags.shaded = 0;
                        wShadeWindow(wwin);
                    }
                    if (wwin->flags.hidden) {
                        WApplication *wapp = wApplicationOf(wwin->main_window);
                        wwin->flags.hidden = 0;
                        if (wapp) {
                            wHideApplication(wapp);
                        }
                        wwin->flags.ignore_next_unmap=1;
                    } else {
                        if (wwin->wm_hints &&
                            (wwin->wm_hints->flags & StateHint) && state<0)
                            state=wwin->wm_hints->initial_state;
                        if (state==IconicState) {
                            wIconifyWindow(wwin);
                            wwin->flags.ignore_next_unmap=1;
                        } else {
                            wClientSetState(wwin, NormalState, None);
                        }
                    }
                }
            }
	    if (state==WithdrawnState) {
		/* move the window back to it's old position */
		XMoveWindow(dpy, children[i], wattribs.x, wattribs.y);
	    }
	}
    }
    scr->flags.startup = 0;
    XUngrabServer(dpy);
    XFree(children);
    while (XPending(dpy)) {
	XEvent ev;
	WMNextEvent(dpy, &ev);
	WMHandleEvent(&ev);
    }
    wWorkspaceChange(scr, 0);
    if (!wPreferences.flags.noclip)
        wDockShowIcons(scr->workspaces[scr->current_workspace]->clip);
}



