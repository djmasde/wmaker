/* action.c- misc. window commands (miniaturize, hide etc.)
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef TWIST
#include <math.h>
#endif

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "window.h"
#include "client.h"
#include "icon.h"
#include "funcs.h"
#include "application.h"
#include "actions.h"
#include "stacking.h"
#include "appicon.h"
#include "dock.h"
#include "appmenu.h"
#include "winspector.h"

#ifdef WMSOUND
#include "wmsound.h"
#endif


/****** Global Variables ******/
extern Time LastTimestamp;
extern Time LastFocusChange;

extern Cursor wCursor[WCUR_LAST];

extern WPreferences wPreferences;

extern Atom _XA_WM_TAKE_FOCUS;

/******* Local Variables *******/
static struct {
    int steps;
    int delay;
} shadePars[5] = {
    {SHADE_STEPS_UF, SHADE_DELAY_UF},
    {SHADE_STEPS_F, SHADE_DELAY_F},
    {SHADE_STEPS_M, SHADE_DELAY_M},
    {SHADE_STEPS_S, SHADE_DELAY_S},
    {SHADE_STEPS_U, SHADE_DELAY_U}};

#define SHADE_STEPS	shadePars[wPreferences.shade_speed].steps
#define SHADE_DELAY	shadePars[wPreferences.shade_speed].delay


static int ignoreTimestamp=0;


#ifdef ANIMATIONS
static void
processEvents(int event_count)
{
    XEvent event;

    /*
     * This is a hack. When animations are enabled, processing of 
     * events ocurred during a animation are delayed until it's end.
     * Calls that consider the TimeStamp, like XSetInputFocus(), will
     * fail because the TimeStamp is too old. Then, for example, if
     * the user changes window focus while a miniaturize animation is
     * in course, the window will not get focus properly after the end
     * of the animation. This tries to workaround it by passing CurrentTime
     * as the TimeStamp for XSetInputFocus() for all events ocurred during
     * the animation.
     */
    ignoreTimestamp=1;
    while (event_count--) {
	WMNextEvent(dpy, &event);
	WMHandleEvent(&event);
    }
    ignoreTimestamp=0;
}
#endif /* ANIMATIONS */


/*
 *---------------------------------------------------------------------- 
 * wSetFocusTo--
 * 	Changes the window focus to the one passed as argument.
 * If the window to focus is not already focused, it will be brought
 * to the head of the list of windows. Previously focused window is
 * unfocused. 
 * 
 * Side effects:
 * 	Window list may be reordered and the window focus is changed.
 * 
 *---------------------------------------------------------------------- 
 */
void
wSetFocusTo(WScreen *scr, WWindow  *wwin)
{
    WWindow *focused=scr->focused_window;
    WWindow *owner = NULL;
    int timestamp=LastTimestamp;
    WApplication *oapp=NULL, *napp=NULL;
    int wasfocused;

    LastFocusChange = timestamp;
    
    if (ignoreTimestamp)
      timestamp=CurrentTime;

    if (focused)
      oapp = wApplicationOf(focused->main_window);

    if (wwin==NULL) {
	XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
	if (focused) {
	    wWindowUnfocus(focused);
	}
	if (oapp) {
	    wAppMenuUnmap(oapp->menu);
#ifdef NEWAPPICON
	    wApplicationDeactivate(oapp);
#endif
	}
	return;
    }
    wasfocused = wwin->flags.focused;
    napp = wApplicationOf(wwin->main_window);

    /* remember last workspace where the app has been */
    if (napp) 
	napp->last_workspace = wwin->screen_ptr->current_workspace;
    
    if (!wwin->window_flags.focusable)
	return;

    if (wwin->flags.mapped) {
	/* install colormap if colormap mode is lock mode */
	if (wPreferences.colormap_mode==WKF_CLICK)
	  InstallWindowColormaps(wwin);
	
	/* set input focus */
	switch (wwin->focus_mode) {
	 case WFM_NO_INPUT:
	    XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
	    break;
	    
	 case WFM_PASSIVE:
	 case WFM_LOCALLY_ACTIVE:
	    XSetInputFocus(dpy, wwin->client_win, RevertToParent, timestamp);
	    break;
	    
	 case WFM_GLOBALLY_ACTIVE:
	    break;
	}
    } else {
	XSetInputFocus(dpy, scr->no_focus_win, RevertToParent, timestamp);
    }
    /* if this is not the focused window focus it */
    if (focused!=wwin) {
	int foo=0;
	
	if (wwin->client_win == focused->transient_for)
	  wwin->flags.semi_focused = 0;
	else if (wwin->transient_for == focused->client_win)
	  focused->flags.semi_focused = 1;
	
	if (wwin->transient_for 
	    && wwin->transient_for == focused->transient_for) {
	    owner = wWindowFor(wwin->transient_for);
	    if (owner && owner->flags.semi_focused) {
		foo=1;
		/* this is to override the unfocusing of the mainwindow
		 * in the next wWindowUnfocus() and avoid flickering */
		owner->flags.semi_focused = 0;
	    }
	}
	/* unfocus previous window */
	wWindowUnfocus(focused);
	if (foo) {
	    owner->flags.semi_focused = 1;
	}
	/* change the focus window list order */
	if (wwin->prev)
	  wwin->prev->next=wwin->next;
	if (wwin->next)
	  wwin->next->prev=wwin->prev;

	wwin->prev = focused;
	focused->next = wwin;
	wwin->next = NULL;
	scr->focused_window = wwin;

	if (oapp && oapp != napp) {
	    wAppMenuUnmap(oapp->menu);
#ifdef NEWAPPICON
	    wApplicationDeactivate(oapp);
#endif
	}
    }
    if ((owner=wWindowFor(wwin->transient_for)) 
	&& !owner->flags.semi_focused) {
	owner->flags.semi_focused = 1;
	wWindowUnfocus(owner);
    }
    wWindowFocus(wwin);
    if (wwin->protocols.TAKE_FOCUS) {
	wClientSendProtocol(wwin, _XA_WM_TAKE_FOCUS, timestamp);
    }
    if (napp && !wasfocused) {
	wAppMenuMap(napp->menu, wwin);
#ifdef NEWAPPICON
	wApplicationActivate(napp);
#endif
    }
    XFlush(dpy);
}


void
wShadeWindow(WWindow  *wwin)
{
    XWindowAttributes attribs;  
#ifdef ANIMATIONS    
    int y, s, w, h;
#endif
 
    if (wwin->flags.shaded)
	return;

    XLowerWindow(dpy, wwin->client_win);    

#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wwin->flags.skip_next_animation
	&& !wPreferences.no_animations) {
	/* do the shading animation */
	h = wwin->frame->core->height;
        s = h/SHADE_STEPS;
        if (s < 1) s=1;
	w = wwin->frame->core->width;
	y = wwin->frame->top_width;
	while (h>wwin->frame->top_width+1) {
	    XMoveWindow(dpy, wwin->client_win, 0, y);
	    XResizeWindow(dpy, wwin->frame->core->window, w, h);
	    XSync(dpy, 0);
            if (SHADE_DELAY > 0)
                wmsleep(SHADE_DELAY*1000L);
	    h-=s;
	    y-=s;
	}
	XMoveWindow(dpy, wwin->client_win, 0, wwin->frame->top_width);
    }
#endif  /* ANIMATIONS */

#ifdef WMSOUND
    wSoundPlay(WMSOUND_SHADE);
#endif

    wwin->flags.skip_next_animation = 0;
    XGetWindowAttributes(dpy, wwin->client_win, &attribs);    
    wwin->flags.shaded=1;
    wwin->flags.mapped=0;
    /* prevent window withdrawal when getting UnmapNotify */
    XSelectInput(dpy, wwin->client_win, 
		 attribs.your_event_mask & ~StructureNotifyMask);
    XUnmapWindow(dpy, wwin->client_win);
    XSelectInput(dpy, wwin->client_win, attribs.your_event_mask);
    
    /* for the client it's just like iconification */
    wFrameWindowResize(wwin->frame, wwin->frame->core->width,
		       wwin->frame->top_width-1);
    wClientSetState(wwin, IconicState, None);

#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup) {
    	XSync(dpy, 0);
    	processEvents(XPending(dpy));
    }
#endif
}


void
wUnshadeWindow(WWindow  *wwin)
{
#ifdef ANIMATIONS
    int y, s, w, h;    
#endif /* ANIMATIONS */


    if (!wwin->flags.shaded)
	return;
    
    wwin->flags.shaded=0;
    wwin->flags.mapped=1;
    XMapWindow(dpy, wwin->client_win);

#ifdef ANIMATIONS
    if (!wPreferences.no_animations && !wwin->flags.skip_next_animation) {
	/* do the shading animation */
	h = wwin->frame->top_width + wwin->frame->bottom_width;
	y = wwin->frame->top_width - wwin->client.height;
        s = abs(y)/SHADE_STEPS;
        if (s<1) s=1;
	w = wwin->frame->core->width;
	XMoveWindow(dpy, wwin->client_win, 0, y);
	if (s>0) {
	    while (h < wwin->client.height + wwin->frame->top_width
		   + wwin->frame->bottom_width) {
		XResizeWindow(dpy, wwin->frame->core->window, w, h);
		XMoveWindow(dpy, wwin->client_win, 0, y);
		XSync(dpy, 0);
                if (SHADE_DELAY > 0)
                    wmsleep(SHADE_DELAY*2000L/3);
		h+=s;
		y+=s;
	    }
	}
	XMoveWindow(dpy, wwin->client_win, 0, wwin->frame->top_width);
    }
#endif /* ANIMATIONS */

#ifdef WMSOUND
    wSoundPlay(WMSOUND_UNSHADE);
#endif

    wwin->flags.skip_next_animation = 0;
    wFrameWindowResize(wwin->frame, wwin->frame->core->width, 
		       wwin->frame->top_width + wwin->client.height
		       + wwin->frame->bottom_width);
    
    wClientSetState(wwin, NormalState, None);
    /* if the window is focused, set the focus again as it was disabled during
     * shading */
    if (wwin->flags.focused)
      wSetFocusTo(wwin->screen_ptr, wwin);
}


void
wMaximizeWindow(WWindow *wwin, int directions)
{
    int new_width, new_height, new_x, new_y;

    if (wwin->flags.shaded) {
	wwin->flags.skip_next_animation = 1;
	wUnshadeWindow(wwin);
    }
    wwin->flags.maximized = directions;
    wwin->old_geometry.width = wwin->client.width;
    wwin->old_geometry.height = wwin->client.height;
    wwin->old_geometry.x = wwin->frame_x;
    wwin->old_geometry.y = wwin->frame_y;
    if (directions & MAX_HORIZONTAL) {
	new_x = 2;
	new_width = wwin->screen_ptr->scr_width-FRAME_BORDER_WIDTH*2 - 4;
	
	if (wPreferences.no_window_under_dock
	    && wwin->screen_ptr->dock) {
	    new_width -= wPreferences.icon_size + DOCK_EXTRA_SPACE;
	    if (!wwin->screen_ptr->dock->on_right_side) 
		new_x += wPreferences.icon_size + DOCK_EXTRA_SPACE;
	}
    } else {
	new_x = wwin->frame_x;
	new_width = wwin->frame->core->width;
    }

    if (directions & MAX_VERTICAL) {
	new_y = 0;
	new_height = wwin->screen_ptr->scr_height-FRAME_BORDER_WIDTH*2;
	if (wPreferences.no_window_over_icons)
		new_height -= wPreferences.icon_size + 2;
    } else {
	new_y = wwin->frame_y;
	new_height = wwin->frame->core->height;
    }
    new_height -= wwin->frame->top_width+wwin->frame->bottom_width;

    wWindowConstrainSize(wwin, &new_width, &new_height);
    wWindowConfigure(wwin, new_x, new_y, new_width, new_height);

#ifdef WMSOUND
    wSoundPlay(WMSOUND_MAXIMIZE);
#endif
}


void
wUnmaximizeWindow(WWindow *wwin)
{
    int restore_x, restore_y;

    if (wwin->flags.shaded) {
	wwin->flags.skip_next_animation = 1;
	wUnshadeWindow(wwin);
    }
    restore_x = (wwin->flags.maximized & MAX_HORIZONTAL) ?
        wwin->old_geometry.x : wwin->frame_x;
    restore_y = (wwin->flags.maximized & MAX_VERTICAL) ?
        wwin->old_geometry.y : wwin->frame_y;
    wwin->flags.maximized = 0;
    wWindowConfigure(wwin, restore_x, restore_y,
		     wwin->old_geometry.width, wwin->old_geometry.height);

#ifdef WMSOUND
    wSoundPlay(WMSOUND_UNMAXIMIZE);
#endif
}

#ifdef ANIMATIONS
#ifdef TWIST
static void 
animateResize(WScreen *scr, int x, int y, int w, int h, 
	      int fx, int fy, int fw, int fh, int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES)
    float cx, cy, cw, ch;
    float xstep, ystep, wstep, hstep;
    XPoint points[5]; 
    float angle, angle_finite, a, d;

    x += w/2;
    y += h/2;
    fx += fw/2;
    fy += fh/2;

    xstep = (float)(fx-x)/MINIATURIZE_ANIMATION_STEPS;
    ystep = (float)(fy-y)/MINIATURIZE_ANIMATION_STEPS;
    wstep = (float)(fw-w)/MINIATURIZE_ANIMATION_STEPS;
    hstep = (float)(fh-h)/MINIATURIZE_ANIMATION_STEPS;
     
    cx = (float)x;
    cy = (float)y;
    cw = (float)w;
    ch = (float)h;
    a = atan(ch/cw);
    d = sqrt((cw/2)*(cw/2)+(ch/2)*(ch/2));

    angle_finite =2*WM_PI*0.5;
    for (angle=0;; angle+=(float)(2*WM_PI*MINIATURIZE_ANIMATION_TWIST/FRAMES)) {
        if (angle > angle_finite)
	  angle = angle_finite;
        points[0].x = cx+cos(angle-a)*d;
        points[0].y = cy+sin(angle-a)*d;
        points[1].x = cx+cos(angle+a)*d;
        points[1].y = cy+sin(angle+a)*d;
        points[2].x = cx+cos(angle-a+WM_PI)*d;
        points[2].y = cy+sin(angle-a+WM_PI)*d;
        points[3].x = cx+cos(angle+a+WM_PI)*d;
        points[3].y = cy+sin(angle+a+WM_PI)*d;
        points[4].x = cx+cos(angle-a)*d;
        points[4].y = cy+sin(angle-a)*d;
        XGrabServer(dpy);
        XDrawLines(dpy, scr->root_win, scr->frame_gc, points, 5, CoordModeOrigin);
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY > 0)	
 	wmsleep(MINIATURIZE_ANIMATION_DELAY);
#endif

	XDrawLines(dpy, scr->root_win, scr->frame_gc, points, 5, CoordModeOrigin);
	XUngrabServer(dpy);
	cx+=xstep;
	cy+=ystep;
	cw+=wstep;
	ch+=hstep;
	a = atan(ch/cw);
	d = sqrt((cw/2)*(cw/2)+(ch/2)*(ch/2));
	if (angle >= angle_finite)
	  break;
 
    }
    XFlush(dpy);
}

#else /* !TWIST */

static void 
animateResize(WScreen *scr, int x, int y, int w, int h, 
	      int fx, int fy, int fw, int fh, int steps)
{
#define FRAMES (MINIATURIZE_ANIMATION_FRAMES)
    float cx[FRAMES], cy[FRAMES], cw[FRAMES], ch[FRAMES];
    float xstep, ystep, wstep, hstep;
    int i, j;
        
    xstep = (float)(fx-x)/steps;
    ystep = (float)(fy-y)/steps;
    wstep = (float)(fw-w)/steps;
    hstep = (float)(fh-h)/steps;
    
    for (j=0; j<FRAMES; j++) {
	cx[j] = (float)x;
	cy[j] = (float)y;
	cw[j] = (float)w;
	ch[j] = (float)h;
    }
    XGrabServer(dpy);
    for (i=0; i<steps; i++) {
	for (j=0; j<FRAMES; j++) {
	    XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
			   (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
	}
	XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY > 0)	
	wmsleep(MINIATURIZE_ANIMATION_DELAY);
#endif
	for (j=0; j<FRAMES; j++) {
	    XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
			   (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
	    if (j<FRAMES-1) {
		cx[j]=cx[j+1];
		cy[j]=cy[j+1];
		cw[j]=cw[j+1];
		ch[j]=ch[j+1];
	    } else {
		cx[j]+=xstep;
		cy[j]+=ystep;
		cw[j]+=wstep;
		ch[j]+=hstep;
	    }
	}
    }

    for (j=0; j<FRAMES; j++) {
	XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
		       (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
    }
    XFlush(dpy);
#if (MINIATURIZE_ANIMATION_DELAY > 0)	
    wmsleep(MINIATURIZE_ANIMATION_DELAY);
#endif
    for (j=0; j<FRAMES; j++) {
	XDrawRectangle(dpy, scr->root_win, scr->frame_gc, 
		       (int)cx[j], (int)cy[j], (int)cw[j], (int)ch[j]);
    }
    
    XUngrabServer(dpy);
}
#undef FRAMES
#endif /* !TWIST */
#endif /* ANIMATIONS */

static void
flushExpose()
{
    XEvent tmpev;
    
    while (XCheckTypedEvent(dpy, Expose, &tmpev))
	WMHandleEvent(&tmpev);
    XSync(dpy, 0);
}

static void
unmapTransientsFor(WWindow *wwin)
{
    WWindow *tmp;
    XWindowAttributes attribs;
    

    tmp = wwin->screen_ptr->focused_window;
    while (tmp) {
	/* unmap the transients for this transient */
	if (tmp!=wwin && tmp->transient_for == wwin->client_win
	    && (tmp->flags.mapped || wwin->screen_ptr->flags.startup
		|| tmp->flags.shaded)) {
	    unmapTransientsFor(tmp);
	    XGetWindowAttributes(dpy, tmp->client_win, &attribs);
	    tmp->flags.miniaturized=1;
	    if (!tmp->flags.shaded) {
		tmp->flags.mapped=0;
		XSelectInput(dpy, tmp->client_win, 
			     attribs.your_event_mask & ~StructureNotifyMask);
		XUnmapWindow(dpy, tmp->client_win);
		XSelectInput(dpy, tmp->client_win, attribs.your_event_mask);
	    }
	    XUnmapWindow(dpy, tmp->frame->core->window);
	    if (!tmp->flags.shaded)
		wClientSetState(tmp, IconicState, None);
	}
	tmp = tmp->prev;
    }
}


static void
mapTransientsFor(WWindow *wwin)
{
    WWindow *tmp;

    tmp = wwin->screen_ptr->focused_window;
    while (tmp) {
	/* recursively map the transients for this transient */
	if (tmp!=wwin && tmp->transient_for == wwin->client_win
	    && /*!tmp->flags.mapped*/ tmp->flags.miniaturized 
	    && tmp->icon==NULL) {
	    mapTransientsFor(tmp);
	    tmp->flags.miniaturized=0;
	    if (!tmp->flags.shaded) {
		tmp->flags.mapped=1;
		XMapWindow(dpy, tmp->client_win);
	    }
	    XMapWindow(dpy, tmp->frame->core->window);
	    if (!tmp->flags.shaded)
		wClientSetState(tmp, NormalState, None);
	}
	tmp = tmp->prev;
    }
}


static void
setupIconGrabs(WIcon *icon)
{
    /* setup passive grabs on the icon */
    XGrabButton(dpy, Button1, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button2, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button3, AnyModifier, icon->core->window, True,
		ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XSync(dpy, 0);
}

static void
removeIconGrabs(WIcon *icon)
{
    /* remove passive grabs on the icon */
    XUngrabButton(dpy, Button1, AnyModifier, icon->core->window);
    XUngrabButton(dpy, Button2, AnyModifier, icon->core->window);
    XUngrabButton(dpy, Button3, AnyModifier, icon->core->window);    
    XSync(dpy, 0);
}


void
wIconifyWindow(WWindow *wwin)
{
    XWindowAttributes attribs;
    int present;


    if (!XGetWindowAttributes(dpy, wwin->client_win, &attribs)) {
	/* the window doesn't exist anymore */
	return;
    }

    if (wwin->flags.miniaturized)
	return;
    
    present = wwin->frame->workspace==wwin->screen_ptr->current_workspace;
 
    /* if the window is in another workspace, simplify process */
    if (present) {
	/* icon creation may take a while */
	XGrabPointer(dpy, wwin->screen_ptr->root_win, False, 
		     ButtonMotionMask|ButtonReleaseMask, GrabModeAsync, 
		     GrabModeAsync, None, None, CurrentTime);
    }
    
    if (!wwin->flags.icon_moved) {
	PlaceIcon(wwin->screen_ptr, &wwin->icon_x, &wwin->icon_y);
    }
    wwin->icon = wIconCreate(wwin);

    wwin->flags.miniaturized=1;
    wwin->flags.mapped=0;

    /* unmap transients */
    
    unmapTransientsFor(wwin);

    if (present) {
	XUngrabPointer(dpy, CurrentTime);
	/* prevent window withdrawal when getting UnmapNotify */
	XSelectInput(dpy, wwin->client_win, 
		     attribs.your_event_mask & ~StructureNotifyMask);
	XUnmapWindow(dpy, wwin->client_win);
	XSelectInput(dpy, wwin->client_win, attribs.your_event_mask);
	XUnmapWindow(dpy, wwin->frame->core->window);
	/* let all Expose events arrive so that we can repaint
	 * something before the animation starts (and the server is grabbed) */
	XSync(dpy, 0);
	wClientSetState(wwin, IconicState, wwin->icon->icon_win);
	flushExpose();
#ifdef ANIMATIONS
	if (!wwin->screen_ptr->flags.startup && !wwin->flags.skip_next_animation
	    && !wPreferences.no_animations) {
	    animateResize(wwin->screen_ptr, wwin->frame_x, wwin->frame_y, 
			  wwin->frame->core->width, wwin->frame->core->height,
			  wwin->icon_x, wwin->icon_y,
			  wwin->icon->core->width, wwin->icon->core->height,
			  MINIATURIZE_ANIMATION_STEPS);
	}
#endif
    }
    
    wwin->flags.skip_next_animation = 0;
    if (wwin->screen_ptr->current_workspace==wwin->frame->workspace ||
        wwin->window_flags.omnipresent || wPreferences.sticky_icons)
        XMapWindow(dpy, wwin->icon->core->window);
    AddToStackList(wwin->icon->core);
#ifndef STRICTNS
    wLowerFrame(wwin->icon->core);
#endif

    if (present) {
	setupIconGrabs(wwin->icon);
	if (wwin->flags.focused && wPreferences.focus_mode==WKF_CLICK) {
	    WWindow *tmp;
	    
	    tmp = wwin->prev;
	    while (tmp) {
		if (tmp->window_flags.focusable 
		    && !(tmp->flags.hidden||tmp->flags.miniaturized))
		    break;
		tmp = tmp->prev;
	    }
	    wSetFocusTo(wwin->screen_ptr, tmp);
	} else if (wPreferences.focus_mode!=WKF_CLICK) {
	    wSetFocusTo(wwin->screen_ptr, NULL);
	}
	
#ifdef ANIMATIONS
	if (!wwin->screen_ptr->flags.startup) {
	    XSync(dpy, 0);
	    processEvents(XPending(dpy));
	}
#endif
    }
    
    if (wwin->flags.selected)
        wIconSelect(wwin->icon);

#ifdef WMSOUND
    if (present) {
	wSoundPlay(WMSOUND_ICONIFY);
    }
#endif
}


void
wDeiconifyWindow(WWindow  *wwin)
{
    wWindowChangeWorkspace(wwin, wwin->screen_ptr->current_workspace);

    if (!wwin->flags.miniaturized)
	return;
    
    wwin->flags.miniaturized=0;
    if (!wwin->flags.shaded)
	wwin->flags.mapped=1;

    if (wwin->icon->selected)
        wIconSelect(wwin->icon);

    XUnmapWindow(dpy, wwin->icon->core->window);
    
    /* if the window is in another workspace, do it silently */
#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations 
	&& !wwin->flags.skip_next_animation) {
	animateResize(wwin->screen_ptr, wwin->icon_x, wwin->icon_y,
		      wwin->icon->core->width, wwin->icon->core->height,
		      wwin->frame_x, wwin->frame_y, 
		      wwin->frame->core->width, wwin->frame->core->height,
		      MINIATURIZE_ANIMATION_STEPS);
    }
#endif /* ANIMATIONS */
    wwin->flags.skip_next_animation = 0;
    XGrabServer(dpy);
    if (!wwin->flags.shaded) {
	XMapWindow(dpy, wwin->client_win);
    }
    XMapWindow(dpy, wwin->frame->core->window);
    wRaiseFrame(wwin->frame->core);
    if (!wwin->flags.shaded) {
	wClientSetState(wwin, NormalState, None);
    }
    mapTransientsFor(wwin);
    RemoveFromStackList(wwin->icon->core);
    removeIconGrabs(wwin->icon);
    wIconDestroy(wwin->icon);
    wwin->icon = NULL;

    XUngrabServer(dpy);
    if (wPreferences.focus_mode==WKF_CLICK
	|| wPreferences.focus_mode==WKF_SLOPPY)
      wSetFocusTo(wwin->screen_ptr, wwin);
#ifdef ANIMATIONS
    if (!wwin->screen_ptr->flags.startup) {
    	XSync(dpy, 0);
    	processEvents(XPending(dpy));
    }
#endif
    
    if (wPreferences.auto_arrange_icons) {
        wArrangeIcons(wwin->screen_ptr);
    }

#ifdef WMSOUND
    wSoundPlay(WMSOUND_DEICONIFY);
#endif
}



static void
hideWindow(WIcon *icon, int icon_x, int icon_y, WWindow *wwin, int animate)
{
    XWindowAttributes attribs;
    
    
    if (wwin->flags.miniaturized) {
	XUnmapWindow(dpy, wwin->icon->core->window);
	wwin->flags.hidden = 1;
	wwin->icon->mapped = 0;
	return;
    }

    if (wwin->flags.inspector_open) {
        WWindow *pwin = wwin->inspector->frame;

        XGetWindowAttributes(dpy, pwin->client_win, &attribs);
        pwin->flags.hidden = 1;
        pwin->flags.mapped = 0;
        /* prevent window withdrawal when getting UnmapNotify */
        XSelectInput(dpy, pwin->client_win,
                     attribs.your_event_mask & ~StructureNotifyMask);
        XUnmapWindow(dpy, pwin->client_win);
        XSelectInput(dpy, pwin->client_win, attribs.your_event_mask);

        XUnmapWindow(dpy, pwin->frame->core->window);
        wClientSetState(pwin, IconicState, icon->icon_win);
    }

    XGetWindowAttributes(dpy, wwin->client_win, &attribs);

    wwin->flags.hidden = 1;
    wwin->flags.mapped = 0;

    /* prevent window withdrawal when getting UnmapNotify */
    XSelectInput(dpy, wwin->client_win, 
		 attribs.your_event_mask & ~StructureNotifyMask);
    XUnmapWindow(dpy, wwin->client_win);
    XSelectInput(dpy, wwin->client_win, attribs.your_event_mask);

    XUnmapWindow(dpy, wwin->frame->core->window);
    wClientSetState(wwin, IconicState, icon->icon_win);
    flushExpose();
#ifdef ANIMATIONS
# if HIDE_ANIMATION_STEPS > 0
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations &&
	!wwin->flags.skip_next_animation && animate) {
	animateResize(wwin->screen_ptr, wwin->frame_x, wwin->frame_y, 
		      wwin->frame->core->width, wwin->frame->core->height,
		      icon_x, icon_y, icon->core->width, icon->core->height,
		      HIDE_ANIMATION_STEPS);
    }
# endif
#endif
    wwin->flags.skip_next_animation = 0;

#ifdef WMSOUND
    wSoundPlay(WMSOUND_HIDE);
#endif
}



void
wHideOtherApplications(WWindow *awin)
{
    WWindow *wwin;
    WApplication *tapp;
#ifdef REDUCE_APPICONS
    char *tinstance, *tclass;
    unsigned int brokenwin = 0, match = 0;
#endif

    if (!awin)
	return;
    wwin = awin->screen_ptr->focused_window;

#ifdef REDUCE_APPICONS
    if (awin->wm_instance == NULL || awin->wm_class == NULL)
	brokenwin++;
#endif

    while (wwin) {
	if (wwin!=awin 
	    && wwin->frame->workspace == awin->screen_ptr->current_workspace
            && !(wwin->flags.miniaturized||wwin->flags.hidden)
            && !wwin->flags.internal_window
            && (!wwin->flags.inspector_open || wwin->inspector->frame!=awin)
	    && wwin->window_flags.hide_others) {
	    
#ifdef REDUCE_APPICONS
	    match = 0;
	    if (!brokenwin) {
		if ((tinstance = wwin->wm_instance) == NULL)
		    tinstance = "";
		if ((tclass = wwin->wm_class) == NULL)
		    tclass = "";
		if ((strcmp(awin->wm_instance, tinstance) == 0) &&
		    (strcmp(awin->wm_class, tclass) == 0) )
			match++;
	    }
#endif

	    if (wwin->main_window==None) {
		if (wwin->window_flags.miniaturizable) {
		    wwin->flags.skip_next_animation = 1;
		    wIconifyWindow(wwin);
		}
	    } else if (wwin->main_window!=None
#ifndef REDUCE_APPICONS
		       && awin->main_window != wwin->main_window) {
#else
			&& (awin->main_window != wwin->main_window && !match)) {
#endif
		tapp = wApplicationOf(wwin->main_window);
		if (tapp) {
		    tapp->flags.skip_next_animation = 1;
		    wHideApplication(tapp);
		} else {
		    if (wwin->window_flags.miniaturizable) {
		    	wwin->flags.skip_next_animation = 1;
		   	 wIconifyWindow(wwin);
		    }
		}
	    }
	}
	wwin = wwin->prev;
    }
    /*
    wSetFocusTo(awin->screen_ptr, awin);
     */
}



void
wHideApplication(WApplication *wapp)
{
#ifdef REDUCE_APPICONS
    WApplication *tapp;
    char *tinstance, *tclass;
    unsigned int nowmhints = 0, matchwmhints = 0, matchworkspace = 0;
#endif
    WScreen *scr;
    WWindow *wlist;
    int hadfocus;
    
    if (!wapp) {
	wwarning("trying to hide a non grouped window");
	return;
    }
    if (!wapp->main_window_desc) {
	wwarning("group leader not found for window group");
	return;
    }
#ifdef REDUCE_APPICONS
    if ((wapp->main_window_desc->wm_instance == NULL) ||
        (wapp->main_window_desc->wm_class == NULL))
        nowmhints++;
#endif
    scr = wapp->main_window_desc->screen_ptr;
    hadfocus = 0;
    wlist = scr->focused_window;
    if (!wlist)
	return;

    if (wlist->main_window == wapp->main_window)
	wapp->last_focused = wlist;
    else
	wapp->last_focused = NULL;
    while (wlist) {
#ifndef REDUCE_APPICONS
	if (wlist->main_window == wapp->main_window) {
#else
	matchwmhints = matchworkspace = 0;
	if (!nowmhints) {
	    tapp = wApplicationOf(wlist->main_window);
	    tinstance = tclass = NULL;
	    if (tapp) {
		if (tapp->main_window_desc) {
		    tinstance = tapp->main_window_desc->wm_instance;
		    tclass = tapp->main_window_desc->wm_class;
		}
	    }
	    if (tapp == NULL || tinstance == NULL || tclass == NULL) {
		/* Should never reach here */
		tinstance = "";
		tclass = "";
	    }
	    if ((strcmp(tinstance, wapp->main_window_desc->wm_instance) == 0) &&
		(strcmp(tclass, wapp->main_window_desc->wm_class) == 0) )
		matchwmhints++;
	}
	if (wlist->frame) {
	    if (wlist->frame->workspace == wapp->main_window_desc->screen_ptr->current_workspace)
		matchworkspace++;
	}
        if ((wlist->main_window == wapp->main_window || matchwmhints) && 
	    matchworkspace) {
#endif
	    if (wlist->flags.focused) {
		hadfocus = 1;
            }
            if (wapp->app_icon)
                hideWindow(wapp->app_icon->icon, wapp->app_icon->x_pos,
                           wapp->app_icon->y_pos, wlist,
                           !wapp->flags.skip_next_animation);
	}
	wlist = wlist->prev;
    }
    
    wapp->flags.skip_next_animation = 0;    

    if (hadfocus) {
	if (wPreferences.focus_mode==WKF_CLICK) {
	    wlist = scr->focused_window;
	    while (wlist) {
		if (wlist->window_flags.focusable && !wlist->flags.hidden
		    && (wlist->flags.mapped || wlist->flags.shaded))
		  break;
		wlist = wlist->prev;
	    }
	    wSetFocusTo(scr, wlist);
	} else {
	    wSetFocusTo(scr, NULL);
	}
    }
    
    wapp->flags.hidden = 1;
}




static void
unhideWindow(WIcon *icon, int icon_x, int icon_y, WWindow *wwin, int animate)
{
    wWindowChangeWorkspace(wwin, wwin->screen_ptr->current_workspace);

    wwin->flags.hidden=0;
    wwin->flags.mapped=1;
#ifdef ANIMATIONS
#if HIDE_ANIMATION_STEPS > 0
    if (!wwin->screen_ptr->flags.startup && !wPreferences.no_animations &&
	!wwin->flags.skip_next_animation && animate) {
	animateResize(wwin->screen_ptr, icon_x, icon_y,
		      icon->core->width, icon->core->height,
		      wwin->frame_x, wwin->frame_y, 
		      wwin->frame->core->width, wwin->frame->core->height,
                      HIDE_ANIMATION_STEPS);
    }
#endif
#endif
    wwin->flags.skip_next_animation = 0;
    XMapWindow(dpy, wwin->client_win);
    XMapWindow(dpy, wwin->frame->core->window);
    wClientSetState(wwin, NormalState, None);
    wRaiseFrame(wwin->frame->core);
    if (wwin->flags.inspector_open) {
        WWindow *pwin = wwin->inspector->frame;

        pwin->flags.hidden = 0;
        pwin->flags.mapped = 1;
        XMapWindow(dpy, pwin->client_win);
        XMapWindow(dpy, pwin->frame->core->window);
        wClientSetState(pwin, NormalState, None);
    }

#ifdef WMSOUND
    wSoundPlay(WMSOUND_UNHIDE);
#endif
}


void
wUnhideApplication(WApplication *wapp, Bool miniwindows, Bool bringToCurrentWS)
{
    WScreen *scr;
    WWindow *wlist, *next;
    WWindow *focused=NULL;
#ifdef REDUCE_APPICONS
    char *tinstance, *tclass;
    unsigned int nowmhints = 0, matchwmhints = 0, matchworkspace = 0;
#endif

    if (!wapp) {
	return;
    }

#ifdef REDUCE_APPICONS
    if ((wapp->main_window_desc->wm_class == NULL) || 
	(wapp->main_window_desc->wm_instance == NULL))
	nowmhints++;
#endif

    scr = wapp->main_window_desc->screen_ptr;
    wlist = scr->focused_window;
    if (!wlist) return;
    /* goto beginning of list */
    while (wlist->prev)
      wlist = wlist->prev;
    
    while (wlist) {
	next = wlist->next;

#ifndef REDUCE_APPICONS	
	if (wlist->main_window == wapp->main_window) {
#else
	matchwmhints = matchworkspace = 0;
	if (!nowmhints) {
	    if ((tinstance = wlist->wm_instance) == NULL)
		tinstance = "";
	    if ((tclass = wlist->wm_class) == NULL)
		tclass = "";
	    if ((strcmp(tinstance, wapp->main_window_desc->wm_instance) == 0) 
		&& (strcmp(tclass, wapp->main_window_desc->wm_class) == 0) )
		matchwmhints++;
	}
	if (wlist->frame) {
	    if (wlist->frame->workspace == wapp->main_window_desc->screen_ptr->current_workspace)
		matchworkspace++;
	}

	if ((wlist->main_window == wapp->main_window || matchwmhints) && 
	    matchworkspace) {
#endif
	    if (wlist->flags.focused)
	      focused = wlist;
	    else if (!focused || !focused->flags.focused)
	      focused = wlist;
	    
	    if (wlist->flags.miniaturized && wlist->icon) {
                if (bringToCurrentWS || wPreferences.sticky_icons
		    || wlist->frame->workspace == scr->current_workspace) {
		    if (!wlist->icon->mapped) {
			XMapWindow(dpy, wlist->icon->core->window);
			wlist->icon->mapped = 1;
		    }
		    wlist->flags.hidden = 0;
		    if (wlist->frame->workspace != scr->current_workspace)
			wWindowChangeWorkspace(wlist, scr->current_workspace);
		}
		if (miniwindows) {
		    wDeiconifyWindow(wlist);
		}
	    } else if (wlist->flags.hidden) {
		unhideWindow(wapp->app_icon->icon, wapp->app_icon->x_pos,
			     wapp->app_icon->y_pos, wlist, 
			     wapp->flags.skip_next_animation);
	    } else {
		if (bringToCurrentWS
		    && wlist->frame->workspace != scr->current_workspace) {
		    wWindowChangeWorkspace(wlist, scr->current_workspace);
		}
		wRaiseFrame(wlist->frame->core);
	    }
	}
	wlist = next;
    }
    
    wapp->flags.skip_next_animation = 0;
    wapp->flags.hidden = 0;
    
    if (focused)
	wSetFocusTo(scr, focused);
    else if (wapp->last_focused)
	wSetFocusTo(scr, wapp->last_focused);
    wapp->last_focused = NULL;
    if (wPreferences.auto_arrange_icons) {
        wArrangeIcons(scr);
    }
}



void
wShowAllWindows(WScreen *scr)
{
    WWindow *wwin, *old_foc;
    WApplication *wapp;

    old_foc = wwin = scr->focused_window;
    while (wwin) {
        if (!wwin->flags.internal_window &&
            (scr->current_workspace == wwin->frame->workspace
	    || wwin->window_flags.omnipresent)) {
	    if (wwin->flags.miniaturized) {
		wwin->flags.skip_next_animation = 1;
		wDeiconifyWindow(wwin);
	    } else if (wwin->flags.hidden) {
		wapp = wApplicationOf(wwin->main_window);
		if (wapp) {
		    wUnhideApplication(wapp, False, False);
		} else {
		    wwin->flags.skip_next_animation = 1;
		    wDeiconifyWindow(wwin);
		}
	    }
	}
	wwin = wwin->prev;
    }
    wSetFocusTo(scr, old_foc);
    /*wRaiseFrame(old_foc->frame->core);*/
}


void
wRefreshDesktop(WScreen *scr)
{
    Window win;
    XSetWindowAttributes attr;

    attr.backing_store = NotUseful;
    attr.save_under = False;
    win = XCreateWindow(dpy, scr->root_win, 0, 0, scr->scr_width, 
			scr->scr_height, 0, CopyFromParent, CopyFromParent,
			(Visual *) CopyFromParent, CWBackingStore|CWSaveUnder,
			&attr);
    XMapWindow(dpy, win);
    XDestroyWindow(dpy, win);
    XFlush(dpy);
}


void
wArrangeIcons(WScreen *scr)
{
    WWindow *wwin = scr->focused_window;
    WAppIcon *aicon;
    int left_margin, right_margin;
    int x, y;

    
    left_margin = 0;
    right_margin = scr->scr_width;

    if (scr->dock) {
	if (scr->dock->on_right_side)
            right_margin -= wPreferences.icon_size + DOCK_EXTRA_SPACE;
        else
	    left_margin += wPreferences.icon_size + DOCK_EXTRA_SPACE;
    }


    /* arrange icons putting the most recently focused window
     * as the last icon */
    x = left_margin;
    y = scr->scr_height - wPreferences.icon_size;

    /* arrange application icons */
    aicon = scr->app_icon_list;
    /* reverse them to avoid unnecessarily sliding of icons */
    while (aicon && aicon->next)
        aicon = aicon->next;

    while (aicon) {
        if (!aicon->docked) {
            if (aicon->x_pos != x || aicon->y_pos != y) {
#ifdef ANIMATIONS
                if (wPreferences.no_animations) {
                    XMoveWindow(dpy, aicon->icon->core->window, x, y);
                } else {
                    SlideWindow(aicon->icon->core->window, aicon->x_pos, aicon->y_pos,
                                x, y);
                }
#else
                XMoveWindow(dpy, aicon->icon->core->window, x, y);
#endif /* ANIMATIONS */
            }
            aicon->x_pos = x;
            aicon->y_pos = y;
            x += wPreferences.icon_size;
        }
        /* we reversed the order so we use prev */
        aicon = aicon->prev;
        if (x >= right_margin) {
            x = left_margin;
            y -= wPreferences.icon_size;
        }
    }
    
    /* arrange miniwindows */

    wwin = scr->focused_window;
    /* reverse them to avoid unnecessarily shuffling */
    while (wwin && wwin->prev)
        wwin = wwin->prev;

    while (wwin) { 
        if (wwin->icon && wwin->flags.miniaturized &&
            (wwin->frame->workspace==scr->current_workspace ||
             wwin->window_flags.omnipresent ||
             wPreferences.sticky_icons)) {
	    if (wwin->icon_x != x || wwin->icon_y != y) {
#ifdef ANIMATIONS
		if (wPreferences.no_animations) {
		    XMoveWindow(dpy, wwin->icon->core->window, x, y);
		} else {
		    SlideWindow(wwin->icon->core->window, wwin->icon_x, 
				wwin->icon_y, x, y);
		}
#else
		XMoveWindow(dpy, wwin->icon->core->window, x, y);
#endif /* ANIMATIONS */
	    }
	    wwin->icon_x = x;
	    wwin->icon_y = y;
	    wwin->flags.icon_moved = 0;
	    x += wPreferences.icon_size;
	}
        /* we reversed the order, so we use next */
        wwin = wwin->next;
	if (x >= right_margin) {
	    x = left_margin;
	    y -= wPreferences.icon_size;
	}
    }
}


