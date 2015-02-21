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
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


/* Xlocale.h and locale.h are the same if X_LOCALE is undefind in wconfig.h,
 * and if X_LOCALE is defined, X's locale emulating functions will be used.
 * See Xlocale.h for more information.
 */
#include <X11/Xlocale.h>

#include "WindowMaker.h"
#include "window.h"
#include "funcs.h"
#include "menu.h"
#include "keybind.h"
#include "xmodifier.h"

#include <proplist.h>


/****** Global Variables ******/

/* general info */

Display	*dpy;
/* the root window */
char 	*ProgName;

/* locale to use. NULL==POSIX or C */
char *Locale=NULL;

WScreen *wScreen=NULL;


WPreferences wPreferences;


proplist_t wDomainName;
proplist_t wAttributeDomainName;

WShortKey wKeyBindings[WKBD_LAST];


/* XContexts */
XContext wWinContext;
XContext wAppWinContext;
XContext wStackContext;

/* Atoms */
Atom _XA_WM_STATE;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_SAVE_YOURSELF;
Atom _XA_WM_CLIENT_LEADER;
Atom _XA_WM_COLORMAP_WINDOWS;

Atom _XA_GNUSTEP_WM_ATTR;
Atom _XA_WINDOWMAKER_WM_MINIATURIZE_WINDOW;
Atom _XA_GNUSTEP_WM_RESIZEBAR;

#ifdef MWM_HINTS
/* MWM support */
Atom _XA_MOTIF_WM_HINTS;
#endif

Atom _XA_WINDOWMAKER_MENU;
Atom _XA_WINDOWMAKER_WM_PROTOCOLS;
Atom _XA_WINDOWMAKER_STATE;

Atom _XA_WINDOWMAKER_WM_FUNCTION;

#ifdef OFFIX_DND
Atom _XA_DND_PROTOCOL;
Atom _XA_DND_SELECTION;
#endif


/* cursors */
Cursor wCursor[WCUR_LAST];

/* last event timestamp for XSetInputFocus */
Time LastTimestamp;
/* timestamp on the last time we did XSetInputFocus() */
Time LastFocusChange;

#ifdef SHAPE
int ShapeEventBase;
#endif

/* temporary stuff */
int wVisualID = -1;


/******** End Global Variables *****/


static char **Arguments;
static int ArgCount;

extern void EventLoop();
extern void StartUp();



void
Restart(char *manager)
{
    char *prog=NULL;
    char *argv[MAX_RESTART_ARGS];
    int i;

    if (manager && manager[0]!=0) {
	prog = argv[0] = strtok(manager, " ");
	for (i=1; i<MAX_RESTART_ARGS; i++) {
	    argv[i]=strtok(NULL, " ");
	    if (argv[i]==NULL) {
		break;
	    }
	}
    }

    XCloseDisplay(dpy);
    if (!prog)
      execvp(Arguments[0], Arguments);
    else {
	execvp(prog, argv);
	/* fallback */
	execv(Arguments[0], Arguments);
    }
    wfatal(_("Restart failed!!!"));
    exit(-1);
}

/*
 *---------------------------------------------------------------------
 * wAbort--
 * 	Do a major cleanup and exit the program
 * 
 *---------------------------------------------------------------------- 
 */
void
wAbort(Bool dumpCore)
{
    RestoreDesktop(wScreen);
    printf(_("%s aborted.\n"), ProgName);
    if (dumpCore)
	abort();
    else
	exit(1);
}


void
print_help()
{
    printf(_("usage: %s [-options]\n"), ProgName);
    puts(_("options:"));
#ifdef USECPP
    puts(_(" -nocpp 		disable preprocessing of configuration files"));
#endif
    puts(_(" -nodock		do not open the application Dock"));
    puts(_(" -noclip		do not open the workspace Clip"));
    /*
    puts(_(" -locale locale		locale to use"));
    */
    puts(_(" -visualid visualid	visual id of visual to use"));
    puts(_(" -display host:dpy	display to use"));
    puts(_(" -version		print version and exit"));
}



void
check_defaults()
{
    char *path, *home;
    int must_free = 0;

    path = getenv("GNUSTEP_USER_ROOT");
    if (!path) {
	home = wgethomedir();
	if (!home) {
	    wfatal(_("could not determine home directory"));
	    exit(1);
	}
	must_free = 1;
	path = wmalloc(strlen(home)+64);
	strcpy(path, home);
	strcat(path, "/GNUstep/");
	strcat(path, DEFAULTS_DIR);
    }
    if (access(path, R_OK)!=0) {
	wwarning(_("could not find user GNUstep directory.\n"
		  "Make sure you have installed WindowMaker correctly and run wmaker.inst"));
	exit(1);
    }
    
    if (must_free)
	free(path);
}



static void
execInitScript()
{
    char *file;
    
    file = wfindfile(DEF_CONFIG_PATHS, DEF_INIT_SCRIPT);
    if (file) {
	if (fork()==0) {
	    execl("/bin/sh", "/bin/sh", "-c", file, NULL);
	    wsyserror(_("%s:could not execute initialization script"), file);
	    exit(1);
	}
	free(file);
    }
}


int
main(int argc, char **argv)
{
    int i, restart=0;
    char *display_name="";
    char *tmp;

    ArgCount = argc;
    Arguments = argv;

    
    ProgName = strrchr(argv[0],'/');
    if (!ProgName)
      ProgName = argv[0];
    else
      ProgName++;

    
    /* check existence of Defaults DB directory */
    check_defaults();
    
    restart = 0;
    
    memset(&wPreferences, 0, sizeof(WPreferences));
    
    if (argc>1) {
	for (i=1; i<argc; i++) {
#ifdef USECPP
	    if (strcmp(argv[i], "-nocpp")==0) {
		wPreferences.flags.nocpp=1;
	    } else
#endif
	    if (strcmp(argv[i], "-nodock")==0) {
		wPreferences.flags.nodock=1;
	    } else
	    if (strcmp(argv[i], "-noclip")==0) {
		wPreferences.flags.noclip=1;
	    } else
	    if (strcmp(argv[i], "-version")==0) {
		printf("WindowMaker %s\n", VERSION);
		exit(0);
	    } else if (strcmp(argv[i], "-locale")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		Locale = argv[i];
	    } else if (strcmp(argv[i], "-display")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		display_name = argv[i];
	    } else if (strcmp(argv[i], "-visualid")==0) {
		i++;
		if (i>=argc) {
		    wwarning(_("too few arguments for %s"), argv[i-1]);
		    exit(0);
		}
		if (sscanf(argv[i], "%i", &wVisualID)!=1) {
		    wwarning(_("bad value for visualid: \"%s\""), argv[i]);
		    exit(0);
		}
	    } else {
		print_help();
		exit(0);
	    }
	}
    }
#if 0
    tmp = getenv("LANG");
    if (tmp) {
    	if (setlocale(LC_ALL,"") == NULL) {
	    wwarning("cannot set locale %s", tmp);
	    wwarning("falling back to C locale");
	    setlocale(LC_ALL,"C");
	    Locale = NULL;
	} else {
	    if (strcmp(tmp, "C")==0 || strcmp(tmp, "POSIX")==0)
	      Locale = NULL;
	    else
	      Locale = tmp;
	}
    } else {
	Locale = NULL;
    }
#endif
    if (!Locale) {
    	tmp = Locale = getenv("LANG");
	setlocale(LC_ALL, "");
    } else {
	setlocale(LC_ALL, Locale);
    }
    if (!Locale || strcmp(Locale, "C")==0 || strcmp(Locale, "POSIX")==0) 
      Locale = NULL;
#ifdef I18N
    if (getenv("NLSPATH"))
      bindtextdomain("WindowMaker", getenv("NLSPATH"));
    else
      bindtextdomain("WindowMaker", NLSDIR);
    textdomain("WindowMaker");
#endif

#ifdef I18N_MB
    if (!XSupportsLocale()) {
	wwarning(_("X server does not support locale"));
    }
    if (XSetLocaleModifiers("") == NULL) {
 	wwarning(_("cannot set locale modifiers"));
    }
#endif
        
    if (display_name && display_name[0]!=0) {
	tmp = wmalloc(strlen(display_name)+64);
	
	sprintf(tmp, "DISPLAY=%s", display_name);
	putenv(tmp);
	free(tmp);
    }
    /* open display */
    dpy = XOpenDisplay(display_name);
    if (dpy == NULL) {
	wfatal(_("could not open display \"%s\""), XDisplayName(display_name));
	exit(1);
    }

#ifdef DEBUG
    XSynchronize(dpy, True);
#endif

    wXModifierInitialize();

#ifdef SOUNDS
    wSoundInitialize();
#endif
    StartUp();

    execInitScript();

    EventLoop();
    return -1;
}
