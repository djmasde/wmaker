/* icon.c - window icon and dock and appicon parent
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
#include <string.h>
#include <ctype.h>
#include <wraster.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "texture.h"
#include "window.h"
#include "icon.h"
#include "actions.h"
#include "funcs.h"
#include "stacking.h"
#include "application.h"
#include "defaults.h"
#include "appicon.h"

/**** Global variables ****/
extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask

extern Cursor wCursor[WCUR_LAST];


static void miniwindowExpose(WObjDescriptor *desc, XEvent *event);
static void miniwindowMouseDown(WObjDescriptor *desc, XEvent *event);
static void miniwindowDblClick(WObjDescriptor *desc, XEvent *event);


INLINE static void
getSize(Drawable d, unsigned int *w, unsigned int *h, unsigned int *dep)
{
    Window rjunk;
    int xjunk, yjunk;
    unsigned int bjunk;
    
    XGetGeometry(dpy, d, &rjunk, &xjunk, &yjunk, w, h, &bjunk, dep);
}


WIcon*
wIconCreate(WWindow *wwin)
{
    WScreen *scr=wwin->screen_ptr;
    WIcon *icon;
    char *file;
    unsigned long vmask = 0;
    XSetWindowAttributes attribs;
    
    icon = wmalloc(sizeof(WIcon));
    memset(icon, 0, sizeof(WIcon));
    icon->core = wCoreCreateTopLevel(scr, wwin->icon_x, wwin->icon_y,
				     wPreferences.icon_size, 
				     wPreferences.icon_size, 0);
    
    if (wPreferences.use_saveunders) {
	vmask |= CWSaveUnder;
	attribs.save_under = True;
    }
    /* a white border for selecting it */
    vmask |= CWBorderPixel;
    attribs.border_pixel = scr->white_pixel;
    
    XChangeWindowAttributes(dpy, icon->core->window, vmask, &attribs);

    
    /* will be overriden if this is an application icon */
    icon->core->descriptor.handle_mousedown = miniwindowMouseDown;
    icon->core->descriptor.handle_expose = miniwindowExpose;
    icon->core->descriptor.parent_type = WCLASS_MINIWINDOW;
    icon->core->descriptor.parent = icon;
    
    icon->core->stacking = wmalloc(sizeof(WStacking));
    icon->core->stacking->above = NULL;
    icon->core->stacking->under = NULL;
    icon->core->stacking->window_level = WMNormalWindowLevel;
    icon->core->stacking->child_of = NULL;

    icon->owner = wwin;
    if (wwin->wm_hints && (wwin->wm_hints->flags & IconWindowHint)) {
	if (wwin->client_win == wwin->main_window) {
	    WApplication *wapp;
	    /* do not let miniwindow steal app-icon's icon window */
	    wapp = wApplicationOf(wwin->client_win);
	    if (!wapp || wapp->app_icon==NULL)
		icon->icon_win = wwin->wm_hints->icon_window;
	} else {
	    icon->icon_win = wwin->wm_hints->icon_window;
	}
    }
#ifdef NO_MINIWINDOW_TITLES
    icon->show_title = 0;
#else
    icon->show_title = 1;
#endif
    icon->image = wDefaultGetImage(scr, wwin->wm_instance, wwin->wm_class);

    file = wDefaultGetIconFile(scr, wwin->wm_instance, wwin->wm_class);
    if (file) {
        icon->file = wstrdup(file);
    }

    MyXGetIconName(dpy, wwin->client_win, &icon->icon_name);

    wIconUpdate(icon);
    
    XFlush(dpy);
    
    return icon;
}


WIcon*
wIconCreateWithName(WScreen *scr, char *instance, char *class)
{
    WIcon *icon;
    char *file;
    unsigned long vmask = 0;
    XSetWindowAttributes attribs;

    icon = wmalloc(sizeof(WIcon));
    memset(icon, 0, sizeof(WIcon));
    icon->core = wCoreCreateTopLevel(scr, 0, 0, wPreferences.icon_size, 
				     wPreferences.icon_size, 0);
    if (wPreferences.use_saveunders) {
	vmask = CWSaveUnder;
	attribs.save_under = True;
    }
    /* a white border for selecting it */
    vmask |= CWBorderPixel;
    attribs.border_pixel = scr->white_pixel;
	
    XChangeWindowAttributes(dpy, icon->core->window, vmask, &attribs);

    /* will be overriden if this is a application icon */
    icon->core->descriptor.handle_mousedown = miniwindowMouseDown;
    icon->core->descriptor.handle_expose = miniwindowExpose;
    icon->core->descriptor.parent_type = WCLASS_MINIWINDOW;
    icon->core->descriptor.parent = icon;

    icon->core->stacking = wmalloc(sizeof(WStacking));
    icon->core->stacking->above = NULL;
    icon->core->stacking->under = NULL;
    icon->core->stacking->window_level = WMNormalWindowLevel;
    icon->core->stacking->child_of = NULL;

    icon->image = wDefaultGetImage(scr, instance, class);

    file = wDefaultGetIconFile(scr, instance, class);
    if (file) {
        icon->file = wstrdup(file);
    }

    wIconUpdate(icon);

    return icon;
}



void
wIconDestroy(WIcon *icon)
{
    WCoreWindow *core = icon->core;
    WScreen *scr = core->screen_ptr;

    if (icon->handlerID)
	WMDeleteTimerHandler(icon->handlerID);

    if (icon->icon_win) {
	int x=0, y=0;

	if (icon->owner) {
	    x = icon->owner->icon_x;
	    y = icon->owner->icon_y;
	}
	XUnmapWindow(dpy, icon->icon_win);
	XReparentWindow(dpy, icon->icon_win, scr->root_win, x, y);
    }
    if (icon->icon_name)
      XFree(icon->icon_name);

    if (icon->pixmap)
      XFreePixmap(dpy, icon->pixmap);

    if (icon->file)
        free(icon->file);

    if (icon->image!=NULL)
	RDestroyImage(icon->image);

    wCoreDestroy(icon->core);
    free(icon);
}



static void
drawIconTitle(WScreen *scr, Pixmap pixmap, int height)
{
    XFillRectangle(dpy, pixmap, scr->icon_title_texture->normal_gc,
		   0, 0, wPreferences.icon_size, height+1);
    XDrawLine(dpy, pixmap, scr->icon_title_texture->light_gc, 0, 0,
	      wPreferences.icon_size, 0);
    XDrawLine(dpy, pixmap, scr->icon_title_texture->light_gc, 0, 0,
	      0, height+1);
    XDrawLine(dpy, pixmap, scr->icon_title_texture->dim_gc, 
	      wPreferences.icon_size-1, 0, wPreferences.icon_size-1, height+1);
}


static Pixmap
makeIcon(WScreen *scr, RImage *icon, int titled, int shadowed)
{
    RImage *tile;
    Pixmap pixmap;
    int x, y, w, h, sx, sy;
    int theight = scr->icon_title_font->height;
    
    tile = RCloneImage(scr->icon_tile);

    if (icon) {
	w = (icon->width > wPreferences.icon_size) 
	    ? wPreferences.icon_size : icon->width;
	x = (wPreferences.icon_size - w) / 2;
	sx = (icon->width - w)/2;
	
	if (!titled) {
	    h = (icon->height > wPreferences.icon_size) 
		? wPreferences.icon_size : icon->height;
	    y = (wPreferences.icon_size - h) / 2;
	    sy = (icon->height - h)/2;
	} else {
	    h = (icon->height+theight > wPreferences.icon_size
		 ? wPreferences.icon_size-theight : icon->height);
	    y = theight + (wPreferences.icon_size - icon->height - theight)/2;
	    sy = (icon->height - h)/2;
	}
	sx=sy=0;
	RCombineArea(tile, icon, sx, sy, w, h, x, y);
    }

    if (shadowed) {
        RColor color;

        color.red   = scr->icon_back_texture->light.red   >> 8;
        color.green = scr->icon_back_texture->light.green >> 8;
        color.blue  = scr->icon_back_texture->light.blue  >> 8;
        color.alpha = 150; /* about 60% */
        RClearImage(tile, &color);
    }

    if (!RConvertImage(scr->rcontext, tile, &pixmap)) {
	wwarning(_("error rendering image:%s"), RErrorString);
    }
    RDestroyImage(tile);

    if (titled)
      drawIconTitle(scr, pixmap, theight);

    return pixmap;
}


void
wIconChangeTitle(WIcon *icon, char *new_title)
{
    int changed;

    changed = (new_title==NULL && icon->icon_name!=NULL)
	|| (new_title!=NULL && icon->icon_name==NULL);

    if (icon->icon_name!=NULL)
	XFree(icon->icon_name);

    icon->icon_name = new_title;

    if (changed)
        icon->force_paint = 1;
    wIconPaint(icon);
}


void
wIconChangeImage(WIcon *icon, RImage *new_image)
{
    assert(icon != NULL);
    
    if (icon->image)
        RDestroyImage(icon->image);

    icon->image = new_image;
    wIconUpdate(icon);
}


Bool
wIconChangeImageFile(WIcon *icon, char *file)
{
    WScreen *scr = icon->core->screen_ptr;
    RImage *image, *tmp;
    char *path;
    int error = 0;

    if (!file) {
	wIconChangeImage(icon, NULL);
	return True;
    }

    path = FindImage(wPreferences.icon_path, file);

    if (path && (image = RLoadImage(scr->rcontext, path, 0))) {
	if (wPreferences.icon_size!=ICON_WIDTH) {
	    int w = image->width*wPreferences.icon_size/ICON_WIDTH;
	    int h = image->height*wPreferences.icon_size/ICON_HEIGHT;
	    
	    tmp = RScaleImage(image, w, h);
	    RDestroyImage(image);
	    image = tmp;
	}
	wIconChangeImage(icon, image);
    } else {
	error = 1;
    }

    if (path)
	free(path);

    return !error;
}


void
wIconChangeIconWindow(WIcon *icon, Window new_window);


static void 
cycleColor(void *data)
{
    WIcon *icon = (WIcon*)data;
    WScreen *scr = icon->core->screen_ptr;

    icon->step = !icon->step;
    if (icon->step) {
	XSetForeground(dpy, scr->icon_select_gc, scr->white_pixel);
	XSetBackground(dpy, scr->icon_select_gc, scr->black_pixel);
    } else {
	XSetForeground(dpy, scr->icon_select_gc, scr->black_pixel);
	XSetBackground(dpy, scr->icon_select_gc, scr->white_pixel);
    }
    XDrawRectangle(dpy, icon->core->window, scr->icon_select_gc, 0, 0,
		   icon->core->width-1, icon->core->height-1);    
    icon->handlerID = WMAddTimerHandler(COLOR_CYCLE_DELAY, cycleColor, icon);
}


void
wIconSelect(WIcon *icon)
{
    icon->selected = !icon->selected;

    if (icon->selected) {
        icon->step = 0;
        icon->handlerID = WMAddTimerHandler(COLOR_CYCLE_DELAY,cycleColor, icon);
    } else {
        if (icon->handlerID) {
            WMDeleteTimerHandler(icon->handlerID);
            icon->handlerID = NULL;
        }
	XClearArea(dpy, icon->core->window, 0, 0, icon->core->width, 
		   icon->core->height, True);
    }
}


void
wIconUpdate(WIcon *icon)
{
    WScreen *scr = icon->core->screen_ptr;
    int title_height = scr->icon_title_font->height;
    WWindow *wwin = icon->owner;

    assert(scr->icon_tile!=NULL);

    if (icon->pixmap!=None)
	XFreePixmap(dpy, icon->pixmap);
    icon->pixmap = None;
    
    
    if (wwin && wwin->window_flags.always_user_icon)
	goto user_icon;
    
    /* use client specified icon window */
    if (icon->icon_win!=None) {
	int resize=0;
	int width, height, depth;
	int theight;
	Pixmap pixmap;

	getSize(icon->icon_win, &width, &height, &depth);

	if (width > wPreferences.icon_size) {
	    resize = 1;
	    width = wPreferences.icon_size;
	}
	if (height > wPreferences.icon_size) {
	    resize = 1;
	    height = wPreferences.icon_size;
	}
	if (icon->show_title 
	    && (height+title_height < wPreferences.icon_size)) {
	    pixmap = XCreatePixmap(dpy, scr->w_win, wPreferences.icon_size,
				   wPreferences.icon_size, scr->w_depth);
	    XSetClipMask(dpy, scr->copy_gc, None);
	    XCopyArea(dpy, scr->icon_tile_pixmap, pixmap, scr->copy_gc, 0, 0,
		      wPreferences.icon_size, wPreferences.icon_size, 0, 0);
	    drawIconTitle(scr, pixmap, title_height);
	    theight = title_height;
	} else {
	    pixmap = None;
	    theight = 0;
	    XSetWindowBackgroundPixmap(dpy, icon->core->window,
				       scr->icon_tile_pixmap);
	}
	
	XSetWindowBorderWidth(dpy, icon->icon_win, 0);
	XReparentWindow(dpy, icon->icon_win, icon->core->window,
			(wPreferences.icon_size-width)/2,
			theight+(wPreferences.icon_size-height-theight)/2);
	if (resize)
	  XResizeWindow(dpy, icon->icon_win, width, height);

	XMapWindow(dpy, icon->icon_win);
	
	XAddToSaveSet(dpy, icon->icon_win);
	
	icon->pixmap = pixmap;

    } else if (wwin && wwin->wm_hints
	       && (wwin->wm_hints->flags & IconPixmapHint)) {
	int x, y;
	unsigned int w, h;
	Window jw;
	int ji, dotitle;
	unsigned int ju, d;
	Pixmap pixmap;
	
	if (!XGetGeometry(dpy, wwin->wm_hints->icon_pixmap, &jw, 
			  &ji, &ji, &w, &h, &ju, &d)) {
	    icon->owner->wm_hints->flags &= ~IconPixmapHint;
	    goto user_icon;
	}
			
	pixmap = XCreatePixmap(dpy, icon->core->window, wPreferences.icon_size,
			       wPreferences.icon_size, scr->w_depth);
	XSetClipMask(dpy, scr->copy_gc, None);
	XCopyArea(dpy, scr->icon_tile_pixmap, pixmap, scr->copy_gc, 0, 0,
		  wPreferences.icon_size, wPreferences.icon_size, 0, 0);

	if (w > wPreferences.icon_size)
	    w = wPreferences.icon_size;
	x = (wPreferences.icon_size-w)/2;

	if (icon->show_title && (title_height < wPreferences.icon_size)) {
	    drawIconTitle(scr, pixmap, title_height);
	    dotitle = 1;
	    
	    if (h > wPreferences.icon_size - title_height - 2) {
		h = wPreferences.icon_size - title_height - 2;
		y = title_height + 1;
	    } else {
		y = (wPreferences.icon_size-h-title_height)/2+title_height + 1;
	    }
	} else {
	    dotitle = 0;
	    if (w > wPreferences.icon_size)
		w = wPreferences.icon_size;
	    y = (wPreferences.icon_size-h)/2;
	}

	if (wwin->wm_hints->flags & IconMaskHint)
	    XSetClipMask(dpy, scr->copy_gc, wwin->wm_hints->icon_mask);
	
	XSetClipOrigin(dpy, scr->copy_gc, x, y);
	
	if (d != scr->w_depth) {
	    XSetForeground(dpy, scr->copy_gc, scr->white_pixel);
	    XSetBackground(dpy, scr->copy_gc, scr->black_pixel);
	    XCopyPlane(dpy, wwin->wm_hints->icon_pixmap, pixmap, scr->copy_gc,
		      0, 0, w, h, x, y, 1);
	} else {
	    XCopyArea(dpy, wwin->wm_hints->icon_pixmap, pixmap, scr->copy_gc,
		      0, 0, w, h, x, y);
	}
	
	XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
	
	icon->pixmap = pixmap;
    } else {
      user_icon:

          if (icon->image) {
              icon->pixmap = makeIcon(scr, icon->image, icon->show_title,
				      icon->shadowed);
          } else {
              /* make default icons */

              if (!scr->def_icon_pixmap) {
                  RImage *image = NULL;
                  char *path;
                  char *file;

                  file = wDefaultGetIconFile(scr, NULL, NULL);
                  if (file) {
                      path = FindImage(wPreferences.icon_path, file);
                      if (!path) {
                          wwarning(_("could not find default icon \"%s\""),file);
                          goto make_icons;
                      }
                      free(path);

                      image = RLoadImage(scr->rcontext, path, 0);
                      if (!image) {
                          wwarning(_("could not load default icon \"%s\""),file);
                      }
                  }
	make_icons:
                  scr->def_icon_pixmap = makeIcon(scr, image, False, False);
                  scr->def_ticon_pixmap = makeIcon(scr, image, True, False);
                  if (image)
                      RDestroyImage(image);
              }

              if (icon->show_title) {
                  XSetWindowBackgroundPixmap(dpy, icon->core->window,
                                             scr->def_ticon_pixmap);
              } else {
                  XSetWindowBackgroundPixmap(dpy, icon->core->window,
                                             scr->def_icon_pixmap);
              }
              icon->pixmap = None;
          }
    }
    if (icon->pixmap != None) {
        XSetWindowBackgroundPixmap(dpy, icon->core->window, icon->pixmap);
    }
    XClearWindow(dpy, icon->core->window);

    wIconPaint(icon);
}



void
wIconPaint(WIcon *icon)
{
    WScreen *scr=icon->core->screen_ptr;
    GC gc = scr->icon_title_gc;
    int x;
#ifndef I18N_MB
    char *tmp;
#endif

    if (icon->force_paint) {
        icon->force_paint = 0;
        wIconUpdate(icon);
        return;
    }
    
    XClearWindow(dpy, icon->core->window);
    
    /* draw the icon title */
    if (icon->show_title && icon->icon_name!=NULL) {
	int l;
	int w;
#ifdef I18N_MB
	l = strlen(icon->icon_name);
	w = MyTextWidth(scr->icon_title_font->font,icon->icon_name,l);
#else
	tmp = ShrinkString(scr->icon_title_font, icon->icon_name, 
			   wPreferences.icon_size-4);
	w = MyTextWidth(scr->icon_title_font->font, tmp, l=strlen(tmp));
#endif
	if (w > icon->core->width - 4)
	  x = (icon->core->width - 4) - w;
	else
	  x = (icon->core->width - w)/2;

#ifdef I18N_MB
 	XmbDrawString(dpy, icon->core->window, scr->icon_title_font->font, gc, 
 		      x, 1 + scr->icon_title_font->y, icon->icon_name, l);
#else
  	XDrawString(dpy, icon->core->window, gc, x, 
  		    1 + scr->icon_title_font->y, tmp, l);
	free(tmp);
#endif
    }
}


/******************************************************************/

static void
miniwindowExpose(WObjDescriptor *desc, XEvent *event)
{
    wIconPaint(desc->parent);
}


static void
miniwindowDblClick(WObjDescriptor *desc, XEvent *event)
{
    WIcon *icon = desc->parent;

    assert(icon->owner!=NULL);

    wDeiconifyWindow(icon->owner);
}


static void 
miniwindowMouseDown(WObjDescriptor *desc, XEvent *event)
{
    WIcon *icon = desc->parent;
    WWindow *wwin = icon->owner;
    XEvent ev;
    int x=wwin->icon_x, y=wwin->icon_y;
    int dx=event->xbutton.x, dy=event->xbutton.y;
    int grabbed=0;

    
    if ((desc->click_timestamp>0) &&
	(event->xbutton.time-desc->click_timestamp <= wPreferences.dblclick_time)) {
	desc->click_timestamp = -1;

	miniwindowDblClick(desc, event);
	return;
    }
    desc->click_timestamp = event->xbutton.time;
    
#ifdef DEBUG
    puts("Moving miniwindow");
#endif
    if (event->xbutton.button==Button1) {	
	if (event->xbutton.state & MOD_MASK)
	    wLowerFrame(icon->core);
	else
	    wRaiseFrame(icon->core);
        if (event->xbutton.state & ShiftMask) {
            wIconSelect(icon);
            wSelectWindow(icon->owner);
        }
    }

    if (XGrabPointer(dpy, icon->core->window, False, ButtonMotionMask
		     |ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, 
		     None, CurrentTime) !=GrabSuccess) {
#ifdef DEBUG0
	wwarning("pointer grab failed for icon move");
#endif
    }
    while(1) {
	WMMaskEvent(dpy, PointerMotionMask|ButtonReleaseMask|ButtonMotionMask
		   |ExposureMask, &ev);
	switch (ev.type) {
	 case Expose:
	    WMHandleEvent(&ev);
	    break;

	 case MotionNotify:
	    if (!grabbed) {
		if (abs(dx-ev.xmotion.x)>=MOVE_THRESHOLD
		    || abs(dy-ev.xmotion.y)>=MOVE_THRESHOLD) {
		    XChangeActivePointerGrab(dpy, ButtonMotionMask
					     |ButtonReleaseMask, 
					     wCursor[WCUR_MOVE], CurrentTime);
		    grabbed=1;
		} else {
		    break;
		}
	    }
	    x = ev.xmotion.x_root - dx;
	    y = ev.xmotion.y_root - dy;
	    XMoveWindow(dpy, icon->core->window, x, y);
	    break;

	 case ButtonRelease:
	    if (wwin->icon_x!=x || wwin->icon_y!=y)
	      wwin->flags.icon_moved = 1;

	    XMoveWindow(dpy, icon->core->window, x, y);

	    wwin->icon_x = x;
	    wwin->icon_y = y;
#ifdef DEBUG
	    puts("End miniwindow move");
#endif
	    XUngrabPointer(dpy, CurrentTime);
	    return;
	    
	}
    }
}


