/* appicon.h- application icon
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


#ifndef WMAPPICON_H_
#define WMAPPICON_H_

#include <wraster.h>

#include "wcore.h"
#include "window.h"
#include "icon.h"
#include "application.h"

#ifdef REDUCE_APPICONS
typedef struct WAppIconAppList {
    struct WAppIconAppList *prev;
    struct WAppIconAppList *next;
    WApplication *wapp;
} WAppIconAppList;
#endif

typedef struct WAppIcon {
    short xindex;
    short yindex;
    struct WAppIcon *next;
    struct WAppIcon *prev;
    WIcon *icon;
    
    char *client_machine;
    
    int x_pos, y_pos;		       /* absolute screen coordinate */
    
    char *command;		       /* command used to launch app */
    
#ifdef OFFIX_DND
    char *dnd_command;		       /* command to use when something is */
				       /* dropped on us */
#endif
    
    char *wm_class;
    char *wm_instance;
    pid_t pid;			       /* for apps launched from the dock */
    Window main_window;
#ifdef REDUCE_APPICONS
    /* There are a number of assumptions about structures in the code,
     * but nowhere do I see them explicitly stated. I'll rip this out later.
     * If applist is not NULL, applist->wapp will always point to a valid
     * structure. Knowing this removes the need for useless checks....
     * AS LONG AS NO ONE VIOLATES THIS ASSUMPTION. -cls
     */
    WAppIconAppList *applist;	       /* list of apps bound to appicon */
#endif
    struct WDock *dock;		       /* In which dock is docked. */
    
    struct _AppSettingsPanel *panel;    /* Settings Panel */
    
    unsigned int gnustep_app:1;	       /* if this is a GNUstep application */
    unsigned int docked:1;
    unsigned int attracted:1;	       /* If it was attracted by the clip */
    unsigned int launching:1;
    unsigned int running:1;	       /* application is already running */
    unsigned int relaunching:1;	       /* launching 2nd instance */

    unsigned int forced_dock:1;
    unsigned int auto_launch:1;	       /* launch app on startup */
    unsigned int remote_start:1;
    unsigned int updated:1;
    unsigned int editing:1;	       /* editing docked icon */
    unsigned int drop_launch:1;	       /* launching from drop action */
    unsigned int destroyed:1;	       /* appicon was destroyed */
    unsigned int buggy_app:1;	       /* do not make dock rely on hints 
					* set by app */
#ifdef REDUCE_APPICONS
    unsigned int num_apps;	       /* length of applist */
#endif
} WAppIcon;


WAppIcon *wAppIconCreate(WWindow *leader_win);
WAppIcon *wAppIconCreateForDock(WScreen *scr, char *command, char *wm_instance,
				char *wm_class, int tile);

void wAppIconDestroy(WAppIcon *aicon);

void wAppIconPaint(WAppIcon *aicon);

Bool wAppIconChangeImage(WAppIcon *icon, char *file);

void wAppIconMove(WAppIcon *aicon, int x, int y);

#ifdef REDUCE_APPICONS
unsigned int wAppIconReduceAppCount(WApplication *wapp);
#endif
#endif
