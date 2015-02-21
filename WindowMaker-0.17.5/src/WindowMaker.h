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

#ifndef WINDOWMAKER_H_
#define WINDOWMAKER_H_

#include "wconfig.h"

#include <assert.h>

#include "WINGs.h"
#include "WUtil.h"

#if HAVE_LIBINTL_H && I18N
# include <libintl.h>
# define _(text) gettext(text)
#else
# define _(text) (text)
#endif


/* max. number of distinct window levels */
#define MAX_WINDOW_LEVELS	5


/* class codes */
typedef enum {
    WCLASS_UNKNOWN = 0,
	WCLASS_WINDOW = 1,	       /* managed client windows */
	WCLASS_MENU = 2,	       /* root menus */
	WCLASS_APPICON = 3,
	WCLASS_DUMMYWINDOW = 4,	       /* window that holds window group leader */
	WCLASS_MINIWINDOW = 5,
	WCLASS_DOCK_ICON = 6,
	WCLASS_PAGER = 7,
	WCLASS_TEXT_INPUT = 8,
	WCLASS_FRAME = 9
} WClassType;



/*
 * WObjDescriptor will be used by the event dispatcher to
 * send events to a particular object through the methods in the 
 * method table. If all objects of the same class share the
 * same methods, the class method table should be used, otherwise
 * a new method table must be created for each object.
 * It is also assigned to find the parent structure of a given
 * window (like the WWindow or WMenu for a button)
 */

typedef struct WObjDescriptor {
    void *self;			       /* the object that will be called */    
				       /* event handlers */
    void (*handle_expose)(struct WObjDescriptor *sender, XEvent *event);
    
    void (*handle_mousedown)(struct WObjDescriptor *sender, XEvent *event);

    void (*handle_anything)(struct WObjDescriptor *sender, XEvent *event);

    void (*handle_enternotify)(struct WObjDescriptor *sender, XEvent *event);
    void (*handle_leavenotify)(struct WObjDescriptor *sender, XEvent *event);

    WClassType parent_type;	       /* type code of the parent */
    void *parent;		       /* parent object (WWindow or WMenu) */
    
    Time click_timestamp;	       /* timestamp of the last click */
} WObjDescriptor;


/* internal buttons */
#define WBUT_CLOSE              0
#define WBUT_BROKENCLOSE        1
#define WBUT_ICONIFY            2

#define PRED_BPIXMAPS		3 /* count of WBUT icons */

/* cursors */
#define WCUR_DEFAULT	0
#define WCUR_NORMAL 	0
#define WCUR_MOVE	1
#define WCUR_RESIZE	2
#define WCUR_WAIT	3
#define WCUR_ARROW	4
#define WCUR_QUESTION	5
#define WCUR_TEXT	6
#define WCUR_LAST	7


/* geometry displays */
#define WDIS_NEW	0	       /* new style */
#define WDIS_CENTER	1	       /* center of screen */
#define WDIS_TOPLEFT	2	       /* top left corner of screen */
#define WDIS_FRAME_CENTER 3	       /* center of the frame */


/* keyboard input focus mode */
#define WKF_CLICK	0
#define WKF_POINTER	1
#define WKF_SLOPPY	2

/* window placement mode */
#define WPM_MANUAL	0
#define WPM_CASCADE	1
#define WPM_SMART	2
#define WPM_RANDOM	3

/* text justification */
#define WTJ_CENTER	0
#define WTJ_LEFT	1
#define WTJ_RIGHT	2


/* icon box positions */
#define WIB_BOTTOM	0
#define WIB_TOP		1
#define WIB_LEFT	2
#define WIB_RIGHT	3


/* switchmenu actions */
#define ACTION_ADD	0
#define ACTION_REMOVE	1
#define ACTION_CHANGE	2
#define ACTION_CHANGE_WORKSPACE 3
#define ACTION_CHANGE_FOCUS	4


/* speeds */
#define SPEED_ULTRAFAST 0
#define SPEED_FAST	1
#define SPEED_MEDIUM	2
#define SPEED_SLOW	3
#define SPEED_ULTRASLOW 4

/* startup warnings */
#define WAR_DEFAULTS_DIR	1 /* created defaults DB directory */


/* window states */
#define WS_FOCUSED	0
#define WS_UNFOCUSED	1
#define WS_PFOCUSED	2

/* clip colors */
#define CLIP_ACTIVE    0
#define CLIP_INACTIVE  1

/* clip title colors */
#define CLIP_NORMAL    0
#define CLIP_COLLAPSED 1


typedef struct W2Color {
    char *color1;
    char *color2;
} W2Color;

typedef struct WCoord {
    int x, y;
} WCoord;



typedef struct WPreferences {
    char **pixmap_path;		       /* NULL terminated array of */
				       /* paths to find pixmaps */
    char **icon_path;		       /* NULL terminated array of */
				       /* paths to find icons */

    char size_display;		       /* display type for resize geometry */
    char move_display;		       /* display type for move geometry */
    char window_placement;	       /* window placement mode */
    char colormap_mode;		       /* colormap focus mode */
    char focus_mode;		       /* window focusing mode */
    
    char opaque_move;		       /* update window position during */
				       /* move */

    char wrap_menus;		       /* wrap menus at edge of screen */
    char scrollable_menus;	       /* let them be scrolled */
    char align_menus;		       /* align menu with their parents */
    
    char use_saveunders;	       /* turn on SaveUnders for menus,
					* icons etc. */ 
    
    char no_window_under_dock;

    char no_window_over_icons;

    WCoord window_place_origin;	       /* Offset for windows placed on
                                        * screen */

    char constrain_window_size;	       /* don't let windows get bigger than 
					* screen */

    char circ_raise;		       /* raise window when Alt-tabbing */

    char ignore_focus_click;

    char on_top_transients;	       /* transient windows are kept on top
					* of their owners */
    char title_justification;	       /* titlebar text alignment */

    char no_dithering;		       /* use dithering or not */
    
    char no_sound;		       /* enable/disable sound */
    char no_animations;		       /* enable/disable animations */
    
    char no_autowrap;		       /* wrap workspace when window is moved
					* to the edge */
    
    char auto_arrange_icons;	       /* automagically arrange icons */
    
    char icon_box_position;	       /* position to place icons */
    
    char swap_menu_buttons;	       /* make the menu be opened by the left
					* button */
    
    char disable_root_mouse;	       /* disable mouse actions in root window */
    
    char auto_focus;		       /* focus window when it's mapped */
    
    char *icon_back_file;	       /* background image for icons */

    WCoord *root_menu_pos;	       /* initial position of the root menu*/
    WCoord *app_menu_pos;

    WCoord *win_menu_pos;

    int raise_delay;		       /* delay for autoraise. 0 is disabled */

    int cmap_size;		       /* size of dithering colormap in colors
					* per channel */

    int icon_size;		       /* size of the icon */

    int ws_change_delay;	       /* Delay between changing workspaces
                                        * with clip */

    char ws_advance;                   /* Create new workspace and advance */

    char ws_cycle;                     /* Cycle existing workspaces */

    unsigned int modifier_mask;	       /* mask to use as kbd modifier */

    char save_session_on_exit;	       /* automatically save session on exit */

    char sticky_icons;		       /* If miniwindows will be onmipresent */

    char dont_confirm_kill;	       /* do not confirm Kill application */
	
    /* Appearance options */
    char new_style;		       /* Use newstyle buttons */
    char superfluous;		       /* Use superfluous things */

    /* some constants */
    int dblclick_time;		       /* double click delay time in ms */

    /* some animation constants */
    /* animate menus */
    int menu_scroll_speed;	       /* how fast menus are scrolled */

    /* animate icon sliding */
    int icon_slide_speed;	       /* icon slide animation speed */

    /* shading animation */
    int shade_speed;

#ifdef NUMLOCK_HACK    
    unsigned int ignore_mod_mask;      /* mask modifiers to be ignored. eg: NumLock */
# define IGNORE_MOD_MASK wPreferences.ignore_mod_mask
#else
# define IGNORE_MOD_MASK LockMask
#endif
    struct {
        unsigned int nodock:1;	       /* don't display the dock */
        unsigned int noclip:1;        /* don't display the clip */
	unsigned int nocpp:1;	       /* don't use cpp */
    } flags;			       /* internal flags */
} WPreferences;


/****** Global Variables  ******/
extern Display	*dpy;
extern char *ProgName;

/****** Global Functions ******/
extern void wAbort(Bool dumpCore);

#endif
