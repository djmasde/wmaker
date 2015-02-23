/*
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
#include <X11/keysym.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

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
#include "defaults.h"

#include "xutil.h"

#ifdef WMSOUND
#include "wmsound.h"
#endif

#if 0
#ifdef SYS_SIGLIST_DECLARED
extern const char * const sys_siglist[];
#endif
#endif

/* for SunOS */
#ifndef SA_RESTART
# define SA_RESTART 0
#endif


/****** Global Variables ******/

extern WPreferences wPreferences;

extern WDDomain *WDWindowMaker;
extern WDDomain *WDRootMenu;
extern WDDomain *WDWindowAttributes;

extern WShortKey wKeyBindings[WKBD_LAST];

extern int wScreenCount;


#ifdef SHAPE
extern Bool wShapeSupported;
extern int wShapeEventBase;
#endif

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
extern Atom _XA_WM_COLORMAP_NOTIFY;

extern Atom _XA_GNUSTEP_WM_ATTR;

extern Atom _XA_WINDOWMAKER_MENU;
extern Atom _XA_WINDOWMAKER_WM_PROTOCOLS;
extern Atom _XA_WINDOWMAKER_STATE;
extern Atom _XA_WINDOWMAKER_WM_FUNCTION;

extern Atom _XA_GNUSTEP_WM_MINIATURIZE_WINDOW;

#ifdef OFFIX_DND
extern Atom _XA_DND_PROTOCOL;
extern Atom _XA_DND_SELECTION;
#endif
#ifdef XDE_DND
extern Atom _XA_XDE_REQUEST;
extern Atom _XA_XDE_ENTER;
extern Atom _XA_XDE_LEAVE;
extern Atom _XA_XDE_DATA_AVAILABLE;
extern Atom _XDE_FILETYPE;
extern Atom _XDE_URLTYPE;
#endif


/* cursors */
extern Cursor wCursor[WCUR_LAST];

/* special flags */
extern char WProgramState;
extern char WDelayedActionSet;

/***** Local *****/

static WScreen **wScreen = NULL;


static unsigned int _NumLockMask = 0;
static unsigned int _ScrollLockMask = 0;



static void manageAllWindows();

extern void NotifyDeadProcess(pid_t pid, unsigned char status);

#ifdef R6SM
extern void _wSessionCloseDescriptors();
#endif


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
    Exit(0);
}


/*
 *----------------------------------------------------------------------
 * delayedAction-
 * 	Action to be executed after the signal() handler is exited.
 *----------------------------------------------------------------------
 */
static void
delayedAction(void *cdata)
{
    WDelayedActionSet = 0;
    /* 
     * Make the event dispatcher do whatever it needs to do,
     * including handling zombie processes, restart and exit 
     * signals.
     */
    DispatchEvent(NULL);
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
    static int already_crashed = 0;
    int dumpcore = 0;
#ifndef NO_EMERGENCY_AUTORESTART
    char *argv[2];
    
    argv[1] = NULL;
#endif
    
    /* 
     * No functions that potentially do Xlib calls should be called from
     * here. Xlib calls are not reentrant so the integrity of Xlib is
     * not guaranteed if a Xlib call is made from a signal handler.
     */
    if (sig == SIGHUP) {
#ifdef SYS_SIGLIST_DECLARED
        wwarning(_("got signal %i (%s) - restarting\n"), sig, sys_siglist[sig]);
#else
        wwarning(_("got signal %i - restarting\n"), sig);
#endif

	WProgramState = WSTATE_NEED_RESTART;

	/* setup idle handler, so that this will be handled when
	 * the select() is returned becaused of the signal, even if
	 * there are no X events in the queue */
	if (!WDelayedActionSet) {
	    WDelayedActionSet = 1;
	    WMAddIdleHandler(delayedAction, NULL);
	}
        return;
    } else if (sig == SIGTERM) {
	printf(_("%s: Received signal SIGTERM. Exiting..."), ProgName);

	WProgramState = WSTATE_NEED_EXIT;

	if (!WDelayedActionSet) {
	    WDelayedActionSet = 1;
	    WMAddIdleHandler(delayedAction, NULL);
	}
	return;
    }

#ifdef SYS_SIGLIST_DECLARED
    wfatal(_("got signal %i (%s)\n"), sig, sys_siglist[sig]);
#else
    wfatal(_("got signal %i\n"), sig);
#endif

    if (sig==SIGSEGV || sig==SIGFPE || sig==SIGBUS || sig==SIGILL) {
	if (already_crashed) {
	    wfatal(_("crashed while trying to do some post-crash cleanup. Aborting immediatelly."));
	    abort();
	}
	already_crashed = 1;

	dumpcore = 1;

#ifndef NO_EMERGENCY_AUTORESTART
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
#endif /* !NO_EMERGENCY_AUTORESTART */
    }

    wAbort(dumpcore);
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
	/* 
	 * Make sure that the kid will be buried even if there are
	 * no events in the X event queue
	 */
	if (!WDelayedActionSet) {
	    WDelayedActionSet = 1;
	    WMAddIdleHandler(delayedAction, NULL);
	}
    }
}


static int
getWorkspaceState(Window root, WWorkspaceState **state)            
{
    Atom type_ret;
    int fmt_ret;    
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;
    CARD32 *data;

    if (XGetWindowProperty(dpy, root, _XA_WINDOWMAKER_STATE, 0, 2,
                           True, _XA_WINDOWMAKER_STATE,
                           &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
                           (unsigned char **)&data)!=Success || !data)
      return 0;

    *state = malloc(sizeof(WWorkspaceState));
    if (*state) {
	(*state)->flags = data[0];
	(*state)->workspace = data[1];
    }
    XFree(data);

    if (*state && type_ret==_XA_WINDOWMAKER_STATE)
      return 1;
    else
      return 0;
}


static void
getOffendingModifiers()
{
    int i;
    XModifierKeymap *modmap;
    KeyCode nlock, slock;
    static mask_table[8] = {
	ShiftMask,LockMask,ControlMask,Mod1Mask,
	    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
    };

    nlock = XKeysymToKeycode(dpy, XK_Num_Lock);
    slock = XKeysymToKeycode(dpy, XK_Scroll_Lock);

    /*
     * Find out the masks for the NumLock and ScrollLock modifiers,
     * so that we can bind the grabs for when they are enabled too.
     */
    modmap = XGetModifierMapping(dpy);
    
    if (modmap!=NULL && modmap->max_keypermod>0) {
	for (i=0; i<8*modmap->max_keypermod; i++) {
	    if (modmap->modifiermap[i]==nlock && nlock!=0)
		_NumLockMask = mask_table[i/modmap->max_keypermod];
	    else if (modmap->modifiermap[i]==slock && slock!=0)
		_ScrollLockMask = mask_table[i/modmap->max_keypermod];
	}
    }

    if (modmap)
	XFreeModifiermap(modmap);
}



#ifdef NUMLOCK_HACK
void 
wHackedGrabKey(int keycode, unsigned int modifiers,
	       Window grab_window, Bool owner_events, int pointer_mode,
	       int keyboard_mode)
{
    if (modifiers == AnyModifier)
	return;
    
    /* grab all combinations of the modifier with CapsLock, NumLock and
     * ScrollLock. How much memory/CPU does such a monstrosity consume
     * in the server?
     */
    if (_NumLockMask)
	XGrabKey(dpy, keycode, modifiers|_NumLockMask, 
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    if (_ScrollLockMask)
	XGrabKey(dpy, keycode, modifiers|_ScrollLockMask,
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    if (_NumLockMask && _ScrollLockMask)
	XGrabKey(dpy, keycode, modifiers|_NumLockMask|_ScrollLockMask,
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    if (_NumLockMask)
	XGrabKey(dpy, keycode, modifiers|_NumLockMask|LockMask,
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    if (_ScrollLockMask)
	XGrabKey(dpy, keycode, modifiers|_ScrollLockMask|LockMask,
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    if (_NumLockMask && _ScrollLockMask)
	XGrabKey(dpy, keycode, modifiers|_NumLockMask|_ScrollLockMask|LockMask,
		 grab_window, owner_events, pointer_mode, keyboard_mode);
    /* phew, I guess that's all, right? */
}
#endif

void 
wHackedGrabButton(unsigned int button, unsigned int modifiers, 
		  Window grab_window, Bool owner_events, 
		  unsigned int event_mask, int pointer_mode, 
		  int keyboard_mode, Window confine_to, Cursor cursor)
{
    XGrabButton(dpy, button, modifiers, grab_window, owner_events,
		event_mask, pointer_mode, keyboard_mode, confine_to, cursor);

    if (modifiers==AnyModifier)
	return;

    XGrabButton(dpy, button, modifiers|LockMask, grab_window, owner_events,
		event_mask, pointer_mode, keyboard_mode, confine_to, cursor);
    
#ifdef NUMLOCK_HACK    
    /* same as above, but for mouse buttons */
    if (_NumLockMask)
	XGrabButton(dpy, button, modifiers|_NumLockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		keyboard_mode, confine_to, cursor);
    if (_ScrollLockMask)
	XGrabButton(dpy, button, modifiers|_ScrollLockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, cursor);
    if (_NumLockMask && _ScrollLockMask)
	XGrabButton(dpy, button, modifiers|_ScrollLockMask|_NumLockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, cursor);
    if (_NumLockMask)
	XGrabButton(dpy, button, modifiers|_NumLockMask|LockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		keyboard_mode, confine_to, cursor);
    if (_ScrollLockMask)
	XGrabButton(dpy, button, modifiers|_ScrollLockMask|LockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, cursor);
    if (_NumLockMask && _ScrollLockMask)
	XGrabButton(dpy, button, modifiers|_ScrollLockMask|_NumLockMask|LockMask,
		    grab_window, owner_events, event_mask, pointer_mode,
		    keyboard_mode, confine_to, cursor);
#endif /* NUMLOCK_HACK */
}

#ifdef notused
void
wHackedUngrabButton(unsigned int button, unsigned int modifiers, 
		    Window grab_window)
{
    XUngrabButton(dpy, button, modifiers|_NumLockMask,
		  grab_window);
    XUngrabButton(dpy, button, modifiers|_ScrollLockMask,
		  grab_window);
    XUngrabButton(dpy, button, modifiers|_NumLockMask|_ScrollLockMask,
		  grab_window);
    XUngrabButton(dpy, button, modifiers|_NumLockMask|LockMask,
		  grab_window);
    XUngrabButton(dpy, button, modifiers|_ScrollLockMask|LockMask,
		  grab_window);
    XUngrabButton(dpy, button, modifiers|_NumLockMask|_ScrollLockMask|LockMask,
		  grab_window);
}
#endif



WScreen*
wScreenWithNumber(int i)
{
    assert(i < wScreenCount);
    
    return wScreen[i];
}

WScreen*
wScreenForRootWindow(Window window)
{
    int i;

    if (wScreenCount==1)
	return wScreen[0];

    /*
     * Since the number of heads will probably be small (normally 2),
     * it should be faster to use this than a hash table, because
     * of the overhead.
     */
    for (i=0; i<wScreenCount; i++) {
	if (wScreen[i]->root_win == window) {
	    return wScreen[i];
	}
    }

    assert("bad_root_window" && 0);
    return NULL;
}

WScreen*
wScreenForWindow(Window window)
{
    XWindowAttributes attr;

    if (wScreenCount==1)
	return wScreen[0];

    if (XGetWindowAttributes(dpy, window, &attr)) {
	return wScreenForRootWindow(attr.root);
    }
    return NULL;
}


void
CloseDescriptors()
{
    if (dpy)
	close(ConnectionNumber(dpy));
#ifdef R6SM
    _wSessionCloseDescriptors();
#endif
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
StartUp(Bool defaultScreenOnly)
{
    WWorkspaceState *ws_state;
    struct sigaction sig_action;
    int j, max;

    /*
     * Ignore CapsLock in modifiers
     */
    ValidModMask = 0xff & ~LockMask;

    getOffendingModifiers();
    /*
     * Ignore NumLock and ScrollLock too
     */
    ValidModMask &= ~(_NumLockMask|_ScrollLockMask);


    memset(&wKeyBindings, 0, sizeof(wKeyBindings));

    wWinContext = XUniqueContext();
    wAppWinContext = XUniqueContext();
    wStackContext = XUniqueContext();

/*    _XA_VERSION = XInternAtom(dpy, "VERSION", False);*/

    _XA_WM_STATE = XInternAtom(dpy, "WM_STATE", False);
    _XA_WM_CHANGE_STATE = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    _XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
    _XA_WM_TAKE_FOCUS = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    _XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    _XA_WM_SAVE_YOURSELF = XInternAtom(dpy, "WM_SAVE_YOURSELF", False);    
    _XA_WM_CLIENT_LEADER = XInternAtom(dpy, "WM_CLIENT_LEADER", False);
    _XA_WM_COLORMAP_WINDOWS = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    _XA_WM_COLORMAP_NOTIFY = XInternAtom(dpy, "WM_COLORMAP_NOTIFY", False);

    _XA_GNUSTEP_WM_ATTR = XInternAtom(dpy, GNUSTEP_WM_ATTR_NAME, False);
#ifdef MWM_HINTS
    _XA_MOTIF_WM_HINTS = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
#endif

    _XA_WINDOWMAKER_MENU = XInternAtom(dpy, "_WINDOWMAKER_MENU", False);
    _XA_WINDOWMAKER_STATE = XInternAtom(dpy, "_WINDOWMAKER_STATE", False);

    _XA_WINDOWMAKER_WM_PROTOCOLS =
      XInternAtom(dpy, "_WINDOWMAKER_WM_PROTOCOLS", False);

    _XA_GNUSTEP_WM_MINIATURIZE_WINDOW = 
      XInternAtom(dpy, GNUSTEP_WM_MINIATURIZE_WINDOW, False);
    
    _XA_WINDOWMAKER_WM_FUNCTION = XInternAtom(dpy, "_WINDOWMAKER_WM_FUNCTION",
					  False);


#ifdef OFFIX_DND
    _XA_DND_SELECTION = XInternAtom(dpy, "DndSelection", False);
    _XA_DND_PROTOCOL = XInternAtom(dpy, "DndProtocol", False);
#endif
#ifdef XDE_DND
    _XA_XDE_ENTER = XInternAtom(dpy, "_XDE_ENTER", False);
    _XA_XDE_REQUEST = XInternAtom(dpy, "_XDE_REQUEST", False);
    _XA_XDE_LEAVE = XInternAtom(dpy, "_XDE_LEAVE", False);
    _XA_XDE_DATA_AVAILABLE = XInternAtom(dpy, "_XDE_DATA_AVAILABLE", False);
    _XDE_FILETYPE = XInternAtom(dpy, "file:ALL", False);
    _XDE_URLTYPE = XInternAtom(dpy, "url:ALL", False);
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
    sig_action.sa_handler = handleSig;
    sigemptyset(&sig_action.sa_mask);

    /* Here we don't care about SA_RESTART since these signals will close
     * wmaker anyway.
     * -Dan */
    sig_action.sa_flags = 0;
    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
    sigaction(SIGQUIT, &sig_action, NULL);
    sigaction(SIGSEGV, &sig_action, NULL);
    sigaction(SIGBUS, &sig_action, NULL);
    sigaction(SIGFPE, &sig_action, NULL);

    /* Here we set SA_RESTART for safety, because SIGHUP may not be handled
     * immediately.
     * -Dan */
    sig_action.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sig_action, NULL);

    /* ignore dead pipe */
    sig_action.sa_handler = ignoreSig;
    sig_action.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sig_action, NULL);

    /* handle dead children */
    sig_action.sa_handler = buryChild;
    sig_action.sa_flags = SA_NOCLDSTOP|SA_RESTART;
    sigaction(SIGCHLD, &sig_action, NULL);

    /* handle X shutdowns a such */
    XSetIOErrorHandler(handleXIO);

    /* set hook for out event dispatcher in WINGs event dispatcher */
    WMHookEventHandler(DispatchEvent);

    /* initialize defaults stuff */
    WDWindowMaker = wDefaultsInitDomain("WindowMaker", True);
    if (!WDWindowMaker->dictionary) {
	wwarning(_("could not read domain \"%s\" from defaults database"),
		 "WindowMaker");
    }

    /* read defaults that don't change until a restart and are
     * screen independent */
    wReadStaticDefaults(WDWindowMaker ? WDWindowMaker->dictionary : NULL);

    /* check sanity of some values */
    if (wPreferences.icon_size < 16) {
	wwarning(_("icon size is configured to %i, but it's too small. Using 16, instead\n"),
		 wPreferences.icon_size);
	wPreferences.icon_size = 16;
    }

    /* init other domains */
    WDRootMenu = wDefaultsInitDomain("WMRootMenu", False);
    if (!WDRootMenu->dictionary) {
	wwarning(_("could not read domain \"%s\" from defaults database"),
		 "WMRootMenu");
    }

    WDWindowAttributes = wDefaultsInitDomain("WMWindowAttributes", True);
    if (!WDWindowAttributes->dictionary) {
	wwarning(_("could not read domain \"%s\" from defaults database"),
		 "WMWindowAttributes");
    }

    XSetErrorHandler((XErrorHandler)catchXError);

    /* Sound init */
#ifdef WMSOUND
    wSoundInit(dpy);
#endif

#ifdef SHAPE
    /* ignore j */
    wShapeSupported = XShapeQueryExtension(dpy, &wShapeEventBase, &j);
#endif

    if (defaultScreenOnly) {
	max = 1;
    } else {
	max = ScreenCount(dpy);
    }
    wScreen = wmalloc(sizeof(WScreen*)*max);

    wScreenCount = 0;

    /* manage the screens */
    for (j = 0; j < max; j++) {
	if (defaultScreenOnly || max==1) {
	    wScreen[wScreenCount] = wScreenInit(DefaultScreen(dpy));
	    if (!wScreen[wScreenCount]) {
		wfatal(_("it seems that there already is a window manager running"));
		Exit(1);
	    }
	} else {
	    wScreen[wScreenCount] = wScreenInit(j);
	    if (!wScreen[wScreenCount]) {
		wwarning(_("could not manage screen %i"), j);
		continue;
	    }
	}
	wScreenCount++;
    }

    /* initialize/restore state for the screens */
    for (j = 0; j < wScreenCount; j++) {
	/* restore workspace state */
	if (!getWorkspaceState(wScreen[j]->root_win, &ws_state)) {
	    ws_state = NULL;
	}

	wScreenRestoreState(wScreen[j]);

	/* manage all windows that were already here before us */
	if (!wPreferences.flags.nodock && wScreen[j]->dock)
	    wScreen[j]->last_dock = wScreen[j]->dock;
	
	manageAllWindows(wScreen[j]);
	
	/* restore saved menus */
	wMenuRestoreState(wScreen[j]);
	
	/* If we're not restarting restore session */
	if (ws_state == NULL)
	    wSessionRestoreState(wScreen[j]);
	
	
	/* auto-launch apps */
	if (!wPreferences.flags.nodock && wScreen[j]->dock) {
	    wScreen[j]->last_dock = wScreen[j]->dock;
	    wDockDoAutoLaunch(wScreen[j]->dock, 0);
	}
	/* auto-launch apps in clip */
	if (!wPreferences.flags.noclip) {
	    int i;
	    for(i=0; i<wScreen[j]->workspace_count; i++) {
		if (wScreen[j]->workspaces[i]->clip) {
		    wScreen[j]->last_dock = wScreen[j]->workspaces[i]->clip;
		    wDockDoAutoLaunch(wScreen[j]->workspaces[i]->clip, i);
		}
	    }
	}

	/* go to workspace where we were before restart */
	if (ws_state) { 
	    wWorkspaceForceChange(wScreen[j], ws_state->workspace);
	    free(ws_state);
	} else {
	    wSessionRestoreLastWorkspace(wScreen[j]);
	}
    }
    
    if (wScreenCount == 0) {
	wfatal(_("could not manage any screen"));
	Exit(1);
    }

    if (!wPreferences.flags.noupdates) {
	/* setup defaults file polling */
	WMAddTimerHandler(3000, wDefaultsCheckDomains, NULL);
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


static Bool
windowInList(Window window, Window *list, int count)
{
    for (; count>=0; count--) {
	if (window == list[count])
	    return True;
    }
    return False;
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
			    /* iconify if it's not a transient */
			    if (wwin->transient_for==None 
				|| wwin->transient_for==wwin->client_win
				|| !windowInList(wwin->transient_for,
						 children, nchildren)) {
				wIconifyWindow(wwin);
				wwin->flags.ignore_next_unmap=1;
			    }
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
    XUngrabServer(dpy);
    XFree(children);
    scr->flags.startup = 0;
    scr->flags.startup2 = 1;
    while (XPending(dpy)) {
	XEvent ev;
	WMNextEvent(dpy, &ev);
	WMHandleEvent(&ev);
    }
    wWorkspaceForceChange(scr, 0);
    if (!wPreferences.flags.noclip)
        wDockShowIcons(scr->workspaces[scr->current_workspace]->clip);
    scr->flags.startup2 = 0;
}



