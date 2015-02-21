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

#ifndef WMFRAMEWINDOW_H_
#define WMFRAMEWINDOW_H_


#define BORDER_TOP	1
#define BORDER_BOTTOM	2
#define BORDER_LEFT	4
#define BORDER_RIGHT	8
#define BORDER_ALL	(1|2|4|8)


#define WFF_TITLEBAR	(1<<0)
#define WFF_LEFT_BUTTON	(1<<1)
#define WFF_RIGHT_BUTTON (1<<2)
#define WFF_RESIZEBAR	(1<<3)
#define WFF_BORDER	(1<<4)
#define WFF_IS_MENU	(1<<5)


typedef struct WFrameWindow {
    WScreen *screen_ptr;	       /* pointer to the screen structure */
    short window_level;
    short workspace;		       /* workspace that the window occupies */
    
    WCoreWindow *core;
    
    WCoreWindow *titlebar;	       /* the titlebar */
    WCoreWindow *left_button;	       /* miniaturize button */
    WCoreWindow *right_button;	       /* close button */

    short top_width;
    short bottom_width;
    
    WCoreWindow *resizebar;	       /* bottom resizebar */

    Pixmap title_back[3];	       /* focused, unfocused, pfocused */
    Pixmap lbutton_back[3];
    Pixmap rbutton_back[3];

    WPixmap *lbutton_image;
    WPixmap *rbutton_image;
    
    union WTexture **title_texture;
    union WTexture **resizebar_texture;
    unsigned long *title_pixel;
    GC *title_gc;
    WFont **font;

    short resizebar_corner_width;

    char *title;		       /* window name (title) */

    /* thing that uses this frame. passed as data to callbacks */
    void *child;
    
    /* callbacks */
    void (*on_click_left)(WCoreWindow *sender, void *data, XEvent *event);
    
    void (*on_click_right)(WCoreWindow *sender, void *data, XEvent *event);
    void (*on_dblclick_right)(WCoreWindow *sender, void *data, XEvent *event);
    
    void (*on_mousedown_titlebar)(WCoreWindow *sender, void *data, XEvent *event);
    void (*on_dblclick_titlebar)(WCoreWindow *sender, void *data, XEvent *event);

    void (*on_mousedown_resizebar)(WCoreWindow *sender, void *data, XEvent *event);
    
    struct {
	unsigned int state:2;	       /* 3 possible states */
	unsigned int justification:2;
	unsigned int titlebar:1;
	unsigned int resizebar:1;
	unsigned int left_button:1;
	unsigned int right_button:1;
	unsigned int need_texture_remake:1;
	unsigned int menu:1;
	
	unsigned int hide_left_button:1;
	unsigned int hide_right_button:1;
	
	unsigned int need_texture_change:1;
	
	unsigned int lbutton_dont_fit:1;
	unsigned int rbutton_dont_fit:1;

	unsigned int repaint_only_titlebar:1;
	unsigned int repaint_only_resizebar:1;
	
	unsigned int is_client_window_frame:1;
    } flags;
} WFrameWindow;


WFrameWindow*
wFrameWindowCreate(WScreen *scr, int wlevel, int x, int y, 
		   int width, int height, int flags,
		   union WTexture **title_texture, 
		   union WTexture **resize_texture,
		   unsigned long *color, GC *gc, WFont **font);

void wFrameWindowUpdateBorders(WFrameWindow *fwin, int flags);

void wFrameWindowDestroy(WFrameWindow *fwin);

void wFrameWindowChangeState(WFrameWindow *fwin, int state);

void wFrameWindowPaint(WFrameWindow *fwin);

void wFrameWindowResize(WFrameWindow *fwin, int width, int height);

void wFrameWindowResizeInternal(WFrameWindow *fwin, int iwidth, int iheight);

void wFrameWindowShowButton(WFrameWindow *fwin, int flags);

void wFrameWindowHideButton(WFrameWindow *fwin, int flags);

void wFrameWindowChangeTitle(WFrameWindow *fwin, char *new_title);


#endif
