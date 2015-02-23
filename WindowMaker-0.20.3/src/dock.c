/* dock.c- built-in Dock module for WindowMaker
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include "WindowMaker.h"
#include "wcore.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "actions.h"
#include "stacking.h"
#include "dock.h"
#include "dialog.h"
#include "funcs.h"
#include "properties.h"
#include "menu.h"
#include "client.h"
#include "defaults.h"
#include "workspace.h"
#include "framewin.h"
#include "superfluous.h"

#include "list.h"

#ifdef WMSOUND
#include "wmsound.h"
#endif

#include <proplist.h>




/**** Local variables ****/
#define CLIP_REWIND       1
#define CLIP_IDLE         0
#define CLIP_FORWARD      2

#define CLIP_BUTTON_SIZE  23


/**** Global variables ****/

/* in dockedapp.c */
extern void DestroyDockAppSettingsPanel();

extern void ShowDockAppSettingsPanel(WAppIcon *aicon);



extern Cursor wCursor[WCUR_LAST];

extern WPreferences wPreferences;

extern XContext wWinContext;

#ifdef OFFIX_DND
extern Atom _XA_DND_PROTOCOL;
#endif


#define MOD_MASK wPreferences.modifier_mask

extern void appIconMouseDown(WObjDescriptor *desc, XEvent *event);

#define ICON_SIZE wPreferences.icon_size


/***** Local variables ****/

static proplist_t dCommand=NULL;
#ifdef OFFIX_DND
static proplist_t dDropCommand=NULL;
#endif
static proplist_t dAutoLaunch, dName, dForced, dBuggyApplication, dYes, dNo;
static proplist_t dHost, dDock, dClip;
static proplist_t dAutoAttractIcons, dKeepAttracted;

static proplist_t dPosition, dApplications, dLowered, dCollapsed, dAutoCollapse;

static void dockIconPaint(WAppIcon *btn);

static void iconMouseDown(WObjDescriptor *desc, XEvent *event);

static pid_t execCommand(WAppIcon *btn, char *command, WSavedState *state);

static void trackDeadProcess(pid_t pid, unsigned char status, WDock *dock);

static int getClipButton(int px, int py);

static void toggleLowered(WDock *dock);

static void toggleCollapsed(WDock *dock);

static void toggleAutoCollapse(WDock *dock);

static void clipIconExpose(WObjDescriptor *desc, XEvent *event);

static void clipLeave(WDock *dock);

static void handleClipChangeWorkspace(WScreen *scr, XEvent *event);

Bool moveIconBetweenDocks(WDock *src, WDock *dest, WAppIcon *icon, int x, int y);

static void clipEnterNotify(WObjDescriptor *desc, XEvent *event);
static void clipLeaveNotify(WObjDescriptor *desc, XEvent *event);
static void clipAutoCollapse(void *cdata);
static void launchDockedApplication(WAppIcon *btn);

static void showClipBalloon(WDock *dock, int workspace);

#ifdef OFFIX_DND

#define DndNotDnd       -1
#define DndUnknown      0
#define DndRawData      1
#define DndFile         2
#define DndFiles        3
#define DndText         4
#define DndDir          5
#define DndLink         6
#define DndExe          7

#define DndEND          8

#endif /* OFFIX_DND */



static void
make_keys()
{
    if (dCommand!=NULL)
	return;
    
    dCommand = PLRetain(PLMakeString("Command"));
#ifdef OFFIX_DND
    dDropCommand = PLRetain(PLMakeString("DropCommand"));
#endif
    dAutoLaunch = PLRetain(PLMakeString("AutoLaunch"));
    dName = PLRetain(PLMakeString("Name"));
    dForced = PLRetain(PLMakeString("Forced"));
    dBuggyApplication = PLRetain(PLMakeString("BuggyApplication"));
    dYes = PLRetain(PLMakeString("Yes"));
    dNo = PLRetain(PLMakeString("No"));
    dHost = PLRetain(PLMakeString("Host"));
    
    dPosition = PLMakeString("Position");
    dApplications = PLMakeString("Applications");
    dLowered = PLMakeString("Lowered");
    dCollapsed = PLMakeString("Collapsed");
    dAutoCollapse = PLMakeString("AutoCollapse");
    dAutoAttractIcons = PLMakeString("AutoAttractIcons");
    dKeepAttracted = PLMakeString("KeepAttracted");
    
    dDock = PLMakeString("Dock");
    dClip = PLMakeString("Clip");
}



static void
renameCallback(WMenu *menu, WMenuEntry *entry)
{
    WDock *dock = entry->clientdata;
    char buffer[128];
    int wspace;
    char *name;

    assert(entry->clientdata!=NULL);

    wspace = dock->screen_ptr->current_workspace;
    
    name = wstrdup(dock->screen_ptr->workspaces[wspace]->name);

    sprintf(buffer, _("Type the name for workspace %i:"), wspace+1);
    if (wInputDialog(dock->screen_ptr, _("Rename Workspace"), buffer,
		     &name)) {
	wWorkspaceRename(dock->screen_ptr, wspace, name);
    }
    if (name) {
	free(name);
    }
}


static void
toggleLoweredCallback(WMenu *menu, WMenuEntry *entry)
{
    assert(entry->clientdata!=NULL);
    
    toggleLowered(entry->clientdata);
    
    entry->flags.indicator_on = !((WDock*)entry->clientdata)->lowered;
	
    wMenuPaint(menu);
} 



static void
killCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *icon;
#ifdef REDUCE_APPICONS
    WAppIconAppList *tapplist;
    
    extern Atom _XA_WM_DELETE_WINDOW;
#endif

    assert(entry->clientdata!=NULL);
    
    icon = (WAppIcon*)entry->clientdata;
    
    icon->editing = 1;
    
#ifdef REDUCE_APPICONS
    /* Send a delete message to the main window of each application
     * bound to this docked appicon. - cls
     */
    tapplist = icon->applist;
    while (tapplist != NULL) {
	if (tapplist->wapp->main_window_desc != NULL) {
	    if (tapplist->wapp->main_window_desc->protocols.DELETE_WINDOW) {
		wClientSendProtocol(tapplist->wapp->main_window_desc, 
				    _XA_WM_DELETE_WINDOW, CurrentTime);
	    } else {
		wClientKill(tapplist->wapp->main_window_desc);
	    }
	}
	tapplist = tapplist->next;
    }
#else
    if (wPreferences.dont_confirm_kill
	|| wMessageDialog(menu->frame->screen_ptr, _("Kill Application"),
			  _("This will kill the application.\nAny unsaved changes will be lost.\nPlease confirm."),
			  _("Yes"), _("No"), NULL)==WAPRDefault) {
	if (icon->icon && icon->icon->owner) {
	    wClientKill(icon->icon->owner);
	}
    }
#endif /* !REDUCE_APPICONS */
    
    icon->editing = 0;
}


static LinkedList*
getSelected(WDock *dock)
{
    LinkedList *ret=NULL;
    WAppIcon *btn;
    int i;

    for (i=1; i<dock->max_icons; i++) {
        btn = dock->icon_array[i];
        if (btn && btn->icon->selected) {
            ret = list_cons(btn, ret);
        }
    }

    return ret;
}


static void
paintClipButtons(WAppIcon *clipIcon, Bool lpushed, Bool rpushed)
{
    Window win = clipIcon->icon->core->window;
    WScreen *scr = clipIcon->icon->core->screen_ptr;
    XPoint p[4];
    int pt = CLIP_BUTTON_SIZE*ICON_SIZE/64;
    int tp = ICON_SIZE - pt;
    int as = pt - 15; /* 15 = 5+5+5 */


    if (rpushed) {
	p[0].x = tp+1;
	p[0].y = 1;
	p[1].x = ICON_SIZE-2;
	p[1].y = 1;
	p[2].x = ICON_SIZE-2;
	p[2].y = pt-1;
    } else if (lpushed) {
	p[0].x = 1;
	p[0].y = tp;
	p[1].x = pt;
	p[1].y = ICON_SIZE-2;
	p[2].x = 1;
	p[2].y = ICON_SIZE-2;
    }
    if (lpushed || rpushed) {
	XSetForeground(dpy, scr->draw_gc, scr->white_pixel);
	XFillPolygon(dpy, win, scr->draw_gc, p, 3, Convex, CoordModeOrigin);
	XSetForeground(dpy, scr->draw_gc, scr->black_pixel);
    }

    p[0].x = p[3].x = ICON_SIZE-6-as;
    p[0].y = p[3].y = 5;
    p[1].x = ICON_SIZE-6;
    p[1].y = 5;
    p[2].x = ICON_SIZE-6;
    p[2].y = 5+as;
    if (rpushed) {
        XFillPolygon(dpy, win, scr->draw_gc, p, 3, Convex, CoordModeOrigin);
        XDrawLines(dpy, win, scr->draw_gc, p, 4, CoordModeOrigin);
    } else {
	XFillPolygon(dpy, win, scr->clip_title_gc, p,3,Convex,CoordModeOrigin);
        XDrawLines(dpy, win, scr->clip_title_gc, p,4,CoordModeOrigin);
    }

    p[0].x = p[3].x = 5;
    p[0].y = p[3].y = ICON_SIZE-6-as;
    p[1].x = 5;
    p[1].y = ICON_SIZE-6;
    p[2].x = 5+as;
    p[2].y = ICON_SIZE-6;
    if (lpushed) {
	XFillPolygon(dpy, win, scr->draw_gc, p, 3, Convex, CoordModeOrigin);
        XDrawLines(dpy, win, scr->draw_gc, p, 4, CoordModeOrigin);
    } else {
        XFillPolygon(dpy, win, scr->clip_title_gc, p,3,Convex,CoordModeOrigin);
        XDrawLines(dpy, win, scr->clip_title_gc, p,4,CoordModeOrigin);
    }
}


RImage*
wClipMakeTile(WScreen *scr, RImage *normalTile)
{
    RImage *tile = RCloneImage(normalTile);
    RColor black;
    RColor dark;
    RColor light;
    int pt, tp;
    
    pt = CLIP_BUTTON_SIZE*wPreferences.icon_size/64;
    tp = wPreferences.icon_size-1 - pt;

    black.alpha = 255;
    black.red = black.green = black.blue = 0;
    
    dark.alpha = 0;
    dark.red = dark.green = dark.blue = 80;

    light.alpha = 0;
    light.red = light.green = light.blue = 80;
    
    
    /* top right */
    ROperateLine(tile, RSubtractOperation, tp, 0, wPreferences.icon_size-2,
		 pt-1, &dark);
    RDrawLine(tile, tp-1, 0, wPreferences.icon_size-1, pt+1, &black);
    ROperateLine(tile, RAddOperation, tp, 2, wPreferences.icon_size-3,
		 pt, &light);
    
    
    /* bottom left */
    ROperateLine(tile, RAddOperation, 2, tp+2, pt-2,
		 wPreferences.icon_size-3, &dark);
    RDrawLine(tile, 0, tp-1, pt+1, wPreferences.icon_size-1, &black);
    ROperateLine(tile, RSubtractOperation, 0, tp-2, pt+1,
		 wPreferences.icon_size-2, &light);

    return tile;
}


static void
removeIconsCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *clickedIcon = (WAppIcon*)entry->clientdata;
    WDock *dock;
    WAppIcon *aicon;
    LinkedList *selectedIcons;
    int keepit;

    assert(clickedIcon!=NULL);
    
    dock = clickedIcon->dock;

    selectedIcons = getSelected(dock);

    if (selectedIcons) {
	if (wMessageDialog(dock->screen_ptr, _("Workspace Clip"),
			   _("All selected icons will be removed!"),
			   _("OK"), _("Cancel"), NULL)!=WAPRDefault) {
	    return;
	}
    } else {
	if (clickedIcon->xindex==0 && clickedIcon->yindex==0)
	    return;
	selectedIcons = list_cons(clickedIcon, NULL);
    }

    while (selectedIcons) {
        aicon = selectedIcons->head;
        keepit = aicon->running && wApplicationOf(aicon->main_window);
        wDockDetach(dock, aicon);
        if (keepit) {
            PlaceIcon(dock->screen_ptr, &aicon->x_pos, &aicon->y_pos);
            XMoveWindow(dpy, aicon->icon->core->window,
                        aicon->x_pos, aicon->y_pos);
            if (!dock->mapped || dock->collapsed)
                XMapWindow(dpy, aicon->icon->core->window);
        }
        list_remove_head(&selectedIcons);
    }
    
    if (wPreferences.auto_arrange_icons)
        wArrangeIcons(dock->screen_ptr, True);
}


static void
keepIconsCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *clickedIcon = (WAppIcon*)entry->clientdata;
    WDock *dock;
    WAppIcon *aicon;
    LinkedList *selectedIcons;

    assert(clickedIcon!=NULL);
    dock = clickedIcon->dock;

    selectedIcons = getSelected(dock);

    if (!selectedIcons && clickedIcon!=dock->screen_ptr->clip_icon) {
	char *command = NULL;
			
	if (!clickedIcon->command && !clickedIcon->editing) {
	    clickedIcon->editing = 1;
	    if (wInputDialog(dock->screen_ptr, _("Keep Icon"),
			     _("Type the command used to launch the application"),
			     &command)) {
		if (command && (command[0]==0 ||
				(command[0]=='-' && command[1]==0))) {
		    free(command);
		    command = NULL;
		}
		clickedIcon->command = command;
		clickedIcon->editing = 0;
	    } else {
		clickedIcon->editing = 0;
		if (command)
		    free(command);
		return;
	    }
	}
	
	selectedIcons = list_cons(clickedIcon, NULL);
    }
    
    while (selectedIcons) {
        aicon = selectedIcons->head;
	if (aicon->icon->selected)
            wIconSelect(aicon->icon);
        if (aicon && aicon->attracted && aicon->command) {
	    aicon->attracted = 0;
	    if (aicon->icon->shadowed) {
		aicon->icon->shadowed = 0;
		aicon->icon->force_paint = 1;
		wAppIconPaint(aicon);
	    }
        }
        list_remove_head(&selectedIcons);
    }
}




static void
toggleAutoAttractCallback(WMenu *menu, WMenuEntry *entry)
{
    WDock *dock = (WDock*)entry->clientdata;

    assert(entry->clientdata!=NULL);

    dock->attract_icons = !dock->attract_icons;
    /*if (!dock->attract_icons)
        dock->keep_attracted = 0;*/

    entry->flags.indicator_on = dock->attract_icons;

    wMenuPaint(menu);
}


static void
toggleKeepCallback(WMenu *menu, WMenuEntry *entry)
{
    WDock *dock = (WDock*)entry->clientdata;
    WAppIcon *btn;
    int i;

    assert(entry->clientdata!=NULL);

    dock->keep_attracted = !dock->keep_attracted;

    if (dock->keep_attracted) {
	for (i=0; i< dock->max_icons; i++) {
	    btn = dock->icon_array[i];
	    if (btn && btn->attracted && btn->command) {
		btn->attracted = 0;
		if (btn->icon->shadowed) {
		    btn->icon->shadowed = 0;
		    btn->icon->force_paint = 1;
		    wAppIconPaint(btn);
		}
	    }
	}
    }

    entry->flags.indicator_on = dock->keep_attracted;

    wMenuPaint(menu);
}


static void
selectCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *icon = (WAppIcon*)entry->clientdata;

    assert(icon!=NULL);

    wIconSelect(icon->icon);

    wMenuPaint(menu);
}


static void
colectIconsCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *clickedIcon = (WAppIcon*)entry->clientdata;
    WDock *clip;
    WAppIcon *aicon;
    int x, y, x_pos, y_pos;

    assert(entry->clientdata!=NULL);
    clip = clickedIcon->dock;

    aicon = clip->screen_ptr->app_icon_list;

    while (aicon) {
        if (!aicon->docked && wDockFindFreeSlot(clip, &x, &y)) {
            x_pos = clip->x_pos + x*ICON_SIZE;
            y_pos = clip->y_pos + y*ICON_SIZE;
            if (aicon->x_pos != x_pos || aicon->y_pos != y_pos) {
#ifdef ANIMATIONS
                if (wPreferences.no_animations) {
                    XMoveWindow(dpy, aicon->icon->core->window, x_pos, y_pos);
                } else {
                    SlideWindow(aicon->icon->core->window,
                                aicon->x_pos, aicon->y_pos, x_pos, y_pos);
                }
#else
                XMoveWindow(dpy, aicon->icon->core->window, x_pos, y_pos);
#endif /* ANIMATIONS */
            }
            aicon->attracted = 1;
	    if (!clip->keep_attracted && !aicon->icon->shadowed) {
                aicon->icon->shadowed = 1;
                aicon->icon->force_paint = 1;
                /* We don't do an wAppIconPaint() here because it's in
                 * wDockAttachIcon(). -Dan
                 */
            }
            wDockAttachIcon(clip, aicon, x, y);
            if (clip->collapsed || !clip->mapped)
                XUnmapWindow(dpy, aicon->icon->core->window);
        }
        aicon = aicon->next;
    }
}


static void
selectIconsCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *clickedIcon = (WAppIcon*)entry->clientdata;
    WDock *dock;
    LinkedList *selectedIcons;
    WAppIcon *btn;
    int i;

    assert(clickedIcon!=NULL);
    dock = clickedIcon->dock;

    selectedIcons = getSelected(dock);

    if (!selectedIcons) {
        for (i=1; i<dock->max_icons; i++) {
            btn = dock->icon_array[i];
            if (btn && !btn->icon->selected) {
		wIconSelect(btn->icon);
            }
        }
    } else {
        while(selectedIcons) {
            btn = selectedIcons->head;
	    wIconSelect(btn->icon);
            list_remove_head(&selectedIcons);
        }
    }

    wMenuPaint(menu);
}


static void
toggleCollapsedCallback(WMenu *menu, WMenuEntry *entry)
{
    assert(entry->clientdata!=NULL);

    toggleCollapsed(entry->clientdata);

    entry->flags.indicator_on = ((WDock*)entry->clientdata)->collapsed;

    wMenuPaint(menu);
}


static void
toggleAutoCollapseCallback(WMenu *menu, WMenuEntry *entry)
{
    assert(entry->clientdata!=NULL);

    toggleAutoCollapse(entry->clientdata);

    entry->flags.indicator_on = ((WDock*)entry->clientdata)->auto_collapse;

    wMenuPaint(menu);
}


static void
launchCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *btn = (WAppIcon*)entry->clientdata;
    
    launchDockedApplication(btn);
}


static void
settingsCallback(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *btn = (WAppIcon*)entry->clientdata;

    if (btn->editing)
	return;
    ShowDockAppSettingsPanel(btn);
}


static void
hideCallback(WMenu *menu, WMenuEntry *entry)
{
    WApplication *wapp;
    WAppIcon *btn = (WAppIcon*)entry->clientdata;
    
    wapp = wApplicationOf(btn->icon->owner->main_window);
    
    if (wapp->flags.hidden) {
	wWorkspaceChange(btn->icon->core->screen_ptr,wapp->last_workspace);
	wUnhideApplication(wapp, False, False);
    } else {
	wHideApplication(wapp);
    }
}


static void
unhideHereCallback(WMenu *menu, WMenuEntry *entry)
{
    WApplication *wapp;
    WAppIcon *btn = (WAppIcon*)entry->clientdata;
    
    wapp = wApplicationOf(btn->icon->owner->main_window);
    
    wUnhideApplication(wapp, False, True);
}


WAppIcon*
mainIconCreate(WScreen *scr, int type)
{
    WAppIcon *btn;
    int x_pos;

    if (type == WM_CLIP) {
        if (scr->clip_icon)
            return scr->clip_icon;
        btn = wAppIconCreateForDock(scr, NULL, "Logo", "WMClip", TILE_CLIP);
        btn->icon->core->descriptor.handle_expose = clipIconExpose;
        btn->icon->core->descriptor.handle_enternotify = clipEnterNotify;
        btn->icon->core->descriptor.handle_leavenotify = clipLeaveNotify;
        /*x_pos = scr->scr_width - ICON_SIZE*2 - DOCK_EXTRA_SPACE;*/
        x_pos = 0;
    }
    else {
        btn = wAppIconCreateForDock(scr, NULL, "Logo", "WMDock", TILE_NORMAL);
        x_pos = scr->scr_width - ICON_SIZE - DOCK_EXTRA_SPACE;
    }

    btn->xindex = 0;
    btn->yindex = 0;

    btn->icon->core->descriptor.handle_mousedown = iconMouseDown;
    btn->icon->core->descriptor.parent_type = WCLASS_DOCK_ICON;
    btn->icon->core->descriptor.parent = btn;
    /*ChangeStackingLevel(btn->icon->core, WMDockLevel);*/
    XMapWindow(dpy, btn->icon->core->window);
    btn->x_pos = x_pos;
    btn->y_pos = 0;
    btn->docked = 1;
    if (type == WM_CLIP)
        scr->clip_icon = btn;

    return btn;
}


static void
switchWSCommand(WMenu *menu, WMenuEntry *entry)
{
    WAppIcon *btn, *icon = (WAppIcon*) entry->clientdata;
    WScreen *scr = icon->icon->core->screen_ptr;
    WDock *src, *dest;
    LinkedList *selectedIcons;
    int x, y;

    if (entry->order == scr->current_workspace)
        return;
    src = icon->dock;
    dest = scr->workspaces[entry->order]->clip;

    selectedIcons = getSelected(src);

    if (selectedIcons) {
        while(selectedIcons) {
            btn = selectedIcons->head;
            if (wDockFindFreeSlot(dest, &x, &y)) {
                moveIconBetweenDocks(src, dest, btn, x, y);
                XUnmapWindow(dpy, btn->icon->core->window);
            }
            list_remove_head(&selectedIcons);
        }
    } else if (icon != scr->clip_icon) {
        if (wDockFindFreeSlot(dest, &x, &y)) {
            moveIconBetweenDocks(src, dest, icon, x, y);
            XUnmapWindow(dpy, icon->icon->core->window);
        }
    }
}



static void
launchDockedApplication(WAppIcon *btn)
{
    WScreen *scr = btn->icon->core->screen_ptr;

    if (!btn->launching && btn->command!=NULL) {
	if (!btn->forced_dock) {
	    btn->relaunching = btn->running;
	    btn->running = 1;
	}
	if (btn->wm_instance || btn->wm_class) {
	    WWindowAttributes attr;
	    memset(&attr, 0, sizeof(WWindowAttributes));
	    wDefaultFillAttributes(scr, btn->wm_instance, btn->wm_class, &attr,
				   True);

	    if (!attr.no_appicon && !btn->buggy_app)
		btn->launching = 1;
	    else 
		btn->running = 0;
	}
	btn->drop_launch = 0;
        scr->last_dock = btn->dock;
        btn->pid = execCommand(btn, btn->command, NULL);
        if (btn->pid>0) {
	    if (btn->buggy_app) {
		/* give feedback that the app was launched */
		btn->launching = 1;
		dockIconPaint(btn);
		btn->launching = 0;
		WMAddTimerHandler(200, (WMCallback*)dockIconPaint, btn);
	    } else {
		dockIconPaint(btn);
	    }
        } else {
	    wwarning(_("could not launch application %s\n"), btn->command);
            btn->launching = 0;
            if (!btn->relaunching)
                btn->running = 0;
        }
    }
}



static void
updateWorkspaceMenu(WMenu *menu, WAppIcon *icon)
{
    WScreen *scr = menu->frame->screen_ptr;
    char title[MAX_WORKSPACENAME_WIDTH+1];
    int i;

    if (!menu || !icon)
        return;

    for (i=0; i<scr->workspace_count; i++) {
        if (i < menu->entry_no) {
            if (strcmp(menu->entries[i]->text,scr->workspaces[i]->name)!=0) {
                free(menu->entries[i]->text);
                strcpy(title, scr->workspaces[i]->name);
                menu->entries[i]->text = wstrdup(title);
                menu->flags.realized = 0;
            }
            menu->entries[i]->clientdata = (void*)icon;
        } else {
            strcpy(title, scr->workspaces[i]->name);

            wMenuAddCallback(menu, title, switchWSCommand, (void*)icon);

            menu->flags.realized = 0;
        }
        if (i == scr->current_workspace) {
            wMenuSetEnabled(menu, i, False);
        } else {
            wMenuSetEnabled(menu, i, True);
        }
    }

    if (!menu->flags.realized)
        wMenuRealize(menu);
}


static WMenu*
makeWorkspaceMenu(WScreen *scr)
{
    WMenu *menu;

    menu = wMenuCreate(scr, NULL, False);
    if (!menu)
        wwarning(_("could not create workspace submenu for Clip menu"));

    wMenuAddCallback(menu, "", switchWSCommand, (void*)scr->clip_icon);

    menu->flags.realized = 0;
    wMenuRealize(menu);

    return menu;
}


static void
updateClipOptionsMenu(WMenu *menu, WDock *dock)
{
    WMenuEntry *entry;
    int index = 0;

    if (!menu || !dock)
        return;

    /* keep on top */
    entry = menu->entries[index];
    entry->flags.indicator_on = !dock->lowered;
    entry->clientdata = dock;

    /* collapsed */
    entry = menu->entries[++index];
    entry->flags.indicator_on = dock->collapsed;
    entry->clientdata = dock;

    /* auto-collapse */
    entry = menu->entries[++index];
    entry->flags.indicator_on = dock->auto_collapse;
    entry->clientdata = dock;

    /* attract icons */
    entry = menu->entries[++index];
    entry->flags.indicator_on = dock->attract_icons;
    entry->clientdata = dock;

    /* keep attracted icons */
    entry = menu->entries[++index];
    entry->flags.indicator_on = dock->keep_attracted;
    entry->clientdata = dock;

    menu->flags.realized = 0;
    wMenuRealize(menu);
}


static WMenu*
makeClipOptionsMenu(WScreen *scr)
{
    WMenu *menu;
    WMenuEntry *entry;

    menu = wMenuCreate(scr, NULL, False);
    if (!menu) {
        wwarning(_("could not create options submenu for Clip menu"));
	return NULL;
    }

    entry = wMenuAddCallback(menu, _("Keep on top"),
                             toggleLoweredCallback, NULL);
    entry->flags.indicator = 1;
    entry->flags.indicator_on = 1;
    entry->flags.indicator_type = MI_CHECK;

    entry = wMenuAddCallback(menu, _("Collapsed"),
                             toggleCollapsedCallback, NULL);
    entry->flags.indicator = 1;
    entry->flags.indicator_on = 1;
    entry->flags.indicator_type = MI_CHECK;

    entry = wMenuAddCallback(menu, _("AutoCollapse"),
                             toggleAutoCollapseCallback, NULL);
    entry->flags.indicator = 1;
    entry->flags.indicator_on = 1;
    entry->flags.indicator_type = MI_CHECK;

    entry = wMenuAddCallback(menu, _("AutoAttract Icons"),
                             toggleAutoAttractCallback, NULL);
    entry->flags.indicator = 1;
    entry->flags.indicator_on = 1;
    entry->flags.indicator_type = MI_CHECK;

    entry = wMenuAddCallback(menu, _("Keep Attracted Icons"),
                             toggleKeepCallback, NULL);
    entry->flags.indicator = 1;
    entry->flags.indicator_on = 1;
    entry->flags.indicator_type = MI_CHECK;

    menu->flags.realized = 0;
    wMenuRealize(menu);

    return menu;
}


static WMenu*
dockMenuCreate(WScreen *scr, int type)
{
    WMenu *menu;
    WMenuEntry *entry;

    if (type == WM_CLIP && scr->clip_menu)
        return scr->clip_menu;

    menu = wMenuCreate(scr, NULL, False);
    if (type != WM_CLIP) {
        entry = wMenuAddCallback(menu, _("Keep on top"),
                                 toggleLoweredCallback, NULL);
        entry->flags.indicator = 1;
        entry->flags.indicator_on = 1;
        entry->flags.indicator_type = MI_CHECK;
    } else {
        entry = wMenuAddCallback(menu, _("Clip Options"), NULL, NULL);
        scr->clip_options = makeClipOptionsMenu(scr);
        if (scr->clip_options)
            wMenuEntrySetCascade(menu, entry, scr->clip_options);

        wMenuAddCallback(menu, _("Rename Workspace"), renameCallback, NULL);

        wMenuAddCallback(menu, _("(Un)Select Icon"), selectCallback, NULL);

        wMenuAddCallback(menu, _("(Un)Select All Icons"), selectIconsCallback, 
			 NULL);

        wMenuAddCallback(menu, _("Keep Icon(s)"), keepIconsCallback, NULL);

        entry = wMenuAddCallback(menu, _("Move Icon(s) To"), NULL, NULL);
        scr->clip_submenu = makeWorkspaceMenu(scr);
        if (scr->clip_submenu)
            wMenuEntrySetCascade(menu, entry, scr->clip_submenu);

        wMenuAddCallback(menu, _("Remove Icon(s)"), removeIconsCallback, NULL);

        wMenuAddCallback(menu, _("Attract Icons"), colectIconsCallback, NULL);
    }

    wMenuAddCallback(menu, _("Launch"), launchCallback, NULL);

    wMenuAddCallback(menu, _("Unhide Here"), unhideHereCallback, NULL);

    wMenuAddCallback(menu, _("(Un)Hide"), hideCallback, NULL);

    wMenuAddCallback(menu, _("Settings..."), settingsCallback, NULL);

    wMenuAddCallback(menu, _("Kill"), killCallback, NULL);

    if (type == WM_CLIP)
        scr->clip_menu = menu;

    return menu;
}


WDock*
wDockCreate(WScreen *scr, int type)
{
    WDock *dock;
    WAppIcon *btn;
    int icon_count;

    make_keys();

    dock = wmalloc(sizeof(WDock));
    memset(dock, 0, sizeof(WDock));

    if (type == WM_CLIP)
        icon_count = CLIP_MAX_ICONS;
    else
        icon_count = scr->scr_height/wPreferences.icon_size;

    dock->icon_array = wmalloc(sizeof(WAppIcon*)*icon_count);
    memset(dock->icon_array, 0, sizeof(WAppIcon*)*icon_count);

    dock->max_icons = icon_count;

    btn = mainIconCreate(scr, type);

    btn->dock = dock;

    dock->x_pos = btn->x_pos;
    dock->y_pos = btn->y_pos;
    dock->screen_ptr = scr;
    dock->type = type;
    dock->icon_count = 1;
    dock->on_right_side = 1;
    dock->collapsed = 0;
    dock->auto_collapse = 0;
    dock->auto_collapse_magic = NULL;
    dock->attract_icons = 0;
    dock->keep_attracted = 0;
    dock->lowered = 1;
    dock->icon_array[0] = btn;
    wRaiseFrame(btn->icon->core);
    XMoveWindow(dpy, btn->icon->core->window, btn->x_pos, btn->y_pos);

    /* create dock menu */
    dock->menu = dockMenuCreate(scr, type);

    return dock;
}


void
wDockDestroy(WDock *dock)
{
    int i;
    WAppIcon *aicon;

    for (i=(dock->type == WM_CLIP) ? 1 : 0; i<dock->max_icons; i++) {
        aicon = dock->icon_array[i];
        if (aicon) {
            int keepit = aicon->running && wApplicationOf(aicon->main_window);
            wDockDetach(dock, aicon);
            if (keepit) {
                PlaceIcon(dock->screen_ptr, &aicon->x_pos, &aicon->y_pos);
                XMoveWindow(dpy, aicon->icon->core->window,
                            aicon->x_pos, aicon->y_pos);
                if (!dock->mapped || dock->collapsed)
                    XMapWindow(dpy, aicon->icon->core->window);
            }
        }
    }
    if (wPreferences.auto_arrange_icons)
        wArrangeIcons(dock->screen_ptr, True);
    free(dock->icon_array);
    if (dock->menu && dock->type!=WM_CLIP)
        wMenuDestroy(dock->menu, True);
    free(dock);
}


void
wClipIconPaint(WAppIcon *aicon)
{
    WScreen *scr = aicon->icon->core->screen_ptr;
    WWorkspace *workspace = scr->workspaces[scr->current_workspace];
    GC gc;
    Window win = aicon->icon->core->window;
    int length, nlength;
    char *ws_name, ws_number[10];
    int ty, tx;

    wIconPaint(aicon->icon);

    length = strlen(workspace->name);
    ws_name = malloc(length + 1);
    sprintf(ws_name, "%s", workspace->name);
    sprintf(ws_number, "%i", scr->current_workspace + 1);
    nlength = strlen(ws_number);

    gc = scr->clip_title_gc;

    if (!workspace->clip->collapsed)
        XSetForeground(dpy, gc, scr->clip_title_pixel[CLIP_NORMAL]);
    else
        XSetForeground(dpy, gc, scr->clip_title_pixel[CLIP_COLLAPSED]);

    ty = ICON_SIZE - scr->clip_title_font->height - 3;
    
    tx = CLIP_BUTTON_SIZE*ICON_SIZE/64;
    
    wDrawString(win, scr->clip_title_font, gc, tx,
		ty + scr->clip_title_font->y, ws_name, length);

    tx = (ICON_SIZE/2 - wTextWidth(scr->clip_title_font->font, ws_number, nlength))/2;

    wDrawString(win, scr->clip_title_font, gc, tx, 
		scr->clip_title_font->y + 2, ws_number, nlength);

    free(ws_name);

    if (aicon->launching) {
	XFillRectangle(dpy, aicon->icon->core->window, scr->stipple_gc, 
		       0, 0, wPreferences.icon_size, wPreferences.icon_size);
    }
    paintClipButtons(aicon, aicon->dock->lclip_button_pushed,
		     aicon->dock->rclip_button_pushed);
}


static void
clipIconExpose(WObjDescriptor *desc, XEvent *event)
{
    wClipIconPaint(desc->parent);
}


static void
dockIconPaint(WAppIcon *btn)
{
    if (btn == btn->icon->core->screen_ptr->clip_icon)
        wClipIconPaint(btn);
    else
        wAppIconPaint(btn);
}


static proplist_t
make_icon_state(WAppIcon *btn)
{
    proplist_t node = NULL;
    proplist_t command, autolaunch, name, forced, host, position, buggy;
    char *tmp;
    char buffer[64];

    if (btn) {
	if (!btn->command)
	    command = PLMakeString("-");
	else
	    command = PLMakeString(btn->command);

	autolaunch = btn->auto_launch ? dYes : dNo;
	
	tmp = EscapeWM_CLASS(btn->wm_instance, btn->wm_class);
	
	name = PLMakeString(tmp);

	free(tmp);

	forced = btn->forced_dock ? dYes : dNo;
	
	buggy = btn->buggy_app ? dYes: dNo;

        if (btn == btn->icon->core->screen_ptr->clip_icon)
            sprintf(buffer, "%i,%i", btn->x_pos, btn->y_pos);
        else
            sprintf(buffer, "%hi,%hi", btn->xindex, btn->yindex);
        position = PLMakeString(buffer);

        node = PLMakeDictionaryFromEntries(dCommand, command,
                                           dName, name,
                                           dAutoLaunch, autolaunch,
                                           dForced, forced,
					   dBuggyApplication, buggy,
                                           dPosition, position,
                                           NULL);
        PLRelease(command);
        PLRelease(name);
        PLRelease(position);
#ifdef OFFIX_DND
        if (btn->dnd_command) {
            command = PLMakeString(btn->dnd_command);
            PLInsertDictionaryEntry(node, dDropCommand, command);
            PLRelease(command);
	}
#endif /* OFFIX_DND */
 
	if (btn->client_machine && btn->remote_start) {
	    host = PLMakeString(btn->client_machine);
	    PLInsertDictionaryEntry(node, dHost, host);
	    PLRelease(host);
	}
    }

    return node;
}


static proplist_t
dockSaveState(WDock *dock)
{
    int i;
    proplist_t icon_info;
    proplist_t list=NULL, dock_state=NULL;
    proplist_t value;
    char buffer[256];

    list = PLMakeArrayFromElements(NULL);

    for (i=(dock->type==WM_DOCK ? 0 : 1); i<dock->max_icons; i++) {
        WAppIcon *btn = dock->icon_array[i];

        if (!btn || (btn->attracted && !dock->keep_attracted))
            continue;

        if ((icon_info = make_icon_state(dock->icon_array[i]))) {
            list = PLAppendArrayElement(list, icon_info);
            PLRelease(icon_info);
        }
    }

    dock_state = PLMakeDictionaryFromEntries(dApplications, list, NULL);

    PLRelease(list);

    if (dock->type == WM_DOCK) {
        sprintf(buffer, "%i,%i", (dock->on_right_side ? -ICON_SIZE : 0),
                                  dock->y_pos);
        value = PLMakeString(buffer);
        PLInsertDictionaryEntry(dock_state, dPosition, value);
        PLRelease(value);
    }

    value = (dock->lowered ? dYes : dNo);
    PLInsertDictionaryEntry(dock_state, dLowered, value);

    if (dock->type == WM_CLIP) {
        value = (dock->collapsed ? dYes : dNo);
        PLInsertDictionaryEntry(dock_state, dCollapsed, value);

        value = (dock->auto_collapse ? dYes : dNo);
	PLInsertDictionaryEntry(dock_state, dAutoCollapse, value);

        value = (dock->attract_icons ? dYes : dNo);
        PLInsertDictionaryEntry(dock_state, dAutoAttractIcons, value);

        value = (dock->keep_attracted ? dYes : dNo);
        PLInsertDictionaryEntry(dock_state, dKeepAttracted, value);
    }

    return dock_state;
}


void
wDockSaveState(WScreen *scr)
{
    proplist_t dock_state;

    dock_state = dockSaveState(scr->dock);

    PLInsertDictionaryEntry(scr->session_state, dDock, dock_state);

    PLRelease(dock_state);
}


void
wClipSaveState(WScreen *scr)
{
    proplist_t clip_state;

    clip_state = make_icon_state(scr->clip_icon);

    PLInsertDictionaryEntry(scr->session_state, dClip, clip_state);

    PLRelease(clip_state);
}


proplist_t
wClipSaveWorkspaceState(WScreen *scr, int workspace)
{
    return dockSaveState(scr->workspaces[workspace]->clip);
}


static WAppIcon*
restore_icon_state(WScreen *scr, proplist_t info, int type, int index)
{
    WAppIcon *aicon;
    char *wclass, *winstance;
    proplist_t cmd, value;
    char *command;


    cmd = PLGetDictionaryEntry(info, dCommand);
    if (!cmd || !PLIsString(cmd)) {
	return NULL;
    }

    /* parse window name */
    value = PLGetDictionaryEntry(info, dName);
    if (!value)
	return NULL;
    
    ParseWindowName(value, &winstance, &wclass, "dock");
    
    if (!winstance && !wclass) {
	return NULL;
    }

    /* get commands */

    if (cmd)
	command = wstrdup(PLGetString(cmd));
    else
	command = NULL;

    if (!command || strcmp(command, "-")==0) {
	if (command)
	    free(command);
	if (wclass)
	    free(wclass);
	if (winstance)
	    free(winstance);

	return NULL;
    }

    aicon = wAppIconCreateForDock(scr, command, winstance, wclass, 
				  TILE_NORMAL);
    if (wclass)
	free(wclass);
    if (winstance)
	free(winstance);
    
    aicon->icon->core->descriptor.handle_mousedown = iconMouseDown;
    if (type == WM_CLIP) {
        aicon->icon->core->descriptor.handle_enternotify = clipEnterNotify;
        aicon->icon->core->descriptor.handle_leavenotify = clipLeaveNotify;
    }
    aicon->icon->core->descriptor.parent_type = WCLASS_DOCK_ICON;
    aicon->icon->core->descriptor.parent = aicon;

    
#ifdef OFFIX_DND
    cmd = PLGetDictionaryEntry(info, dDropCommand);
    if (cmd)
	aicon->dnd_command = wstrdup(PLGetString(cmd));
#endif

    /* check auto launch */
    value = PLGetDictionaryEntry(info, dAutoLaunch);

    aicon->auto_launch = 0;
    if (value) {
        if (PLIsString(value)) {
            if (strcasecmp(PLGetString(value), "YES")==0)
                aicon->auto_launch = 1;
        } else {
            wwarning(_("bad value in docked icon state info %s"),
                     PLGetString(dAutoLaunch));
        }
    }

    /* check if it wasn't normally docked */
    value = PLGetDictionaryEntry(info, dForced);

    aicon->forced_dock = 0;
    if (value) {
        if (PLIsString(value)) {
            if (strcasecmp(PLGetString(value), "YES")==0)
                aicon->forced_dock = 1;
        } else {
            wwarning(_("bad value in docked icon state info %s"),
                     PLGetString(dForced));
        }
    }

    /* check if we can rely on the stuff in the app */
    value = PLGetDictionaryEntry(info, dBuggyApplication);

    aicon->buggy_app = 0;
    if (value) {
        if (PLIsString(value)) {
            if (strcasecmp(PLGetString(value), "YES")==0)
                aicon->buggy_app = 1;
        } else {
            wwarning(_("bad value in docked icon state info %s"),
                     PLGetString(dBuggyApplication));
        }
    }

    /* get position in the dock */
    value = PLGetDictionaryEntry(info, dPosition);
    if (value && PLIsString(value)) {
        if (sscanf(PLGetString(value), "%hi,%hi", &aicon->xindex,
                   &aicon->yindex)!=2)
            wwarning(_("bad value in docked icon state info %s"),
                     PLGetString(dPosition));

        /* check position sanity */
        /* incomplete section! */
        if (type == WM_DOCK) {
            aicon->xindex = 0;
            if (aicon->yindex < 0)
                wwarning(_("bad value in docked icon position %i,%i"),
                         aicon->xindex, aicon->yindex);
        }
    } else {
        aicon->yindex = index;
        aicon->xindex = 0;
    }

    aicon->running = 0;
    aicon->docked = 1;

    return aicon;
}


#define COMPLAIN(key) wwarning(_("bad value in dock state info:%s"), key)


WAppIcon*
wClipRestoreState(WScreen *scr, proplist_t clip_state)
{
    WAppIcon *icon;
    proplist_t value;


    icon = mainIconCreate(scr, WM_CLIP);

    if (!clip_state)
	return icon;
    else
        PLRetain(clip_state);

    /* restore position */

    value = PLGetDictionaryEntry(clip_state, dPosition);

    if (value) {
	if (!PLIsString(value))
	    COMPLAIN("Position");
	else {
	    if (sscanf(PLGetString(value), "%i,%i", &icon->x_pos,
		       &icon->y_pos)!=2)
		COMPLAIN("Position");
	    
	    /* check position sanity */
	    if (icon->y_pos < 0)
		icon->y_pos = 0;
	    else if (icon->y_pos > scr->scr_height-ICON_SIZE)
                icon->y_pos = scr->scr_height-ICON_SIZE;

            if (icon->x_pos < 0)
                icon->x_pos = 0;
            else if (icon->x_pos > scr->scr_width-ICON_SIZE)
                icon->x_pos = scr->scr_width-ICON_SIZE;
	}
    }

#ifdef OFFIX_DND
    value = PLGetDictionaryEntry(clip_state, dDropCommand);
    if (value && PLIsString(value))
	icon->dnd_command = wstrdup(PLGetString(value));
#endif

    PLRelease(clip_state);

    return icon;
}


WDock*
wDockRestoreState(WScreen *scr, proplist_t dock_state, int type)
{
    WDock *dock;
    proplist_t apps;
    proplist_t value;
    WAppIcon *aicon, *old_top;
    int count, i;
    
    
    dock = wDockCreate(scr, type);
    
    if (!dock_state)
        return dock;
    
    if (dock_state)
        PLRetain(dock_state);
    
    
    /* restore position */
    
    value = PLGetDictionaryEntry(dock_state, dPosition);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("Position");
        else {
            if (sscanf(PLGetString(value), "%i,%i", &dock->x_pos, 
                       &dock->y_pos)!=2)
                COMPLAIN("Position");
            
            /* check position sanity */
            if (dock->y_pos < 0) 
                dock->y_pos = 0;
            else if (dock->y_pos > scr->scr_height-ICON_SIZE)
                dock->y_pos = scr->scr_height-ICON_SIZE;
            
            /* This is no more needed. ??? */
            if (type == WM_CLIP) {
                if (dock->x_pos < 0)
                    dock->x_pos = 0;
                else if (dock->x_pos > scr->scr_width-ICON_SIZE)
                    dock->x_pos = scr->scr_width-ICON_SIZE;
            }
            else {
                if (dock->x_pos >= 0) {
                    dock->x_pos = DOCK_EXTRA_SPACE;
                    dock->on_right_side = 0;
                } else {
                    dock->x_pos = scr->scr_width - DOCK_EXTRA_SPACE - ICON_SIZE;
                    dock->on_right_side = 1;
                }
            }
        }
    }
    
    /* restore lowered/raised state */
    
    dock->lowered = 0;
    
    value = PLGetDictionaryEntry(dock_state, dLowered);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("Lowered");
        else {
            if (strcasecmp(PLGetString(value), "YES")==0)
                dock->lowered = 1;
        }
    }
    
    
    /* restore collapsed state */
    
    dock->collapsed = 0;
    
    value = PLGetDictionaryEntry(dock_state, dCollapsed);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("Collapsed");
        else {
            if (strcasecmp(PLGetString(value), "YES")==0)
                dock->collapsed = 1;
        }
    }
    
    
    /* restore auto-collapsed state */
    
    value = PLGetDictionaryEntry(dock_state, dAutoCollapse);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("AutoCollapse");
        else { 
	    if (strcasecmp(PLGetString(value), "YES")==0) {
                dock->auto_collapse = 1;
		dock->collapsed = 1;
	    }
        }
    }
    
    
    /* restore attract icons state */
    
    dock->attract_icons = 0;
    
    value = PLGetDictionaryEntry(dock_state, dAutoAttractIcons);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("AutoAttractIcons");
        else {
            if (strcasecmp(PLGetString(value), "YES")==0)
                dock->attract_icons = 1;
        }
    }
    
    
    /* restore keep attracted icons state */
    
    dock->keep_attracted = 0;
    
    value = PLGetDictionaryEntry(dock_state, dKeepAttracted);
    
    if (value) {
        if (!PLIsString(value))
            COMPLAIN("KeepAttracted");
        else {
            if (strcasecmp(PLGetString(value), "YES")==0)
                dock->keep_attracted = 1;
        }
    }
    
    
    /* application list */
    
    apps = PLGetDictionaryEntry(dock_state, dApplications);
    
    if (!apps) {
        goto finish;
    }
    
    count = PLGetNumberOfElements(apps);
    
    if (count==0) 
        goto finish;

    old_top = dock->icon_array[0];

    /* dock->icon_count is set to 1 when dock is created.
     * Since Clip is already restored, we want to keep it so for clip,
     * but for dock we may change the default top tile, so we set it to 0.
     */
    if (type == WM_DOCK)
        dock->icon_count = 0;

    for (i=0; i<count; i++) {
        if (dock->icon_count >= dock->max_icons) {
            wwarning(_("there are too many icons stored in dock. Ignoring what doesn't fit"));
            break;
        }

        value = PLGetArrayElement(apps, i);
        aicon = restore_icon_state(scr, value, type, dock->icon_count);

        dock->icon_array[dock->icon_count] = aicon;

        if (aicon) {
            aicon->dock = dock;
            aicon->x_pos = dock->x_pos + (aicon->xindex*ICON_SIZE);
            aicon->y_pos = dock->y_pos + (aicon->yindex*ICON_SIZE);
            
            if (dock->lowered)
                ChangeStackingLevel(aicon->icon->core, WMNormalLevel);
            else
                ChangeStackingLevel(aicon->icon->core, WMDockLevel);
            
            wCoreConfigure(aicon->icon->core, aicon->x_pos, aicon->y_pos,
                           0, 0);
            
            if (!dock->collapsed)
                XMapWindow(dpy, aicon->icon->core->window);
            wRaiseFrame(aicon->icon->core);

            dock->icon_count++;
        } else if (dock->icon_count==0 && type==WM_DOCK)
            dock->icon_count++;
    }
    
    /* if the first icon is not defined, use the default */
    if (dock->icon_array[0]==NULL) {
        /* update default icon */
        old_top->x_pos = dock->x_pos;
        old_top->y_pos = dock->y_pos;
        if (dock->lowered)         
            ChangeStackingLevel(old_top->icon->core, WMNormalLevel);
        else
            ChangeStackingLevel(old_top->icon->core, WMDockLevel);
        dock->icon_array[0] = old_top;
        XMoveWindow(dpy, old_top->icon->core->window, dock->x_pos, dock->y_pos);
        /* we don't need to increment dock->icon_count here because it was
         * incremented in the loop above.
         */
    } else if (old_top!=dock->icon_array[0]) {
        if (old_top == scr->clip_icon)
            scr->clip_icon = dock->icon_array[0];
        wAppIconDestroy(old_top);
    }
    
finish:
    if (dock_state)
        PLRelease(dock_state);
    
    return dock;
}



void
wDockLaunchWithState(WDock *dock, WAppIcon *btn, WSavedState *state)
{
    if (btn && btn->command && !btn->running && !btn->launching) {

        btn->drop_launch = 0;

        btn->pid = execCommand(btn, btn->command, state);

        if (btn->pid>0) {
            if (!btn->forced_dock && !btn->buggy_app) {
                btn->launching = 1;
                dockIconPaint(btn);
            }
        }
    } else {
        free(state);
    }
}


void
wDockDoAutoLaunch(WDock *dock, int workspace)
{
    WAppIcon *btn;
    WSavedState *state;
    int i;

    for (i=0; i < dock->max_icons; i++) {
        btn = dock->icon_array[i];
        if (!btn || !btn->auto_launch)
            continue;

        state = wmalloc(sizeof(WSavedState));
        memset(state, 0, sizeof(WSavedState));
        state->workspace = workspace;
	/* TODO: this is klugy and is very difficult to understand
	 * what's going on. Try to clean up */
        wDockLaunchWithState(dock, btn, state);
    }
}

#ifdef REDUCE_APPICONS
void
wDockSimulateLaunch(WDock *dock, WAppIcon *btn)
{
    if ((btn == NULL) || (dock == NULL))
	return;

    if (!btn->running) {
	if ((btn->icon->owner == NULL) && (btn->applist))
	    btn->icon->owner = btn->applist->wapp->main_window_desc;
	if (!btn->forced_dock)	
	   btn->launching = 1;
        dockIconPaint(btn);
	wusleep(5000);
    }
}
#endif

#ifdef OFFIX_DND
static WDock*
findDock(WScreen *scr, XEvent *event, int *icon_pos)
{
    WDock *dock;
    int i;

    *icon_pos = -1;
    if ((dock = scr->dock)!=NULL) {
        for (i=0; i<dock->max_icons; i++) {
            if (dock->icon_array[i]
                && dock->icon_array[i]->icon->core->window==event->xclient.window) {
                *icon_pos = i;
                break;
            }
        }
    }
    if (*icon_pos<0 && (dock = scr->workspaces[scr->current_workspace]->clip)!=NULL) {
        for (i=0; i<dock->max_icons; i++) {
            if (dock->icon_array[i]
                && dock->icon_array[i]->icon->core->window==event->xclient.window) {
                *icon_pos = i;
                break;
            }
        }
    }
    if(*icon_pos>=0)
        return dock;
    return NULL;
}


int
wDockReceiveDNDDrop(WScreen *scr, XEvent *event)
{
    WDock *dock;
    WAppIcon *btn;
    int icon_pos;
    
    dock = findDock(scr, event, &icon_pos);
    if (!dock)
	return False;
    	
    /*
     * Return True if the drop was on an application icon window.
     * In this case, let the ClientMessage handler redirect the
     * message to the app.
     */
    if (dock->icon_array[icon_pos]->icon->icon_win!=None)
        return True;

    if (dock->icon_array[icon_pos]->dnd_command!=NULL) {
        scr->flags.dnd_data_convertion_status = 0;

        btn = dock->icon_array[icon_pos];

        if (!btn->forced_dock) {
            btn->relaunching = btn->running;
            btn->running = 1;
        }
        if (btn->wm_instance || btn->wm_class) {
            WWindowAttributes attr;
	    memset(&attr, 0, sizeof(WWindowAttributes));
            wDefaultFillAttributes(btn->icon->core->screen_ptr,
                                   btn->wm_instance,
                                   btn->wm_class, &attr, True);

            if (!attr.no_appicon)
                btn->launching = 1;
            else
                btn->running = 0;
        }

        btn->drop_launch = 1;
        scr->last_dock = dock;
        btn->pid = execCommand(btn, btn->dnd_command, NULL);
        if (btn->pid>0) {
            dockIconPaint(btn);
        } else {
            btn->launching = 0;
            if (!btn->relaunching) {
                btn->running = 0;
            }
        }
    }
    return False;
}
#endif /* OFFIX_DND */



Bool
wDockAttachIcon(WDock *dock, WAppIcon *icon, int x, int y)
{
    WWindow *wwin;
    char **argv;
    int argc;
    int index;

    wwin = icon->icon->owner;
    if (icon->command==NULL) {
	icon->editing = 0;
	if (XGetCommand(dpy, wwin->client_win, &argv, &argc) && argc>0) {

	    icon->command = FlattenStringList(argv, argc);
	    XFreeStringList(argv);
	} else {
	    char *command=NULL;

/*	    icon->forced_dock = 1;*/
            if (!icon->attracted || dock->type!=WM_CLIP || dock->keep_attracted) {
		icon->editing = 1;
                if (wInputDialog(dock->screen_ptr, _("Dock Icon"),
                                 _("Type the command used to launch the application"),
				 &command)) {
                    if (command && (command[0]==0 ||
                                    (command[0]=='-' && command[1]==0))) {
                        free(command);
                        command = NULL;
                    }
                    icon->command = command;
		    icon->editing = 0;
                } else {
		    icon->editing = 0;
		    if (command)
			free(command);
		    /* If the target is the dock, reject the icon. If
		     * the target is the clip, make it an attracted icon
		     */
		    if (dock->type==WM_CLIP) {
			icon->attracted = 1;
			if (!icon->icon->shadowed) {
			    icon->icon->shadowed = 1;
			    icon->icon->force_paint = 1;
			}
		    } else {
			return False;
		    }
                }
            }
	}
    } else {
	icon->editing = 0;
    }

    for (index=1; index<dock->max_icons; index++)
        if (dock->icon_array[index] == NULL)
            break;
    /* if (index == dock->max_icons)
         return; */

    assert(index < dock->max_icons);

    dock->icon_array[index] = icon;
    icon->yindex = y;
    icon->xindex = x;

    icon->x_pos = dock->x_pos + x*ICON_SIZE;
    icon->y_pos = dock->y_pos + y*ICON_SIZE;

    dock->icon_count++;

    icon->running = 1;
    icon->launching = 0;
    icon->docked = 1;
    icon->dock = dock;
    icon->icon->core->descriptor.handle_mousedown = iconMouseDown;
    if (dock->type == WM_CLIP) {
        icon->icon->core->descriptor.handle_enternotify = clipEnterNotify;
        icon->icon->core->descriptor.handle_leavenotify = clipLeaveNotify;
    }
    icon->icon->core->descriptor.parent_type = WCLASS_DOCK_ICON;
    icon->icon->core->descriptor.parent = icon;

    MoveInStackListUnder(dock->icon_array[index-1]->icon->core,
                         icon->icon->core);
    wAppIconMove(icon, icon->x_pos, icon->y_pos);
    wAppIconPaint(icon);

    if (wPreferences.auto_arrange_icons)
        wArrangeIcons(dock->screen_ptr, True);
    
#ifdef OFFIX_DND
    if (icon->command && !icon->dnd_command) {
	icon->dnd_command = wmalloc(strlen(icon->command)+8);
	sprintf(icon->dnd_command, "%s %%d", icon->command);
    }
#endif

    return True;
}


void
reattachIcon(WDock *dock, WAppIcon *icon, int x, int y)
{
    int index;

    for(index=1; index<dock->max_icons; index++) {
        if(dock->icon_array[index] == icon)
            break;
    }
    assert(index < dock->max_icons);
    
    icon->yindex = y;
    icon->xindex = x;

    icon->x_pos = dock->x_pos + x*ICON_SIZE;
    icon->y_pos = dock->y_pos + y*ICON_SIZE;
}


Bool
moveIconBetweenDocks(WDock *src, WDock *dest, WAppIcon *icon, int x, int y)
{
    WWindow *wwin;
    char **argv;
    int argc;
    int index;

    if (dest == NULL)
        return False;

    wwin = icon->icon->owner;

    /*
     * For the moment we can't do this if we move icons in Clip from one
     * workspace to other, because if we move two or more icons without
     * command, the dialog box will not be able to tell us to which of the
     * moved icons it applies. -Dan
     */
    if ((dest->type==WM_DOCK /*|| dest->keep_attracted*/) && icon->command==NULL) {
        if (XGetCommand(dpy, wwin->client_win, &argv, &argc) && argc>0) {

            icon->command = FlattenStringList(argv, argc);
            XFreeStringList(argv);
        } else {
            char *command=NULL;

	    icon->editing = 1;
/*            icon->forced_dock = 1;*/
            if (wInputDialog(src->screen_ptr, _("Dock Icon"),
			     _("Type the command used to launch the application"),
                             &command)) {
                if (command && (command[0]==0 ||
                                (command[0]=='-' && command[1]==0))) {
                    free(command);
                    command = NULL;
                }
                icon->command = command;
            } else {
		icon->editing = 0;
		if (command)
		    free(command);
		return False;
            }
	    icon->editing = 0;
        }
    }

    for(index=1; index<src->max_icons; index++) {
        if(src->icon_array[index] == icon)
            break;
    }
    assert(index < src->max_icons);

    src->icon_array[index] = NULL;
    src->icon_count--;

    for(index=1; index<dest->max_icons; index++) {
        if(dest->icon_array[index] == NULL)
            break;
    }
    /* if (index == dest->max_icons)
        return; */

    assert(index < dest->max_icons);

    dest->icon_array[index] = icon;
    icon->dock = dest;

    /* deselect the icon */
    if (icon->icon->selected)
        wIconSelect(icon->icon);

    if (dest->type == WM_DOCK) {
        icon->icon->core->descriptor.handle_enternotify = NULL;
        icon->icon->core->descriptor.handle_leavenotify = NULL;
    } else {
        icon->icon->core->descriptor.handle_enternotify = clipEnterNotify;
        icon->icon->core->descriptor.handle_leavenotify = clipLeaveNotify;
    }

    /* set it to be kept when moving to dock, or to a clip that keep the
     * attracted icons.
     * Unless the icon does not have a command set 
     */
    if (icon->command && (dest->type==WM_DOCK || dest->keep_attracted)) {
	icon->attracted = 0;
	if (icon->icon->shadowed) {
	    icon->icon->shadowed = 0;
	    icon->icon->force_paint = 1;
	}
    }
    
    if (src->auto_collapse)
        clipLeave(src);

    icon->yindex = y;
    icon->xindex = x;

    icon->x_pos = dest->x_pos + x*ICON_SIZE;
    icon->y_pos = dest->y_pos + y*ICON_SIZE;

    dest->icon_count++;

    MoveInStackListUnder(dest->icon_array[index-1]->icon->core,
                         icon->icon->core);
    wAppIconPaint(icon);
    
    return True;
}

void
wDockDetach(WDock *dock, WAppIcon *icon)
{
    int index;

    /* make the settings panel be closed */
    if (icon->panel) {
	DestroyDockAppSettingsPanel(icon->panel);
    }
    
    icon->docked = 0;
    icon->dock = NULL;
    icon->attracted = 0;
    if (icon->icon->shadowed) {
	icon->icon->shadowed = 0;
	icon->icon->force_paint = 1;
    }

    /* deselect the icon */
    if (icon->icon->selected)
	wIconSelect(icon->icon);

    if (icon->command) {
	free(icon->command);
	icon->command = NULL;
    }
#ifdef OFFIX_DND
    if (icon->dnd_command) {
        free(icon->dnd_command);
        icon->dnd_command = NULL;
    }
#endif

    for (index=1; index<dock->max_icons; index++)
        if (dock->icon_array[index] == icon)
            break;
    assert(index < dock->max_icons);
    dock->icon_array[index] = NULL;
    icon->yindex = -1;
    icon->xindex = -1;
    dock->icon_count--;
    
    /* if the dock is not attached to an application or
     * the the application did not set the approriate hints yet,
     * destroy the icon */
#ifdef REDUCE_APPICONS
    if ((icon->num_apps == 0) && (!icon->running || !wApplicationOf(icon->main_window)) )
#else
    if (!icon->running || !wApplicationOf(icon->main_window))
#endif
	wAppIconDestroy(icon);
    else {
	icon->icon->core->descriptor.handle_mousedown = appIconMouseDown;
	icon->icon->core->descriptor.handle_enternotify = NULL;
	icon->icon->core->descriptor.handle_leavenotify = NULL;
        icon->icon->core->descriptor.parent_type = WCLASS_APPICON;
	icon->icon->core->descriptor.parent = icon;

	ChangeStackingLevel(icon->icon->core, NORMAL_ICON_LEVEL);

	wAppIconPaint(icon);
        if (wPreferences.auto_arrange_icons) {
	    wArrangeIcons(dock->screen_ptr, True);
	}
    }
    if (dock->auto_collapse)
        clipLeave(dock);
}


/*
 * returns the closest Dock slot index for the passed 
 * coordinates.
 * 
 * Returns False if icon can't be docked.
 */
Bool
wDockSnapIcon(WDock *dock, WAppIcon *icon, int req_x, int req_y, 
	      int *ret_x, int *ret_y, int redocking)
{
    WScreen *scr = dock->screen_ptr;
    int dx, dy;
    int ex_x, ex_y;
    int i, offset = ICON_SIZE/2;
    WAppIcon *aicon = NULL;
    WAppIcon *nicon = NULL;


    if (wPreferences.flags.noupdates)
	return False;

    dx = dock->x_pos;
    dy = dock->y_pos;

    /* if the dock is full */
    if (!redocking &&
        (dock->icon_count >= dock->max_icons)) {
	return False;
    }
    
    /* exact position */
    if (req_y < dy)
        ex_y = (req_y - offset - dy)/ICON_SIZE;
    else
        ex_y = (req_y + offset - dy)/ICON_SIZE;

    if (req_x < dx)
        ex_x = (req_x - offset - dx)/ICON_SIZE;
    else
        ex_x = (req_x + offset - dx)/ICON_SIZE;

    /* check if the icon is outside the screen boundaries */
    if (dx + ex_x*ICON_SIZE < -ICON_SIZE+2 ||
	dx + ex_x*ICON_SIZE > scr->scr_width-1 ||
	dy + ex_y*ICON_SIZE < -ICON_SIZE+2 ||
	dy + ex_y*ICON_SIZE > scr->scr_height-1)
	return False;

    if (dock->type == WM_DOCK) {
	if (icon->dock != dock && ex_x != 0)
	    return False;

	for (i=0; i<dock->max_icons; i++) {
	    nicon = dock->icon_array[i];
	    if (nicon && nicon->yindex == ex_y) {
		aicon = nicon;
		break;
	    }
	}

	*ret_x = 0;

	if (redocking) {
	    int sig, done, closest;

	    /* Possible cases when redocking:
	     *
	     * icon dragged out of range of any slot -> false
	     * icon dragged to range of free slot
	     * icon dragged to range of same slot
	     * icon dragged to range of different icon
	     */
	    if (abs(ex_x) > DOCK_DETTACH_THRESHOLD)
		return False;

	    if (ex_y >=0 && (aicon == icon || !aicon)) {

		*ret_y = ex_y;

		return True;
	    }

	    /* start looking at the upper slot or lower? */
	    if (ex_y*ICON_SIZE < (req_y + offset - dy))
		sig = 1;
	    else
		sig = -1;

	    closest = -1;
	    done = 0;
	    /* look for closest free slot */
	    for (i=0; i<(DOCK_DETTACH_THRESHOLD+1)*2 && !done; i++) {
		int j;

		done = 1;
		closest = sig*(i/2) + ex_y;
		/* check if this slot is used */
		if (closest >= 0) {
		    for (j = 0; j<dock->max_icons; j++) {
			if (dock->icon_array[j] 
			    && dock->icon_array[j]->yindex==closest) {
			    /* slot is used by someone else */
			    if (dock->icon_array[j]!=icon)
				done = 0;
			    break;
			}
		    }
		}
		sig = -sig;
	    }
	    if (done && closest >= 0 &&
		((ex_y >= closest && ex_y - closest < DOCK_DETTACH_THRESHOLD+1)
		||
		 (ex_y < closest && closest - ex_y <= DOCK_DETTACH_THRESHOLD+1))) {

		*ret_y = closest;

		return True;
	    }
	} else { /* !redocking */

	    /* if slot is free and the icon is close enough, return it */
	    if (!aicon && ex_x==0 && ex_y>=0) {
		*ret_y = ex_y;
		return True;
	    }
	}
    } else { /* CLIP */
	int neighbours = 0;

	for (i=0; i<dock->max_icons; i++) {
	    nicon = dock->icon_array[i];
	    if (nicon && nicon->xindex == ex_x && nicon->yindex == ex_y) {
		aicon = nicon;
		break;
	    }
	}
	
	for (i=0; i<dock->max_icons; i++) {
	    nicon = dock->icon_array[i];
	    if (nicon && nicon != icon && /* Icon can't be it's own neighbour */
		(abs(nicon->xindex - ex_x) <= CLIP_ATTACH_VICINITY &&
		 abs(nicon->yindex - ex_y) <= CLIP_ATTACH_VICINITY)) {
		neighbours = 1;
		break;
	    }
	}

	if (neighbours && (aicon==NULL || (redocking && aicon == icon))) {
	    *ret_x = ex_x;
	    *ret_y = ex_y;
	    return True;
	}
    }
    return False;
}

#define MIN(x, y)  ((x) > (y) ? (y) : (x))
#define MAX(x, y)  ((x) < (y) ? (y) : (x))
    
#define ON_SCREEN(x, y, sx, ex, sy, ey) \
    ((((x)+ICON_SIZE/2) >= (sx)) && (((y)+ICON_SIZE/2) >= (sy)) && \
    (((x) + (ICON_SIZE/2)) <= (ex)) && (((y) + (ICON_SIZE/2)) <= ey))


/*
 * returns true if it can find a free slot in the dock,
 * in which case it changes x_pos and y_pos accordingly.
 * Else returns false.
 */
Bool
wDockFindFreeSlot(WDock *dock, int *x_pos, int *y_pos)
{
    WScreen *scr = dock->screen_ptr;
    WAppIcon *btn;
    unsigned char *slot_map;
    int mwidth;
    int r;
    int x, y;
    int i, done = False;
    int corner;
    int sx=0, sy=0, ex=scr->scr_width, ey=scr->scr_height;


    /* if the dock is full */
    if (dock->icon_count >= dock->max_icons) {
	return False;
    }

    if (!wPreferences.flags.nodock && scr->dock) {
        if (scr->dock->on_right_side)
            ex -= ICON_SIZE + DOCK_EXTRA_SPACE;
        else
            sx += ICON_SIZE + DOCK_EXTRA_SPACE;
    }

    if (ex < dock->x_pos)
        ex = dock->x_pos;
    if (sx > dock->x_pos+ICON_SIZE)
        sx = dock->x_pos+ICON_SIZE;
#define C_NONE 0
#define C_NW 1
#define C_NE 2
#define C_SW 3
#define C_SE 4
    
    /* check if clip is in a corner */
    if (dock->type==WM_CLIP) {
	if (dock->x_pos < 1 && dock->y_pos < 1)
	    corner = C_NE;
	else if (dock->x_pos < 1 && dock->y_pos >= (ey-ICON_SIZE))
	    corner = C_SE;
	else if (dock->x_pos >= (ex-ICON_SIZE)&& dock->y_pos >= (ey-ICON_SIZE))
	    corner = C_SW;
	else if (dock->x_pos >= (ex-ICON_SIZE) && dock->y_pos < 1)
	    corner = C_NW;
	else
	    corner = C_NONE;
    } else
	corner = C_NONE;
    
    /* If the clip is in the corner, use only slots that are in the border
     * of the screen */
    if (corner!=C_NONE) {
	char *hmap, *vmap;
	int hcount, vcount;

	hcount = MIN(dock->max_icons, scr->scr_width/ICON_SIZE);
	vcount = MIN(dock->max_icons, scr->scr_height/ICON_SIZE);
	hmap = wmalloc(hcount+1);
	memset(hmap, 0, hcount+1);
	vmap = wmalloc(vcount+1);
	memset(vmap, 0, vcount+1);

	/* mark used positions */
	switch (corner) {
	 case C_NE:
	    for (i=0; i<dock->max_icons; i++) {
		btn = dock->icon_array[i];
		if (!btn)
		    continue;
		
		if (btn->xindex==0 && btn->yindex > 0 && btn->yindex < vcount)
		    vmap[btn->yindex] = 1;
		else if (btn->yindex==0 && btn->xindex>0 && btn->xindex<hcount)
		    hmap[btn->xindex] = 1;
	    }
	 case C_NW:
	    for (i=0; i<dock->max_icons; i++) {
		btn = dock->icon_array[i];
		if (!btn)
		    continue;
		
		if (btn->xindex==0 && btn->yindex > 0 && btn->yindex < vcount)
		    vmap[btn->yindex] = 1;
		else if (btn->yindex==0 && btn->xindex<0 &&btn->xindex>-hcount)
		    hmap[-btn->xindex] = 1;
	    }
	 case C_SE:
	    for (i=0; i<dock->max_icons; i++) {
		btn = dock->icon_array[i];
		if (!btn)
		    continue;

		if (btn->xindex==0 && btn->yindex < 0 && btn->yindex > -vcount)
		    vmap[-btn->yindex] = 1;
		else if (btn->yindex==0 && btn->xindex>0 && btn->xindex<hcount)
		    hmap[btn->xindex] = 1;
	    }
	 case C_SW:
	 default:
	    for (i=0; i<dock->max_icons; i++) {
		btn = dock->icon_array[i];
		if (!btn)
		    continue;

		if (btn->xindex==0 && btn->yindex < 0 && btn->yindex > -vcount)
		    vmap[-btn->yindex] = 1;
		else if (btn->yindex==0 && btn->xindex<0 &&btn->xindex>-hcount)
		    hmap[-btn->xindex] = 1;
	    }
	}
	x=0; y=0;
	done = 0;
	/* search a vacant slot */
	for (i=1; i<MAX(vcount, hcount); i++) {
	    if (i < vcount && vmap[i]==0) {
		/* found a slot */
		x = 0;
		y = i;
		done = 1;
		break;
	    } else if (i < hcount && hmap[i]==0) {
		/* found a slot */
		x = i;
		y = 0;
		done = 1;
		break;
	    }
	}
	free(vmap);
	free(hmap);
	/* If found a slot, translate and return */
        if (done) {
	    if (corner==C_NW || corner==C_NE) {
		*y_pos = y;
	    } else {
		*y_pos = -y;
	    }
	    if (corner==C_NE || corner==C_SE) {
		*x_pos = x;
	    } else {
		*x_pos = -x;
	    }
	    return True;
	}
	/* else, try to find a slot somewhere else */
    }
    
    /* a map of mwidth x mwidth would be enough if we allowed icons to be
     * placed outside of screen */
    mwidth = (int)ceil(sqrt(dock->max_icons));

    /* In the worst case (the clip is in the corner of the screen),
     * the amount of icons that fit in the clip is smaller.
     * Double the map to get a safe value.
     */
    mwidth += mwidth;

    r = (mwidth-1)/2;

    slot_map = wmalloc(mwidth*mwidth);
    memset(slot_map, 0, mwidth*mwidth);

#define XY2OFS(x,y) (MAX(abs(x),abs(y)) > r) ? 0 : (((y)+r)*(mwidth)+(x)+r)

    /* mark used slots in the map. If the slot falls outside the map
     * (for example, when all icons are placed in line), ignore them. */
    for (i=0; i<dock->max_icons; i++) {
	btn = dock->icon_array[i];
	if (btn)
	    slot_map[XY2OFS(btn->xindex, btn->yindex)] = 1;
    }
    /* Find closest slot from the center that is free by scanning the
     * map from the center to outward in circular passes.
     * This will not result in a neat layout, but will be optimal
     * in the sense that there will not be holes left.
     */
    done = 0;
    for (i = 1; i <= r && !done; i++) {
	int tx, ty;
	
	/* top and bottom parts of the ring */
	for (x = -i; x <= i && !done; x++) {
	    tx = dock->x_pos + x*ICON_SIZE;
	    y = -i;
	    ty = dock->y_pos + y*ICON_SIZE;
	    if (slot_map[XY2OFS(x,y)]==0
		&& ON_SCREEN(tx, ty, sx, ex, sy, ey)) {
		*x_pos = x;
		*y_pos = y;
		done = 1;
		break;
	    }
	    y = i;
	    ty = dock->y_pos + y*ICON_SIZE;
	    if (slot_map[XY2OFS(x,y)]==0
		&& ON_SCREEN(tx, ty, sx, ex, sy, ey)) {
		*x_pos = x;
		*y_pos = y;
		done = 1;
		break;
	    }
	}
	/* left and right parts of the ring */
	for (y = -i+1; y <= i-1; y++) {
	    ty = dock->y_pos + y*ICON_SIZE;
	    x = -i;
	    tx = dock->x_pos + x*ICON_SIZE;
	    if (slot_map[XY2OFS(x,y)]==0
		&& ON_SCREEN(tx, ty, sx, ex, sy, ey)) {
		*x_pos = x;
		*y_pos = y;
		done = 1;
		break;
	    }
	    x = i;
	    tx = dock->x_pos + x*ICON_SIZE;
	    if (slot_map[XY2OFS(x,y)]==0
		&& ON_SCREEN(tx, ty, sx, ex, sy, ey)) {
		*x_pos = x;
		*y_pos = y;
		done = 1;
		break;
	    }
	}
    }
    free(slot_map);
#undef XY2OFS
    return done;
}


static void
moveDock(WDock *dock, int new_x, int new_y)
{
    WAppIcon *btn;
    int i;

    dock->x_pos = new_x;
    dock->y_pos = new_y;
    for (i=0; i<dock->max_icons; i++) {
	btn = dock->icon_array[i];
        if (btn) {
            btn->x_pos = new_x + btn->xindex*ICON_SIZE;
	    btn->y_pos = new_y + btn->yindex*ICON_SIZE;
	    XMoveWindow(dpy, btn->icon->core->window, btn->x_pos, btn->y_pos);
	}
    }
}


static void
swapDock(WDock *dock)
{
   WScreen *scr = dock->screen_ptr;
   WAppIcon *btn;
   int x, i;


    if (dock->on_right_side) {
	x = dock->x_pos = scr->scr_width - ICON_SIZE - DOCK_EXTRA_SPACE;
    } else {
	x = dock->x_pos = DOCK_EXTRA_SPACE;
    }

    for (i=0; i<dock->max_icons; i++) {
	btn = dock->icon_array[i];
	if (btn) {
	    btn->x_pos = x;
	    XMoveWindow(dpy, btn->icon->core->window, btn->x_pos, btn->y_pos);
	}
    }
}


static pid_t
execCommand(WAppIcon *btn, char *command, WSavedState *state)
{
    WScreen *scr = btn->icon->core->screen_ptr;
    pid_t pid;
    char **argv;
    int argc;
    char *cmdline;

    cmdline = ExpandOptions(scr, command);

    if (scr->flags.dnd_data_convertion_status || !cmdline) {
	if (cmdline)
	    free(cmdline);
        if (state)
            free(state);
        return 0;
    }

    ParseCommand(cmdline, &argv, &argc);

    if (argv==NULL) {
        if (cmdline)
            free(cmdline);
        if (state)
            free(state);
	return 0;
    }

    if ((pid=fork())==0) {
	char **args;
	int i;
	
	SetupEnvironment(scr);

	CloseDescriptors();

#ifdef HAVE_SETPGID
        setpgid(0, 0);
#endif
	
	args = malloc(sizeof(char*)*(argc+1));
	if (!args)
	    exit(111);
	for (i=0; i<argc; i++) {
	    args[i] = argv[i];
	}
	args[argc] = NULL;
	execvp(argv[0], args);
	exit(111);
    }
    while (argc > 0)
        free(argv[--argc]);
    free(argv);

    if (pid > 0) {
        if (!state) {
            state = wmalloc(sizeof(WSavedState));
            memset(state, 0, sizeof(WSavedState));
            state->hidden = -1;
            state->miniaturized = -1;
            state->shaded = -1;
            if (btn->dock == scr->dock)
                state->workspace = -1;
            else
                state->workspace = scr->current_workspace;
        }
        wWindowAddSavedState(btn->wm_instance, btn->wm_class, cmdline, pid,
			     state);
        wAddDeathHandler(pid, (WDeathHandler*)trackDeadProcess,
                         btn->dock);
    } else if (state) {
        free(state);
    }
    free(cmdline);
    return pid;
}


void
wDockHideIcons(WDock *dock)
{
    int i;
    WAppIcon *btn;

    if (dock==NULL)
        return;

    btn = dock->icon_array[0];

    for (i=1; i<dock->max_icons; i++) {
        if (dock->icon_array[i])
            XUnmapWindow(dpy, dock->icon_array[i]->icon->core->window);
    }
    dock->mapped = 0;

    dockIconPaint(btn);
}


void
wDockShowIcons(WDock *dock)
{
    int i, newlevel;
    WAppIcon *btn;

    if (dock==NULL)
        return;

    btn = dock->icon_array[0];
    moveDock(dock, btn->x_pos, btn->y_pos);

    newlevel = dock->lowered ? WMNormalLevel : WMDockLevel;
    ChangeStackingLevel(btn->icon->core, newlevel);

    for (i=1; i<dock->max_icons; i++) {
        if (dock->icon_array[i]) {
            MoveInStackListAbove(dock->icon_array[i]->icon->core,
                                 btn->icon->core);
            break;
        }
    }

    if (!dock->collapsed) {
        for (i=1; i<dock->max_icons; i++) {
            if (dock->icon_array[i]) {
                XMapWindow(dpy, dock->icon_array[i]->icon->core->window);
            }
        }
    }
    dock->mapped = 1;

    dockIconPaint(btn);
}


void
wDockLower(WDock *dock)
{
    int i;

    for (i=0; i<dock->max_icons; i++) {
        if (dock->icon_array[i])
            wLowerFrame(dock->icon_array[i]->icon->core);
    }
}


void
wDockRaise(WDock *dock)
{
    int i;

    for (i=dock->max_icons-1; i>=0; i--) {
        if (dock->icon_array[i])
            wRaiseFrame(dock->icon_array[i]->icon->core);
    }
}


void
wDockRaiseLower(WDock *dock)
{
    if (!dock->icon_array[0]->icon->core->stacking->above
        ||(dock->icon_array[0]->icon->core->stacking->window_level
           !=dock->icon_array[0]->icon->core->stacking->above->stacking->window_level))
        wDockLower(dock);
    else
        wDockRaise(dock);
}


void
wDockFinishLaunch(WDock *dock, WAppIcon *icon)
{
    icon->launching = 0;
    icon->relaunching = 0;
    dockIconPaint(icon);
}


WAppIcon*
wDockFindIconFor(WDock *dock, Window window)
{
    WAppIcon *icon;
    int i;
    
    for (i=0; i<dock->max_icons; i++) {
	icon = dock->icon_array[i];
	if (icon && icon->main_window == window)
	    return icon;
    }
    return NULL;
}


void
wDockTrackWindowLaunch(WDock *dock, Window window)
{
    WAppIcon *icon;
#ifdef REDUCE_APPICONS
    WAppIconAppList *tapplist;
#endif
    char *wm_class, *wm_instance;
    int i;


    if (!PropGetWMClass(window, &wm_class, &wm_instance) ||
	(!wm_class && !wm_instance))
	return;

    for (i=0; i<dock->max_icons; i++) {
	icon = dock->icon_array[i];
	if (!icon)
	    continue;

	/* kluge. If this does not exist, some windows attach themselves
	 * to more than one icon. Find out why */
	if (icon->main_window == window) {
	    break;
	}
	if ((icon->wm_instance || icon->wm_class)
	    && (icon->launching
		|| (dock->screen_ptr->flags.startup && !icon->running))) {

	    if (icon->wm_instance && wm_instance && 
		strcmp(icon->wm_instance, wm_instance)!=0) {
		continue;
	    }
	    if (icon->wm_class && wm_class && 
		strcmp(icon->wm_class, wm_class)!=0) {
		continue;
	    }

	    if (!icon->relaunching) {
		WApplication *wapp;
		
		/* Possibly an application that was docked with dockit,
		 * but the user did not update WMState to indicate that
		 * it was docked by force */
		wapp = wApplicationOf(window);
		if (!wapp) {
		    icon->forced_dock = 1;
		    icon->running = 0;
		}
		if (!icon->forced_dock)
		    icon->main_window = window;
#ifdef REDUCE_APPICONS
		tapplist = wmalloc(sizeof(WAppIconAppList));
		memset(tapplist, 0, sizeof(WAppIconAppList));
		tapplist->next = icon->applist;
		if (icon->applist)
		   icon->applist->prev = tapplist;
		icon->applist = tapplist;
		tapplist->wapp = wApplicationOf(window);
		icon->num_apps++;
#endif
	    }
	    wDockFinishLaunch(dock, icon);
	    break;
	}
    }
    
    if (wm_class)
	XFree(wm_class);
    if (wm_instance)
	XFree(wm_instance);
}



void
wClipUpdateForWorkspaceChange(WScreen *scr, int workspace)
{
    if (!wPreferences.flags.noclip) {
        scr->clip_icon->dock = scr->workspaces[workspace]->clip;
        if (scr->current_workspace != workspace) {
            WDock *old_clip = scr->workspaces[scr->current_workspace]->clip;

            wDockHideIcons(old_clip);
	    if (old_clip->auto_collapse && !old_clip->collapsed)
                old_clip->collapsed = 1;
            wDockShowIcons(scr->workspaces[workspace]->clip);
        }
	if (scr->flags.clip_balloon_mapped)
	    showClipBalloon(scr->clip_icon->dock, workspace);
    }
}



static void
trackDeadProcess(pid_t pid, unsigned char status, WDock *dock)
{
    WAppIcon *icon;
    int i;
    
    for (i=0; i<dock->max_icons; i++) {
	icon = dock->icon_array[i];
	if (!icon)
	    continue;

	if (icon->launching && icon->pid == pid) {
	    if (!icon->relaunching) {
		icon->running = 0;
		icon->main_window = None;
	    }
	    wDockFinishLaunch(dock, icon);
	    icon->pid = 0;
	    if (status==111) {
		char msg[PATH_MAX];
#ifdef OFFIX_DND
		sprintf(msg, _("Could not execute command \"%s\""),
			icon->drop_launch && icon->dnd_command 
			? icon->dnd_command : icon->command);
#else
		sprintf(msg, _("Could not execute command \"%s\""),
			icon->command);
#endif
		wMessageDialog(dock->screen_ptr, _("Error"), msg, 
			       _("OK"), NULL, NULL);
	    }
	    break;
	}
    }
}


static void
toggleLowered(WDock *dock)
{
    WAppIcon *tmp;
    int newlevel, i;

    /* lower/raise Dock */
    if (!dock->lowered) {
	newlevel = WMNormalLevel;
	dock->lowered = 1;
    } else {
	newlevel = WMDockLevel;
	dock->lowered = 0;
    }
    
    for (i=0; i<dock->max_icons; i++) {
	tmp = dock->icon_array[i];
	if (!tmp)
	    continue;
	
	ChangeStackingLevel(tmp->icon->core, newlevel);
	if (dock->lowered)
	    wLowerFrame(tmp->icon->core);
    }
}


static void
toggleCollapsed(WDock *dock)
{
    if (dock->collapsed) {
        dock->collapsed = 0;
        wDockShowIcons(dock);
    }
    else {
        dock->collapsed = 1;
        wDockHideIcons(dock);
    }
}


static void
toggleAutoCollapse(WDock *dock)
{
    dock->auto_collapse = !dock->auto_collapse;
    if (dock->auto_collapse_magic) {
        WMDeleteTimerHandler(dock->auto_collapse_magic);
        dock->auto_collapse_magic = NULL;
    }
}


static void
openDockMenu(WDock *dock, WAppIcon *aicon, XEvent *event)
{
    WScreen *scr = dock->screen_ptr;
    WObjDescriptor *desc;
    WMenuEntry *entry;
    int index = 0;
    int x_pos;
    int appIsRunning = aicon->running && aicon->icon && aicon->icon->owner;
    
    if (dock->type == WM_DOCK) {
	/* keep on top */
	entry = dock->menu->entries[index];
	entry->flags.indicator_on = !dock->lowered;
	entry->clientdata = dock;
    } else {
	/* clip options */
	if (scr->clip_options)
	    updateClipOptionsMenu(scr->clip_options, dock);

	/* Rename Workspace */
	entry = dock->menu->entries[++index];
	entry->clientdata = dock;

	/* select icon */
	entry = dock->menu->entries[++index];
	entry->clientdata = aicon;
	wMenuSetEnabled(dock->menu, index, aicon!=scr->clip_icon);
	
	/* (un)select all icons */
	entry = dock->menu->entries[++index];
	entry->clientdata = aicon;
	wMenuSetEnabled(dock->menu, index, dock->icon_count > 1);
	
	/* keep icon(s) */
	entry = dock->menu->entries[++index];
	entry->clientdata = aicon;
	wMenuSetEnabled(dock->menu, index, dock->icon_count > 1);
	
	/* this is the workspace submenu part */
	if (scr->clip_submenu)
	    updateWorkspaceMenu(scr->clip_submenu, aicon);
	index++;
		
	/* remove icon(s) */
	entry = dock->menu->entries[++index];
	entry->clientdata = aicon;
	wMenuSetEnabled(dock->menu, index, dock->icon_count > 1);

	/* attract icon(s) */
	entry = dock->menu->entries[++index];
	entry->clientdata = aicon;
    }
    
    /* launch */
    entry = dock->menu->entries[++index];
    entry->clientdata = aicon;
    wMenuSetEnabled(dock->menu, index, aicon->command!=NULL);
           
    /* unhide here */
    entry = dock->menu->entries[++index];
    entry->clientdata = aicon;
    wMenuSetEnabled(dock->menu, index, appIsRunning);
   
    /* hide */
    entry = dock->menu->entries[++index];
    entry->clientdata = aicon;
    wMenuSetEnabled(dock->menu, index, appIsRunning);

    /* settings */
    entry = dock->menu->entries[++index];
    entry->clientdata = aicon; 
    wMenuSetEnabled(dock->menu, index, !aicon->editing
		    && !wPreferences.flags.noupdates);

    /* kill */
    entry = dock->menu->entries[++index];
    entry->clientdata = aicon;
    wMenuSetEnabled(dock->menu, index, appIsRunning);
    
    if (!dock->menu->flags.realized)
	wMenuRealize(dock->menu);
    
    if (dock->type == WM_CLIP) {
	x_pos = event->xbutton.x_root+2;
    } else {
	x_pos = dock->on_right_side ?
	    scr->scr_width - dock->menu->frame->core->width - 2 : 0;
    }
    
    wMenuMapAt(dock->menu, x_pos, event->xbutton.y_root+2, False);
    
    /* allow drag select */
    event->xany.send_event = True;
    desc = &dock->menu->menu->descriptor;
    (*desc->handle_mousedown)(desc, event);
}


static void
openClipWorkspaceMenu(WScreen *scr, int x, int y)
{
    if (!scr->clip_ws_menu) {
	scr->clip_ws_menu = wWorkspaceMenuMake(scr, False);
    }
    wWorkspaceMenuUpdate(scr, scr->clip_ws_menu);
    wMenuMapAt(scr->clip_ws_menu, x, y, False);
}


/******************************************************************/
static void 
iconDblClick(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *btn = desc->parent;
    WDock *dock = btn->dock;
    WApplication *wapp = NULL;
    int unhideHere = 0;

#ifdef REDUCE_APPICONS
    if ((btn->icon->owner && !(event->xbutton.state & ControlMask)) || 
	((btn->icon->owner == NULL) && (btn->applist != NULL))) {
	    if (btn->icon->owner == NULL)
	    	btn->icon->owner = btn->applist->wapp->main_window_desc;
#ifdef I_HATE_THIS
    }
#endif
#else
    if (btn->icon->owner && !(event->xbutton.state & ControlMask)) {
#endif
	wapp = wApplicationOf(btn->icon->owner->main_window);

	assert(wapp!=NULL);

	unhideHere = (event->xbutton.state & ShiftMask);

        /* go to the last workspace that the user worked on the app */
	if (!unhideHere) {
	    wWorkspaceChange(dock->screen_ptr, wapp->last_workspace);
	}

	wUnhideApplication(wapp, event->xbutton.button==Button2,
			   unhideHere);
	
	if (event->xbutton.state & MOD_MASK) {
	    wHideOtherApplications(btn->icon->owner);
	}
    } else {
	if (event->xbutton.button==Button1) {
	
	    if (event->xbutton.state & MOD_MASK) {
                /* raise/lower dock */
                toggleLowered(dock);
            } else if (btn == dock->screen_ptr->clip_icon) {
		if (getClipButton(event->xbutton.x, event->xbutton.y)==CLIP_IDLE)
		    toggleCollapsed(dock);
		else
		    handleClipChangeWorkspace(dock->screen_ptr, event);
	    } else if (btn->command) {
		if (!btn->launching &&
		    (!btn->running || (event->xbutton.state & ControlMask))) {
		    launchDockedApplication(btn);
                }
            }
        }
    }
}


static void
handleDockMove(WDock *dock, WAppIcon *aicon, XEvent *event)
{
    WScreen *scr = dock->screen_ptr;
    int ofs_x=event->xbutton.x, ofs_y=event->xbutton.y;
    int x, y;
    XEvent ev;
    int grabbed = 0, swapped = 0, done;
    Pixmap ghost = None;
    int superfluous = wPreferences.superfluous; /* we catch it to avoid problems */

#ifdef DEBUG
    puts("moving dock");
#endif
    if (XGrabPointer(dpy, aicon->icon->core->window, True, ButtonMotionMask
		     |ButtonReleaseMask|ButtonPressMask, GrabModeAsync,
		     GrabModeAsync, None, None, CurrentTime) !=GrabSuccess) {
	wwarning("pointer grab failed for dock move");
    }
    y = 0;
    for (x=0; x<dock->max_icons; x++) {
        if (dock->icon_array[x]!=NULL &&
            dock->icon_array[x]->yindex > y)
            y = dock->icon_array[x]->yindex;
    }
    y++;
    XResizeWindow(dpy, scr->dock_shadow, ICON_SIZE, ICON_SIZE*y);
    
    done = 0;
    while (!done) {
	WMMaskEvent(dpy, PointerMotionMask|ButtonReleaseMask|ButtonPressMask
		    |ButtonMotionMask|ExposureMask, &ev);
	switch (ev.type) {
	 case Expose:
	    WMHandleEvent(&ev);
	    break;

	 case MotionNotify:
	    if (!grabbed) {
		if (abs(ofs_x-ev.xmotion.x)>=MOVE_THRESHOLD
		    || abs(ofs_y-ev.xmotion.y)>=MOVE_THRESHOLD) {
		    XChangeActivePointerGrab(dpy, ButtonMotionMask
					    |ButtonReleaseMask|ButtonPressMask,
					     wCursor[WCUR_MOVE], CurrentTime);
		    grabbed=1;
		}
		break;
	    }
            if (dock->type == WM_CLIP) {
                if (ev.xmotion.x_root - ofs_x < 0) {
                    x = 0;
                } else if (ev.xmotion.x_root - ofs_x + ICON_SIZE >
                           scr->scr_width) {
                    x = scr->scr_width - ICON_SIZE;
                } else {
                    x = ev.xmotion.x_root - ofs_x;
                }
                if (ev.xmotion.y_root - ofs_y < 0) {
                    y = 0;
                } else if (ev.xmotion.y_root - ofs_y + ICON_SIZE >
                           scr->scr_height) {
                    y = scr->scr_height - ICON_SIZE;
                } else {
                    y = ev.xmotion.y_root - ofs_y;
                }
                moveDock(dock, x, y);
            }
            else {
            /* move vertically if pointer is inside the dock*/
	    if ((dock->on_right_side &&
		 ev.xmotion.x_root >= dock->x_pos - ICON_SIZE)
		|| (!dock->on_right_side &&
		    ev.xmotion.x_root <= dock->x_pos + ICON_SIZE*2)) {
		
		if (ev.xmotion.y_root - ofs_y < 0) {
		    y = 0;
		} else if (ev.xmotion.y_root - ofs_y + ICON_SIZE >
			   scr->scr_height) {
		    y = scr->scr_height - ICON_SIZE;
		} else {
		    y = ev.xmotion.y_root - ofs_y;
		}
                moveDock(dock, dock->x_pos, y);
	    }
	    /* move horizontally to change sides */
	    x = ev.xmotion.x_root - ofs_x;
	    if (!dock->on_right_side) {
		
		/* is on left */
		
		if (ev.xmotion.x_root > dock->x_pos + ICON_SIZE*2) {
		    XMoveWindow(dpy, scr->dock_shadow, scr->scr_width-ICON_SIZE
				      -DOCK_EXTRA_SPACE, dock->y_pos);
                    if (superfluous) {
		    if (ghost==None) {
			ghost = MakeGhostDock(dock, dock->x_pos,
					      scr->scr_width-ICON_SIZE
                                                  -DOCK_EXTRA_SPACE,
                                                  dock->y_pos);
			XSetWindowBackgroundPixmap(dpy, scr->dock_shadow,
						   ghost);
			XClearWindow(dpy, scr->dock_shadow);
		    }
                    }
		    XMapRaised(dpy, scr->dock_shadow);
		    swapped = 1;
		} else {
                    if (superfluous && ghost!=None) {
			XFreePixmap(dpy, ghost);
			ghost = None;
		    }
		    XUnmapWindow(dpy, scr->dock_shadow);
		    swapped = 0;
		}
	    } else {
		/* is on right */
		if (ev.xmotion.x_root < dock->x_pos - ICON_SIZE) {
		    XMoveWindow(dpy, scr->dock_shadow, 
				DOCK_EXTRA_SPACE, dock->y_pos);
                    if (superfluous) {
			if (ghost==None) {
			    ghost = MakeGhostDock(dock, dock->x_pos,
						  DOCK_EXTRA_SPACE, dock->y_pos);
			    XSetWindowBackgroundPixmap(dpy, scr->dock_shadow,
						       ghost);
			    XClearWindow(dpy, scr->dock_shadow);
			}
                    }
		    XMapRaised(dpy, scr->dock_shadow);
		    swapped = -1;
		} else {
		    XUnmapWindow(dpy, scr->dock_shadow);
		    swapped = 0;
		    if (superfluous && ghost!=None) {
			XFreePixmap(dpy, ghost);
			ghost = None;
		    }
		}
            }
            }
	    break;

	 case ButtonPress:
	    break;
	    
	 case ButtonRelease:
	    if (ev.xbutton.button != event->xbutton.button)
		break;
	    XUngrabPointer(dpy, CurrentTime);
	    XUnmapWindow(dpy, scr->dock_shadow);
	    XResizeWindow(dpy, scr->dock_shadow, ICON_SIZE, ICON_SIZE);
            if (dock->type == WM_DOCK) {
                if (swapped!=0) {
                    if (swapped>0)
                        dock->on_right_side = 1;
                    else
                        dock->on_right_side = 0;
                    swapDock(dock);
                    wArrangeIcons(scr, False);
                }
            }
	    done = 1;
	    break;
	}
    }
    if (superfluous) {
	if (ghost!=None)
	    XFreePixmap(dpy, ghost);
	XSetWindowBackground(dpy, scr->dock_shadow, scr->white_pixel);
    }
#ifdef DEBUG
    puts("End dock move");
#endif
}



static void
handleIconMove(WDock *dock, WAppIcon *aicon, XEvent *event)
{
    WScreen *scr = dock->screen_ptr;
    Window wins[2];
    WIcon *icon = aicon->icon;
    WDock *dock2 = NULL, *last_dock = dock;
    int ondock, grabbed = 0, change_dock = 0, collapsed = 0;
    XEvent ev;
    int x = aicon->x_pos, y = aicon->y_pos;
    int ofs_x = event->xbutton.x, ofs_y = event->xbutton.y;
    int shad_x = x, shad_y = y;
    int ix = aicon->xindex, iy = aicon->yindex;
    int tmp;
    Pixmap ghost = None;
    Bool docked;
    int superfluous = wPreferences.superfluous; /* we catch it to avoid problems */

    if (wPreferences.flags.noupdates)
	return;
    
    if (XGrabPointer(dpy, icon->core->window, True, ButtonMotionMask
		     |ButtonReleaseMask|ButtonPressMask, GrabModeAsync,
		     GrabModeAsync, None, None, CurrentTime) !=GrabSuccess) {
#ifdef DEBUG0
	wwarning("pointer grab failed for icon move");
#endif
    }

    wRaiseFrame(icon->core);

    if (dock == scr->dock && !wPreferences.flags.noclip)
        dock2 = scr->workspaces[scr->current_workspace]->clip;
    else if (dock != scr->dock && !wPreferences.flags.nodock)
        dock2 = scr->dock;

    wins[0] = icon->core->window;
    wins[1] = scr->dock_shadow;
    XRestackWindows(dpy, wins, 2);
    XMoveResizeWindow(dpy, scr->dock_shadow, aicon->x_pos, aicon->y_pos,
		      ICON_SIZE, ICON_SIZE);
    if (superfluous) {
	if (icon->pixmap!=None)
	    ghost = MakeGhostIcon(scr, icon->pixmap);
	else
	    ghost = MakeGhostIcon(scr, icon->core->window);
	
	XSetWindowBackgroundPixmap(dpy, scr->dock_shadow, ghost);
	XClearWindow(dpy, scr->dock_shadow);
    }
    XMapWindow(dpy, scr->dock_shadow);
    
    ondock = 1;


    while(1) {
	XMaskEvent(dpy, PointerMotionMask|ButtonReleaseMask|ButtonPressMask
		   |ButtonMotionMask|ExposureMask, &ev);
	switch (ev.type) {
	 case Expose:
	    WMHandleEvent(&ev);
	    break;

	 case MotionNotify:
	    if (!grabbed) {
		if (abs(ofs_x-ev.xmotion.x)>=MOVE_THRESHOLD
		    || abs(ofs_y-ev.xmotion.y)>=MOVE_THRESHOLD) {
		    XChangeActivePointerGrab(dpy, ButtonMotionMask
					    |ButtonReleaseMask|ButtonPressMask,
					     wCursor[WCUR_MOVE], CurrentTime);
		    grabbed=1;
		} else {
		    break;
		}
	    }
		
	    x = ev.xmotion.x_root - ofs_x;
	    y = ev.xmotion.y_root - ofs_y;
            tmp = wDockSnapIcon(dock, aicon, x, y, &ix, &iy, True);
            if (tmp && dock2) {
                change_dock = 0;
                if (last_dock != dock && collapsed) {
                    last_dock->collapsed = 1;
                    wDockHideIcons(last_dock);
                    collapsed = 0;
                }
                if (!collapsed && (collapsed = dock->collapsed)) {
                    dock->collapsed = 0;
                    wDockShowIcons(dock);
                }
                last_dock = dock;
            }
            else if (dock2) {		
                tmp = wDockSnapIcon(dock2, aicon, x, y, &ix, &iy, False);
                if (tmp) {
                    change_dock = 1;
                    if (last_dock != dock2 && collapsed) {
                        last_dock->collapsed = 1;
                        wDockHideIcons(last_dock);
                        collapsed = 0;
                    }
                    if (!collapsed && (collapsed = dock2->collapsed)) {
                        dock2->collapsed = 0;
                        wDockShowIcons(dock2);
                    }
                    last_dock = dock2;
                }
            }
            if (aicon->launching
		|| (aicon->running && !(ev.xmotion.state & MOD_MASK))
		|| (!aicon->running && tmp)) {
                shad_x = last_dock->x_pos + ix*wPreferences.icon_size;
		shad_y = last_dock->y_pos + iy*wPreferences.icon_size;
		
		XMoveWindow(dpy, scr->dock_shadow, shad_x, shad_y);
                
		if (!ondock) {
		    XMapWindow(dpy, scr->dock_shadow);
#if 0
                    if (!collapsed && (collapsed = last_dock->collapsed)) {
                        last_dock->collapsed = 0;
                        wDockShowIcons(last_dock);
                    }
#endif
		}
		ondock = 1;
	    } else {
                if (ondock) {
                    XUnmapWindow(dpy, scr->dock_shadow);
#if 0
                    if (last_dock && collapsed &&
                        aicon->running && (ev.xmotion.state & MOD_MASK)) {
                        last_dock->collapsed = 1;
                        wDockHideIcons(last_dock);
                        collapsed = 0;
                    }
#endif
                }
		ondock = 0;
	    }
	    XMoveWindow(dpy, icon->core->window, x, y);
	    break;

	 case ButtonPress:
	    break;

	 case ButtonRelease:
	    if (ev.xbutton.button != event->xbutton.button)
		break;
	    XUngrabPointer(dpy, CurrentTime);
	    if (ondock) {
		SlideWindow(icon->core->window, x, y, shad_x, shad_y);
                XUnmapWindow(dpy, scr->dock_shadow);
                if (!change_dock)
                    reattachIcon(dock, aicon, ix, iy);
                else {
                    docked = moveIconBetweenDocks(dock, dock2, aicon, ix, iy);
		    if (!docked) {
			/* Slide it back if dock rejected it */
			SlideWindow(icon->core->window, x, y, aicon->x_pos,  
				    aicon->y_pos);
			reattachIcon(dock, aicon, aicon->xindex,aicon->yindex);
		    }
                    if (last_dock->type==WM_CLIP && last_dock->auto_collapse) {
                        collapsed = 0;
                    }
                }
	    } else {
		aicon->x_pos = x;
		aicon->y_pos = y;
                if (superfluous) {
		    if (!aicon->running && !wPreferences.no_animations) {
			/* We need to deselect it, even if is deselected in
			 * wDockDetach(), because else DoKaboom() will fail.
			 */
			if (aicon->icon->selected)
			    wIconSelect(aicon->icon);
			DoKaboom(scr,aicon->icon->core->window, x, y);
		    }
                }
		wDockDetach(dock, aicon);
            }
            if (collapsed) {
                last_dock->collapsed = 1;
                wDockHideIcons(last_dock);
                collapsed = 0;
            }
            if (superfluous) {
		if (ghost!=None)
		    XFreePixmap(dpy, ghost);
		XSetWindowBackground(dpy, scr->dock_shadow, scr->white_pixel);
            }
#ifdef DEBUG
	    puts("End icon move");
#endif
	    return;
	}
    }
}


static int
getClipButton(int px, int py)
{
    int pt = (CLIP_BUTTON_SIZE+2)*ICON_SIZE/64;

    if (px < 0 || py < 0 || px >= ICON_SIZE || py >= ICON_SIZE)
	return CLIP_IDLE;
    
    if (py <= pt-((int)ICON_SIZE-1-px))
	return CLIP_FORWARD;
    else if (px <= pt-((int)ICON_SIZE-1-py))
	return CLIP_REWIND;

    return CLIP_IDLE;
}


static void
handleClipChangeWorkspace(WScreen *scr, XEvent *event)
{
    XEvent ev;
    int done, direction, new_ws;
    int new_dir;
    WDock *clip = scr->clip_icon->dock;

    direction = getClipButton(event->xbutton.x, event->xbutton.y);

    clip->lclip_button_pushed = direction==CLIP_REWIND;
    clip->rclip_button_pushed = direction==CLIP_FORWARD;

    wClipIconPaint(scr->clip_icon);
    done = 0;
    while(!done) {
	WMMaskEvent(dpy, ExposureMask|ButtonMotionMask|ButtonReleaseMask
		    |ButtonPressMask, &ev);
        switch (ev.type) {
        case Expose:
            WMHandleEvent(&ev);
            break;

        case MotionNotify:
            new_dir = getClipButton(ev.xmotion.x, ev.xmotion.y);
            if (new_dir != direction) {
		direction = new_dir;
		clip->lclip_button_pushed = direction==CLIP_REWIND;
		clip->rclip_button_pushed = direction==CLIP_FORWARD;
                wClipIconPaint(scr->clip_icon);
            }
            break;

	case ButtonPress:
	    break;

        case ButtonRelease:
	    if (ev.xbutton.button == event->xbutton.button)
		done = 1;
        }
    }
    
    clip->lclip_button_pushed = 0;
    clip->rclip_button_pushed = 0;

    new_ws = wPreferences.ws_advance || (event->xbutton.state & ControlMask);

    if (direction == CLIP_FORWARD) {
        if (scr->current_workspace < scr->workspace_count-1)
            wWorkspaceChange(scr, scr->current_workspace+1);
        else if (new_ws && scr->current_workspace < MAX_WORKSPACES-1)
            wWorkspaceChange(scr, scr->current_workspace+1);
        else if (wPreferences.ws_cycle)
            wWorkspaceChange(scr, 0);
    }
    else if (direction == CLIP_REWIND) {
        if (scr->current_workspace > 0)
            wWorkspaceChange(scr, scr->current_workspace-1);
        else if (scr->current_workspace==0 && wPreferences.ws_cycle)
            wWorkspaceChange(scr, scr->workspace_count-1);
    }

    wClipIconPaint(scr->clip_icon);
}


static void 
iconMouseDown(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *aicon = desc->parent;
    WDock *dock = aicon->dock;
    WScreen *scr = aicon->icon->core->screen_ptr;

    if (aicon->editing)
	return;

    scr->last_dock = dock;

    if (dock->menu->flags.mapped)
	wMenuUnmap(dock->menu);

    if (IsDoubleClick(scr, event)) {
	/* double-click was not in the main clip icon */
        if (dock->type != WM_CLIP || aicon->xindex!=0 || aicon->yindex!=0
	    || getClipButton(event->xbutton.x, event->xbutton.y)==CLIP_IDLE) {
            iconDblClick(desc, event);
            return;
        }
    }
    
    if (dock->type == WM_CLIP && scr->flags.clip_balloon_mapped) {
	XUnmapWindow(dpy, scr->clip_balloon);
	scr->flags.clip_balloon_mapped = 0;
    }
    
#ifdef DEBUG
    puts("handling dock");
#endif
    if (event->xbutton.button == Button1) {
        if (event->xbutton.state & MOD_MASK)
            wDockLower(dock);
        else
            wDockRaise(dock);

        if ((event->xbutton.state & ShiftMask) && aicon!=scr->clip_icon &&
            dock->type!=WM_DOCK) {
            wIconSelect(aicon->icon);
            return;
        }

        if (aicon->yindex==0 && aicon->xindex==0) {
            if (getClipButton(event->xbutton.x, event->xbutton.y)!=CLIP_IDLE
		&& dock->type==WM_CLIP)
                handleClipChangeWorkspace(scr, event);
            else
                handleDockMove(dock, aicon, event);
        } else
	    handleIconMove(dock, aicon, event);
	
    } else if (event->xbutton.button==Button2 && dock->type==WM_CLIP &&
               aicon->xindex==0 && aicon->yindex==0) {
        openClipWorkspaceMenu(scr, event->xbutton.x_root+2,
			      event->xbutton.y_root+2);
        if (scr->clip_ws_menu) {
            WMenu *menu;
	    menu = scr->clip_ws_menu;
            desc = &menu->menu->descriptor;

            event->xany.send_event = True;
            (*desc->handle_mousedown)(desc, event);
        }
    } else if (event->xbutton.button == Button3) {
	openDockMenu(dock, aicon, event);
    }
}


static void
showClipBalloon(WDock *dock, int workspace)
{
    int w, h;
    int x, y;
    WScreen *scr = dock->screen_ptr;
    char *text;
    Window stack[2];

    scr->flags.clip_balloon_mapped = 1;
    XMapWindow(dpy, scr->clip_balloon);

    text = scr->workspaces[workspace]->name;

    w = wTextWidth(scr->clip_title_font->font, text, strlen(text));

    h = scr->clip_title_font->height;
    XResizeWindow(dpy, scr->clip_balloon, w, h);

    x = dock->x_pos + CLIP_BUTTON_SIZE*ICON_SIZE/64;
    y = dock->y_pos + ICON_SIZE-scr->clip_title_font->height - 3;

    if (x+w > scr->scr_width) {
	x = scr->scr_width - w;
        if (dock->y_pos + ICON_SIZE + h > scr->scr_height)
            y = dock->y_pos - h - 1;
        else
            y = dock->y_pos + ICON_SIZE;
        XRaiseWindow(dpy, scr->clip_balloon);
    } else {
	stack[0] = scr->clip_icon->icon->core->window;
	stack[1] = scr->clip_balloon;
	XRestackWindows(dpy, stack, 2);
    }
    XMoveWindow(dpy, scr->clip_balloon, x, y);
    XSetForeground(dpy, scr->clip_title_gc, 
		   scr->clip_title_pixel[CLIP_NORMAL]);
    XClearWindow(dpy, scr->clip_balloon);
    wDrawString(scr->clip_balloon, scr->clip_title_font, scr->clip_title_gc,
		0, scr->clip_title_font->y, text, strlen(text));
}
    

static void
clipEnterNotify(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *btn = (WAppIcon*)desc->parent;
    WDock *dock;

    assert(event->type==EnterNotify);

    if(desc->parent_type!=WCLASS_DOCK_ICON)
        return;

    dock = btn->dock;
    if (!dock || dock->type!=WM_CLIP)
        return;

    if (btn->xindex == 0 && btn->yindex == 0)
	showClipBalloon(dock, dock->screen_ptr->current_workspace);
    else {
	if (dock->screen_ptr->flags.clip_balloon_mapped) {
	    XUnmapWindow(dpy, dock->screen_ptr->clip_balloon);
	    dock->screen_ptr->flags.clip_balloon_mapped = 0;
	}
    }
    if (!dock->auto_collapse)
	return;

    if (dock->auto_collapse_magic) {
        WMDeleteTimerHandler(dock->auto_collapse_magic);
        dock->auto_collapse_magic = NULL;
    }

    if (dock->collapsed)
        toggleCollapsed(dock);
}


static void
clipLeave(WDock *dock)
{
    if (!dock || dock->type!=WM_CLIP || !dock->auto_collapse)
	return;
    
    if (dock->auto_collapse_magic) {
        WMDeleteTimerHandler(dock->auto_collapse_magic);
        dock->auto_collapse_magic = NULL;
    }
    if (!dock->collapsed) {
        dock->auto_collapse_magic = WMAddTimerHandler(AUTO_COLLAPSE_DELAY,
                                                     clipAutoCollapse,
                                                     (void *)dock);
    }
}


static void
clipLeaveNotify(WObjDescriptor *desc, XEvent *event)
{
    WAppIcon *btn = (WAppIcon*)desc->parent;

    assert(event->type==LeaveNotify);

    if(desc->parent_type!=WCLASS_DOCK_ICON)
        return;

    clipLeave(btn->dock);
}


static void
clipAutoCollapse(void *cdata)
{
    WDock *dock = (WDock *)cdata;

    if (dock->type!=WM_CLIP || !dock->auto_collapse)
        return;

    if (!dock->collapsed && dock->auto_collapse_magic!=NULL) {
        toggleCollapsed(dock);
    }
    dock->auto_collapse_magic = NULL;
}

