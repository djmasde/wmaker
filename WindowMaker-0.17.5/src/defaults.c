/* defaults.c - manage configuration through defaults db
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
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <wraster.h>


#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "texture.h"
#include "screen.h"
#include "resources.h"
#include "defaults.h"
#include "keybind.h"
#include "xmodifier.h"
#include "icon.h"
#include "funcs.h"
#include "actions.h"
#include "dock.h"
#include "workspace.h"


/*
 * Our own proplist reader parser. This one will not accept any
 * syntax errors and is more descriptive in the error messages.
 * It also doesn't seem to crash.
 */
extern proplist_t ReadProplistFromFile(char *file);


/*
extern proplist_t wDomainName;
extern proplist_t wAttributeDomainName;
 */
extern WPreferences wPreferences;

extern WShortKey wKeyBindings[WKBD_LAST];

typedef struct {
    char *key;
    char *default_value;
    void *extra_data;
    void *addr;
    int (*convert)();
    int (*update)();
    proplist_t plkey;
    proplist_t plvalue;
} WDefaultEntry;



/* type converters */
static int getBool();
static int getInt();
static int getCoord();
#if 0
/* this is not used yet */
static int getString();
#endif
static int getPathList();
static int getFocusMode();
static int getPlacement();
static int getGeomDisp();
static int getTexture();
static int getBackground();
static int getJust();
static int getFont();
static int getColor();
static int getKeybind();
static int getModMask();
static int getSpeed();

/* value setting functions */
static int setJustify();
static int setIfDockPresent();
static int setStickyIcons();
static int setPositive();
static int setWidgetColor();
static int setIconTile();
static int setWinTitleFont();
static int setMenuTitleFont();
static int setMenuTextFont();
static int setIconTitleFont();
static int setDisplayFont();
static int setWTitleColor();
static int setFTitleBack();
static int setPTitleBack();
static int setUTitleBack();
static int setWorkspaceBack();
static int setMenuTitleColor();
static int setMenuTextColor();
static int setMenuDisabledColor();
static int setMenuTitleBack();
static int setMenuTextBack();
static int setHightlight();
static int setHightlightText();
static int setKeyGrab();

static int setClipTitleFont();
static int setClipTitleColor();
static int setClipColor();

/*
 * Other points of interaction with the defaults DB:
 * - screen.c (menus)
 */

/*
 * All entries in the tables bellow, NEED to have a default value
 * defined, and this one needs to be correct. Dan.
 */

/* these options will only affect the window manager on startup 
 *
 * static defaults can't access the screen data, because it is
 * created after these defaults are read
 */
WDefaultEntry staticOptionList[] = {    
    
    {"DisableDithering",	"NO",			NULL,
	  &wPreferences.no_dithering,	getBool,	NULL
    },
    {"ColormapSize",	"4",			NULL,
	  &wPreferences.cmap_size,	getInt,		NULL
    },
    /* static by laziness */
    {"IconSize",	"64",			NULL,
	  &wPreferences.icon_size,	getInt,		NULL
    },
    {"ModifierKey",	"Mod1",			NULL,
	  &wPreferences.modifier_mask,	getModMask,	NULL
    },
    {"DisableWSMouseActions", "NO",		NULL,
	  &wPreferences.disable_root_mouse, getBool,	NULL
    },
    {"FocusMode",	"manual",		(void*)False,
	  &wPreferences.focus_mode,	getFocusMode,	NULL
    }, /* have a problem when switching from manual to sloppy without restart */
    {"NewStyle",	"NO",			NULL,
	  &wPreferences.new_style, 	getBool, 	NULL
    },
    {"DisableDock",	"NO",		(void*) WM_DOCK,
	  NULL, 	getBool,        setIfDockPresent
    },
    {"DisableClip",	"NO",		(void*) WM_CLIP,
	  NULL, 	getBool,        setIfDockPresent
    },
};



WDefaultEntry optionList[] = {
      /* dynamic options */
    {"SwapMenuButton",	"NO",			NULL,
	  &wPreferences.swap_menu_buttons,getBool,	NULL
    },
    {"PixmapPath",	DEF_PIXMAP_PATHS,	NULL,
	  &wPreferences.pixmap_path,	getPathList,	NULL
    },
    {"IconPath",	DEF_ICON_PATHS,	NULL,
	  &wPreferences.icon_path,	getPathList,	NULL
    },
    {"ColormapMode",	"auto",			(void*)True,
	  &wPreferences.colormap_mode,	getFocusMode,	NULL
    },
    {"AutoFocus", 	"YES",			NULL,
	  &wPreferences.auto_focus,	getBool,	NULL
    },
    {"RaiseDelay",	"0",			NULL,
	  &wPreferences.raise_delay,	getInt,		NULL
    },
    {"CirculateRaise",	"NO",			NULL,
	  &wPreferences.circ_raise, 	getBool, 	NULL
    },
    {"Superfluous",	"NO",			NULL,
	  &wPreferences.superfluous, 	getBool, 	NULL
    },
    {"WorkspaceChangeDelay", "700",		NULL,
	  &wPreferences.ws_change_delay, getInt,        NULL
    },
    {"AdvanceToNewWorkspace", "NO",		NULL,
	  &wPreferences.ws_advance,      getBool,       NULL
    },
    {"CycleWorkspaces", "NO",			NULL,
	  &wPreferences.ws_cycle,        getBool,	NULL
    },
    {"StickyIcons", "NO",			NULL,
	  &wPreferences.sticky_icons, getBool, setStickyIcons
    },
    {"SaveSessionOnExit", "NO",			NULL,
	  &wPreferences.save_session_on_exit, getBool,	NULL
    },
    {"WrapMenus", "NO",				NULL,
	  &wPreferences.wrap_menus,	getBool,	NULL
    },
    {"ScrollableMenus",	"YES",			NULL,
	  &wPreferences.scrollable_menus, getBool,	NULL
    },
    {"MenuScrollSpeed",	"medium",    		NULL,
	  &wPreferences.menu_scroll_speed, getSpeed,    NULL
    },
    {"IconSlideSpeed",	"medium",		NULL,
	  &wPreferences.icon_slide_speed, getSpeed,     NULL
    },
    {"ShadeSpeed",	"medium",    		NULL,
	  &wPreferences.shade_speed, getSpeed,		NULL
    },
    {"DoubleClickTime",	"250",   (void*) &wPreferences.dblclick_time,
	  &wPreferences.dblclick_time, getInt,        setPositive
    },
    {"AlignSubmenus",	"NO",			NULL,
	  &wPreferences.align_menus,	getBool,	NULL
    },
    {"OnTopTransients",	"YES",			NULL,
	  &wPreferences.on_top_transients, getBool,	NULL
    },
    {"WindowPlacement",	"auto",		NULL,
	  &wPreferences.window_placement, getPlacement,	NULL
    },
    {"IgnoreFocusClick","NO",			NULL,
	  &wPreferences.ignore_focus_click, getBool,	NULL
    },
    {"UseSaveUnders",	"NO",			NULL,
	  &wPreferences.use_saveunders,	getBool,	NULL
    },
    {"OpaqueMove",	"NO",			NULL,
	  &wPreferences.opaque_move,	getBool,	NULL
    },
    {"DisableSound",	"NO",			NULL,
	  &wPreferences.no_sound,	getBool,	NULL
    },
    {"DisableAnimations",	"NO",			NULL,
	  &wPreferences.no_animations,	getBool,	NULL
    },
    {"DontLinkWorkspaces","NO",			NULL,
	  &wPreferences.no_autowrap,	getBool,	NULL
    },
    {"AutoArrangeIcons", "NO",			NULL,
	  &wPreferences.auto_arrange_icons, getBool,	NULL
    },
    {"NoWindowUnderDock", "NO",			NULL,
	  &wPreferences.no_window_under_dock, getBool,	NULL
    },
    {"NoWindowOverIcons", "NO",			NULL,
	  &wPreferences.no_window_over_icons, getBool,  NULL
    },
    {"WindowPlaceOrigin", "(0, 0)",		NULL,
	  &wPreferences.window_place_origin, getCoord,	NULL
    },
/* TODO: finish removing this option
    {"ConstrainWindowSize", "NO",		NULL,
	  &wPreferences.constrain_window_size, getBool,	NULL
    },
*/
    {"ResizeDisplay",	"corner",	       	NULL,
	  &wPreferences.size_display,	getGeomDisp,	NULL
    },
    {"MoveDisplay",	"corner",      		NULL,
	  &wPreferences.move_display,	getGeomDisp,	NULL
    },
    {"DontConfirmKill",	"NO",			NULL,
	  &wPreferences.dont_confirm_kill,	getBool,NULL
    },
      /* style options */
    {"WidgetColor",	"(solid, gray)",	NULL,
	  NULL,				getTexture,	setWidgetColor,
    },
    {"WorkspaceBack",	"(solid, black)",	NULL,
	  NULL,				getBackground, 	setWorkspaceBack
    },
    {"IconBack",	"(solid, gray)",       	NULL,
	  NULL,				getTexture,	setIconTile
    },
    {"TitleJustify",	"center",		NULL,
	  &wPreferences.title_justification, getJust,	setJustify
    },
    {"WindowTitleFont",	DEF_TITLE_FONT,	       	NULL,
	  NULL,				getFont,	setWinTitleFont
    },
    {"MenuTitleFont",	DEF_MENU_TITLE_FONT,	NULL,
	  NULL,				getFont,	setMenuTitleFont
    },
    {"MenuTextFont",	DEF_MENU_ENTRY_FONT,	NULL,
	  NULL,				getFont,	setMenuTextFont
    },
    {"IconTitleFont",	DEF_ICON_TITLE_FONT,	NULL,
	  NULL,				getFont,	setIconTitleFont
    },
    {"ClipTitleFont",	DEF_CLIP_TITLE_FONT,	NULL,
	  NULL,				getFont,	setClipTitleFont
    },
    {"DisplayFont",	DEF_INFO_TEXT_FONT,	NULL,
	  NULL,				getFont,	setDisplayFont
    },
    {"HighlightColor",	"white",		NULL,
	  NULL,				getColor,	setHightlight
    },
    {"HighlightTextColor",	"black",	NULL,
	  NULL,				getColor,	setHightlightText
    },
    {"ClipTitleColor",	"black",		(void*)CLIP_NORMAL,
	  NULL,				getColor,	setClipTitleColor
    },
    {"CClipTitleColor", "\"#616161\"",		(void*)CLIP_COLLAPSED,
	  NULL,				getColor,	setClipTitleColor
    },
    {"AClipColor",      DEF_CLIP_ACTIVE,	(void*)CLIP_ACTIVE,
	  NULL,				getColor,	setClipColor
    },
    {"IClipColor",      DEF_CLIP_INACTIVE,	(void*)CLIP_INACTIVE,
	  NULL,				getColor,	setClipColor
    },
    {"FTitleColor",	"white",		(void*)WS_FOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"PTitleColor",	"white",		(void*)WS_PFOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"UTitleColor",	"black",		(void*)WS_UNFOCUSED,
	  NULL,				getColor,	setWTitleColor
    },
    {"FTitleBack",	"(solid, black)",      	NULL,
	  NULL,				getTexture,	setFTitleBack
    },
    {"PTitleBack",	"(solid, \"#616161\")",	NULL,
	  NULL,				getTexture,	setPTitleBack
    },
    {"UTitleBack",	"(solid, gray)",	NULL,
	  NULL,				getTexture,	setUTitleBack
    },
    {"MenuTitleColor",	"white",       		NULL,
	  NULL,				getColor,	setMenuTitleColor
    },
    {"MenuTextColor",	"black",       		NULL,
	  NULL,				getColor,	setMenuTextColor
    },
    {"MenuDisabledColor", "\"#616161\"",       	NULL,
	  NULL,				getColor,	setMenuDisabledColor
    },
    {"MenuTitleBack",	"(solid, black)",      	NULL,
	  NULL,				getTexture,	setMenuTitleBack
    },
    {"MenuTextBack",	"(solid, gray)",       	NULL,
	  NULL,				getTexture,	setMenuTextBack
    },

	/* keybindings */
    {"RootMenuKey",	"None",			(void*)WKBD_ROOTMENU,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"WindowListKey",	"None",			(void*)WKBD_WINDOWLIST,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"WindowMenuKey",	"None",		       	(void*)WKBD_WINDOWMENU,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipLowerKey",   "None",		        (void*)WKBD_CLIPLOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipRaiseKey",   "None",		        (void*)WKBD_CLIPRAISE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ClipRaiseLowerKey", "None",		 (void*)WKBD_CLIPRAISELOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"MiniaturizeKey",	"None",	       		(void*)WKBD_MINIATURIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"HideKey", "None",	       			(void*)WKBD_HIDE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"CloseKey", "None",       			(void*)WKBD_CLOSE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"MaximizeKey", "None",			(void*)WKBD_MAXIMIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"VMaximizeKey", "None",			(void*)WKBD_VMAXIMIZE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"RaiseKey", "Meta+Up",			(void*)WKBD_RAISE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"LowerKey", "Meta+Down",			(void*)WKBD_LOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"RaiseLowerKey", "None",			(void*)WKBD_RAISELOWER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"ShadeKey", "None",			(void*)WKBD_SHADE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"FocusNextKey", "None",		       	(void*)WKBD_FOCUSNEXT,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"FocusPrevKey", "None",			(void*)WKBD_FOCUSPREV,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"NextWorkspaceKey", "None",		(void*)WKBD_NEXTWORKSPACE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"PrevWorkspaceKey", "None",       		(void*)WKBD_PREVWORKSPACE,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"NextWorkspaceLayerKey", "None",		(void*)WKBD_NEXTWSLAYER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"PrevWorkspaceLayerKey", "None", 		(void*)WKBD_PREVWSLAYER,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace1Key", "None",			(void*)WKBD_WORKSPACE1,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace2Key", "None",			(void*)WKBD_WORKSPACE2,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace3Key", "None",			(void*)WKBD_WORKSPACE3,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace4Key", "None",			(void*)WKBD_WORKSPACE4,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace5Key", "None",			(void*)WKBD_WORKSPACE5,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace6Key", "None",			(void*)WKBD_WORKSPACE6,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace7Key", "None",			(void*)WKBD_WORKSPACE7,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace8Key", "None",			(void*)WKBD_WORKSPACE8,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace9Key", "None",			(void*)WKBD_WORKSPACE9,
	  NULL,				getKeybind,	setKeyGrab
    },
    {"Workspace10Key", "None",			(void*)WKBD_WORKSPACE10,
	  NULL,				getKeybind,	setKeyGrab
    },
};



/* Option names */
static proplist_t DCenter, DCorner, DFloating, DLine;
static proplist_t JLeft, JCenter, JRight;

#if 0
static void rereadDefaults(void);
#endif

#if 0
static void
rereadDefaults(void)
{
    /* must defer the update because accessing X data from a
     * signal handler can mess up Xlib */
    
}
#endif

static void
initDefaults()
{
    int i;
    WDefaultEntry *entry;

    PLSetStringCmpHook(StringCompareHook);

    DCenter = PLMakeString("Center");
    DCorner = PLMakeString("Corner");
    DFloating = PLMakeString("Floating");
    DLine = PLMakeString("Line");

    JLeft = PLMakeString("Left");
    JCenter = PLMakeString("Center");
    JRight = PLMakeString("Right");

    for (i=0; i<sizeof(optionList)/sizeof(WDefaultEntry); i++) {
	entry = &optionList[i];

	entry->plkey = PLMakeString(entry->key);
	if (entry->default_value)
	    entry->plvalue = PLGetProplistWithDescription(entry->default_value);
	else
	    entry->plvalue = NULL;
    }
    
    for (i=0; i<sizeof(staticOptionList)/sizeof(WDefaultEntry); i++) {
	entry = &staticOptionList[i];
	
	entry->plkey = PLMakeString(entry->key);
	if (entry->default_value)
	    entry->plvalue = PLGetProplistWithDescription(entry->default_value);
	else
	    entry->plvalue = NULL;
    }
        
/*
    wDomainName = PLMakeString(WMDOMAIN_NAME);
    wAttributeDomainName = PLMakeString(WMATTRIBUTE_DOMAIN_NAME);

    PLRegister(wDomainName, rereadDefaults);
    PLRegister(wAttributeDomainName, rereadDefaults);
 */
}




#if 0
proplist_t
wDefaultsInit(int screen_number)
{
    static int defaults_inited = 0;
    proplist_t dict;
    
    if (!defaults_inited) {
	initDefaults();
    }
    
    dict = PLGetDomain(wDomainName);
    if (!dict) {
	wwarning(_("could not read domain \"%s\" from defaults database"),
		 PLGetString(wDomainName));
    }

    return dict;
}
#endif


void
wDefaultsDestroyDomain(WDDomain *domain)
{
    if (domain->dictionary)
	PLRelease(domain->dictionary);
    free(domain->path);
    free(domain);
}

char*
wDefaultsPathForDomain(char *domain)
{
    char path[PATH_MAX];
    char *gspath;

    gspath = getenv("GNUSTEP_USER_ROOT");
    if (gspath) {
	strcpy(path, gspath);
	strcat(path, "/");
    } else {
	strcpy(path, wgethomedir());
	strcat(path, "/GNUstep/");
    }
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    gspath = wstrdup(path);

    return gspath;
}


WDDomain*
wDefaultsInitDomain(char *domain, Bool requireDictionary)
{
    WDDomain *db;
    struct stat stbuf;
    static int inited = 0;
    char path[PATH_MAX];
    int global = 0;
    char *the_path;
    proplist_t shared_dict=NULL;

    if (!inited) {
	inited = 1;
	initDefaults();
    }

    db = wmalloc(sizeof(WDDomain));
    memset(db, 0, sizeof(WDDomain));
    db->domain_name = domain;
    db->path = wDefaultsPathForDomain(domain);
    the_path = NULL;
    if (access(db->path, R_OK)!=0) {
	char *gsroot;
	
	gsroot = getenv("GNUSTEP_LOCAL_ROOT");
	if (!gsroot)
	    gsroot = getenv("GNUSTEP_SYSTEM_ROOT");
	if (gsroot)
	    strcpy(path, gsroot);
	else
	    strcpy(path, "/usr/local/GNUstep");
	strcat(path, "/");
	strcat(path, DEFAULTS_DIR);
        strcat(path, "/");
        strcat(path, domain);
	
	if (access(path, R_OK)==0) {
	    global = True;
	    the_path = (char*)path;
	}
    } else {
	the_path = db->path;
    }
    
    
    if (the_path && stat(the_path, &stbuf)>=0) {
	db->dictionary = ReadProplistFromFile(the_path);
	if (db->dictionary) {
	    if (requireDictionary && !PLIsDictionary(db->dictionary)) {
		PLRelease(db->dictionary);
		db->dictionary = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 domain, the_path);
	    }
	    db->timestamp = stbuf.st_mtime;
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     domain);
	}
    }

    /* global system dictionary */
    sprintf(path, "%s/%s/%s", PKGDATADIR, DEFAULTS_DIR, domain);
    if (stat(path, &stbuf)>=0) {
        shared_dict = ReadProplistFromFile(path);
        if (shared_dict) {
	    if (requireDictionary && !PLIsDictionary(shared_dict)) {
		wwarning(_("Domain %s (%s) of global defaults database is corrupted!"),
			 domain, path);
		PLRelease(shared_dict);
		shared_dict = NULL;
	    } else {
		if (db->dictionary && PLIsDictionary(shared_dict) &&
		    PLIsDictionary(db->dictionary)) {
		    PLMergeDictionaries(shared_dict, db->dictionary);
		    PLRelease(db->dictionary);
		    db->dictionary = shared_dict;
		    db->timestamp = stbuf.st_mtime;
		} else if (!db->dictionary) {
		    db->dictionary = shared_dict;
		    db->timestamp = stbuf.st_mtime;
		}
            }
        } else {
	    wwarning(_("could not load domain %s from global defaults database"),
		     domain);
	}
    }

    /* set to save it in user's directory, no matter from where it was read */
    if (db->dictionary) {
        proplist_t tmp = PLMakeString(db->path);

        PLSetFilename(db->dictionary, tmp);
        PLRelease(tmp);
    }

    return db;
}


void
wReadStaticDefaults(proplist_t dict)
{
    proplist_t plvalue;
    WDefaultEntry *entry;
    int i;
    void *tdata;

    
    for (i=0; i<sizeof(staticOptionList)/sizeof(WDefaultEntry); i++) {
	entry = &staticOptionList[i];

	if (dict)
	  plvalue = PLGetDictionaryEntry(dict, entry->plkey);
	else
	  plvalue = NULL;
	
	if (!plvalue) {
	    /* no default in the DB. Use builtin default */
	    plvalue = entry->plvalue;
	}
	
	if (plvalue) {
	    /* convert data */
	    (*entry->convert)(NULL, entry, plvalue, entry->addr, &tdata);
            if (entry->update) {
                (*entry->update)(NULL, entry, tdata, entry->extra_data);
            }
	}
    }
}


void
wDefaultsCheckDomains(void *screen)
{
    WScreen *scr = (WScreen*)screen;
    struct stat stbuf;
    proplist_t dict;
    
    if (stat(scr->dmain->path, &stbuf)>=0
	&& scr->dmain->timestamp < stbuf.st_mtime) {
	dict = ReadProplistFromFile(scr->dmain->path);
	if (dict) {
	    if (!PLIsDictionary(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WindowMaker", scr->dmain->path);
	    } else {
		wReadDefaults(scr, dict);
		if (scr->dmain->dictionary)
		    PLRelease(scr->dmain->dictionary);
		scr->dmain->dictionary = dict;
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WindowMaker");
	}
	scr->dmain->timestamp = stbuf.st_mtime;
    }

    if (stat(scr->dwindow->path, &stbuf)>=0
	&& scr->dwindow->timestamp < stbuf.st_mtime) {
	dict = ReadProplistFromFile(scr->dwindow->path);
	if (dict) {
	    if (!PLIsDictionary(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WMWindowAttributes", scr->dwindow->path);
	    } else {
		if (scr->dwindow->dictionary)
		    PLRelease(scr->dwindow->dictionary);
		scr->dwindow->dictionary = dict;
		wDefaultUpdateIcons(scr);
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WMWindowAttributes");
	}
	scr->dwindow->timestamp = stbuf.st_mtime;
    }


    if (stat(scr->droot_menu->path, &stbuf)>=0
	&& scr->droot_menu->timestamp < stbuf.st_mtime) {
	dict = ReadProplistFromFile(scr->droot_menu->path);
	if (dict) {
	    if (!PLIsDictionary(dict) || !PLIsString(dict)) {
		PLRelease(dict);
		dict = NULL;
		wwarning(_("Domain %s (%s) of defaults database is corrupted!"),
			 "WMRootMenu", scr->droot_menu->path);
	    } else {
		if (scr->droot_menu->dictionary)
		    PLRelease(scr->droot_menu->dictionary);
		scr->droot_menu->dictionary = dict;
	    }
	} else {
	    wwarning(_("could not load domain %s from user defaults database"),
		     "WMRootMenu");
	}
	scr->droot_menu->timestamp = stbuf.st_mtime;
    }

    WMAddTimerHandler(DEFAULTS_CHECK_INTERVAL, wDefaultsCheckDomains, screen);
}



#define REFRESH_WINDOW_TEXTURES	(1<<0)
#define REFRESH_MENU_TEXTURES	(1<<1)
#define REFRESH_WINDOW_FONT	(1<<2)
#define REFRESH_MENU_TITLE_FONT	(1<<3)
#define REFRESH_MENU_FONT	(1<<4)
#define REFRESH_FORE_COLOR	(1<<5)
#define REFRESH_ICON_TILE	(1<<6)
#define REFRESH_ICON_FONT	(1<<7)

static void 
refreshMenus(WScreen *scr, int flags)
{
    WMenu *menu;
    
    menu = scr->root_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    
    menu = scr->workspace_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->switch_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->window_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    menu = scr->icon_menu;
    if (menu)
	wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);

    if (scr->dock) {
	menu = scr->dock->menu;
	if (menu)
	    wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    }
    menu = scr->clip_menu;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    
    menu = scr->clip_submenu;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
    
    menu = scr->clip_options;
    if (menu)
        wMenuRefresh(!menu->flags.brother ? menu : menu->brother, flags);
}


static void
refreshAppIcons(WScreen *scr, int flags)
{
    WAppIcon *aicon = scr->app_icon_list;

    while (aicon) {
        if (aicon->icon) {
            aicon->icon->force_paint = 1;
        }
        aicon = aicon->next;
    }
}


static void
refreshWindows(WScreen *scr, int flags)
{
    WWindow *wwin;

    wwin = scr->focused_window;
    while (wwin) {
        if (flags & REFRESH_WINDOW_FONT) {
            wWindowConfigureBorders(wwin);
        }
        if ((flags & (REFRESH_ICON_TILE|REFRESH_WINDOW_TEXTURES)) &&
            wwin->flags.miniaturized && wwin->icon) {
            wwin->icon->force_paint = 1;
        }
        if (flags & REFRESH_WINDOW_TEXTURES) {
            wwin->frame->flags.need_texture_remake = 1;
        }
        wwin = wwin->prev;
    }
}


void
wReadDefaults(WScreen *scr, proplist_t new_dict)
{
    proplist_t plvalue, old_value;
    WDefaultEntry *entry;
    int i, changed, must_update;
    int needs_refresh;
    void *tdata;
    proplist_t old_dict = (scr->dmain->dictionary!=new_dict 
			   ? scr->dmain->dictionary : NULL);
    
    changed = 0;
    must_update = 0;
    
    needs_refresh = 0;
    
    for (i=0; i<sizeof(optionList)/sizeof(WDefaultEntry); i++) {
	entry = &optionList[i];
	
	if (new_dict)
	    plvalue = PLGetDictionaryEntry(new_dict, entry->plkey);
	else
	    plvalue = NULL;

	if (!old_dict)
	    old_value = NULL;
	else
	    old_value = PLGetDictionaryEntry(old_dict, entry->plkey);

	
	if (!plvalue && !old_value) {
	    /* no default in  the DB. Use builtin default */
	    plvalue = entry->plvalue;
	    if (plvalue && new_dict) {
		PLInsertDictionaryEntry(new_dict, entry->plkey, plvalue);
		changed = 1;
		must_update = 1;
	    }
	} else if (!plvalue) {
	    /* value was deleted from DB. Keep current value */
	    continue;
	} else if (!old_value) {
	    /* set value for the 1st time */
	    changed = 1;
	} else if (!PLIsEqual(plvalue, old_value)) {	    
	    /* value has changed */
	    changed = 1;
	} else {
	    /* value was not changed since last time */
	    continue;
	}
	
	if (plvalue) {
#ifdef DEBUG
	    printf("Updating %s to %s\n", entry->key, PLGetDescription(plvalue));
#endif
	    /* convert data */
	    if ((*entry->convert)(scr, entry, plvalue, entry->addr, &tdata)) {
		if (entry->update) {
		    needs_refresh |= 
			(*entry->update)(scr, entry, tdata, entry->extra_data);
		}
	    }
	}
    }

    if (needs_refresh!=0) {
	int foo;

	foo = 0;
	if (needs_refresh & REFRESH_MENU_TEXTURES)
	    foo |= MR_TEXT_BACK;
	if (needs_refresh & REFRESH_MENU_FONT)
	    foo |= MR_RESIZED;
	if (needs_refresh & REFRESH_MENU_TITLE_FONT)
	    foo |= MR_TITLE_TEXT;

        if (foo)
            refreshMenus(scr, foo);

        if (needs_refresh & (REFRESH_WINDOW_TEXTURES|REFRESH_WINDOW_FONT|
                             REFRESH_ICON_TILE))
            refreshWindows(scr, needs_refresh);

        if (needs_refresh & REFRESH_ICON_TILE)
            refreshAppIcons(scr, needs_refresh);

        wRefreshDesktop(scr);
    }
}


void
wDefaultUpdateIcons(WScreen *scr)
{
    WAppIcon *aicon = scr->app_icon_list;
    WWindow *wwin = scr->focused_window;
    char *file;

    while(aicon) {
        file = wDefaultGetIconFile(scr, aicon->wm_instance, aicon->wm_class);
        if ((file && aicon->icon->file && strcmp(file, aicon->icon->file)!=0)
            || (file && !aicon->icon->file)) {
            RImage *new_image;

            if (aicon->icon->file)
                free(aicon->icon->file);
            aicon->icon->file = wstrdup(file);

            new_image = wDefaultGetImage(scr, aicon->wm_instance,
					 aicon->wm_class);
            if (new_image) {
                wIconChangeImage(aicon->icon, new_image);
                wAppIconPaint(aicon);
            }
        }
        aicon = aicon->next;
    }

    if (!wPreferences.flags.noclip)
        wClipIconPaint(scr->clip_icon);

    while (wwin) {
        if (wwin->icon && wwin->flags.miniaturized) {
            file = wDefaultGetIconFile(scr, wwin->wm_instance, wwin->wm_class);
            if ((file && wwin->icon->file && strcmp(file, wwin->icon->file)!=0)
                || (file && !wwin->icon->file)) {
                RImage *new_image;

                if (wwin->icon->file)
                    free(wwin->icon->file);
                wwin->icon->file = wstrdup(file);

                new_image = wDefaultGetImage(scr, wwin->wm_instance,
					     wwin->wm_class);
                if (new_image)
                    wIconChangeImage(wwin->icon, new_image);
            }
        }
        wwin = wwin->prev;
    }
}


/* --------------------------- Local ----------------------- */

#define STRINGP(x) if (!PLIsString(value)) { \
	wwarning(_("Wrong option format for key \"%s\". Should be %s."), \
	PLGetString(entry->plkey), x); \
	return False; }

/*
 * value - is the value in the defaults DB
 * addr - is the address to store the data
 * ret - is the address to store a pointer to a temporary buffer. ret
 * 	must not be freed and is used by the set functions
 */
static int 
getBool(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
	void **ret)
{
    static char data;
    char *val;
    int second_pass=0;

    STRINGP("Boolean");

    val = PLGetString(value);

again:
    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y' || val[0]=='T' 
			  || val[0]=='t' || val[0]=='1'))
	|| (strcasecmp(val, "YES")==0 || strcasecmp(val, "TRUE")==0)) {

	data = 1;
    } else if ((val[1]=='\0' 
	      && (val[0]=='n' || val[0]=='N' || val[0]=='F' 
		  || val[0]=='f' || val[0]=='0')) 
	     || (strcasecmp(val, "NO")==0 || strcasecmp(val, "FALSE")==0)) {

      data = 0;
    } else {
        wwarning(_("can't convert \"%s\" to boolean for key \"%s\""),
                 val, PLGetString(entry->plkey));
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
	return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr) {
	*(char*)addr = data;
    }
    
    return True;
}


static int 
getInt(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
       void **ret)
{
    static int data;
    char *val;
    
    
    STRINGP("Integer");
    
    val = PLGetString(value);
    
    if (sscanf(val, "%i", &data)!=1) {
        wwarning(_("can't convert \"%s\" to integer for key \"%s\""),
                 val, PLGetString(entry->plkey));
        val = PLGetString(entry->plvalue);
        wwarning(_("using default \"%s\" instead"), val);
        if (sscanf(val, "%i", &data)!=1) {
            return False;
        }
    }
    
    if (ret)
      *ret = &data;
    
    if (addr) {
	*(int*)addr = data;
    }
    return True;
}


static int 
getCoord(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
         void **ret)
{
    static WCoord data;
    char *val_x, *val_y;
    int nelem, changed=0;
    proplist_t elem_x, elem_y;

again:
    if (!PLIsArray(value)) {
        wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 PLGetString(entry->plkey), "Coordinate");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    nelem = PLGetNumberOfElements(value);
    if (nelem != 2) {
        wwarning(_("Incorrect number of elements in array for key \"%s\"."),
                 PLGetString(entry->plkey));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    elem_x = PLGetArrayElement(value, 0);
    elem_y = PLGetArrayElement(value, 1);
    
    if (!elem_x || !elem_y || !PLIsString(elem_x) || !PLIsString(elem_y)) {
        wwarning(_("Wrong value for key \"%s\". Should be Coordinate."),
                 PLGetString(entry->plkey));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    val_x = PLGetString(elem_x);
    val_y = PLGetString(elem_y);

    if (sscanf(val_x, "%i", &data.x)!=1 || sscanf(val_y, "%i", &data.y)!=1) {
        wwarning(_("can't convert array to integers for \"%s\"."),
                 PLGetString(entry->plkey));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    if (data.x < 0)
        data.x = 0;
    else if (data.x > scr->scr_width/3)
        data.x = scr->scr_width/3;
    if (data.y < 0)
        data.y = 0;
    else if (data.y > scr->scr_height/3)
        data.y = scr->scr_height/3;

    if (ret)
        *ret = &data;

    if (addr) {
        *(WCoord*)addr = data;
    }

    return True;
}


#if 0
/* This function is not used at the moment. */
static int 
getString(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	  void **ret)
{
    static char *data;
    
    STRINGP("String");
    
    data = PLGetString(value);
    
    if (!data) {
        data = PLGetString(entry->plvalue);
        if (!data)
            return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(char**)addr = wstrdup(data);
    
    return True;
}
#endif


static int 
getPathList(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr,
	      void **ret)
{
    static char **data;
    int i, count;
    proplist_t d;
    int changed=0;

again:
    if (!PLIsArray(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 PLGetString(entry->plkey), "an array of paths");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
	return False; 
    }
    
    i = 0;
    count = PLGetNumberOfElements(value);
    if (count < 1) {
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }
    
    data = wmalloc(sizeof(char*)*(count+1));
    for (i=0; i<count; i++) {
	d = PLGetArrayElement(value, i);
	if (!d || !PLIsString(d)) {
	    break;
	}
	data[i] = wstrdup(PLGetString(d));
    }
    data[i]=NULL;
    
    if (*(char***)addr!=NULL) {
	char **tmp = *(char***)addr;
	for (i=0; tmp[i]!=NULL; i++) {
	    free(tmp[i]);
	}
	free(tmp);
    }
    *(char***)addr = data;

    return True;
}


static int 
getFocusMode(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	     void **ret)
{
    static char data;
    char *val;
    int second_pass=0;

    STRINGP("one of Manual, Auto or Sloppy");

    val = PLGetString(value);

again:
    if (strcasecmp(val, "manual")==0 || strcasecmp(val, "clicktofocus")==0)
      data = WKF_CLICK;
    else if (strcasecmp(val, "auto")==0 || strcasecmp(val, "focusfollowsmouse")==0)
      data = WKF_POINTER;
    else if ((strcasecmp(val, "semiauto")==0 || strcasecmp(val, "sloppy")==0)
	     && !(int)entry->extra_data)
      data = WKF_SLOPPY;
    else {
	if (!(int)entry->extra_data)
	    wwarning(_("Invalid focus mode \"%s\". Should be Manual, "
		       "Auto or Sloppy."), PLGetString(value));
	else
	    wwarning(_("Invalid colormap focus mode \"%s\". Should be Manual or "
		       "Auto."), PLGetString(value));
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
        return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(char*)addr = data;
    
    return True;
}


static int 
getPlacement(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	     void **ret)
{
    static char data;
    char *val;
    int second_pass=0;

    STRINGP("one of Auto, Cascade, Random or Manual");

    val = PLGetString(value);

again:
    if (strcasecmp(val, "auto")==0 || strcasecmp(val, "smart")==0)
      data = WPM_SMART;
    else if (strcasecmp(val, "cascade")==0)
      data = WPM_CASCADE;
    else if (strcasecmp(val, "manual")==0)
      data = WPM_MANUAL;
    else if (strcasecmp(val, "random")==0)
      data = WPM_RANDOM;
    else {
	wwarning(_("Invalid window placement mode \"%s\". "
		   "Should be Auto, Cascade, Random or Manual."),
		 PLGetString(value));
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
	return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(char*)addr = data;
    
    return True;    
}



static int 
getGeomDisp(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	    void **ret)
{
    static char data;
    int changed=0;

    STRINGP("one of Center, Corner, Floating or Line");

again:
    if(PLIsEqual(value, DCenter))
      data = WDIS_CENTER;
    else if(PLIsEqual(value, DCorner))
      data = WDIS_TOPLEFT;
    else if(PLIsEqual(value, DFloating))
      data = WDIS_FRAME_CENTER;
    else if(PLIsEqual(value, DLine))
      data = WDIS_NEW;
    else {
	wwarning(_("Invalid geometry display type \"%s\". Should "
		   "be Center, Corner, Floating or Line."),
		 PLGetString(value));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
	return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(char*)addr = data;
    
    return True;
}


static int 
getSpeed(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	 void **ret)
{
    static int data;
    char *val;
    int second_pass=0;

    STRINGP("one of UltraFast, Fast, Medium, Slow or UltraSlow");

    val = PLGetString(value);

again:
    
    if (strcasecmp(val, "ultrafast")==0)
      data = SPEED_ULTRAFAST;
    else if (strcasecmp(val, "fast")==0)
      data = SPEED_FAST;
    else if (strcasecmp(val, "medium")==0)
      data = SPEED_MEDIUM;
    else if (strcasecmp(val, "slow")==0)
      data = SPEED_SLOW;
    else if (strcasecmp(val, "ultraslow")==0)
      data = SPEED_ULTRASLOW;
    else {
	wwarning(_("Invalid speed \"%s\". Should be UltraFast, Fast, "
                   "Medium, Slow or UltraSlow."), PLGetString(value));
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
        return False;
    }
    
    if (ret)
      *ret = &data;
    
    if (addr)
      *(int*)addr = data;
    
    return True;
}


/*
 * (solid <color>)
 * (hgradient <color> <color>)
 * (vgradient <color> <color>)
 * (dgradient <color> <color>)
 * (mhgradient <color> <color> ...)
 * (mvgradient <color> <color> ...)
 * (tpixmap <file> <color>)
 * (spixmap <file> <color>)
 * (compose <texture> <texture> <opacity>)
 */

static WTexture*
parse_texture(WScreen *scr, proplist_t pl)
{
    proplist_t elem;
    char *val;
    int nelem;
    WTexture *texture=NULL;
    
    nelem = PLGetNumberOfElements(pl);
    if (nelem < 1)
      return NULL;
    
    
    elem = PLGetArrayElement(pl, 0);
    if (!elem || !PLIsString(elem))
      return NULL;
    val = PLGetString(elem);

    
    if (strcasecmp(val, "solid")==0) {
	XColor color;

	if (nelem != 2)
	    return NULL;

	/* get color */
	
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->colormap, val, &color)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	texture = (WTexture*)wTextureMakeSolid(scr, &color);
    } else if (strcasecmp(val, "dgradient")==0
	       || strcasecmp(val, "vgradient")==0
	       || strcasecmp(val, "hgradient")==0) {
	XColor color1, color2;
	int type;

	if (nelem != 3) {
	    wwarning(_("bad number of arguments in gradient specification"));
	    return NULL;
	}

	if (val[0]=='d' || val[0]=='D')
	  type = WTEX_DGRADIENT;
	else if (val[0]=='h' || val[0]=='H')
	  type = WTEX_HGRADIENT;
	else
	  type = WTEX_VGRADIENT;

	
	/* get from color */
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->colormap, val, &color1)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	/* get to color */
	elem = PLGetArrayElement(pl, 2);
	if (!elem || !PLIsString(elem)) {
	    return NULL;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->colormap, val, &color2)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	texture = (WTexture*)wTextureMakeGradient(scr, type, &color1, &color2);
	
    } else if (strcasecmp(val, "mhgradient")==0
	       || strcasecmp(val, "mvgradient")==0
	       || strcasecmp(val, "mdgradient")==0) {
	XColor color;
	RColor **colors;
	int i, count;
	int type;
	
	if (nelem < 3) {
	    wwarning(_("too few arguments in multicolor gradient specification"));
	    return NULL;
	}

	if (val[1]=='h' || val[1]=='H')
	    type = WTEX_MHGRADIENT;
	else if (val[1]=='v' || val[1]=='V')
	    type = WTEX_MVGRADIENT;
	else
	    type = WTEX_MDGRADIENT;

	count = nelem-1;
	
	colors = wmalloc(sizeof(RColor*)*(count+1));
	
	for (i=0; i<count; i++) {
	    elem = PLGetArrayElement(pl, i+1);
	    if (!elem || !PLIsString(elem)) {
		for ( ; i>=0; --i) {
		    free(colors[i]);
		}
		free(colors);
	    }
	    val = PLGetString(elem);

	    if (!XParseColor(dpy, scr->colormap, val, &color)) {
		wwarning(_("\"%s\" is not a valid color name"), val);
		for ( ; i>=0; --i) {
		    free(colors[i]);
		}
		free(colors);
	    } else {
		colors[i] = wmalloc(sizeof(RColor));
		colors[i]->red = color.red >> 8;
		colors[i]->green = color.green >> 8;
		colors[i]->blue = color.blue >> 8;
	    }
	}
	colors[i] = NULL;
	
	texture = (WTexture*)wTextureMakeMGradient(scr, type, colors);
    } else if (strcasecmp(val, "spixmap")==0 ||
               strcasecmp(val, "tpixmap")==0) {
	XColor color;
        int type;

	if (nelem != 3) 
	    return NULL;

        if (val[0] == 's' || val[0] == 'S')
            type = WTP_SCALE;
        else
            type = WTP_TILE;

	/* get color */
	elem = PLGetArrayElement(pl, 2);
	if (!elem || !PLIsString(elem)) {
	    return NULL;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->colormap, val, &color)) {
	    wwarning(_("\"%s\" is not a valid color name"), val);
	    return NULL;
	}
	
	/* file name */
	elem = PLGetArrayElement(pl, 1);
	if (!elem || !PLIsString(elem)) 
	  return NULL;
	val = PLGetString(elem);

	texture = (WTexture*)wTextureMakePixmap(scr, type, val, &color);

    } else if (strcasecmp(val, "compose")==0) {
	if (nelem != 4)
	    return NULL;
	
    } else {
	return NULL;
    }
    return texture;
}



static int 
getTexture(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	   void **ret)
{
    static WTexture *texture;
    int changed=0;

again:
    if (!PLIsArray(value)) { 
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
		 PLGetString(entry->plkey), "Texture");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    texture = parse_texture(scr, value);

    if (!texture) {
	wwarning(_("Error in texture specification for key \"%s\""),
		 PLGetString(entry->plkey));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }
    
    if (ret) 
      *ret = &texture;

    if (addr)
      *(WTexture**)addr = texture;
    
    return True;
}


static int 
getBackground(WScreen *scr, WDefaultEntry *entry, proplist_t value,
              void *addr, void **ret)
{
    proplist_t elem;
    char *val;
    int nelem;
    static WTexture *texture=NULL;
    int changed=0;

again:
    if (!PLIsArray(value)) {
	wwarning(_("Wrong option format for key \"%s\". Should be %s."),
                 "WorkspaceBack", "Texture");
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    nelem = PLGetNumberOfElements(value);
    if (nelem < 1) {
        wwarning(_("Too few elements in array for key \"WorkspaceBack\"."));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    elem = PLGetArrayElement(value, 0);
    if (!elem || !PLIsString(elem)) {
        wwarning(_("Wrong type for workspace background. Should be Texture."));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }
    val = PLGetString(elem);

    if (strcasecmp(val, "None")==0)
        return False;

    if (strcasecmp(val, "spixmap")!=0 && strcasecmp(val, "tpixmap")!=0) {
        texture = parse_texture(scr, value);
    }
    else {
        /* spixmap || tpixmap */
        XColor color;
        char *file, cpc[30], *style = "-s";
        char *program = "wmsetbg";

        if (nelem != 3) {
            wwarning(_("Too few elements in array for key \"WorkspaceBack\"."));
            if (changed==0) {
                value = entry->plvalue;
                changed = 1;
                wwarning(_("using default \"%s\" instead"), entry->default_value);
                goto again;
            }
            return False;
        }

        if (val[0] == 't' || val[0] == 'T')
            style = "-t";

        sprintf(cpc, "%i", wPreferences.cmap_size);

	/* get color */
	elem = PLGetArrayElement(value, 2);
	if (!elem || !PLIsString(elem)) {
            wwarning(_("Cannot get color entry for key \"WorkspaceBack\"."));
            if (changed==0) {
                value = entry->plvalue;
                changed = 1;
                wwarning(_("using default \"%s\" instead"), entry->default_value);
                goto again;
            }
	    return False;
	}
	val = PLGetString(elem);

	if (!XParseColor(dpy, scr->colormap, val, &color)) {
            wwarning(_("key \"WorkspaceBack\" has invalid color \"%s\""),
                     val);
            if (changed==0) {
                value = entry->plvalue;
                changed = 1;
                wwarning(_("using default \"%s\" instead"), entry->default_value);
                goto again;
            }
	    return False;
	}
	
	/* file name */
	elem = PLGetArrayElement(value, 1);
        if (!elem || !PLIsString(elem)) {
            wwarning(_("Cannot get file entry for key \"WorkspaceBack\"."));
            if (changed==0) {
                value = entry->plvalue;
                changed = 1;
                wwarning(_("using default \"%s\" instead"), entry->default_value);
                goto again;
            }
            return False;
        }

        val = PLGetString(elem);

        file = FindImage(wPreferences.pixmap_path, val);
        if (file) {
            if (fork()==0) {
                execlp(program, program, style, "-c", cpc, file, NULL);
                wwarning(_("could not run \"%s\""), program);
                exit(-1);
            }
            free(file);
        }
        else {
            wwarning(_("could not find background image \"%s\""), val);
            /*return False;*/
        }

        /* This is to let WindowMaker put a color in the background
         * until the pixmap is loaded, if the image is big and loads slow.
         * It assumes that the color will be set before the image is set
         * by the child. Is this true for very small images?
         */
	texture = (WTexture*)wTextureMakeSolid(scr, &color);
    }

    if (!texture) {
        wwarning(_("Error in texture specification for key \"WorkspaceBack\""));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    if (ret)
        *ret = &texture;

    if (addr)
        *(WTexture**)addr = texture;

    return True;
}


static int 
getJust(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	void **ret)
{
    static char data;
    int changed=0;

    STRINGP("one of Left, Center or Right");

again:
    if(PLIsEqual(value, JLeft))
      data = WTJ_LEFT;
    else if(PLIsEqual(value, JRight))
      data = WTJ_RIGHT;
    else if(PLIsEqual(value, JCenter))
      data = WTJ_CENTER;
    else {
	wwarning(_("Invalid justification type \"%s\". Should be "
		   "Left, Center or Right"), PLGetString(value));
        if (changed==0) {
            value = entry->plvalue;
            changed = 1;
            wwarning(_("using default \"%s\" instead"), entry->default_value);
            goto again;
        }
        return False;
    }

    if (ret)
      *ret = &data;
    
    if (addr)
      *(char*)addr = data;
    
    return True;
}


static int 
getFont(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	void **ret)
{
    static WFont *font;
    char *val;
    
    STRINGP("Font");
    
    val = PLGetString(value);
    
    font = wLoadFont(val);
    if (!font) {
	wwarning(_("could not load any usable font"));
	return False;
    }

    if (ret)
      *ret = font;
    
    if (addr) {
	wwarning("BUG:can't assign font value outside update function");
    }
    
    return True;
}


static int 
getColor(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	 void **ret)
{
    static unsigned long pixel;    
    XColor color;
    char *val;
    int second_pass=0;

    STRINGP("Color");
    
    val = PLGetString(value);

again:
    if (!wGetColor(scr, val, &color)) {
        wwarning(_("could not get color for key \"%s\""),
                 PLGetString(entry->plkey));
        if (second_pass==0) {
            val = PLGetString(entry->plvalue);
            second_pass = 1;
            wwarning(_("using default \"%s\" instead"), val);
            goto again;
        }
        return False;
    }
    
    pixel = color.pixel;
    
    if (ret)
      *ret = &pixel;
    
    if (addr)
      *(unsigned long*)addr = pixel;
    
    return True;
}



static int
getKeybind(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	 void **ret)
{
    static WShortKey shortcut;
    KeySym ksym;
    char *val;
    char *k;
    char buf[128], *b;


    STRINGP("Key spec");
    
    val = PLGetString(value);
    
    if (!val || strcasecmp(val, "NONE")==0) {
	shortcut.keycode = 0;
	shortcut.modifier = 0;
	if (ret)
	    *ret = &shortcut;
	return True;
    }

    strcpy(buf, val);
    
    b = (char*)buf;

    /* get modifiers */
    shortcut.modifier = 0;
    while ((k = strchr(b, '+'))!=NULL) {
	int mod;
	
	*k = 0;
	mod = wXModifierFromKey(b);
	if (mod<0) {
	    wwarning(_("invalid key modifier \"%s\""), val);
	    return False;
	}
	shortcut.modifier |= mod;
	
	b = k+1;
    }
    
    /* get key */
    ksym = XStringToKeysym(b);
    
    if (ksym==NoSymbol) {
	wwarning(_("invalid kbd shortcut specification \"%s\""), val);
	return False;
    }
    
    shortcut.keycode = XKeysymToKeycode(dpy, ksym);
    if (shortcut.keycode==0) {
	wwarning(_("invalid key in shortcut \"%s\""), val);
	return False;
    }

    if (ret)
      *ret = &shortcut;

    return True;
}


static int
getModMask(WScreen *scr, WDefaultEntry *entry, proplist_t value, void *addr, 
	   void **ret)
{
    unsigned int mask;
    char *str;

    STRINGP("Modifier Key");

    str = PLGetString(value);
    if (!str)
	return False;
    
    mask = wXModifierFromKey(str);
    
    if (addr)
	*(unsigned int*)addr = mask;
    
    if (ret)
	*ret = &mask;
    
    return True;
}


/* ---------------- value setting functions --------------- */
static int
setJustify(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    return REFRESH_FORE_COLOR;
}


static int
setIfDockPresent(WScreen *scr, WDefaultEntry *entry, int *flag, long which)
{
    switch (which) {
    case WM_DOCK:
        wPreferences.flags.nodock = wPreferences.flags.nodock || *flag;
        break;
    case WM_CLIP:
        wPreferences.flags.noclip = wPreferences.flags.noclip || *flag;
        break;
    default:
        break;
    }
    return 0;
}


static int
setStickyIcons(WScreen *scr, WDefaultEntry *entry, void *bar, void *foo)
{
    if (scr->workspaces) {
        wWorkspaceForceChange(scr, scr->current_workspace);
        wArrangeIcons(scr);
    }
    return 0;
}


static int
setPositive(WScreen *scr, WDefaultEntry *entry, int *value, void *foo)
{
    if (*value <= 0)
        *(int*)foo = 1;

    return 0;
}


static int
setIconTile(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    Pixmap pixmap;
    RImage *img;
    int reset = 0;
   
    img = wTextureRenderImage(*texture, wPreferences.icon_size,
			      wPreferences.icon_size, 
			      ((*texture)->any.type & WREL_BORDER_MASK) 
			      ? WREL_ICON : WREL_FLAT);
    if (!img) {
	wwarning(_("could not render texture for icon background"));
	if (!entry->addr)
	  wTextureDestroy(scr, *texture);
	return 0;
    }
    RConvertImage(scr->rcontext, img, &pixmap);
    
    if (scr->icon_tile) {
	reset = 1;
	RDestroyImage(scr->icon_tile);
	XFreePixmap(dpy, scr->icon_tile_pixmap);
    }
    
    scr->icon_tile = img;
    
    scr->icon_tile_pixmap = pixmap;
    
    if (scr->def_icon_pixmap) {
        XFreePixmap(dpy, scr->def_icon_pixmap);
        scr->def_icon_pixmap = None;
    }
    if (scr->def_ticon_pixmap) {
        XFreePixmap(dpy, scr->def_ticon_pixmap);
        scr->def_ticon_pixmap = None;
    }
    
    if (scr->icon_back_texture) {
	wTextureDestroy(scr, (WTexture*)scr->icon_back_texture);
    }
    scr->icon_back_texture = wTextureMakeSolid(scr, &((*texture)->any.color));
    
    /*
     * Free the texture as nobody else will use it, nor refer to it.
     */
    if (!entry->addr)
      wTextureDestroy(scr, *texture);

    return (reset ? REFRESH_ICON_TILE : 0);
}



static int 
setWinTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->title_font) {
	wFreeFont(scr->title_font);
    }
    
    scr->title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->window_title_gc, font->font->fid);
#endif
    
    return REFRESH_WINDOW_FONT;
}


static int 
setMenuTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->menu_title_font) {
	wFreeFont(scr->menu_title_font);
    }
    
    scr->menu_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->menu_title_gc, font->font->fid);
#endif

    return REFRESH_MENU_TITLE_FONT;
}


static int 
setMenuTextFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->menu_entry_font) {
	wFreeFont(scr->menu_entry_font);
    }
    
    scr->menu_entry_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->menu_entry_gc, font->font->fid);
    XSetFont(dpy, scr->disabled_menu_entry_gc, font->font->fid);
    XSetFont(dpy, scr->select_menu_gc, font->font->fid);
#endif
    
    return REFRESH_MENU_FONT;
}



static int 
setIconTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->icon_title_font) {
	wFreeFont(scr->icon_title_font);
    }
    
    scr->icon_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->icon_title_gc, font->font->fid);
#endif
		 
    return REFRESH_ICON_FONT;
}


static int 
setClipTitleFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->clip_title_font) {
	wFreeFont(scr->clip_title_font);
    }
    
    scr->clip_title_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->clip_title_gc, font->font->fid);
#endif
		 
    return REFRESH_ICON_FONT;
}


static int
setDisplayFont(WScreen *scr, WDefaultEntry *entry, WFont *font, void *foo)
{
    if (scr->info_text_font) {
	wFreeFont(scr->info_text_font);
    }
    
    scr->info_text_font = font;

#ifndef I18N_MB
    XSetFont(dpy, scr->info_text_gc, font->font->fid);
    XSetFont(dpy, scr->line_gc, font->font->fid);
#endif

    /* This test works because the scr structure is initially zeroed out
       and None = 0. Any other time, the window should be valid. */
    if (scr->geometry_display != None) {
        wGetGeometryWindowSize(scr, &scr->geometry_display_width,
			       &scr->geometry_display_height);
	XResizeWindow(dpy, scr->geometry_display,
	    scr->geometry_display_width, scr->geometry_display_height);
    }

    return 0;
}


static int
setHightlight(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
   if (scr->select_pixel!=scr->white_pixel &&
	scr->select_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->select_pixel);
    }
    
    scr->select_pixel = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setHightlightText(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
   if (scr->select_text_pixel!=scr->white_pixel &&
	scr->select_text_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->select_text_pixel);
    }
    
    scr->select_text_pixel = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setClipTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
    if (scr->clip_title_pixel[index]!=scr->white_pixel &&
	scr->clip_title_pixel[index]!=scr->black_pixel) {
	wFreeColor(scr, scr->clip_title_pixel[index]);
    }
    
    scr->clip_title_pixel[index] = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setClipColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
    if (scr->clip_pixel[index]!=scr->white_pixel &&
	scr->clip_pixel[index]!=scr->black_pixel) {
	wFreeColor(scr, scr->clip_pixel[index]);
    }

    scr->clip_pixel[index] = color->pixel;

    return REFRESH_FORE_COLOR;
}


static int
setWTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, long index)
{
    if (scr->window_title_pixel[index]!=scr->white_pixel &&
	scr->window_title_pixel[index]!=scr->black_pixel) {
	wFreeColor(scr, scr->window_title_pixel[index]);
    }
    
    scr->window_title_pixel[index] = color->pixel;

    if (index == WS_FOCUSED)
	XSetForeground(dpy, scr->icon_title_gc, color->pixel);
    else if (index == WS_UNFOCUSED)
	XSetForeground(dpy, scr->info_text_gc, color->pixel);
    
    return REFRESH_FORE_COLOR;
}


static int 
setMenuTitleColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    if (scr->menu_title_pixel[0]!=scr->white_pixel &&
	scr->menu_title_pixel[0]!=scr->black_pixel) {
	wFreeColor(scr, scr->menu_title_pixel[0]);
    }
    
    scr->menu_title_pixel[0] = color->pixel;
    
    XSetForeground(dpy, scr->menu_title_gc, color->pixel);
    
    return REFRESH_FORE_COLOR;
}


static int 
setMenuTextColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    XGCValues gcv;
#define gcm (GCForeground|GCBackground|GCFillStyle)

    if (scr->mtext_pixel!=scr->white_pixel &&
	scr->mtext_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->mtext_pixel);
    }
    
    scr->mtext_pixel = color->pixel;
    
    XSetForeground(dpy, scr->menu_entry_gc, color->pixel);


    if (scr->dtext_pixel == scr->mtext_pixel) {
	gcv.foreground = scr->white_pixel;
	gcv.background = scr->black_pixel;
	gcv.fill_style = FillStippled;
    } else {
	gcv.foreground = scr->dtext_pixel;
	gcv.fill_style = FillSolid;
    }
    XChangeGC(dpy, scr->disabled_menu_entry_gc, gcm, &gcv);
    
    return REFRESH_FORE_COLOR;
#undef gcm
}


static int 
setMenuDisabledColor(WScreen *scr, WDefaultEntry *entry, XColor *color, void *foo)
{
    XGCValues gcv;
#define gcm (GCForeground|GCBackground|GCFillStyle)

    if (scr->dtext_pixel!=scr->white_pixel &&
	scr->dtext_pixel!=scr->black_pixel) {
	wFreeColor(scr, scr->dtext_pixel);
    }
    
    scr->dtext_pixel = color->pixel;

    if (scr->dtext_pixel == scr->mtext_pixel) {
	gcv.foreground = scr->white_pixel;
	gcv.background = scr->black_pixel;
	gcv.fill_style = FillStippled;
    } else {
	gcv.foreground = scr->dtext_pixel;
	gcv.fill_style = FillSolid;
    }
    XChangeGC(dpy, scr->disabled_menu_entry_gc, gcm, &gcv);

    return REFRESH_FORE_COLOR;
#undef gcm
}



static int
setWorkspaceBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, 
		 void *foo)
{
    Pixmap pixmap;
    RImage *img;

    if ((*texture)->any.type==WTEX_SOLID) {
	XSetWindowBackground(dpy, scr->root_win, (*texture)->solid.normal.pixel);
	XClearWindow(dpy, scr->root_win);
    } else {
	int w, h;
	
	switch ((*texture)->any.type) {
	 case WTEX_HGRADIENT:
	 case WTEX_MHGRADIENT:
	    w = scr->scr_width;
	    h = 8;
	    break;
	    
	 case WTEX_VGRADIENT:
	 case WTEX_MVGRADIENT:
	    w = 8;
	    h = scr->scr_height;
	    break;
	    
	 case WTEX_DGRADIENT:
         case WTEX_MDGRADIENT:
            w = scr->scr_width;
	    h = scr->scr_height;
	    break;
            
	 default:
	    if (!entry->addr)
		wTextureDestroy(scr, *texture);
	    return 0;
	}
	
	img = wTextureRenderImage(*texture, w, h, WREL_FLAT);
	if (!img) {
	    wwarning(_("could not render texture for workspace background"));
	    if (!entry->addr)
		wTextureDestroy(scr, *texture);
	    return 0;
	}
	RConvertImage(scr->rcontext, img, &pixmap);
	RDestroyImage(img);
	XSetWindowBackgroundPixmap(dpy, scr->root_win, pixmap);
	XClearWindow(dpy, scr->root_win);
    }

    /*
     * Free the texture as nobody else will use it, nor refer to it.
     */
    if (!entry->addr)
      wTextureDestroy(scr, *texture);

    return 0;
}


static int 
setWidgetColor(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->widget_texture) {
	wTextureDestroy(scr, (WTexture*)scr->widget_texture);
    }
    scr->widget_texture = *(WTexSolid**)texture;

    return 0;
}


static int 
setFTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{		       
    if (scr->window_title_texture[WS_FOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_FOCUSED]);
    }
    scr->window_title_texture[WS_FOCUSED] = *texture;

    if (scr->icon_title_texture) {
	wTextureDestroy(scr, (WTexture*)scr->icon_title_texture);
    }
    scr->icon_title_texture 
      = wTextureMakeSolid(scr, &scr->window_title_texture[WS_FOCUSED]->any.color);
    
    return REFRESH_WINDOW_TEXTURES;
}


static int 
setPTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->window_title_texture[WS_PFOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_PFOCUSED]);
    }
    scr->window_title_texture[WS_PFOCUSED] = *texture;
    
    return REFRESH_WINDOW_TEXTURES;
}


static int 
setUTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->window_title_texture[WS_UNFOCUSED]) {
	wTextureDestroy(scr, scr->window_title_texture[WS_UNFOCUSED]);
    }
    scr->window_title_texture[WS_UNFOCUSED] = *texture;
    
    if (scr->resizebar_texture[0]) {
	wTextureDestroy(scr, (WTexture*)scr->resizebar_texture[0]);
    }
    scr->resizebar_texture[0]
      = wTextureMakeSolid(scr, &scr->window_title_texture[WS_UNFOCUSED]->any.color);
    
    if (scr->geometry_display != None)
    	XSetWindowBackground(dpy, scr->geometry_display, 
			 scr->resizebar_texture[0]->normal.pixel);
			 
    return REFRESH_WINDOW_TEXTURES;
}


static int 
setMenuTitleBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->menu_title_texture[0]) {
	wTextureDestroy(scr, scr->menu_title_texture[0]);
    }
    scr->menu_title_texture[0] = *texture;

    return REFRESH_MENU_TEXTURES;
}


static int 
setMenuTextBack(WScreen *scr, WDefaultEntry *entry, WTexture **texture, void *foo)
{
    if (scr->menu_item_texture) {
	wTextureDestroy(scr, scr->menu_item_texture);
	wTextureDestroy(scr, (WTexture*)scr->menu_item_auxtexture);
    }
    scr->menu_item_texture = *texture;

    scr->menu_item_auxtexture 
      = wTextureMakeSolid(scr, &scr->menu_item_texture->any.color);
    
    return REFRESH_MENU_TEXTURES;
}


static int
setKeyGrab(WScreen *scr, WDefaultEntry *entry, WShortKey *shortcut, long index)
{
    WWindow *wwin;
    wKeyBindings[index] = *shortcut;
    
    wwin = scr->focused_window;
    
    while (wwin!=NULL) {
	XUngrabKey(dpy, AnyKey, AnyModifier, wwin->frame->core->window);

	if (wwin->window_flags.bind_keys) {
	    wWindowSetKeyGrabs(wwin);
	}
	wwin = wwin->next;
    }
    
    return 0;
}



