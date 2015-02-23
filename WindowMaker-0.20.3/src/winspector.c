/* winspector.c - window attribute inspector
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
#include <stdio.h>
#include <string.h>

#include "WindowMaker.h"
#include "screen.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "workspace.h"
#include "funcs.h"
#include "defaults.h"
#include "dialog.h"
#include "icon.h"
#include "stacking.h"
#include "application.h"
#include "appicon.h"
#include "actions.h"
#include "winspector.h"
#include "dock.h"

#include <proplist.h>

extern WDDomain *WDWindowAttributes;

static InspectorPanel *panelList=NULL;

extern WPreferences wPreferences;

static proplist_t ANoTitlebar = NULL;
static proplist_t ANoResizebar;
static proplist_t ANoMiniaturizeButton;
static proplist_t ANoCloseButton;
static proplist_t ANoHideOthers;
static proplist_t ANoMouseBindings;
static proplist_t ANoKeyBindings;
static proplist_t ANoAppIcon;
static proplist_t AKeepOnTop;
static proplist_t AKeepOnBottom;
static proplist_t AOmnipresent;
static proplist_t ASkipWindowList;
static proplist_t AKeepInsideScreen;
static proplist_t AUnfocusable;
static proplist_t AAlwaysUserIcon;
static proplist_t AStartMiniaturized;
static proplist_t ADontSaveSession;
static proplist_t AEmulateAppIcon;

static proplist_t AStartWorkspace;

static proplist_t AIcon;

/* application wide options */
static proplist_t AStartHidden;


static proplist_t AnyWindow;
static proplist_t EmptyString;
static proplist_t Yes, No;


#define PWIDTH	270
#define PHEIGHT	350


static void applySettings(WMButton *button, InspectorPanel *panel);

static void
make_keys()
{
    if (ANoTitlebar!=NULL)
	return;

    AIcon = PLMakeString("Icon");
    ANoTitlebar = PLMakeString("NoTitlebar");
    ANoResizebar = PLMakeString("NoResizebar");
    ANoMiniaturizeButton = PLMakeString("NoMiniaturizeButton");
    ANoCloseButton = PLMakeString("NoCloseButton");
    ANoHideOthers = PLMakeString("NoHideOthers");
    ANoMouseBindings = PLMakeString("NoMouseBindings");
    ANoKeyBindings = PLMakeString("NoKeyBindings");
    ANoAppIcon = PLMakeString("NoAppIcon");
    AKeepOnTop = PLMakeString("KeepOnTop");
    AKeepOnBottom = PLMakeString("KeepOnBottom");
    AOmnipresent = PLMakeString("Omnipresent");
    ASkipWindowList = PLMakeString("SkipWindowList");
    AKeepInsideScreen = PLMakeString("KeepInsideScreen");
    AUnfocusable = PLMakeString("Unfocusable");
    AAlwaysUserIcon = PLMakeString("AlwaysUserIcon");
    AStartMiniaturized = PLMakeString("StartMiniaturized");
    AStartHidden = PLMakeString("StartHidden");
    ADontSaveSession = PLMakeString("DontSaveSession");
    AEmulateAppIcon = PLMakeString("EmulateAppIcon");

    AStartWorkspace = PLMakeString("StartWorkspace");

    AnyWindow = PLMakeString("*");
    EmptyString = PLMakeString("");
    Yes = PLMakeString("Yes");
    No = PLMakeString("No");
}



static void
freeInspector(InspectorPanel *panel)
{
    panel->destroyed = 1;
    if (panel->choosingIcon)
	return;

    WMDestroyWidget(panel->win);

    XDestroyWindow(dpy, panel->parent);

    free(panel);
}


static void
destroyInspector(WCoreWindow *foo, void *data, XEvent *event)
{
    InspectorPanel *panel;
    InspectorPanel *tmp;

    panel = panelList;
    while (panel->frame!=data)
	panel = panel->nextPtr;
    
    if (panelList == panel) 
	panelList = panel->nextPtr;
    else {
	tmp = panelList;
	while (tmp->nextPtr!=panel) {
	    tmp = tmp->nextPtr;
	}
	tmp->nextPtr = panel->nextPtr;
    }
    panel->inspected->flags.inspector_open = 0;
    panel->inspected->inspector = NULL;

    WMRemoveNotificationObserver(panel);

    XUnmapWindow(dpy, panel->parent);
    XReparentWindow(dpy, panel->parent, panel->frame->screen_ptr->root_win,
		    0, 0);
    wUnmanageWindow(panel->frame, False);

    freeInspector(panel);
}



void
wDestroyInspectorPanels()
{
    InspectorPanel *panel;

    while (panelList != NULL) {
        panel = panelList;
        panelList = panelList->nextPtr;
        WMDestroyWidget(panel->win);
        wUnmanageWindow(panel->frame, False);

        panel->inspected->flags.inspector_open = 0;
        panel->inspected->inspector = NULL;

        free(panel);
    }
}


static void
changePage(WMPopUpButton *bPtr, InspectorPanel *panel)
{
    int page;
    
    page = WMGetPopUpButtonSelectedItem(bPtr);
    
    if (page == 0) {
	WMMapWidget(panel->specFrm);
	WMMapWidget(panel->specLbl);
    } else if (page == 1) {
	WMMapWidget(panel->attrFrm);	
    } else if (page == 2) {	
	WMMapWidget(panel->moreFrm);
    } else if (page == 3) {
	WMMapWidget(panel->iconFrm);
	WMMapWidget(panel->wsFrm);
    } else {
	WMMapWidget(panel->appFrm);
    }
    
    if (page != 0) {
	WMUnmapWidget(panel->specFrm);
	WMUnmapWidget(panel->specLbl);
    }
    if (page != 1)
	WMUnmapWidget(panel->attrFrm);
    if (page != 2)
	WMUnmapWidget(panel->moreFrm);
    if (page != 3) {
	WMUnmapWidget(panel->iconFrm);
	WMUnmapWidget(panel->wsFrm);
    }
    if (page != 4 && panel->appFrm)
	WMUnmapWidget(panel->appFrm);
}


#define USE_TEXT_FIELD          1
#define UPDATE_TEXT_FIELD       2
#define REVERT_TO_DEFAULT       4


static int
showIconFor(WMScreen *scrPtr, InspectorPanel *panel,
            char *wm_instance, char *wm_class, int flags)
{
    WMPixmap *pixmap = (WMPixmap*) NULL;
    char *file=NULL, *path=NULL;
    char *db_icon=NULL;

    if ((flags & USE_TEXT_FIELD) != 0) {
        file = WMGetTextFieldText(panel->fileText);
        if (file && file[0] == 0) {
            free(file);
            file = NULL;
        }
    } else {
        db_icon = wDefaultGetIconFile(panel->inspected->screen_ptr,
                                      wm_instance, wm_class, False);
        if(db_icon != NULL)
            file = wstrdup(db_icon);
    }
    if (db_icon!=NULL && (flags & REVERT_TO_DEFAULT)!=0) {
	if (file)
	    file = wstrdup(db_icon);
        flags |= UPDATE_TEXT_FIELD;
    }

    if ((flags & UPDATE_TEXT_FIELD) != 0) {
        WMSetTextFieldText(panel->fileText, file);
    }

    if (file) {
        path = FindImage(wPreferences.icon_path, file);

        if (!path) {
	    char *buf;
	    
	    buf = wmalloc(strlen(file)+80);
	    sprintf(buf, _("Could not find icon \"%s\" specified for this window"),
		    file);
            wMessageDialog(panel->frame->screen_ptr, _("Error"), buf, 
			   _("OK"), NULL, NULL);
	    free(buf);
	    free(file);
            return -1;
        }

        pixmap = WMCreatePixmapFromFile(scrPtr, path);
        free(path);

        if (!pixmap) {
	    char *buf;
	    
	    buf = wmalloc(strlen(file)+80);
	    sprintf(buf, _("Could not open specified icon \"%s\":%s"),
		    file, RMessageForError(RErrorCode));
            wMessageDialog(panel->frame->screen_ptr, _("Error"), buf,
			   _("OK"), NULL, NULL);
	    free(buf);
	    free(file);
            return -1;
        }
        free(file);
    }

    WMSetLabelImage(panel->iconLbl, pixmap);
    if (pixmap)
        WMReleasePixmap(pixmap);

    return 0;
}

#if 0
static void
updateIcon(WMButton *button, InspectorPanel *panel)
{
    showIconFor(WMWidgetScreen(button), panel, NULL, NULL, USE_TEXT_FIELD);
}
#endif

static int 
getBool(proplist_t value)
{
    char *val;

    if (!PLIsString(value)) {
        return 0;
    }
    if (!(val = PLGetString(value))) {
        return 0;
    }

    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y' || val[0]=='T'
                          || val[0]=='t' || val[0]=='1'))
        || (strcasecmp(val, "YES")==0 || strcasecmp(val, "TRUE")==0)) {

        return 1;
    } else if ((val[1]=='\0'
                && (val[0]=='n' || val[0]=='N' || val[0]=='F'
                    || val[0]=='f' || val[0]=='0'))
               || (strcasecmp(val, "NO")==0 || strcasecmp(val, "FALSE")==0)) {

        return 0;
    } else {
        wwarning(_("can't convert \"%s\" to boolean"), val);
        return 0;
    }
}


#define UPDATE_DEFAULTS    1
#define IS_BOOLEAN         2


/*
 *  Will insert the attribute = value; pair in window's list,
 * if it's different from the defaults.
 *  Defaults means either defaults database, or attributes saved
 * for the default window "*". This is to let one revert options that are
 * global because they were saved for all windows ("*").
 *
 */


static void
insertAttribute(proplist_t dict, proplist_t window, proplist_t attr,
                proplist_t value, int *modified, int flags)
{
    proplist_t def_win, def_value=NULL;
    int update = 0;

    if (!(flags & UPDATE_DEFAULTS) && dict) {
        if ((def_win = PLGetDictionaryEntry(dict, AnyWindow)) != NULL) {
            def_value = PLGetDictionaryEntry(def_win, attr);
        }
    }

    /* If we could not find defaults in database, fall to hardcoded values.
     * Also this is true if we save defaults for all windows
     */
    if (!def_value)
        def_value = ((flags & IS_BOOLEAN) != 0) ? No : EmptyString;

    if ((flags & IS_BOOLEAN))
        update = (getBool(value) != getBool(def_value));
    else {
        update = !PLIsEqual(value, def_value);
    }

    if (update) {
        PLInsertDictionaryEntry(window, attr, value);
        *modified = 1;
    }
}


static void
saveSettings(WMButton *button, InspectorPanel *panel)
{
    WWindow *wwin = panel->inspected;
    WDDomain *db = WDWindowAttributes;
    proplist_t dict = db->dictionary;
    proplist_t winDic, value, key;
    char buffer[256], *icon_file;
    int flags = 0;
    int different = 0;

    /* Save will apply the changes and save them */
    applySettings(panel->applyBtn, panel);
	
    if (WMGetButtonSelected(panel->instRb) != 0)
        key = PLMakeString(wwin->wm_instance);
    else if (WMGetButtonSelected(panel->clsRb) != 0)
        key = PLMakeString(wwin->wm_class);
    else if (WMGetButtonSelected(panel->bothRb) != 0) {
        strcat(strcat(strcpy(buffer, wwin->wm_instance), "."), wwin->wm_class);
        key = PLMakeString(buffer);
    }
    else if (WMGetButtonSelected(panel->defaultRb) != 0) {
        key = PLRetain(AnyWindow);
        flags = UPDATE_DEFAULTS;
    }
    else
        key = NULL;

    if (!key)
        return;

    if (!dict) {
        dict = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
        if (dict) {
            db->dictionary = dict;
            value = PLMakeString(db->path);
            PLSetFilename(dict, value);
            PLRelease(value);
        }
        else {
            PLRelease(key);
            return;
        }
    }

    if (showIconFor(WMWidgetScreen(button), panel, NULL, NULL,
		    USE_TEXT_FIELD) < 0)
        return;

    PLSetStringCmpHook(NULL);

    winDic = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    /* Update icon for window */
    icon_file = WMGetTextFieldText(panel->fileText);
    if (icon_file) {
        if (icon_file[0] != 0) {
            value = PLMakeString(icon_file);
            insertAttribute(dict, winDic, AIcon, value, &different, flags);
            PLRelease(value);
        }
        free(icon_file);
    }

    if (WMGetButtonSelected(panel->curRb) != 0) {
	value = PLMakeString("");
	insertAttribute(dict, winDic, AStartWorkspace, value, &different, flags);
	PLRelease(value);
    } else if (WMGetButtonSelected(panel->setRb) != 0) {
        char *ws_name = WMGetTextFieldText(panel->wsText);
        if (ws_name) {
            if (ws_name[0] != 0) {
                value = PLMakeString(ws_name);
                insertAttribute(dict, winDic, AStartWorkspace, value, &different, flags);
                PLRelease(value);
            }
            free(ws_name);
        }
    }

    flags |= IS_BOOLEAN;

    value = (WMGetButtonSelected(panel->alwChk)!=0) ? Yes : No;
    insertAttribute(dict, winDic, AAlwaysUserIcon,    value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[0])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoTitlebar,        value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[1])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoResizebar,       value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[2])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoCloseButton,       value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[3])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoMiniaturizeButton, value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[4])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AKeepOnTop,         value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[5])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AKeepOnBottom,         value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[6])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AOmnipresent,       value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[7])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AStartMiniaturized, value, &different, flags);

    value = (WMGetButtonSelected(panel->attrChk[8])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ASkipWindowList,    value, &different, flags);


    value = (WMGetButtonSelected(panel->moreChk[0])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoHideOthers,      value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[1])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoKeyBindings,     value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[2])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ANoMouseBindings,   value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[3])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AKeepInsideScreen,  value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[4])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AUnfocusable, value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[5])!=0) ? Yes : No;
    insertAttribute(dict, winDic, ADontSaveSession,   value, &different, flags);

    value = (WMGetButtonSelected(panel->moreChk[6])!=0) ? Yes : No;
    insertAttribute(dict, winDic, AEmulateAppIcon,   value, &different, flags);

    /* application wide settings for when */
    /* the window is the leader, save the attribute with the others */
    if (panel->inspected->main_window == panel->inspected->client_win) {
	
	value = (WMGetButtonSelected(panel->appChk[0])!=0) ? Yes : No;
	insertAttribute(dict, winDic, AStartHidden, value, &different, flags);

	value = (WMGetButtonSelected(panel->appChk[1])!=0) ? Yes : No;
	insertAttribute(dict, winDic, ANoAppIcon, value, &different, flags);
    } 

    PLRemoveDictionaryEntry(dict, key);
    if (different) {
        PLInsertDictionaryEntry(dict, key, winDic);
    }
    PLRelease(key); 
    PLRelease(winDic); 

    different = 0;
    
    /* application wide settings */
    if (panel->inspected->main_window != panel->inspected->client_win
	&& !(flags & UPDATE_DEFAULTS)) {
	WApplication *wapp;
	proplist_t appDic;

	wapp = wApplicationOf(panel->inspected->main_window);
	if (wapp) {
	    char *iconFile;
	    
	    appDic = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

	    assert(wapp->main_window_desc->wm_instance!=NULL);
	    assert(wapp->main_window_desc->wm_class!=NULL);
		    
	    strcat(strcpy(buffer, wapp->main_window_desc->wm_instance), ".");
	    strcat(buffer, wwin->wm_class);
	    key = PLMakeString(buffer);
	    
	    iconFile = wDefaultGetIconFile(wwin->screen_ptr, 
					   wapp->main_window_desc->wm_instance,
					   wapp->main_window_desc->wm_class,
					   False);

	    if (iconFile && iconFile[0]!=0) {
		value = PLMakeString(iconFile);
		insertAttribute(dict, appDic, AIcon, value, &different, 
				flags&~IS_BOOLEAN);
		PLRelease(value);
	    }
		
	    value = (WMGetButtonSelected(panel->appChk[0])!=0) ? Yes : No;
	    insertAttribute(dict, appDic, AStartHidden,  value, &different, flags);

	    value = (WMGetButtonSelected(panel->appChk[1])!=0) ? Yes : No;
	    insertAttribute(dict, appDic, ANoAppIcon,  value, &different, flags);

	    PLRemoveDictionaryEntry(dict, key);
	    if (different) {
		PLInsertDictionaryEntry(dict, key, appDic);
	    }
	    PLRelease(key);
	    PLRelease(appDic);
	}
    }

    PLSave(dict, YES);

    /* clean up */
    PLSetStringCmpHook(StringCompareHook);
}


static void
makeAppIconFor(WApplication *wapp)
{
    WScreen *scr = wapp->main_window_desc->screen_ptr;

    if (wapp->app_icon)
        return;

    if (!wapp->main_window_desc->window_flags.no_appicon)
        wapp->app_icon = wAppIconCreate(wapp->main_window_desc);
    else
        wapp->app_icon = NULL;

    if (wapp->app_icon) {
        WIcon *icon = wapp->app_icon->icon;
        WDock *clip = scr->workspaces[scr->current_workspace]->clip;
        int x=0, y=0;

        wapp->app_icon->main_window = wapp->main_window;

        if (clip && clip->attract_icons && wDockFindFreeSlot(clip, &x, &y)) {
            wapp->app_icon->attracted = 1;
	    if (!clip->keep_attracted && !wapp->app_icon->icon->shadowed) {
		wapp->app_icon->icon->shadowed = 1;
		wapp->app_icon->icon->force_paint = 1;
	    }
            wDockAttachIcon(clip, wapp->app_icon, x, y);
        } else {
            PlaceIcon(scr, &x, &y);
	    wAppIconMove(wapp->app_icon, x, y);
        }
        if (!clip || !wapp->app_icon->attracted || !clip->collapsed)
	    XMapWindow(dpy, icon->core->window);

        if (wPreferences.auto_arrange_icons && !wapp->app_icon->attracted)
            wArrangeIcons(wapp->main_window_desc->screen_ptr, True);
    }
}


static void
removeAppIconFor(WApplication *wapp)
{
    if (!wapp->app_icon)
        return;

    if (wapp->app_icon->docked && 
	(!wapp->app_icon->attracted || wapp->app_icon->dock->keep_attracted)) {
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
    } else if (wapp->app_icon->docked) {
        wapp->app_icon->running = 0;
        wDockDetach(wapp->app_icon->dock, wapp->app_icon);
    } else {
        wAppIconDestroy(wapp->app_icon);
    }
    wapp->app_icon = NULL;
    if (wPreferences.auto_arrange_icons)
        wArrangeIcons(wapp->main_window_desc->screen_ptr, True);
}


static void
applySettings(WMButton *button, InspectorPanel *panel)
{
    WWindow *wwin = panel->inspected;
    WWindowAttributes *wflags = &wwin->window_flags;
    WWindowAttributes oldFlags = *wflags;
    WApplication *wapp = wApplicationOf(wwin->main_window);
    int floating, sunken, skip_window_list;

    showIconFor(WMWidgetScreen(button), panel, NULL, NULL, USE_TEXT_FIELD);

    wflags->no_titlebar        =  WMGetButtonSelected(panel->attrChk[0]);
    wflags->no_resizebar       =  WMGetButtonSelected(panel->attrChk[1]);
    wflags->no_close_button    =  WMGetButtonSelected(panel->attrChk[2]);
    wflags->no_miniaturize_button = WMGetButtonSelected(panel->attrChk[3]);
    floating                   =  WMGetButtonSelected(panel->attrChk[4]);
    sunken                     =  WMGetButtonSelected(panel->attrChk[5]);
    wflags->omnipresent        =  WMGetButtonSelected(panel->attrChk[6]);
    wflags->start_miniaturized =  WMGetButtonSelected(panel->attrChk[7]);
    skip_window_list           =  WMGetButtonSelected(panel->attrChk[8]);

    wflags->no_hide_others     =  WMGetButtonSelected(panel->moreChk[0]);
    wflags->no_bind_keys       =  WMGetButtonSelected(panel->moreChk[1]);
    wflags->no_bind_mouse      =  WMGetButtonSelected(panel->moreChk[2]);
    wflags->dont_move_off      =  WMGetButtonSelected(panel->moreChk[3]);
    wflags->no_focusable       =  WMGetButtonSelected(panel->moreChk[4]);
    wflags->dont_save_session  =  WMGetButtonSelected(panel->moreChk[5]);
    wflags->emulate_appicon    =  WMGetButtonSelected(panel->moreChk[6]);
    wflags->always_user_icon   =  WMGetButtonSelected(panel->alwChk);
    
    if (wflags->no_titlebar && wwin->flags.shaded)
        wUnshadeWindow(wwin);
    wflags->no_shadeable = wflags->no_titlebar;

    if (floating) {
	if (!wflags->floating)
	    ChangeStackingLevel(wwin->frame->core, WMFloatingLevel);
    } else if (sunken) {
	if (!wflags->sunken)
	    ChangeStackingLevel(wwin->frame->core, WMSunkenLevel);
    } else {
	if (wflags->floating || wflags->sunken)
	    ChangeStackingLevel(wwin->frame->core, WMNormalLevel);
    }

    wflags->sunken = sunken;
    wflags->floating = floating;

    
    if (wflags->skip_window_list != skip_window_list) {
        int action = ((wflags->skip_window_list = skip_window_list))
            ? ACTION_REMOVE : ACTION_ADD;
        UpdateSwitchMenu(wwin->screen_ptr, wwin, action);
    }
    
    if (wflags->no_bind_keys != oldFlags.no_bind_keys) {
	if (!wflags->no_bind_keys) {
	    XUngrabKey(dpy, AnyKey, AnyModifier, wwin->frame->core->window);
	} else {
	    wWindowSetKeyGrabs(wwin);
	}
    }
    
    if (wflags->no_bind_mouse != oldFlags.no_bind_mouse) {
	wWindowResetMouseGrabs(wwin);
    }

    wwin->frame->flags.need_texture_change = 1;
    wWindowConfigureBorders(wwin);
    wFrameWindowPaint(wwin->frame);

    /*
     * Can't apply emulate_appicon because it will probably cause problems.
     */
    
    if (wapp) {
	/* do application wide stuff */
	wflags = &wapp->main_window_desc->window_flags;

	wflags->start_hidden =  WMGetButtonSelected(panel->appChk[0]);
	wflags->no_appicon  =  WMGetButtonSelected(panel->appChk[1]);
	
        if (wflags->no_appicon)
            removeAppIconFor(wapp);
	else
            makeAppIconFor(wapp);

        if (wapp->app_icon && wapp->main_window == wwin->client_win) {
            char *file = WMGetTextFieldText(panel->fileText);

	    if (file[0] == 0) {
		free(file);
		file = NULL;
	    }
	    wIconChangeImageFile(wapp->app_icon->icon, file);
	    if (file)
		free(file);
	    wAppIconPaint(wapp->app_icon);
        }
    }
}


static void
revertSettings(WMButton *button, InspectorPanel *panel)
{
    WWindow *wwin = panel->inspected;
    WApplication *wapp = wApplicationOf(wwin->main_window);
    int i, n, floating, sunken, skip_window_list;
    char *wm_instance = NULL;
    char *wm_class = NULL;

    if (WMGetButtonSelected(panel->instRb) != 0)
        wm_instance = wwin->wm_instance;
    else if (WMGetButtonSelected(panel->clsRb) != 0)
        wm_class = wwin->wm_class;
    else if (WMGetButtonSelected(panel->bothRb) != 0) {
        wm_instance = wwin->wm_instance;
        wm_class = wwin->wm_class;
    }
    memset(&wwin->window_flags, 0, sizeof(WWindowAttributes));
    wDefaultFillAttributes(wwin->screen_ptr, wm_instance, wm_class,
                           &wwin->window_flags, True);

    wWindowCheckAttributeSanity(wwin, &wwin->window_flags);

    wwin->window_flags.kill_close = (wwin->protocols.DELETE_WINDOW) ? 0 : 1;
    /* transients can't be iconified or maximized */
    if (wwin->window_flags.no_miniaturizable) {
	wwin->window_flags.no_miniaturize_button = 1;
    }
    /* if the window can't be resized, remove the resizebar */
    if (wwin->window_flags.no_resizable) {
	wwin->window_flags.no_resizebar = 1;
    }

    wwin->window_flags.no_shadeable = wwin->window_flags.no_titlebar;

    for (i=0; i < 9; i++) {
	int flag = 0;
	
	switch (i) {
	 case 0:
            flag = wwin->window_flags.no_titlebar;
	    break;
	 case 1:
	    flag = wwin->window_flags.no_resizebar;
	    break;
	 case 2:
	    flag = wwin->window_flags.no_close_button;
	    break;
	 case 3:
	    flag = wwin->window_flags.no_miniaturize_button;
	    break;
	 case 4:
            flag = wwin->window_flags.floating;
	    break;
	 case 5:
            flag = wwin->window_flags.sunken;
	    break;
	 case 6:
	    flag = wwin->window_flags.omnipresent;
	    break;
	 case 7:
	    flag = wwin->window_flags.no_focusable;
	    break;
	 case 8:
	    flag = wwin->window_flags.skip_window_list;
	    break;
	}
        WMSetButtonSelected(panel->attrChk[i], flag);
    }
    for (i=0; i < 7; i++) {
	int flag = 0;
	
	switch (i) {
	 case 0:
	    flag = wwin->window_flags.no_hide_others;
	    break;
	 case 1:
	    flag = wwin->window_flags.no_bind_keys;
	    break;
	 case 2:
	    flag = wwin->window_flags.no_bind_mouse;
	    break;
	 case 3:
	    flag = wwin->window_flags.dont_move_off;
	    break;
	 case 4:
	    flag = wwin->window_flags.start_miniaturized;
	    break;
	 case 5:
	    flag = wwin->window_flags.dont_save_session;
	    break;
	 case 6:
	    flag = wwin->window_flags.emulate_appicon;
	    break;
	}
	WMSetButtonSelected(panel->moreChk[i], flag);
    }
    if (panel->appFrm && wapp) {
	for (i=0; i < 2; i++) {
	    int flag = 0;
	    
	    switch (i) {
	     case 0:
		flag = wapp->main_window_desc->window_flags.start_hidden;
		break;
	     case 1:
		flag = wapp->main_window_desc->window_flags.no_appicon;
		break;
	    }
	    WMSetButtonSelected(panel->appChk[i], flag);
	}
    }
    WMSetButtonSelected(panel->alwChk, wwin->window_flags.always_user_icon);

    showIconFor(WMWidgetScreen(panel->alwChk), panel, wm_instance, wm_class,
		REVERT_TO_DEFAULT);

    n = wDefaultGetStartWorkspace(wwin->screen_ptr, wm_instance, wm_class);
    
    if (n >= 0 && n <= wwin->screen_ptr->workspace_count) {
	WMPerformButtonClick(panel->setRb);
	WMSetTextFieldText(panel->wsText, wwin->screen_ptr->workspaces[n]->name);
    } else {
	WMPerformButtonClick(panel->curRb);
    }
}


static void
chooseIconCallback(WMWidget *self, void *clientData)
{
    char *file;
    InspectorPanel *panel = (InspectorPanel*)clientData;
    int result;

    panel->choosingIcon = 1;
    
    WMSetButtonEnabled(panel->browseIconBtn, False);
    
    result = wIconChooserDialog(panel->frame->screen_ptr, &file);
    
    panel->choosingIcon = 0;

    if (!panel->destroyed) { /* kluge */    
	if (result) {	    
	    WMSetTextFieldText(panel->fileText, file);
	    showIconFor(WMWidgetScreen(self), panel, NULL, NULL,
			USE_TEXT_FIELD);
	    free(file);
	}
	WMSetButtonEnabled(panel->browseIconBtn, True);
    } else {
	freeInspector(panel);
    }
}


static void
textEditedObserver(void *observerData, WMNotification *notification)
{
    InspectorPanel *panel = (InspectorPanel*)observerData;

    if ((int)WMGetNotificationClientData(notification) != WMReturnTextMovement)
	return;

    if (observerData == panel->fileText) {
	showIconFor(WMWidgetScreen(panel->win), panel, NULL, NULL, 
		    USE_TEXT_FIELD);
    /*
     WMPerformButtonClick(panel->updateIconBtn);
     */
    } else
	WMPerformButtonClick(panel->setRb);
}

static InspectorPanel*
createInspectorForWindow(WWindow *wwin)
{
    WScreen *scr = wwin->screen_ptr;
    InspectorPanel *panel;
    Window parent;
    char charbuf[128];
    int i;
    int x, y;
    int btn_width, frame_width;
#ifdef wrong_behaviour
    WMPixmap *pixmap;
#endif
    panel = wmalloc(sizeof(InspectorPanel));
    
    panel->destroyed = 0;

    
    panel->inspected = wwin;

    panel->nextPtr = panelList;
    panelList = panel;
    

    sprintf(charbuf, "Inspecting  %s.%s", 
	    wwin->wm_instance ? wwin->wm_instance : "?",
	    wwin->wm_class ? wwin->wm_class : "?");

    panel->win = WMCreateWindow(scr->wmscreen, "windowInspector");
    WMResizeWidget(panel->win, PWIDTH, PHEIGHT);
    

    /**** create common stuff ****/

    /* command buttons */
    /* (PWIDTH - (left and right margin) - (btn interval)) / 3 */
    btn_width = (PWIDTH - (2 * 15) - (2 * 10)) / 3;
    panel->saveBtn = WMCreateCommandButton(panel->win);
    WMSetButtonAction(panel->saveBtn, (WMAction*)saveSettings, panel);
    WMMoveWidget(panel->saveBtn, 15, 310);
    WMSetButtonText(panel->saveBtn, _("Save"));
    WMResizeWidget(panel->saveBtn, btn_width, 28);
    if (wPreferences.flags.noupdates)
	WMSetButtonEnabled(panel->saveBtn, False);

    panel->applyBtn = WMCreateCommandButton(panel->win);
    WMSetButtonAction(panel->applyBtn, (WMAction*)applySettings, panel);
    WMMoveWidget(panel->applyBtn, btn_width + 10 + 15, 310);
    WMSetButtonText(panel->applyBtn, _("Apply"));
    WMResizeWidget(panel->applyBtn, btn_width, 28);

    panel->revertBtn = WMCreateCommandButton(panel->win);
    WMSetButtonAction(panel->revertBtn, (WMAction*)revertSettings, panel);
    WMMoveWidget(panel->revertBtn, (2 * (btn_width + 10)) + 15, 310);
    WMSetButtonText(panel->revertBtn, _("Revert"));
    WMResizeWidget(panel->revertBtn, btn_width, 28);

    /* page selection popup button */
    panel->pagePopUp = WMCreatePopUpButton(panel->win);
    WMSetPopUpButtonAction(panel->pagePopUp, (WMAction*)changePage, panel);
    WMMoveWidget(panel->pagePopUp, 25, 15);
    WMResizeWidget(panel->pagePopUp, PWIDTH - 50, 20);

    WMAddPopUpButtonItem(panel->pagePopUp, _("Window Specification"));
    WMAddPopUpButtonItem(panel->pagePopUp, _("Window Attributes"));
    WMAddPopUpButtonItem(panel->pagePopUp, _("Advanced Options"));
    WMAddPopUpButtonItem(panel->pagePopUp, _("Icon and Initial Workspace"));
    WMAddPopUpButtonItem(panel->pagePopUp, _("Application Specific"));

    /**** window spec ****/
    frame_width = PWIDTH - (2 * 15);

    panel->specFrm = WMCreateFrame(panel->win);
    WMSetFrameTitle(panel->specFrm, _("Window Specification"));
    WMMoveWidget(panel->specFrm, 15, 65);
    WMResizeWidget(panel->specFrm, frame_width, 105);


    panel->defaultRb = WMCreateRadioButton(panel->specFrm);
    WMMoveWidget(panel->defaultRb, 10, 78);
    WMResizeWidget(panel->defaultRb, frame_width - (2 * 10), 20);
    WMSetButtonText(panel->defaultRb, _("Defaults for all windows"));
    WMSetButtonSelected(panel->defaultRb, False);
    
    
    if (wwin->wm_class && wwin->wm_instance) {
	sprintf(charbuf, "%s.%s", wwin->wm_instance, wwin->wm_class);
	panel->bothRb = WMCreateRadioButton(panel->specFrm);
	WMMoveWidget(panel->bothRb, 10, 18);
	WMResizeWidget(panel->bothRb, frame_width - (2 * 10), 20);
	WMSetButtonText(panel->bothRb, charbuf);
	WMSetButtonSelected(panel->bothRb, True);
	WMGroupButtons(panel->defaultRb, panel->bothRb);
    }

    if (wwin->wm_instance) {
	panel->instRb = WMCreateRadioButton(panel->specFrm);
	WMMoveWidget(panel->instRb, 10, 38);
	WMResizeWidget(panel->instRb, frame_width - (2 * 10), 20);
	WMSetButtonText(panel->instRb, wwin->wm_instance);
	WMSetButtonSelected(panel->instRb, False);
	WMGroupButtons(panel->defaultRb, panel->instRb);
    }

    if (wwin->wm_class) {
	panel->clsRb = WMCreateRadioButton(panel->specFrm);
	WMMoveWidget(panel->clsRb, 10, 58);
	WMResizeWidget(panel->clsRb, frame_width - (2 * 10), 20);
	WMSetButtonText(panel->clsRb, wwin->wm_class);
	WMSetButtonSelected(panel->clsRb, False);
	WMGroupButtons(panel->defaultRb, panel->clsRb);
    }

    panel->specLbl = WMCreateLabel(panel->win);
    WMMoveWidget(panel->specLbl, 15, 170);
    WMResizeWidget(panel->specLbl, frame_width, 100);
    WMSetLabelText(panel->specLbl,
		   _("The configuration will apply to all\n"
		     "windows that have their WM_CLASS property"
		     " set to the above selected\nname, when saved."));
    WMSetLabelTextAlignment(panel->specLbl, WACenter);
    
    /**** attributes ****/
    panel->attrFrm = WMCreateFrame(panel->win);
    WMSetFrameTitle(panel->attrFrm, _("Attributes"));
    WMMoveWidget(panel->attrFrm, 15, 50);
    WMResizeWidget(panel->attrFrm, frame_width, 240);

    for (i=0; i < 9; i++) {
	char *caption = NULL;
	int flag = 0;
	
	switch (i) {
	 case 0:
	    caption = _("Disable titlebar");
            flag = wwin->window_flags.no_titlebar;
	    break;
	 case 1:
	    caption = _("Disable resizebar");
	    flag = wwin->window_flags.no_resizebar;
	    break;
	 case 2:
	    caption = _("Disable close button");
	    flag = wwin->window_flags.no_close_button;
	    break;
	 case 3:
	    caption = _("Disable miniaturize button");
	    flag = wwin->window_flags.no_miniaturize_button;
	    break;
	 case 4:
	    caption = _("Keep on top / floating");
	    flag = wwin->window_flags.floating;
	    break;
	 case 5:
	    caption = _("Keep on bottom / sunken");
	    flag = wwin->window_flags.sunken;
	    break;
	 case 6:
	    caption = _("Omnipresent");
	    flag = wwin->window_flags.omnipresent;
	    break;
	 case 7:
	    caption = _("Start Miniaturized");
	    flag = wwin->window_flags.start_miniaturized;
	    break;
	 case 8:
	    caption = _("Skip window list");
	    flag = wwin->window_flags.skip_window_list;
	    break;
	}
	panel->attrChk[i] = WMCreateSwitchButton(panel->attrFrm);
	WMMoveWidget(panel->attrChk[i], 10, 20*(i+1));
	WMResizeWidget(panel->attrChk[i], frame_width-15, 20);
	WMSetButtonSelected(panel->attrChk[i], flag);
	WMSetButtonText(panel->attrChk[i], caption);
    }


    /**** more attributes ****/
    panel->moreFrm = WMCreateFrame(panel->win);
    WMSetFrameTitle(panel->moreFrm, _("Advanced"));
    WMMoveWidget(panel->moreFrm, 15, 50);
    WMResizeWidget(panel->moreFrm, frame_width, 240);

    for (i=0; i < 7; i++) {
	char *caption = NULL;
	int flag = 0;
	
	switch (i) {
	 case 0:
	    caption = _("Ignore HideOthers");
	    flag = wwin->window_flags.no_hide_others;
	    break;
	 case 1:
	    caption = _("Don't bind keyboard shortcuts");
	    flag = wwin->window_flags.no_bind_keys;
	    break;
	 case 2:
	    caption = _("Don't bind mouse clicks");
	    flag = wwin->window_flags.no_bind_mouse;
	    break;
	 case 3:
	    caption = _("Keep inside screen");
	    flag = wwin->window_flags.dont_move_off;
	    break;
	 case 4:
	    caption = _("Don't let it take focus");
	    flag = wwin->window_flags.no_focusable;
	    break;
	 case 5:
	    caption = _("Don't Save Session");
	    flag = wwin->window_flags.dont_save_session;
	    break;
	 case 6:
	    caption = _("Emulate Application Icon");
	    flag = wwin->window_flags.emulate_appicon;
	    break;
	}
	panel->moreChk[i] = WMCreateSwitchButton(panel->moreFrm);
	WMMoveWidget(panel->moreChk[i], 10, 20*(i+1));
	WMResizeWidget(panel->moreChk[i], frame_width-15, 20);
	WMSetButtonSelected(panel->moreChk[i], flag);
	WMSetButtonText(panel->moreChk[i], caption);
    }

    panel->moreLbl = WMCreateLabel(panel->moreFrm);
    WMResizeWidget(panel->moreLbl, frame_width - (2 * 5), 60);
    WMMoveWidget(panel->moreLbl, 5, 160);
    WMSetLabelText(panel->moreLbl, 
		   _("Enable the \"Don't bind...\" options to allow the "
		     "application to receive all mouse or keyboard events."));

    /* miniwindow/workspace */
    panel->iconFrm = WMCreateFrame(panel->win);
    WMMoveWidget(panel->iconFrm, 15, 50);
    WMResizeWidget(panel->iconFrm,  PWIDTH - (2 * 15), 170);
    WMSetFrameTitle(panel->iconFrm, _("Miniwindow Image"));

    panel->iconLbl = WMCreateLabel(panel->iconFrm);
    WMMoveWidget(panel->iconLbl, PWIDTH - (2 * 15) - 22 - 64, 30);
    WMResizeWidget(panel->iconLbl, 64, 64);
    WMSetLabelRelief(panel->iconLbl, WRRaised);
    WMSetLabelImagePosition(panel->iconLbl, WIPImageOnly);
    
    panel->browseIconBtn = WMCreateCommandButton(panel->iconFrm);
    WMSetButtonAction(panel->browseIconBtn, chooseIconCallback, panel);
    WMMoveWidget(panel->browseIconBtn, 22, 30);
    WMResizeWidget(panel->browseIconBtn, 100, 26);
    WMSetButtonText(panel->browseIconBtn, _("Browse..."));

#if 0
    panel->updateIconBtn = WMCreateCommandButton(panel->iconFrm);
    WMSetButtonAction(panel->updateIconBtn, (WMAction*)updateIcon, panel);
    WMMoveWidget(panel->updateIconBtn, 22, 65);
    WMResizeWidget(panel->updateIconBtn, 100, 26);
    WMSetButtonText(panel->updateIconBtn, _("Update"));
#endif
#ifdef wrong_behaviour
    WMSetButtonImagePosition(panel->updateIconBtn, WIPRight);
    pixmap = WMGetSystemPixmap(scr->wmscreen, WSIReturnArrow);
    WMSetButtonImage(panel->updateIconBtn, pixmap);
    WMReleasePixmap(pixmap);
    pixmap = WMGetSystemPixmap(scr->wmscreen, WSIHighlightedReturnArrow);
    WMSetButtonAltImage(panel->updateIconBtn, pixmap);
    WMReleasePixmap(pixmap);
#endif

    panel->fileLbl = WMCreateLabel(panel->iconFrm);
    WMMoveWidget(panel->fileLbl, 20, 95);
    WMResizeWidget(panel->fileLbl, PWIDTH - (2 * 15) - (2 * 20), 14);
    WMSetLabelText(panel->fileLbl, _("Icon file name:"));

    panel->fileText = WMCreateTextField(panel->iconFrm);
    WMMoveWidget(panel->fileText, 20, 115);
    WMResizeWidget(panel->fileText, PWIDTH - (2 * 15) - (2 * 15), 20);
    WMSetTextFieldText(panel->fileText, NULL);
    WMAddNotificationObserver(textEditedObserver, panel, 
			      WMTextDidEndEditingNotification, 
			      panel->fileText);
    panel->alwChk = WMCreateSwitchButton(panel->iconFrm);
    WMMoveWidget(panel->alwChk, 20, 140);
    WMResizeWidget(panel->alwChk, PWIDTH - (2 * 15) - (2 * 15), 20);
    WMSetButtonText(panel->alwChk, _("Ignore client supplied icon"));
    WMSetButtonSelected(panel->alwChk, wwin->window_flags.always_user_icon);


    panel->wsFrm = WMCreateFrame(panel->win);
    WMMoveWidget(panel->wsFrm, 15, 225);
    WMResizeWidget(panel->wsFrm, PWIDTH - (2 * 15), 70);
    WMSetFrameTitle(panel->wsFrm, _("Initial Workspace"));
    
    panel->curRb = WMCreateRadioButton(panel->wsFrm);
    WMMoveWidget(panel->curRb, 10, 15);
    WMResizeWidget(panel->curRb, frame_width - (2 * 10), 20);
    WMSetButtonText(panel->curRb, _("Nowhere in particular"));

    
    panel->setRb = WMCreateRadioButton(panel->wsFrm);
    WMMoveWidget(panel->setRb, 10, 40);
    WMResizeWidget(panel->setRb, 25, 20);
    WMGroupButtons(panel->curRb, panel->setRb);
    WMSetButtonText(panel->setRb, NULL);

    panel->wsText = WMCreateTextField(panel->wsFrm);
    WMMoveWidget(panel->wsText, 30, 40);
    WMResizeWidget(panel->wsText, PWIDTH - (2 * 15) - 25 - 10 - (2 * 5), 20);
    WMAddNotificationObserver(textEditedObserver, panel, 
			      WMTextDidEndEditingNotification,
			      panel->wsText);
    
    
    i = wDefaultGetStartWorkspace(wwin->screen_ptr, wwin->wm_instance,
                                  wwin->wm_class);
    if (i >= 0 && i <= wwin->screen_ptr->workspace_count) {
	WMSetButtonSelected(panel->curRb, False);
	WMSetButtonSelected(panel->setRb, True);
	WMSetTextFieldText(panel->wsText,
			   wwin->screen_ptr->workspaces[i]->name);
    } else {
	WMSetButtonSelected(panel->curRb, True);
	WMSetButtonSelected(panel->setRb, False);
    }

    /* application wide attributes */
    if (wwin->main_window != None) {
	WApplication *wapp = wApplicationOf(wwin->main_window);
	
	panel->appFrm = WMCreateFrame(panel->win);
	WMSetFrameTitle(panel->appFrm, _("Application Wide"));
	WMMoveWidget(panel->appFrm, 15, 50);
	WMResizeWidget(panel->appFrm, frame_width, 240);
	
	for (i=0; i < 2; i++) {
	    char *caption = NULL;
	    int flag = 0;
	    
	    switch (i) {
	     case 0:
		caption = _("Start Hidden");
		flag = wapp->main_window_desc->window_flags.start_hidden;
		break;
	     case 1:
		caption = _("No application icon");
		flag = wapp->main_window_desc->window_flags.no_appicon;
		break;
	    }
	    panel->appChk[i] = WMCreateSwitchButton(panel->appFrm);
	    WMMoveWidget(panel->appChk[i], 10, 20*(i+1));
	    WMResizeWidget(panel->appChk[i], 205, 20);
	    WMSetButtonSelected(panel->appChk[i], flag);
	    WMSetButtonText(panel->appChk[i], caption);
	}
	
	if (wwin->window_flags.emulate_appicon) {
	    WMSetButtonEnabled(panel->appChk[1], False);
	    WMSetButtonEnabled(panel->moreChk[6], True);
	} else {
	    WMSetButtonEnabled(panel->appChk[1], True);
	    WMSetButtonEnabled(panel->moreChk[6], False);
	}
    } else {
	int tmp;
	
	if (wwin->transient_for!=None
	    && wwin->transient_for!=scr->root_win)
	    tmp = False;
	else
	    tmp = True;
	WMSetButtonEnabled(panel->moreChk[6], tmp);

	WMSetPopUpButtonItemEnabled(panel->pagePopUp, 4, False);
	panel->appFrm = NULL;
    }
   
    /* if the window is a transient, don't let it have a miniaturize
     * button */
    if (wwin->transient_for!=None && wwin->transient_for!=scr->root_win)
	WMSetButtonEnabled(panel->attrChk[3], False);
    else
	WMSetButtonEnabled(panel->attrChk[3], True);


    WMRealizeWidget(panel->win);

    WMMapSubwidgets(panel->win);
    WMMapSubwidgets(panel->specFrm);
    WMMapSubwidgets(panel->attrFrm);
    WMMapSubwidgets(panel->moreFrm);
    WMMapSubwidgets(panel->iconFrm);
    WMMapSubwidgets(panel->wsFrm);
    if (panel->appFrm)
	WMMapSubwidgets(panel->appFrm);

    WMSetPopUpButtonSelectedItem(panel->pagePopUp, 0);
    changePage(panel->pagePopUp, panel);

    
    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, PWIDTH, PHEIGHT, 
				 0, 0, 0);
    XSelectInput(dpy, parent, KeyPressMask|KeyReleaseMask);
    panel->parent = parent;
    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    WMMapWidget(panel->win);
    
    XSetTransientForHint(dpy, parent, wwin->client_win);

    x = wwin->frame_x+wwin->frame->core->width/2;
    y = wwin->frame_y+wwin->frame->top_width*2;
    if (y + PHEIGHT > scr->scr_height)
	y = scr->scr_height - PHEIGHT - 30;
    panel->frame = wManageInternalWindow(scr, parent, wwin->client_win, 
					 charbuf, x, y, PWIDTH, PHEIGHT);
    
    /* kluge to know who should get the key events */
    panel->frame->client_leader = WMWidgetXID(panel->win);
    
    panel->frame->window_flags.no_closable = 0;
    panel->frame->window_flags.no_close_button = 0;
    wWindowUpdateButtonImages(panel->frame);
    wFrameWindowShowButton(panel->frame->frame, WFF_RIGHT_BUTTON);
    panel->frame->frame->on_click_right = destroyInspector;

    wWindowMap(panel->frame);

    showIconFor(WMWidgetScreen(panel->alwChk), panel, wwin->wm_instance, 
		wwin->wm_class, UPDATE_TEXT_FIELD);
    return panel;
}


void
wShowInspectorForWindow(WWindow *wwin)
{
    if (wwin->flags.inspector_open)
      return;

    make_keys();
    wwin->flags.inspector_open = 1;
    wwin->inspector = createInspectorForWindow(wwin);;
}


