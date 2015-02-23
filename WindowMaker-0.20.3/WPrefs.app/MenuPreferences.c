/* MenuPreferences.c- menu related preferences
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
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


#include "WPrefs.h"

typedef struct _Panel {
    WMFrame *frame;

    char *sectionName;

    CallbackRec callbacks;
    
    WMWindow *win;

    WMFrame *scrF;
    WMButton *scrB[5];
    
    WMFrame *aliF;
    WMButton *aliyB;
    WMButton *alinB;
    
    WMFrame *optF;
    WMButton *autoB;
    WMButton *wrapB;

} _Panel;



#define ICON_FILE	"menuprefs"
#define SPEED_IMAGE "speed%i"
#define SPEED_IMAGE_S "speed%is"

#define MENU_ALIGN1 "menualign1"
#define MENU_ALIGN2 "menualign2"


static void
showData(_Panel *panel)
{
    WMPerformButtonClick(panel->scrB[GetSpeedForKey("MenuScrollSpeed")]);
    
    if (GetBoolForKey("AlignSubmenus"))
	WMPerformButtonClick(panel->aliyB);
    else
	WMPerformButtonClick(panel->alinB);
    
    WMSetButtonSelected(panel->wrapB, GetBoolForKey("WrapMenus"));
    
    WMSetButtonSelected(panel->autoB, GetBoolForKey("ScrollableMenus"));
}


static void
storeData(_Panel *panel)
{
    int i;
    
    for (i=0; i<5; i++) {
	if (WMGetButtonSelected(panel->scrB[i]))
	    break;
    }
    SetSpeedForKey(i, "MenuScrollSpeed");

    SetBoolForKey(WMGetButtonSelected(panel->aliyB), "AlignSubmenus");

    SetBoolForKey(WMGetButtonSelected(panel->wrapB), "WrapMenus");
    SetBoolForKey(WMGetButtonSelected(panel->autoB), "ScrollableMenus");
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->win);

    WMPixmap *icon;
    int i;
    char *buf1, *buf2;
    char *path;
    
    panel->frame = WMCreateFrame(panel->win);
    WMResizeWidget(panel->frame, FRAME_WIDTH, FRAME_HEIGHT);
    WMMoveWidget(panel->frame, FRAME_LEFT, FRAME_TOP);
    
    
    /***************** Menu Scroll Speed ****************/
    panel->scrF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->scrF, 235, 90);
    WMMoveWidget(panel->scrF, 25, 20);
    WMSetFrameTitle(panel->scrF, _("Menu Scrolling Speed"));
    

    buf1 = wmalloc(strlen(SPEED_IMAGE)+1);
    buf2 = wmalloc(strlen(SPEED_IMAGE_S)+1);
    for (i = 0; i < 5; i++) {
	panel->scrB[i] = WMCreateCustomButton(panel->scrF, WBBStateChangeMask);
	WMResizeWidget(panel->scrB[i], 40, 40);
	WMMoveWidget(panel->scrB[i], 15+(40*i), 30);
	WMSetButtonBordered(panel->scrB[i], False);
	WMSetButtonImagePosition(panel->scrB[i], WIPImageOnly);
	if (i > 0) {
	    WMGroupButtons(panel->scrB[0], panel->scrB[i]);
	}
	sprintf(buf1, SPEED_IMAGE, i);
	sprintf(buf2, SPEED_IMAGE_S, i);
	path = LocateImage(buf1);
	if (path) {
	    icon = WMCreatePixmapFromFile(scr, path);
	    if (icon) {
		WMSetButtonImage(panel->scrB[i], icon);
		WMReleasePixmap(icon);
	    } else {
		wwarning(_("could not load icon file %s"), path);
	    }
	    free(path);
	}
	path = LocateImage(buf2);
	if (path) {
	    icon = WMCreatePixmapFromFile(scr, path);
	    if (icon) {
		WMSetButtonAltImage(panel->scrB[i], icon);
		WMReleasePixmap(icon);
	    } else {
		wwarning(_("could not load icon file %s"), path);
	    }
	    free(path);
	}
    }
    free(buf1);
    free(buf2);

    WMMapSubwidgets(panel->scrF);

    /***************** Submenu Alignment ****************/
    
    panel->aliF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->aliF, 220, 90);
    WMMoveWidget(panel->aliF, 280, 20);
    WMSetFrameTitle(panel->aliF, _("Submenu Alignment"));
    
    panel->alinB = WMCreateButton(panel->aliF, WBTOnOff);
    WMResizeWidget(panel->alinB, 48, 48);
    WMMoveWidget(panel->alinB, 56, 25);
    WMSetButtonImagePosition(panel->alinB, WIPImageOnly);
    path = LocateImage(MENU_ALIGN1);
    if (path) {
	icon = WMCreatePixmapFromFile(scr, path);
	if (icon) {
	    WMSetButtonImage(panel->alinB, icon);
	    WMReleasePixmap(icon);
	} else {
	    wwarning(_("could not load icon file %s"), path);
	}
	free(path);
    }
    panel->aliyB = WMCreateButton(panel->aliF, WBTOnOff);
    WMResizeWidget(panel->aliyB, 48, 48);
    WMMoveWidget(panel->aliyB, 120, 25);
    WMSetButtonImagePosition(panel->aliyB, WIPImageOnly);
    path = LocateImage(MENU_ALIGN2);
    if (path) {
	icon = WMCreatePixmapFromFile(scr, path);
	if (icon) {
	    WMSetButtonImage(panel->aliyB, icon);
	    WMReleasePixmap(icon);
	} else {
	    wwarning(_("could not load icon file %s"), path);
	}
    }
    WMGroupButtons(panel->alinB, panel->aliyB);
    
    WMMapSubwidgets(panel->aliF);
    
    /***************** Options ****************/
    panel->optF = WMCreateFrame(panel->frame);
    WMResizeWidget(panel->optF, 475, 80);
    WMMoveWidget(panel->optF, 25, 130);
    
    panel->wrapB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->wrapB, 440, 32);
    WMMoveWidget(panel->wrapB, 25, 8);
    WMSetButtonText(panel->wrapB, _("Always open submenus inside the screen, instead of scrolling.\nNote: this can be an annoyance at some circumstances."));

    panel->autoB = WMCreateSwitchButton(panel->optF);
    WMResizeWidget(panel->autoB, 440, 20);
    WMMoveWidget(panel->autoB, 25, 45);
    WMSetButtonText(panel->autoB, _("Scroll off-screen menus when pointer is moved over them."));
    
    WMMapSubwidgets(panel->optF);
    
    WMRealizeWidget(panel->frame);
    WMMapSubwidgets(panel->frame);

    showData(panel);
}



Panel*
InitMenuPreferences(WMScreen *scr, WMWindow *win)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Menu Preferences");

    panel->win = win;
    
    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;

    AddSection(panel, ICON_FILE);

    return panel;
}
