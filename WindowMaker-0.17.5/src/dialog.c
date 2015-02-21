/* dialog.c - dialog windows for internal use
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX DEFAULT_PATH_MAX
#endif

#include "WindowMaker.h"
#include "GNUstep.h"
#include "screen.h"
#include "dialog.h"
#include "funcs.h"
#include "framewin.h"
#include "window.h"


extern WPreferences wPreferences;



typedef struct {
    int done;
    int result;
} pair;


static void
handler(pid_t pid, unsigned int status, void *cdata)
{
    ((pair*)cdata)->result = (char)status;
    ((pair*)cdata)->done = 1;
}


int
wMessageDialog(WScreen *scr, char *title, char *message, int type)
{
    WMAlertPanel *panel;
    Window parent;
    WWindow *wwin;
    int result;

    
    switch (type) {
     case WD_CONFIRMATION:
	panel = WMCreateAlertPanel(scr->wmscreen, title, message,
				 _("OK"), _("Cancel"), NULL);
	break;
	
     case WD_EXIT_CONFIRM:
	panel = WMCreateAlertPanel(scr->wmscreen, title, message, 
				   _("Exit"), _("Cancel"), NULL);
	break;
	
     case WD_INFORMATION:
     case WD_ERROR:
     default:
	panel = WMCreateAlertPanel(scr->wmscreen, title, message,
				   _("OK"), NULL, NULL);
	break;
	
     case WD_YESNO:
	panel = WMCreateAlertPanel(scr->wmscreen, title, message,
				   _("Yes"), _("No"), NULL);
	break;
    }
    

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 400, 180, 0, 0, 0);
    
    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);
    
    wwin = wManageInternalWindow(scr, parent, None, NULL, 
				 (scr->scr_width - 400)/2, 
				 (scr->scr_height - 180)/2, 400, 180);

    
    WMMapWidget(panel->win);

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }

    result = panel->result;

    WMUnmapWidget(panel->win);

    wUnmanageWindow(wwin, False);

    WMDestroyAlertPanel(panel);

    XDestroyWindow(dpy, parent);

    switch (type) {
     case WD_CONFIRMATION:
	if (result==WAPRDefault)
	    result = WDB_OK;
	else
	    result = WDB_CANCEL;
	break;

     case WD_EXIT_CONFIRM:
	if (result==WAPRDefault)
	    result = WDB_EXIT;
	else
	    result = WDB_CANCEL;
	break;

     case WD_YESNO:
	if (result==WAPRDefault)
	    result = WDB_YES;
	else
	    result = WDB_NO;
	break;
	

     case WD_INFORMATION:
     case WD_ERROR:
     default:
	result = WDB_OK;
	break;
    }
    
    return result;
}



int
wInputDialog(WScreen *scr, char *title, char *message, char **text)
{
    WWindow *wwin;
    Window parent;
    WMInputPanel *panel;
    char *result;

    
    panel = WMCreateInputPanel(scr->wmscreen, title, message, *text,
			_("OK"), _("Cancel"));


    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 320, 160, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, NULL, 
				 (scr->scr_width - 320)/2,
				 (scr->scr_height - 160)/2, 320, 160);

    WMMapWidget(panel->win);
    

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
    
    if (panel->result == WAPRDefault)
	result = WMGetTextFieldText(panel->text);
    else
	result = NULL;
    
    wUnmanageWindow(wwin, False);
    
    WMDestroyInputPanel(panel);

    XDestroyWindow(dpy, parent);

    if (result==NULL)
	return WDB_CANCEL;
    else {
        if (*text)
            free(*text);
        *text = result;

        return WDB_OK;
    }
}


/*
 *****************************************************************
 * Icon Selection Panel
 *****************************************************************
 */

typedef struct IconPanel {
    
    WScreen *scr;
    
    WMWindow *win;

    WMLabel *dirLabel;
    WMLabel *iconLabel;
    
    WMList *dirList;
    WMList *iconList;
    
    WMLabel *iconView;
    
    WMLabel *fileLabel;
    WMTextField *fileField;
    
    WMButton *okButton;
    WMButton *cancelButton;
#if 0
    WMButton *chooseButton;
#endif
    short done;
    short result;
} IconPanel;



static void
listPixmaps(WScreen *scr, WMList *lPtr, char *path)
{
    struct dirent *dentry;
    DIR *dir;
    char pbuf[PATH_MAX+16];
    char *apath;
    
    apath = wexpandpath(path);
    dir = opendir(apath);
    
    if (!dir) {
	char *msg;
	char *tmp;
	tmp = _("Could not open directory ");
	msg = wmalloc(strlen(tmp)+strlen(path)+6);
	strcpy(msg, tmp);
	strcat(msg, path);
	
	wMessageDialog(scr, _("Error"), msg, WD_ERROR);
	free(msg);
	free(apath);
	return;
    }

    /* list contents in the column */
    while ((dentry = readdir(dir))) {
	struct stat statb;
	
	if (strcmp(dentry->d_name, ".")==0 ||
	    strcmp(dentry->d_name, "..")==0)
	    continue;

	strcpy(pbuf, apath);
	strcat(pbuf, "/");
	strcat(pbuf, dentry->d_name);
	
	if (stat(pbuf, &statb)<0)
	    continue;
	
	if (statb.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)
	    && statb.st_mode & (S_IFREG|S_IFLNK)) {
	    WMAddSortedListItem(lPtr, dentry->d_name);
	}
    }

    closedir(dir);
    free(apath);
}



static void
setViewedImage(IconPanel *panel, char *file)
{
    WMPixmap *pixmap;
    RColor color;
		
    color.red = 0xae;
    color.green = 0xaa;
    color.blue = 0xae;
    color.alpha = 0;
    pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(panel->win),
					   file, &color);
    if (!pixmap) {
	char *msg;
	char *tmp;

	WMSetButtonDisabled(panel->okButton, True);
	    
	tmp = _("Could not load image file ");
	msg = wmalloc(strlen(tmp)+strlen(file)+6);
	strcpy(msg, tmp);
	strcat(msg, file);
	
	wMessageDialog(panel->scr, _("Error"), msg, WD_ERROR);
	free(msg);
	
	WMSetLabelImage(panel->iconView, NULL);
    } else {
	WMSetButtonDisabled(panel->okButton, False);
	
	WMSetLabelImage(panel->iconView, pixmap);
	WMReleasePixmap(pixmap);
    }
}


static void
listCallback(void *self, void *data)
{
    WMList *lPtr = (WMList*)self;
    IconPanel *panel = (IconPanel*)data;
    char *path;

    if (lPtr==panel->dirList) {
	path = WMGetListSelectedItem(lPtr)->text;
	
	WMSetTextFieldText(panel->fileField, path);

	WMSetLabelImage(panel->iconView, NULL);

	WMSetButtonDisabled(panel->okButton, True);

	WMClearList(panel->iconList);
	listPixmaps(panel->scr, panel->iconList, path);	
    } else {
	char *tmp, *iconFile;
	
	path = WMGetListSelectedItem(panel->dirList)->text;
	tmp = wexpandpath(path);

	iconFile = WMGetListSelectedItem(panel->iconList)->text;

	path = wmalloc(strlen(tmp)+strlen(iconFile)+4);
	strcpy(path, tmp);
	strcat(path, "/");
	strcat(path, iconFile);
	free(tmp);
	WMSetTextFieldText(panel->fileField, path);
	setViewedImage(panel, path);
	free(path);
    }
}


static void
listIconPaths(WMList *lPtr)
{
    int i;
    
    for (i=0; wPreferences.icon_path[i]!=NULL; i++) {
	/* do not sort, because the order implies the order of
	 * directories searched */
	WMAddListItem(lPtr, wPreferences.icon_path[i]);
    }
}



static void
buttonCallback(void *self, void *clientData)
{
    WMButton *bPtr = (WMButton*)self;
    IconPanel *panel = (IconPanel*)clientData;
    
    
    if (bPtr==panel->okButton) {
	panel->done = True;
	panel->result = True;
    } else if (bPtr==panel->cancelButton) {
	panel->done = True;
	panel->result = False;
    }
#if 0
    else if (bPtr==panel->chooseButton) {
	WMOpenPanel *op;
	
	op = WMCreateOpenPanel(WMWidgetScreen(bPtr));
	
	if (WMRunModalOpenPanelForDirectory(op, "/usr/local", NULL, NULL)) {
	    char *path;
	    path = WMGetFilePanelFile(op);
	    WMSetTextFieldText(panel->fileField, path);
	    setViewedImage(panel, path);
	    free(path);
	}
	WMDestroyFilePanel(op);
    }
#endif
}


Bool
wIconChooserDialog(WScreen *scr, char **file)
{
    WWindow *wwin;
    Window parent;
    IconPanel *panel;
    WMColor *color;
    WMFont *boldFont;
    
    panel = wmalloc(sizeof(IconPanel));
    memset(panel, 0, sizeof(IconPanel));

    panel->scr = scr;
    
    panel->win = WMCreateWindow(scr->wmscreen, "iconChooser");
    WMResizeWidget(panel->win, 450, 280);
    
    boldFont = WMBoldSystemFontOfSize(scr->wmscreen, 12);
    
    panel->dirLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->dirLabel, 200, 20);
    WMMoveWidget(panel->dirLabel, 10, 7);
    WMSetLabelText(panel->dirLabel, _("Directories"));
    WMSetLabelFont(panel->dirLabel, boldFont);
    WMSetLabelTextAlignment(panel->dirLabel, WACenter);
        
    WMSetLabelRelief(panel->dirLabel, WRSunken);

    panel->iconLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconLabel, 140, 20);
    WMMoveWidget(panel->iconLabel, 215, 7);
    WMSetLabelText(panel->iconLabel, _("Icons"));
    WMSetLabelFont(panel->iconLabel, boldFont);
    WMSetLabelTextAlignment(panel->iconLabel, WACenter);

    WMReleaseFont(boldFont);
    
    color = WMWhiteColor(scr->wmscreen);
    WMSetLabelTextColor(panel->dirLabel, color); 
    WMSetLabelTextColor(panel->iconLabel, color);
    WMReleaseColor(scr->wmscreen, color);

    color = WMDarkGrayColor(scr->wmscreen);
    WMSetWidgetBackgroundColor(panel->iconLabel, color);
    WMSetWidgetBackgroundColor(panel->dirLabel, color); 
    WMReleaseColor(scr->wmscreen, color);

    WMSetLabelRelief(panel->iconLabel, WRSunken);

    panel->dirList = WMCreateList(panel->win);
    WMResizeWidget(panel->dirList, 200, 170);
    WMMoveWidget(panel->dirList, 10, 30);
    WMSetListAction(panel->dirList, listCallback, panel);
    
    panel->iconList = WMCreateList(panel->win);
    WMResizeWidget(panel->iconList, 140, 170);
    WMMoveWidget(panel->iconList, 215, 30); 
    WMSetListAction(panel->iconList, listCallback, panel);
   
    panel->iconView = WMCreateLabel(panel->win);
    WMResizeWidget(panel->iconView, 75, 75);
    WMMoveWidget(panel->iconView, 365, 60);
    WMSetLabelImagePosition(panel->iconView, WIPImageOnly);
    WMSetLabelRelief(panel->iconView, WRSunken);
    
    panel->fileLabel = WMCreateLabel(panel->win);
    WMResizeWidget(panel->fileLabel, 62, 20);
    WMMoveWidget(panel->fileLabel, 10, 210);
    WMSetLabelText(panel->fileLabel, _("File Name:"));
    
    panel->fileField = WMCreateTextField(panel->win);
    WMResizeWidget(panel->fileField, 365, 20);
    WMMoveWidget(panel->fileField, 75, 210);
    WMSetTextFieldDisabled(panel->fileField, True);
    
    panel->okButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->okButton, 80, 28);
    WMMoveWidget(panel->okButton, 360, 240);
    WMSetButtonText(panel->okButton, _("OK"));
    WMSetButtonDisabled(panel->okButton, True);
    WMSetButtonAction(panel->okButton, buttonCallback, panel);
    
    panel->cancelButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->cancelButton, 80, 28);
    WMMoveWidget(panel->cancelButton, 270, 240);
    WMSetButtonText(panel->cancelButton, _("Cancel"));
    WMSetButtonAction(panel->cancelButton, buttonCallback, panel);
#if 0
    panel->chooseButton = WMCreateCommandButton(panel->win);
    WMResizeWidget(panel->chooseButton, 110, 28);
    WMMoveWidget(panel->chooseButton, 150, 240);
    WMSetButtonText(panel->chooseButton, _("Choose File"));
    WMSetButtonAction(panel->chooseButton, buttonCallback, panel);
#endif
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 450, 280, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, _("Icon Chooser"),
				 (scr->scr_width - 450)/2,
				 (scr->scr_height - 280)/2, 450, 280);

    
    /* put icon paths in the list */
    listIconPaths(panel->dirList);
    
    
    WMMapWidget(panel->win);

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
    
    if (panel->result) {
	char *defaultPath, *wantedPath;

	/* check if the file the user selected is not the one that
	 * would be loaded by default with the current search path */
	*file = WMGetListSelectedItem(panel->iconList)->text;
	if ((*file)[0]==0) {
	    free(*file);
	    *file = NULL;
	} else {
	    defaultPath = FindImage(wPreferences.icon_path, *file);
	    wantedPath = WMGetTextFieldText(panel->fileField);
	    /* if the file is not the default, use full path */
	    if (strcmp(wantedPath, defaultPath)!=0) {
		*file = wantedPath;
	    } else {
		*file = wstrdup(*file);
		free(wantedPath);
	    }
	    free(defaultPath);
	}
    } else {
	*file = NULL;
    }

    WMUnmapWidget(panel->win);

    WMDestroyWidget(panel->win);    

    wUnmanageWindow(wwin, False);

    free(panel);

    XDestroyWindow(dpy, parent);
    
    return panel->result;
}


/*
 ***********************************************************************
 * Info Panel
 ***********************************************************************
 */


typedef struct {
    WScreen *scr;
    
    WMWindow *win;

    WMLabel *logoL;
    WMLabel *name1L;
    WMLabel *name2L;

    WMLabel *versionL;
    
    WMLabel *copyrL;
    
    int done;
} InfoPanel;



#define COPYRIGHT_TEXT  \
     "Copyright \xa9 1997, 1998 Alfredo K. Kojima <kojima@windowmaker.org>\n"\
     "Copyright \xa9 1998 Dan Pascu <dan@windowmaker.org>"

 

static InfoPanel *thePanel = NULL;

static void
destroyInfoPanel(WCoreWindow *foo, void *data, XEvent *event)
{
    thePanel->done = 1;
}


void
wShowInfoPanel(WScreen *scr)
{
    InfoPanel *panel;
    WMPixmap *logo;
    WMSize size;
    WMFont *font;
    char version[32];
    Window parent;
    WWindow *wwin;

    if (thePanel)
	return;

    panel = wmalloc(sizeof(InfoPanel));

    panel->scr = scr;
    
    panel->win = WMCreateWindow(scr->wmscreen, "info");
    WMResizeWidget(panel->win, 390, 237);
    
    logo = WMGetApplicationIconImage(scr->wmscreen);
    if (logo) {
	size = WMGetPixmapSize(logo);
	panel->logoL = WMCreateLabel(panel->win);
	WMResizeWidget(panel->logoL, 64, 64);
	WMMoveWidget(panel->logoL, 30, 20);
	WMSetLabelImagePosition(panel->logoL, WIPImageOnly);
	WMSetLabelImage(panel->logoL, logo);
    }

    panel->name1L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name1L, 180, 30);
    WMMoveWidget(panel->name1L, 140, 30);
    font = WMBoldSystemFontOfSize(scr->wmscreen, 24);
    if (font) {
	WMSetLabelFont(panel->name1L, font);
	WMReleaseFont(font);
    }
    WMSetLabelText(panel->name1L, "WindowMaker");

    panel->name2L = WMCreateLabel(panel->win);
    WMResizeWidget(panel->name2L, 200, 24);
    WMMoveWidget(panel->name2L, 130, 60);
    font = WMBoldSystemFontOfSize(scr->wmscreen, 18);
    if (font) {
	WMSetLabelFont(panel->name2L, font);
	WMReleaseFont(font);
	font = NULL;
    }
    WMSetLabelText(panel->name2L, "X11 Window Manager");

    
    sprintf(version, "Version %s", VERSION);
    panel->versionL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->versionL, 150, 16);
    WMMoveWidget(panel->versionL, 200, 95);
    WMSetLabelTextAlignment(panel->versionL, WARight);
    WMSetLabelText(panel->versionL, version);

    panel->copyrL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->copyrL, 340, 40);
    WMMoveWidget(panel->copyrL, 20, 190);
    WMSetLabelTextAlignment(panel->copyrL, WALeft);
    WMSetLabelText(panel->copyrL, COPYRIGHT_TEXT);
    font = WMSystemFontOfSize(scr->wmscreen, 10);
    if (font) {
	WMSetLabelFont(panel->copyrL, font);
	WMReleaseFont(font);
    }
    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 390, 237, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, "Info",
				 (scr->scr_width - 470)/2,
				 (scr->scr_height - 270)/2, 390, 237);

    wwin->window_flags.closable = 1;
    wwin->window_flags.close_button = 1;
    wWindowUpdateButtonImages(wwin);
    wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
    wwin->frame->on_click_right = destroyInfoPanel;
    
    WMMapWidget(panel->win);

    panel->done = 0;
    
    thePanel = panel;
    
    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }

    WMUnmapWidget(panel->win);

    WMDestroyWidget(panel->win);    

    wUnmanageWindow(wwin, False);

    free(panel);
    
    thePanel = NULL;
}


/*
 ***********************************************************************
 * Legal Panel
 ***********************************************************************
 */

typedef struct {
    WScreen *scr;
    
    WMWindow *win;
    
    WMLabel *licenseL;
    
    unsigned int done:1;
} LegalPanel;



#define LICENSE_TEXT \
	"    WindowMaker is free software; you can redistribute it and/or modify "\
	"it under the terms of the GNU General Public License as published "\
	"by the Free Software Foundation; either version 2 of the License, "\
	"or (at your option) any later version.\n\n\n"\
	"    WindowMaker is distributed in the hope that it will be useful, but "\
	"WITHOUT ANY WARRANTY; without even the implied warranty of "\
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "\
	"General Public License for more details.\n\n\n"\
	"    You should have received a copy of the GNU General Public License "\
	"along with this program; if not, write to the Free Software "\
	"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA "\
	"02111-1307, USA."
 

static LegalPanel *legalPanel = NULL;

static void
destroyLegalPanel(WCoreWindow *foo, void *data, XEvent *event)
{
    legalPanel->done = 1;
}


void
wShowLegalPanel(WScreen *scr)
{
    LegalPanel *panel;
    Window parent;
    WWindow *wwin;

    if (legalPanel)
	return;

    panel = wmalloc(sizeof(LegalPanel));

    panel->scr = scr;
    
    panel->win = WMCreateWindow(scr->wmscreen, "legal");
    WMResizeWidget(panel->win, 420, 250);

    
    panel->licenseL = WMCreateLabel(panel->win);
    WMResizeWidget(panel->licenseL, 400, 230);
    WMMoveWidget(panel->licenseL, 10, 10);
    WMSetLabelTextAlignment(panel->licenseL, WALeft);
    WMSetLabelText(panel->licenseL, LICENSE_TEXT);
    WMSetLabelRelief(panel->licenseL, WRGroove);

    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    parent = XCreateSimpleWindow(dpy, scr->root_win, 0, 0, 420, 250, 0, 0, 0);

    XReparentWindow(dpy, WMWidgetXID(panel->win), parent, 0, 0);

    wwin = wManageInternalWindow(scr, parent, None, "Legal",
				 (scr->scr_width - 420)/2,
				 (scr->scr_height - 250)/2, 420, 250);

    wwin->window_flags.closable = 1;
    wwin->window_flags.close_button = 1;
    wWindowUpdateButtonImages(wwin);
    wFrameWindowShowButton(wwin->frame, WFF_RIGHT_BUTTON);
    wwin->frame->on_click_right = destroyLegalPanel;
    
    WMMapWidget(panel->win);

    panel->done = 0;
    
    legalPanel = panel;
    
    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }

    WMUnmapWidget(panel->win);

    WMDestroyWidget(panel->win);    

    wUnmanageWindow(wwin, False);

    free(panel);
    
    legalPanel = NULL;
}

