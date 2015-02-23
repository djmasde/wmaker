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

#include <X11/Xlib.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "WindowMaker.h"
#include "menu.h"
#include "window.h"
#include "icon.h"
#include "appicon.h"
#include "application.h"
#include "appmenu.h"
#include "properties.h"
#include "funcs.h"
#include "stacking.h"
#include "actions.h"
#include "defaults.h"
#include "workspace.h"

#include "dock.h"

#ifdef WMSOUND
#include "wmsound.h"
#endif


/******** Global variables ********/

extern XContext wAppWinContext;
extern XContext wWinContext;
extern WPreferences wPreferences;

extern WDDomain *WDWindowAttributes;

/******** Local variables ********/


static WWindow*
makeMainWindow(WScreen *scr, Window window)
{
    WWindow *wwin;
    XWindowAttributes attr;

    
    if (!XGetWindowAttributes(dpy, window, &attr)) {
	return NULL;
    }

    wwin = wWindowCreate();
    wwin->screen_ptr = scr;
    wwin->client_win = window;
    wwin->main_window = window;
    wwin->wm_hints = XGetWMHints(dpy, window);
/*    if (!MyXFetchName(dpy, window, &(wwin->frame->title))) {
	wwin->frame->title = NULL;
    }
 */
    PropGetWMClass(window, &wwin->wm_class, &wwin->wm_instance);

    wDefaultFillAttributes(scr, wwin->wm_instance, wwin->wm_class,
			   &wwin->window_flags, True);
    
    XSelectInput(dpy, window, attr.your_event_mask | PropertyChangeMask
		 | StructureNotifyMask );
    return wwin;
}


WApplication*
wApplicationOf(Window window)
{
    WApplication *wapp;

    if (window == None) 
      return NULL;
    if (XFindContext(dpy, window, wAppWinContext, (XPointer*)&wapp)!=XCSUCCESS)
      return NULL;
    return wapp;
}


static WAppIcon*
findDockIconFor(WDock *dock, Window main_window)
{
    WAppIcon *aicon = NULL;

    aicon = wDockFindIconFor(dock, main_window);
    if (!aicon) {
        wDockTrackWindowLaunch(dock, main_window);
        aicon = wDockFindIconFor(dock, main_window);
    }
    return aicon;
}


static void
extractIcon(WWindow *wwin)
{
    int argc;
    char **argv;
    
    if (!XGetCommand(dpy, wwin->client_win, &argv, &argc) || argc < 1)
	return;

    wApplicationExtractDirPackIcon(wwin->screen_ptr,argv[0], 
				   wwin->wm_instance,
				   wwin->wm_class);
    XFreeStringList(argv);
}


static void
saveIconNameFor(char *iconPath, char *wm_instance, char *wm_class)
{
    proplist_t dict = WDWindowAttributes->dictionary;
    proplist_t adict, key, iconk;
    proplist_t val;
    char *tmp;
    int i;
	
    i = 0;
    if (wm_instance)
	i += strlen(wm_instance);
    if (wm_class)
	i += strlen(wm_class);

    tmp = wmalloc(i+8);
    *tmp = 0;
    if (wm_class && wm_instance) {
	sprintf(tmp, "%s.%s", wm_instance, wm_class);
    } else {
	if (wm_instance)
	    strcat(tmp, wm_instance);
	if (wm_class)
	    strcat(tmp, wm_class);
    }
    
    key = PLMakeString(tmp); 
    free(tmp);
    adict = PLGetDictionaryEntry(dict, key);
    
    iconk = PLMakeString("Icon");
    
    if (adict) {
	val = PLGetDictionaryEntry(adict, iconk);
    } else {
	/* no dictionary for app, so create one */
	adict = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
	PLInsertDictionaryEntry(dict, key, adict);
	PLRelease(adict);
	val = NULL;
    }
    if (!val) {
	val = PLMakeString(iconPath);
	PLInsertDictionaryEntry(adict, iconk, val);
	PLRelease(val);
    }
    PLRelease(key);

    if (val && !wPreferences.flags.noupdates)
	PLSave(dict, YES);
}


void
wApplicationExtractDirPackIcon(WScreen *scr, char *path, 
				char *wm_instance, char *wm_class)
{
    char *iconPath=NULL;
    /* Maybe the app is a .app and it has an icon in it, like
     * /usr/local/GNUstep/Apps/WPrefs.app/WPrefs.tiff
     */
    if (strstr(path, ".app")) {
	char *tmp;
	
	tmp = wmalloc(strlen(path)+16);
	
	if (scr->flags.supports_tiff) {
	    strcpy(tmp, path);
	    strcat(tmp, ".tiff");
	    if (access(tmp, R_OK)==0)
		iconPath = tmp;
	}
	if (!path) {
	    strcpy(tmp, path);
	    strcat(tmp, ".xpm");
	    if (access(tmp, R_OK)==0)
		iconPath = tmp;
	}
	if (!iconPath)
	    free(tmp);
    }
    
    if (iconPath) {
	saveIconNameFor(iconPath, wm_instance, wm_class);

	free(iconPath);
    }
}


static Bool
extractClientIcon(WAppIcon *icon)
{
    char *path;

    path = wIconStore(icon->icon);
    if (!path)
	return False;

    saveIconNameFor(path, icon->wm_instance, icon->wm_class);

    free(path);

    return True;
}


WApplication*
wApplicationCreate(WScreen *scr, Window main_window)
{
    WApplication *wapp;
    WWindow *leader;

    if (main_window==None || main_window==scr->root_win) {
#ifdef DEBUG0
	wwarning("trying to create application for %x",(unsigned)main_window);
#endif
	return NULL;
    }

    {
	Window root;
	int foo;
	unsigned int bar;
	/* check if the window is valid */
	if (!XGetGeometry(dpy, main_window, &root, &foo, &foo, &bar, &bar,
			  &bar, &bar)) {
	    return NULL;
	}
    }
    
    wapp = wApplicationOf(main_window);
    if (wapp) {
	wapp->refcount++;
	return wapp;
    }
    
    wapp = wmalloc(sizeof(WApplication));
    memset(wapp, 0, sizeof(WApplication));

    wapp->refcount = 1;
    wapp->last_focused = NULL;
    
    wapp->last_workspace = 0;
      
    wapp->main_window = main_window;
    wapp->main_window_desc = makeMainWindow(scr, main_window);
    if (!wapp->main_window_desc) {
	free(wapp);
	return NULL;
    }
    
    extractIcon(wapp->main_window_desc);

    leader = wWindowFor(main_window);
    if (leader) {
	leader->main_window = main_window;
    }
    wapp->menu = wAppMenuGet(scr, main_window);


    /*
     * Set application wide attributes from the leader.
     */
    wapp->flags.hidden = wapp->main_window_desc->window_flags.start_hidden;
    
    wapp->flags.emulated = wapp->main_window_desc->window_flags.emulate_appicon;

    /* application descriptor */
    XSaveContext(dpy, main_window, wAppWinContext, (XPointer)wapp);

    if (!wapp->main_window_desc->window_flags.no_appicon) {
        wapp->app_icon = NULL;
        if (scr->last_dock)
            wapp->app_icon = findDockIconFor(scr->last_dock, main_window);
        /* check main dock if we did not find it in last dock */
        if (!wapp->app_icon && scr->dock)
	    wapp->app_icon = findDockIconFor(scr->dock, main_window);
        /* finally check clips */
        if (!wapp->app_icon) {
            int i;
            for (i=0; i<scr->workspace_count; i++) {
                WDock *dock = scr->workspaces[i]->clip;
                if (dock)
                    wapp->app_icon = findDockIconFor(dock, main_window);
                if (wapp->app_icon)
                    break;
            }
        }
                    
        if (wapp->app_icon) {
            WWindow *mainw = wapp->main_window_desc;

            wapp->app_icon->running = 1;
            wapp->app_icon->icon->force_paint = 1;
            wapp->app_icon->icon->owner = mainw;
            if (mainw->wm_hints && (mainw->wm_hints->flags&IconWindowHint))
                wapp->app_icon->icon->icon_win = mainw->wm_hints->icon_window;
            wAppIconPaint(wapp->app_icon);
        } else {
	    wapp->app_icon = wAppIconCreate(wapp->main_window_desc);
#ifdef REDUCE_APPICONS
	/* This is so we get the appearance of invoking the app sitting
	 * on the dock. -cls */
	    if (wapp->app_icon) {
		if (wapp->app_icon->docked && wapp->app_icon->num_apps == 1) {
		    wapp->app_icon->launching = 0;
		    wapp->app_icon->running = 1;
		    wapp->app_icon->icon->force_paint = 1;
		    wAppIconPaint(wapp->app_icon);
		}
	    }
#endif
	}
    } else {
	wapp->app_icon = NULL;
    }
    
    if (wapp->app_icon) {
        wapp->app_icon->main_window = main_window;
#ifdef WMSOUND
        wSoundServerGrab(wapp->app_icon->wm_class, main_window);
#endif
    }

#ifndef REDUCE_APPICONS
    if (wapp->app_icon && !wapp->app_icon->docked) {
#else
    if (wapp->app_icon && !wapp->app_icon->docked && wapp->app_icon->num_apps == 1) {
#ifdef THIS_SUCKS
    }
#endif
#endif
        WIcon *icon = wapp->app_icon->icon;
        WDock *clip = scr->workspaces[scr->current_workspace]->clip;
        int x=0, y=0;

        if (clip && clip->attract_icons && wDockFindFreeSlot(clip, &x, &y)) {
            wapp->app_icon->attracted = 1;
            if (!clip->keep_attracted && !wapp->app_icon->icon->shadowed) {
                wapp->app_icon->icon->shadowed = 1;
                wapp->app_icon->icon->force_paint = 1;
                /* We don't do an wAppIconPaint() here because it's in
                 * wDockAttachIcon(). -Dan.
                 */
            }
            wDockAttachIcon(clip, wapp->app_icon, x, y);
        } else {
            PlaceIcon(scr, &x, &y);
	    wAppIconMove(wapp->app_icon, x, y);
	    wLowerFrame(icon->core);
        }
        if (!clip || !wapp->app_icon->attracted || !clip->collapsed)
	    XMapWindow(dpy, icon->core->window);
    }

    if (wPreferences.auto_arrange_icons && wapp->app_icon && !wapp->app_icon->attracted) {
	wArrangeIcons(scr, True);
    }

    if (wapp->app_icon) {
	char *tmp;

	/* if the displayed icon was supplied by the client, save the icon */
	tmp = wDefaultGetIconFile(scr, wapp->app_icon->wm_instance,
				  wapp->app_icon->wm_class, True);
	if (!tmp)
	    extractClientIcon(wapp->app_icon);
    }
    
    wapp->prev = NULL; 
    wapp->next = scr->wapp_list;
    if (scr->wapp_list)
	scr->wapp_list->prev = wapp;
    scr->wapp_list = wapp;

#ifdef WMSOUND
    wSoundPlay(WMSOUND_APPSTART);
#endif

#ifdef DEBUG
    printf("Created application for %x\n", (unsigned)main_window);
#endif
    return wapp;
}


void
wApplicationDestroy(WApplication *wapp)
{
    Window main_window;
    WWindow *wwin;
    WScreen *scr;
#ifdef REDUCE_APPICONS
    unsigned int napps;
#endif

    if (!wapp)
      return;
    
    wapp->refcount--;
    if (wapp->refcount>0)
      return;


    scr = wapp->main_window_desc->screen_ptr;
    main_window = wapp->main_window;
#ifdef REDUCE_APPICONS
    napps = wAppIconReduceAppCount(wapp);
#endif

    if (wapp == scr->wapp_list) {
        if (wapp->next)
            wapp->next->prev = NULL;
        scr->wapp_list = wapp->next;
    } else {
        if (wapp->next)
            wapp->next->prev = wapp->prev;
        if (wapp->prev)
            wapp->prev->next = wapp->next;
    }

    XDeleteContext(dpy, wapp->main_window, wAppWinContext);
    wAppMenuDestroy(wapp->menu);
    if (wapp->app_icon) {
        if (wapp->app_icon->docked
            && (!wapp->app_icon->attracted || 
		wapp->app_icon->dock->keep_attracted)) {
#ifdef REDUCE_APPICONS
	    if (napps == 0) {
#endif
		wapp->app_icon->running = 0;
		/* since we keep it, we don't care if it was attracted or not */
		wapp->app_icon->attracted = 0;
		wapp->app_icon->icon->shadowed = 0;
		wapp->app_icon->main_window = None;
		wapp->app_icon->pid = 0;
		wapp->app_icon->icon->owner = NULL;
		wapp->app_icon->icon->icon_win = None;
		wapp->app_icon->icon->force_paint = 1;
		wAppIconPaint(wapp->app_icon);
#ifdef REDUCE_APPICONS
	    }
#endif
        } else if (wapp->app_icon->docked) {
#ifdef REDUCE_APPICONS
	    if (napps == 0)  {
#endif
		wapp->app_icon->running = 0;
		wDockDetach(wapp->app_icon->dock, wapp->app_icon);
#ifdef REDUCE_APPICONS
	    }
#endif 
        } else {
#ifdef REDUCE_APPICONS
    	    if (napps == 0) {
#endif 
		wAppIconDestroy(wapp->app_icon);
#ifdef REDUCE_APPICONS
    	    }
#endif
	}
    }
    wwin = wWindowFor(wapp->main_window_desc->client_win);

    wWindowDestroy(wapp->main_window_desc);
    if (wwin) {
    	/* undelete client window context that was deleted in
     	 * wWindowDestroy */
    	XSaveContext(dpy, wwin->client_win, wWinContext, 
		     (XPointer)&wwin->client_descriptor);
    }
    free(wapp);
    
#ifdef DEBUG
    printf("Destroyed application for %x\n", (unsigned)main_window);
#endif
    if (wPreferences.auto_arrange_icons) {
        wArrangeIcons(scr, True);
    }

#ifdef WMSOUND
    wSoundPlay(WMSOUND_APPEXIT);
#endif
}


