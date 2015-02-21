/* 
 * WindowMaker interface definitions
 * 
 * Copyright (C) 1997 Alfredo K. Kojima
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *  
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _WMLIB_H_
#define _WMLIB_H_

/* the definitions in this file can change at any time. WINGs has more
 * stable definitions */

#include <X11/Xlib.h>
#include <X11/Xmd.h>

typedef struct {
    CARD32 flags;
    CARD32 window_style;
    CARD32 window_level;
    CARD32 reserved;
    Pixmap miniaturize_pixmap;	       /* pixmap for miniaturize button */
    Pixmap close_pixmap;	       /* pixmap for close button */
    Pixmap miniaturize_mask;	       /* miniaturize pixmap mask */
    Pixmap close_mask;		       /* close pixmap mask */
    CARD32 extra_flags;
} GNUstepWMAttributes;

#define GSWindowStyleAttr 	(1<<0)
#define GSWindowLevelAttr 	(1<<1)
#define GSMiniaturizePixmapAttr	(1<<3)
#define GSClosePixmapAttr	(1<<4)
#define GSMiniaturizeMaskAttr	(1<<5)
#define GSCloseMaskAttr		(1<<6)
#define GSExtraFlagsAttr       	(1<<7)



#define GSClientResizeFlag	(1<<0)
#define GSFullKeyboardEventsFlag (1<<1)
#define GSMenuWindowFlag	(1<<2)
#define GSIconWindowFlag	(1<<3)
#define GSSkipWindowListFlag	(1<<4)
#define GSNoApplicationIconFlag	(1<<5)
#define GSDarkGrayTitlebarFlag	(1<<8)


#define WMFHideOtherApplications	10
#define WMFHideApplication		12


#if !defined(_NSWindow_h_) && !defined(_GNUstep_H_GUITypes)
enum {
  NSNormalWindowLevel   = 0,
  NSFloatingWindowLevel  = 3,
  NSDockWindowLevel   = 5,
  NSSubmenuWindowLevel  = 10,
  NSMainMenuWindowLevel  = 20
};

enum {
  NSBorderlessWindowMask = 0,
  NSTitledWindowMask = 1,
  NSClosableWindowMask = 2,
  NSMiniaturizableWindowMask = 4,
  NSResizableWindowMask = 8
};
#endif

typedef struct _wmAppContext WMAppContext;

typedef struct _wmMenu WMMenu;

typedef void (*WMMenuAction)(void *clientdata, int code, Time timestamp);

typedef void (*WMFreeFunction)(void *clientdata);

int WMProcessEvent(WMAppContext *app, XEvent *event);


WMAppContext *WMAppCreateWithMain(Display *display, int screen_number,
				  Window main_window);

WMAppContext *WMAppCreate(Display *display, int screen_number);

int WMAppAddWindow(WMAppContext *app, Window window);

int WMAppSetMainMenu(WMAppContext *app, WMMenu *menu);


int WMRealizeMenus(WMAppContext *app);


void WMSetWindowAttributes(Display *dpy, Window window, 
			   GNUstepWMAttributes *attributes);


void WMHideApplication(WMAppContext *app);
void WMHideOthers(WMAppContext *app);

WMMenu *WMMenuCreate(WMAppContext *app, char *title);

int WMMenuAddItem(WMMenu *menu, char *text, WMMenuAction action,
		  void *clientData, WMFreeFunction freedata, char *rtext);

int WMMenuInsertItem(WMMenu *menu, int index, char *text, 
		     WMMenuAction *action, char *rtext);

int WMMenuRemoveItem(WMMenu *menu, int index);

int WMMenuAddSubmenu(WMMenu *menu, char *title, WMMenu *submenu);

void WMMenuSetEnabled(WMMenu *menu, int index, int enabled);

void WMMenuDestroy(WMMenu *menu, int submenus);


#endif
