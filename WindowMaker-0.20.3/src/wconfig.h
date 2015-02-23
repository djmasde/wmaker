/*
 * wconfig.h- default configuration and definitions + compile time options
 * 
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef WMCONFIG_H_
#define WMCONFIG_H_

#include "config.h"

/*** Change it (wconfig.h) *after* you ran configure ***/

/*
 *--------------------------------------------------------------------
 * 			Feature Selection
 * 
 * 	Comment out the following #defines if you want to 
 * disable a feature. 
 * 	Also check the features you can enable through configure.
 *--------------------------------------------------------------------
 */

/* undefine ANIMATIONS if you don't want animations for iconification,
 * shading, icon arrangement etc. */
#define ANIMATIONS


/*
 * undefine USECPP if you don't want your config files to be preprocessed
 * by cpp
 */
#define USECPP

/* #define CPP_PATH /usr/bin/cpp */

/*
 * Internationalization (I18N) support
 * Multi-byte (japanese, korean, chinese etc.) character support
 * set by configure
 */
#undef I18N_MB


/*
 * sound support
 */
#define WMSOUND


/*
 * support for OffiX DND drag and drop in the Dock
 */
#define OFFIX_DND

/*
 * support for XDE drang and drop in the Dock. still in beta
 */
#undef XDE_DND


/*
 * Undefine BALLOON_TEXT if you don't want balloons for showing extra
 * information, like window titles that are not fully visible.
 */
#define BALLOON_TEXT

/*
 * If balloons should be shaped or be simple rectangles.
 * The X server must support the shape extensions and it's support
 * must be enabled (default).
 */
#define SHAPED_BALLOON


/*
 * Define NO_EMERGENCY_AUTORESTART if you don't want another window manager
 * automatically started when WindowMaker crashes. The X session will die
 * in some cases if wmaker crashes and autorestart is disabled.
 */
#undef NO_EMERGENCY_AUTORESTART

/* 
 * The window manager that is autorestarted
 */
#define FALLBACK_WINDOWMANAGER "blackbox"


/* Define if you want MWM hint support (and consequently GNOME). */
#define MWM_HINTS


/* Define if you have a 5 button mouse and want to use button 4
 * (in the root window) for switching to the previous workspace
 * and 5 for the next */
#undef MOUSE_WS_SWITCH
 
/*
 * Turn on a hack to make mouse and keyboard actions work even if
 * the NumLock or ScrollLock modifiers are turned on. They might
 * inflict a performance/memory penalty.
 *
 * If you're an X expert (knows the implementation of XGrabKey() in X)
 * and knows that the penalty is small (or not), please tell me.
 */
#define NUMLOCK_HACK

/*
 * define REDUCE_APPICONS if you want apps with the same WM_INSTANCE &&
 * WM_CLASS to share an appicon
 */
#undef REDUCE_APPICONS


/*
 * define OPTIMIZE_SHAPE if you want the shape setting code to be optimized
 * for applications that change their shape frequently (like xdaliclock
 * -shape), removing flickering. If wmaker and your display are on
 * different machines and the network connection is slow, it is not
 * recommended. 
 */
#undef OPTIMIZE_SHAPE

/* define CONFIGURE_WINDOW_WHILE_MOVING if you want WindowMaker to send
 * the synthetic ConfigureNotify event to windows while moving at every
 * single movement. Default is to send a synthetic ConfigureNotify event
 * only at the end of window moving, which improves performance.
 */
#undef CONFIGURE_WINDOW_WHILE_MOVING


/*
 *..........................................................................
 * The following options WILL NOT BE MADE RUN-TIME. Please do not request.
 * They will only add unneeded bloat.
 *..........................................................................
 */

/*
 * define SHADOW_RESIZEBAR if you want a resizebar with shadows like in
 * NextStep 3.x ??, instead of the default Openstep look. NEXTSTEP 3.3
 * also does not have these shadows.
 */
#undef SHADOW_RESIZEBAR


/*
 * Define DEMATERIALIZE_ICON if you want the undocked icon animation
 * to be a progressive disaparison animation. 
 */
#undef DEMATERIALIZE_ICON

/*
 * Define ICON_KABOOM_EXTRA if you want extra fancy icon undocking
 * explosion animation.
 */
#undef ICON_KABOOM_EXTRA

/*
 * #undef if you dont want the window creation animation when superfluous
 * is enabled.
 */
#undef WINDOW_BIRTH_ZOOM


/*
 *--------------------------------------------------------------------
 * 			Default Configuration
 * 
 * 	Some of the following options can be configured in
 * the preference files, but if for some reason, they can't
 * be used, these defaults will be.
 * 	There are also some options that can only be configured here,
 * at compile time.
 *--------------------------------------------------------------------
 */

/* list of paths to look for the config files, searched in order 
 * of appearance */
#define DEF_CONFIG_PATHS \
"~/GNUstep/Library/WindowMaker:"PKGDATADIR

#define DEF_MENU_FILE	"menu"

/* name of the script to execute at startup */
#define DEF_INIT_SCRIPT "autostart"

#define DEF_EXIT_SCRIPT "exitscript"

#define DEFAULTS_DIR "Defaults"


/* pixmap path */
#define DEF_PIXMAP_PATHS \
"(\"~/pixmaps\",\"~/GNUstep/Library/WindowMaker/Pixmaps\",\""PIXMAPDIR"\")"

/* icon path */
#define DEF_ICON_PATHS \
"(\"~/pixmaps\",\"~/GNUstep/Library/Icons\",\"/usr/include/X11/pixmaps/\",\""PIXMAPDIR"\")"


/* window title to use for untitled windows */
#define DEF_WINDOW_TITLE "Untitled"

/* default style */
#define DEF_FRAME_COLOR   "white"


#ifdef I18N_MB
#define DEF_TITLE_FONT "\"-*-*-medium-r-normal--14-*\""
#define DEF_MENU_TITLE_FONT "\"-*-*-medium-r-normal--14-*\""
#define DEF_MENU_ENTRY_FONT "\"-*-*-medium-r-normal--14-*\""
#define DEF_ICON_TITLE_FONT "\"-*-*-medium-r-normal--10-*\""
#define DEF_CLIP_TITLE_FONT "\"-*-*-medium-r-normal--10-*\""
#define DEF_INFO_TEXT_FONT "\"-*-*-medium-r-normal--14-*\""
#else /* I18N_MB */
#define DEF_TITLE_FONT "\"-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*\""
#define DEF_MENU_TITLE_FONT "\"-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*\""
#define DEF_MENU_ENTRY_FONT "\"-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*\""
#define DEF_ICON_TITLE_FONT "\"-*-helvetica-medium-r-normal-*-8-*-*-*-*-*-*-*\""
#define DEF_CLIP_TITLE_FONT "\"-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*\""
#define DEF_INFO_TEXT_FONT "\"-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*\""
#endif /* !I18N_MB */

#define HELVETICA10_FONT "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*"

#define DEF_FRAME_THICKNESS 1	       /* linewidth of the move/resize frame */

#define DEF_XPM_CLOSENESS	40000 

/* default position of application menus */
#define DEF_APPMENU_X		10
#define DEF_APPMENU_Y		10


/* Window level where icons reside */
#define NORMAL_ICON_LEVEL WMNormalLevel

/* do not divide main menu and submenu in different tiers, 
 * opposed to OpenStep */
#define SINGLE_MENULEVEL

/* max. time to spend doing animations in seconds. If the animation
 * time exceeds this value, it is immediately finished. Usefull for
 * moments of high-load.
 */
#define MAX_ANIMATION_TIME	1

/* Zoom animation */
#define MINIATURIZE_ANIMATION_FRAMES_Z   5
#define MINIATURIZE_ANIMATION_STEPS_Z    12
#define MINIATURIZE_ANIMATION_DELAY_Z    500
/* Twist animation */
#define MINIATURIZE_ANIMATION_FRAMES_T   12
#define MINIATURIZE_ANIMATION_STEPS_T    16
#define MINIATURIZE_ANIMATION_DELAY_T    1000
#define MINIATURIZE_ANIMATION_TWIST_T    0.5
/* Flip animation */
#define MINIATURIZE_ANIMATION_FRAMES_F   12
#define MINIATURIZE_ANIMATION_STEPS_F    16
#define MINIATURIZE_ANIMATION_DELAY_F    1000
#define MINIATURIZE_ANIMATION_TWIST_F    0.5
  

#define HIDE_ANIMATION_STEPS (MINIATURIZE_ANIMATION_STEPS*2/3)

/* delay before balloon is showed */
#define BALLOON_DELAY	1000

/* delay for menu item selection hysteresis */
#define MENU_SELECT_DELAY 300

/* animation speed constants */

/* icon slide */
#define ICON_SLIDE_SLOWDOWN_UF	20
#define ICON_SLIDE_DELAY_UF	0
#define ICON_SLIDE_STEPS_UF	15

#define ICON_SLIDE_SLOWDOWN_F	30
#define ICON_SLIDE_DELAY_F	0
#define ICON_SLIDE_STEPS_F	10

#define ICON_SLIDE_SLOWDOWN_M	40
#define ICON_SLIDE_DELAY_M	0
#define ICON_SLIDE_STEPS_M	5

#define ICON_SLIDE_SLOWDOWN_S	50
#define ICON_SLIDE_DELAY_S	0
#define ICON_SLIDE_STEPS_S	3

#define ICON_SLIDE_SLOWDOWN_U	50
#define ICON_SLIDE_DELAY_U	3
#define ICON_SLIDE_STEPS_U	3

/* menu scrolling */
#define MENU_SCROLL_STEPS_UF	14
#define MENU_SCROLL_DELAY_UF	0

#define MENU_SCROLL_STEPS_F	10
#define MENU_SCROLL_DELAY_F	5

#define MENU_SCROLL_STEPS_M	6
#define MENU_SCROLL_DELAY_M	5

#define MENU_SCROLL_STEPS_S	4
#define MENU_SCROLL_DELAY_S	6

#define MENU_SCROLL_STEPS_U	1
#define MENU_SCROLL_DELAY_U	8

/* shade animation */
#define SHADE_STEPS_UF		5
#define SHADE_DELAY_UF		0

#define SHADE_STEPS_F		10
#define SHADE_DELAY_F		0

#define SHADE_STEPS_M		15
#define SHADE_DELAY_M		0

#define SHADE_STEPS_S		30
#define SHADE_DELAY_S		0

#define SHADE_STEPS_U		20
#define SHADE_DELAY_U		10

/* window birth animation steps (DO NOT MAKE IT RUN-TIME) */
#define WINDOW_BIRTH_STEPS	20

/* number of steps for icon dematerialization. */
#define DEMATERIALIZE_STEPS	16

/* Delay when cycling colors of selected icons. */
#define COLOR_CYCLE_DELAY	200

/* size of the pieces in the undocked icon explosion */
#define ICON_KABOOM_PIECE_SIZE 4


/* Position increment for smart placement. >= 1  raise these values if it's
 * too slow for you */
#define PLACETEST_HSTEP	8
#define PLACETEST_VSTEP	8


#define DOCK_EXTRA_SPACE	1

/* Vicinity in which an icon can be attached to the clip */
#define CLIP_ATTACH_VICINITY	1

/* The amount of space (in multiples of the icon size)
 * a docked icon must be dragged out to detach it */
#define DOCK_DETTACH_THRESHOLD	3

/* Delay (in ms) after which the clip will autocollapse when leaved */
#define AUTO_COLLAPSE_DELAY     1000


/* Max. number of icons the clip can have */
#define CLIP_MAX_ICONS		32

/* blink interval when invoking a menu item */
#define MENU_BLINK_DELAY	60000
#define MENU_BLINK_COUNT	2

#define CURSOR_BLINK_RATE	300

#define MOVE_THRESHOLD	3 /* how many pixels to move before dragging windows */

#define HRESIZE_THRESHOLD	3

#define MAX_WORKSPACENAME_WIDTH	16
#define MAX_WINDOWLIST_WIDTH	160     /* max width of window title in
					 * window list */

#define DEFAULTS_CHECK_INTERVAL 2000	/* how often wmaker will check for
					 * changes in the config files */

/* if your keyboard don't have arrow keys */
#undef ARROWLESS_KBD


/* don't put titles in miniwindows */
#undef NO_MINIWINDOW_TITLES

/*
 * disable/enable workspace indicator in the dock
 */

#undef WS_INDICATOR 


/*
 *----------------------------------------------------------------------
 * You should not modify the following values, unless you know
 * what you're doing.
 *----------------------------------------------------------------------
 */



#define WM_PI 3.14159265358979323846

#define FRAME_BORDER_COLOR "black"

#define FRAME_BORDER_WIDTH 1	       /* width of window border for frames */

#define RESIZEBAR_HEIGHT 8	       /* height of the resizebar */
#define RESIZEBAR_MIN_WIDTH 20	       /* min. width of handles-corner_width */
#define RESIZEBAR_CORNER_WIDTH 28      /* width of the corner of resizebars */

#define TITLEBAR_EXTRA_HEIGHT 8

#define MENU_INDICATOR_SPACE	12

/* minimum size for windows */
#define MIN_WINDOW_SIZE	5

#define MIN_TITLEFONT_HEIGHT(h)   ((h)>14 ? (h) : 14)

#define ICON_WIDTH	64	       /* size of the icon window */
#define ICON_HEIGHT	64
#define ICON_BORDER_WIDTH 2

#define MAX_ICON_WIDTH	60	       /* size of the icon pixmap */
#define MAX_ICON_HEIGHT 48

#define MAX_WORKSPACES  100

#define MAX_MENU_TEXT_LENGTH 512

#define MAX_RESTART_ARGS	16

#define MAX_COMMAND_SIZE	1024

#define MAX_DEAD_PROCESSES	128


#define MAXLINE		1024


#ifdef _MAX_PATH
# define DEFAULT_PATH_MAX	_MAX_PATH
#else
# define DEFAULT_PATH_MAX	512
#endif


#define DEBUG0

/* some rules */

#ifndef SHAPE
#undef SHAPED_BALLOON
#endif


#ifdef XKB_MODELOCK
#define KEEP_XKB_LOCK_STATUS
#endif

#if HAVE_LIBINTL_H && I18N
# include <libintl.h>
# define _(text) gettext(text)
#else
# define _(text) (text)
#endif


#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define INLINE inline
#else
# define INLINE
#endif

#endif /* WMCONFIG_H_ */

