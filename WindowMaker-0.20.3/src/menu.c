/* menu.c- generic menu, used for root menu, application menus etc.
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
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "framewin.h"
#include "menu.h"
#include "actions.h"
#include "funcs.h"
#include "stacking.h"
#include "text.h"


/****** Global Variables ******/

extern Cursor wCursor[WCUR_LAST];

extern XContext wWinContext;

extern WPreferences wPreferences;

#define MOD_MASK wPreferences.modifier_mask

#define MENU_SCROLL_STEP  menuScrollParameters[(int)wPreferences.menu_scroll_speed].steps
#define MENU_SCROLL_DELAY menuScrollParameters[(int)wPreferences.menu_scroll_speed].delay



#define MENUW(m)	((m)->frame->core->width+2*FRAME_BORDER_WIDTH)
#define MENUH(m)	((m)->frame->core->height+2*FRAME_BORDER_WIDTH)


/***** Local Stuff ******/

static struct {
    int steps;
    int delay;
} menuScrollParameters[5] = {
    {MENU_SCROLL_STEPS_UF, MENU_SCROLL_DELAY_UF},
    {MENU_SCROLL_STEPS_F, MENU_SCROLL_DELAY_F},
    {MENU_SCROLL_STEPS_M, MENU_SCROLL_DELAY_M},
    {MENU_SCROLL_STEPS_S, MENU_SCROLL_DELAY_S},
    {MENU_SCROLL_STEPS_U, MENU_SCROLL_DELAY_U}};


static void menuMouseDown(WObjDescriptor *desc, XEvent *event);
static void menuExpose(WObjDescriptor *desc, XEvent *event);

static void menuTitleDoubleClick(WCoreWindow *sender, void *data, XEvent *event);
static void menuTitleMouseDown(WCoreWindow *sender, void *data, XEvent *event);

static void menuCloseClick(WCoreWindow *sender, void *data, XEvent *event);



static void selectEntry(WMenu *menu, int entry_no);
static void closeCascade(WMenu *menu);



/*
 *----------------------------------------------------------------------
 * wMenuCreate--
 * 	Creates a new empty menu with the specified title. If main_menu
 * is True, the created menu will be a main menu, which has some special
 * properties such as being placed over other normal menus.
 * 	If title is NULL, the menu will have no titlebar.
 * 
 * Returns:
 * 	The created menu.
 *---------------------------------------------------------------------- 
 */
WMenu*
wMenuCreate(WScreen *screen, char *title, int main_menu)
{
    WMenu *menu;
    static int brother=0;
    int tmp, flags;
    
    menu = wmalloc(sizeof(WMenu));

    memset(menu, 0, sizeof(WMenu));

#ifdef SINGLE_MENULEVEL
    tmp = WMSubmenuLevel;
#else
    tmp = (main_menu ? WMMainMenuLevel : WMSubmenuLevel);
#endif

    flags = WFF_SINGLE_STATE;
    if (title) {
	flags |= WFF_TITLEBAR|WFF_RIGHT_BUTTON;
	menu->flags.titled = 1;
    }
    menu->frame =
	wFrameWindowCreate(screen, tmp, 8, 2, 1, 1, flags,
			   screen->menu_title_texture, NULL,
			   screen->menu_title_pixel, &screen->menu_title_gc, 
			   &screen->menu_title_font);

    menu->frame->core->descriptor.parent = menu;
    menu->frame->core->descriptor.parent_type = WCLASS_MENU;
    menu->frame->core->descriptor.handle_mousedown = menuMouseDown;
    
    wFrameWindowHideButton(menu->frame, WFF_RIGHT_BUTTON);

    if (title) {
	menu->frame->title = wstrdup(title);
    }

    menu->frame->flags.justification = WTJ_LEFT;
    
    menu->frame->rbutton_image = screen->b_pixmaps[WBUT_CLOSE];

    menu->entry_no = 0;
    menu->alloced_entries = 0;
    menu->selected_entry = -1;
    menu->entries = NULL;
    
    menu->frame_x = screen->app_menu_x;
    menu->frame_y = screen->app_menu_y;
    
    menu->frame->child = menu;

    menu->flags.lowered = 0;

    /* create borders */
    if (title) {
	/* setup object descriptors */
	menu->frame->on_mousedown_titlebar = menuTitleMouseDown;
	menu->frame->on_dblclick_titlebar = menuTitleDoubleClick;
    }
    
    menu->frame->on_click_right = menuCloseClick;
    
    
    menu->menu = wCoreCreate(menu->frame->core, 0, menu->frame->top_width,
			     menu->frame->core->width, 10);

    menu->menu->descriptor.parent = menu;
    menu->menu->descriptor.parent_type = WCLASS_MENU;
    menu->menu->descriptor.handle_expose = menuExpose;
    menu->menu->descriptor.handle_mousedown = menuMouseDown;
    
    menu->menu_texture_data = None;

    XMapWindow(dpy, menu->menu->window);

    XFlush(dpy);
    
    if (!brother) {
	brother = 1;
	menu->brother = wMenuCreate(screen, title, main_menu);
	brother = 0;
	menu->brother->flags.brother = 1;
	menu->brother->brother = menu;
    }
    

    return menu;
}




WMenu*
wMenuCreateForApp(WScreen *screen, char *title, int main_menu)
{
    WMenu *menu;
    
    menu = wMenuCreate(screen, title, main_menu);
    if (!menu)
      return NULL;
    menu->flags.app_menu = 1;
    menu->brother->flags.app_menu = 1;

    return menu;
}



static void
insertEntry(WMenu *menu, WMenuEntry *entry, int index)
{
    int i;
    
    for (i=menu->entry_no; i>index; i--) {
	menu->entries[i]->order++;
	menu->entries[i+1]=menu->entries[i];
    }
    menu->entries[index] = entry;
}


void
wMenuRefresh(WMenu *menu, int flags)
{
    int i;
    
    if (flags & MR_TEXT_BACK) {
	menu->frame->flags.need_texture_remake = 1;
    }
    
    if (flags & (MR_RESIZED|MR_TITLE_TEXT)) {
	menu->flags.realized = 0;
    }

    wMenuRealize(menu);

    if (menu->flags.titled)
	wFrameWindowPaint(menu->frame);
    
    if (!menu->flags.brother) {
	if (menu->brother)
	    wMenuRefresh(menu->brother, flags);

	for (i=0; i < menu->cascade_no; i++) {
	    if (!menu->cascades[i]->flags.brother)
		wMenuRefresh(menu->cascades[i], flags);
	    else
		wMenuRefresh(menu->cascades[i]->brother, flags);
	}
    }
}



WMenuEntry*
wMenuInsertCallback(WMenu *menu, int index, char *text,
		    void (*callback)(WMenu *menu, WMenuEntry *entry), 
		    void *clientdata)
{
    WMenuEntry *entry;

#ifdef DEBUG
    if (!menu) {
	printf("Passed NULL as menu parameter to wMenuAddCallback() \n");
	return NULL;
    }    
#endif

    assert(menu->flags.brother==0);
    menu->flags.realized = 0;
    menu->brother->flags.realized = 0;
    
    /* reallocate array if it's too small */
    if (menu->entry_no >= menu->alloced_entries) {
	void *tmp;
#ifdef DEBUG
	puts("doing wrealloc()");
#endif	
	tmp = wrealloc(menu->entries,
		       sizeof(WMenuEntry)*(menu->alloced_entries+5));
	if (tmp==NULL) {
	    wwarning(_("wrealloc() failed while trying to add menu item"));
	    return NULL;
	}

	menu->entries = tmp;
	menu->alloced_entries += 5;
	
	menu->brother->entries = tmp;
	menu->brother->alloced_entries = menu->alloced_entries;
    }
    entry = wmalloc(sizeof(WMenuEntry));
    memset(entry, 0, sizeof(WMenuEntry));
    entry->flags.enabled = 1;
    entry->text = wstrdup(text);
    entry->cascade = -1;
    entry->clientdata = clientdata;
    entry->callback = callback;
    if (index<0 || index>=menu->entry_no) {
	entry->order = menu->entry_no;
	menu->entries[menu->entry_no] = entry;
    } else {
	entry->order = index;
	insertEntry(menu, entry, index);
    }
    
    menu->entry_no++;
    menu->brother->entry_no = menu->entry_no;
    
    return entry;
}



void
wMenuEntrySetCascade(WMenu *menu, WMenuEntry *entry, WMenu *cascade)
{
    WMenu *brother = menu->brother;
    int i, done;

    assert(menu->flags.brother==0);
    
    if (entry->cascade>=0) {
	menu->flags.realized = 0;
	brother->flags.realized = 0;
    }
    
    cascade->parent = menu;

    cascade->brother->parent = brother;

    done = 0;
    for (i=0; i<menu->cascade_no; i++) {
	if (menu->cascades[i]==NULL) {
	    menu->cascades[i] = cascade;
	    brother->cascades[i] = cascade->brother;
	    done = 1;
	    entry->cascade = i;
	    break;
	}
    }
    if (!done) {
	entry->cascade = menu->cascade_no;

	menu->cascades = wrealloc(menu->cascades, 
				  sizeof(WMenu)*(menu->cascade_no+1));
	menu->cascades[menu->cascade_no++] = cascade;
	

	brother->cascades = wrealloc(brother->cascades, 
				     sizeof(WMenu)*(brother->cascade_no+1));
	brother->cascades[brother->cascade_no++] = cascade->brother;
    }


    if (menu->flags.lowered) {
	
	cascade->flags.lowered = 1;
	ChangeStackingLevel(cascade->frame->core, WMNormalLevel);
	
	cascade->brother->flags.lowered = 1;
	ChangeStackingLevel(cascade->brother->frame->core, WMNormalLevel);
    }
    
    if (!menu->flags.realized)
	wMenuRealize(menu);
}


void
wMenuEntryRemoveCascade(WMenu *menu, WMenuEntry *entry)
{    
    assert(menu->flags.brother==0);
    
    /* destroy cascade menu */
    if (entry->cascade>=0 && menu->cascades
	&& menu->cascades[entry->cascade]!=NULL) {

	wMenuDestroy(menu->cascades[entry->cascade], True);

	menu->cascades[entry->cascade] = NULL;
	menu->brother->cascades[entry->cascade] = NULL;

	entry->cascade = -1;
    }
}


void
wMenuRemoveItem(WMenu *menu, int index)
{
    int i;
    
    if (menu->flags.brother) {
	wMenuRemoveItem(menu->brother, index);
	return;
    }
    
    if (index>=menu->entry_no) return;

    /* destroy cascade menu */
    wMenuEntryRemoveCascade(menu, menu->entries[index]);
    
    /* destroy unshared data */

    if (menu->entries[index]->text)
      free(menu->entries[index]->text);
    
    if (menu->entries[index]->rtext)
      free(menu->entries[index]->rtext);
	
    if (menu->entries[index]->free_cdata && menu->entries[index]->clientdata)
      (*menu->entries[index]->free_cdata)(menu->entries[index]->clientdata);

    free(menu->entries[index]);
	
    for (i=index; i<menu->entry_no-1; i++) {
	menu->entries[i+1]->order--;
	menu->entries[i]=menu->entries[i+1];
    }
    menu->entry_no--;
    menu->brother->entry_no--;
}



void 
wMenuRealize(WMenu *menu)
{
    int i;
    int width, rwidth, mrwidth, mwidth;
    int theight, twidth, eheight;
    WScreen *scr = menu->frame->screen_ptr;
    static int brother_done=0;
    int flags;

    if (!brother_done) {
	brother_done = 1;
	wMenuRealize(menu->brother);
	brother_done = 0;
    }

    flags = WFF_SINGLE_STATE;
    if (menu->flags.titled)
	flags |= WFF_TITLEBAR|WFF_RIGHT_BUTTON;

    wFrameWindowUpdateBorders(menu->frame, flags);

    if (menu->flags.titled) {
	twidth = wTextWidth(scr->menu_title_font->font, menu->frame->title,
			     strlen(menu->frame->title));
        theight = menu->frame->top_width;
        twidth += theight + theight/2 + (wPreferences.new_style ? 8 : 0);
    } else {
	twidth = 0;
	theight = 0;
    }
    eheight = scr->menu_entry_font->height + 6;
    menu->entry_height = eheight;
    mrwidth = 0;
    mwidth = 0;
    for (i=0; i<menu->entry_no; i++) {
	char *text;

	/* search widest text */
	text = menu->entries[i]->text;
	width = wTextWidth(scr->menu_entry_font->font, text, strlen(text))+12;

	if (menu->entries[i]->flags.indicator) {
	    width += MENU_INDICATOR_SPACE;
	}

	if (width > mwidth)
	    mwidth = width;
	
	/* search widest text on right */
	text = menu->entries[i]->rtext;
	if (text)
	    rwidth = wTextWidth(scr->menu_entry_font->font, text,
				 strlen(text)) + 5;
	else if (menu->entries[i]->cascade>=0)
	    rwidth = eheight;
	else
	    rwidth = 4;

	if (rwidth > mrwidth)
	    mrwidth = rwidth;
    }
    mwidth += mrwidth;
    
    if (mwidth < twidth)
	mwidth = twidth;

    wCoreConfigure(menu->menu, 0, theight, mwidth, menu->entry_no*eheight -1);

    wFrameWindowResizeInternal(menu->frame, mwidth, menu->entry_no*eheight -1);


    /* setup background texture */
    switch (scr->menu_item_texture->any.type) {
     case WTEX_DGRADIENT:
     case WTEX_VGRADIENT:
     case WTEX_HGRADIENT:
     case WTEX_MHGRADIENT:
     case WTEX_MVGRADIENT:
     case WTEX_MDGRADIENT:
     case WTEX_PIXMAP:
	if (!menu->flags.brother) {
	    FREE_PIXMAP(menu->menu_texture_data);

	    wTextureRender(scr, scr->menu_item_texture,
			   &menu->menu_texture_data, menu->menu->width,
			   menu->entry_height, WREL_MENUENTRY);
	    XSetWindowBackgroundPixmap(dpy, menu->menu->window,
				       menu->menu_texture_data);
	    XClearWindow(dpy, menu->menu->window);

	    XSetWindowBackgroundPixmap(dpy, menu->brother->menu->window,
				       menu->menu_texture_data);
	    XClearWindow(dpy, menu->brother->menu->window);
	}
	break;
	
     default:
	XSetWindowBackground(dpy, menu->menu->window,
			     scr->menu_item_texture->any.color.pixel);
	XClearWindow(dpy, menu->menu->window);
    }

    menu->flags.realized = 1;

    if (menu->flags.mapped)
	wMenuPaint(menu);
    if (menu->brother->flags.mapped)
	wMenuPaint(menu->brother);
}


void
wMenuDestroy(WMenu *menu, int recurse)
{
    int i;

    /* remove any pending timers */
    if (menu->timer)
	WMDeleteTimerHandler(menu->timer);
    menu->timer = NULL;
    
    /* call destroy handler */
    if (menu->on_destroy)
	(*menu->on_destroy)(menu);
    
    /* Destroy items if this menu own them. If this is the "brother" menu,
     * leave them alone as it is shared by them.
     */
    if (!menu->flags.brother) {
	for (i=0; i<menu->entry_no; i++) {

	    free(menu->entries[i]->text);

	    if (menu->entries[i]->rtext)
	      free(menu->entries[i]->rtext);

	    if (menu->entries[i]->free_cdata && menu->entries[i]->clientdata) {
		(*menu->entries[i]->free_cdata)(menu->entries[i]->clientdata);
	    }
	    free(menu->entries[i]);
	}

	if (recurse) {
	    for (i=0; i<menu->cascade_no; i++) {
		if (menu->cascades[i]) {
		    if (menu->cascades[i]->flags.brother)
		      wMenuDestroy(menu->cascades[i]->brother, recurse);
		    else
		      wMenuDestroy(menu->cascades[i], recurse);
		}
	    }
	}
	
	if (menu->entries)
	    free(menu->entries);
	
    }

    FREE_PIXMAP(menu->menu_texture_data);
    
    if (menu->cascades)
      free(menu->cascades);

    wCoreDestroy(menu->menu);
    wFrameWindowDestroy(menu->frame);
    
    /* destroy copy of this menu */
    if (!menu->flags.brother && menu->brother)
	wMenuDestroy(menu->brother, False);
    
    free(menu);
}


static void
drawFrame(WScreen *scr, Window win, int y, int w, int h)
{
    XSegment segs[2];

    segs[0].x1 = 0;
    segs[0].y1 = y;
    segs[0].x2 = w-1;
    segs[0].y2 = y;
    segs[1].x1 = 0;
    segs[1].y1 = y;
    segs[1].x2 = 0;
    segs[1].y2 = y + h - 2;
    XDrawSegments(dpy, win, scr->menu_item_auxtexture->light_gc, segs, 2);

    XDrawLine(dpy, win, scr->menu_item_auxtexture->dark_gc, 0, y+h-1,
	      w-1, y+h-1);

    segs[0].x1 = 1;
    segs[0].y1 = segs[0].y2 = y + h-2;
    segs[0].x2 = w-1;
    segs[1].x1 = segs[1].x2 = w-1;
    segs[1].y1 = y + 1;
    segs[1].y2 = y + h-2;
    XDrawSegments(dpy, win, scr->menu_item_auxtexture->dim_gc, segs, 2);
}


static void
paintEntry(WMenu *menu, int index, int selected)
{
    int x, y, w, h, tw;
    GC light, dim, dark, textGC;
    WScreen *scr=menu->frame->screen_ptr;
    Window win = menu->menu->window;
    WMenuEntry *entry=menu->entries[index];

    if (!menu->flags.realized) return;
    h = menu->entry_height;
    w = menu->menu->width;
    y = index * h;

    light = scr->menu_item_auxtexture->light_gc;
    dim = scr->menu_item_auxtexture->dim_gc;
    dark = scr->menu_item_auxtexture->dark_gc;

    /* paint background */
    if (selected) {
	XSetForeground(dpy, scr->select_menu_gc, scr->select_pixel);
	XFillRectangle(dpy, win, scr->select_menu_gc, 1, y+1, w-2, h-3);
	if (scr->menu_item_texture->any.type == WTEX_SOLID)
	    drawFrame(scr, win, y, w, h);
    } else { 
	if (scr->menu_item_texture->any.type == WTEX_SOLID) {
	    XClearArea(dpy, win, 0, y + 1, w - 1, h - 3, False);
	    /* draw the frame */
	    drawFrame(scr, win, y, w, h);
	} else {
	    XClearArea(dpy, win, 0, y, w, h, False);
	}
    }

    if (selected) {
	textGC = scr->select_menu_gc;
	if (entry->flags.enabled) 
	    XSetForeground(dpy, textGC, scr->select_text_pixel);
	else
	    XSetForeground(dpy, textGC, scr->dtext_pixel);
    } else if (!entry->flags.enabled) {
	textGC = scr->disabled_menu_entry_gc;
    } else { 
	textGC = scr->menu_entry_gc;
    }
    /* draw text */    
    x = 5;
    if (entry->flags.indicator)
	x += MENU_INDICATOR_SPACE + 2;

    wDrawString(win, scr->menu_entry_font, 
		  textGC, x, 3+y+scr->menu_entry_font->y, entry->text, 
		  strlen(entry->text));

    if (entry->cascade>=0) {
	/* draw the cascade indicator */
	XDrawLine(dpy,win,dim, w-11, y+6, w-6, y+h/2-1);
	XDrawLine(dpy,win,light, w-11, y+h-8, w-6, y+h/2-1);
	XDrawLine(dpy,win,dark, w-12, y+6, w-12, y+h-8);
    }

    /* draw indicator */
    if (entry->flags.indicator && entry->flags.indicator_on) {
	int iw, ih;
	WPixmap *indicator;

	
	switch (entry->flags.indicator_type) {
	 case MI_CHECK:
	    indicator = scr->menu_check_indicator;
	    break;
	 case MI_MINIWINDOW:
	    indicator = scr->menu_mini_indicator;
	    break;
	 case MI_HIDDEN:
	    indicator = scr->menu_hide_indicator;
	    break;
	 case MI_SHADED:
	    indicator = scr->menu_shade_indicator;
	    break;
	 case MI_DIAMOND:
	 default:
	    indicator = scr->menu_radio_indicator;
	    break;
	}
	
	iw = indicator->width;
	ih = indicator->height;
	XSetClipMask(dpy, scr->copy_gc, indicator->mask);
	XSetClipOrigin(dpy, scr->copy_gc, 5, y+(h-ih)/2);
	if (selected)
	    XSetForeground(dpy, scr->copy_gc, scr->black_pixel);
	else
	    XSetForeground(dpy, scr->copy_gc, scr->mtext_pixel);
	XFillRectangle(dpy, win, scr->copy_gc, 5, y+(h-ih)/2, iw, ih);
	/*
	XCopyArea(dpy, indicator->image, win, scr->copy_gc, 0, 0,
		  iw, ih, 5, y+(h-ih)/2);
	 */
	XSetClipOrigin(dpy, scr->copy_gc, 0, 0);
    }

    /* draw right text */
    
    if (entry->rtext && entry->cascade<0) {
	tw = wTextWidth(scr->menu_entry_font->font, entry->rtext,
			 strlen(entry->rtext));

	wDrawString(win, scr->menu_entry_font, textGC, w-6-tw, 
		    3+y+scr->menu_entry_font->y, entry->rtext,
		    strlen(entry->rtext));
    }
}

static void
move_menus(WMenu *menu, int x, int y)
{
    while (menu->parent) {
	menu = menu->parent;
	x -= MENUW(menu);
	if (!wPreferences.align_menus && menu->selected_entry>=0) {
	    y -= menu->selected_entry*menu->entry_height;
	}
    }
    wMenuMove(menu, x, y, True);
}

static void
makeVisible(WMenu *menu)
{
    WScreen *scr = menu->frame->screen_ptr;
    int x1, y1, x2, y2, new_x, new_y, move;
    
    if (menu->entry_no<0) return;
    
    x1 = menu->frame_x;
    y1 = menu->frame_y+menu->frame->top_width
	+ menu->selected_entry*menu->entry_height;
    x2 = x1 + MENUW(menu);
    y2 = y1 + menu->entry_height;
    
    new_x = x1;
    new_y = y1;
    move = 0;

    if (x1 < 0) {
	new_x = 0;
	move = 1;
    } else if (x2 >= scr->scr_width) {
	new_x = scr->scr_width - MENUW(menu) - 1;
	move = 1;
    }
    
    if (y1 < 0) {
	new_y = 0;
	move = 1;
    } else if (y2 >= scr->scr_height) {
	new_y = scr->scr_height - menu->entry_height - 1;
	move = 1;
    }
    
    new_y = new_y - menu->frame->top_width
	- menu->selected_entry*menu->entry_height;
    move_menus(menu, new_x, new_y);

}


static int
check_key(WMenu *menu, XKeyEvent *event)
{
    int i, ch, s;
    char buffer[32];
    
    if (XLookupString(event, buffer, 32, NULL, NULL)<1)
	return -1;
    
    ch = toupper(buffer[0]);
    
    s = (menu->selected_entry>=0 ? menu->selected_entry+1 : 0);
    
again:
    for (i=s; i<menu->entry_no; i++) {
	if (ch==toupper(menu->entries[i]->text[0])) {
	    return i;
	}
    }
    /* no match. Retry from start, if previous started from a selected entry */
    if (s!=0) {
	s = 0;
	goto again;
    }	
    return -1;
}


static int
keyboardMenu(WMenu *menu)
{
    XEvent event;
    KeySym ksym=NoSymbol;
    int done=0;
    int index;
    WMenuEntry *entry;
    int old_pos_x = menu->frame_x;
    int old_pos_y = menu->frame_y;
    int new_x = old_pos_x, new_y = old_pos_y;
    int scr_width = menu->frame->screen_ptr->scr_width;
    int scr_height = menu->frame->screen_ptr->scr_height;

    if (menu->flags.editing)
	return False;


    XGrabKeyboard(dpy, menu->frame->core->window, True, GrabModeAsync,
		  GrabModeAsync, CurrentTime);

    if (menu->frame_y+menu->frame->top_width >= scr_height)
	new_y = scr_height - menu->frame->top_width;
    
    if (menu->frame_x+MENUW(menu) >= scr_width)
	new_x = scr_width-MENUW(menu)-1;
    
    move_menus(menu, new_x, new_y);

    while (!done && menu->flags.mapped) {
	XAllowEvents(dpy, AsyncKeyboard, CurrentTime);
	WMMaskEvent(dpy, ExposureMask|ButtonMotionMask|ButtonPressMask
		    |ButtonReleaseMask|KeyPressMask|KeyReleaseMask
		    |SubstructureNotifyMask, &event);

	switch (event.type) {
	 case KeyPress:
	    ksym = XLookupKeysym(&event.xkey, 0);
	    switch (ksym) {
	     case XK_Escape:
		done = 1;
		break;

	     case XK_Home:
	     case XK_KP_Home:
		selectEntry(menu, 0);
		makeVisible(menu);
		break;
		
	     case XK_End:
	     case XK_KP_End:
		selectEntry(menu, menu->entry_no-1);
		makeVisible(menu);
		break;
		
	     case XK_Up:
#ifdef ARROWLESS_KBD
	     case XK_k:
#endif
	     case XK_KP_Up:
		if (menu->selected_entry <= 0)
                    selectEntry(menu, menu->entry_no-1);
		else
		    selectEntry(menu, menu->selected_entry-1);
		makeVisible(menu);
		break;
		
	     case XK_Down:
#ifdef ARROWLESS_KBD
	     case XK_j:
#endif
	     case XK_KP_Down:
		if (menu->selected_entry<0) 
                    selectEntry(menu, 0);
                else if (menu->selected_entry == menu->entry_no-1)
                    selectEntry(menu, 0);
		else if (menu->selected_entry < menu->entry_no-1)
		    selectEntry(menu, menu->selected_entry+1);
		makeVisible(menu);
		break;
		
	     case XK_Right:
#ifdef ARROWLESS_KBD
	     case XK_l:
#endif
	     case XK_KP_Right:
		if (menu->selected_entry>=0) {
		    WMenuEntry *entry;
		    entry = menu->entries[menu->selected_entry];

		    if (entry->cascade >= 0 && menu->cascades
			&& menu->cascades[entry->cascade]->entry_no > 0) {

			XUngrabKeyboard(dpy, CurrentTime);

			selectEntry(menu->cascades[entry->cascade], 0);
			if (!keyboardMenu(menu->cascades[entry->cascade]))
			    done = 1;

			XGrabKeyboard(dpy, menu->frame->core->window, True,
				      GrabModeAsync, GrabModeAsync,
				      CurrentTime);
		    }
		}
		break;

	     case XK_Left:
#ifdef ARROWLESS_KBD
	     case XK_h:
#endif
	     case XK_KP_Left:
		if (menu->parent!=NULL && menu->parent->selected_entry>=0) {
		    selectEntry(menu, -1);
		    move_menus(menu, old_pos_x, old_pos_y);
		    return True;
		}
		break;
		
	     case XK_Return:
		done = 2;
		break;
		
	     default:
		index = check_key(menu, &event.xkey);
		if (index>=0) {
		    selectEntry(menu, index);
		}
	    }
	    break;
	    
	 default:
	    if (event.type==ButtonPress)
		done = 1;
	    
	    WMHandleEvent(&event);
	}
    }

    XUngrabKeyboard(dpy, CurrentTime);

    if (done==2 && menu->selected_entry>=0) {
	entry = menu->entries[menu->selected_entry];
    } else {
	entry = NULL;
    }

    if (entry && entry->callback!=NULL && entry->flags.enabled
	&& entry->cascade < 0) {
#if (MENU_BLINK_COUNT > 0)
	int sel = menu->selected_entry;
	int i;
	
	for (i=0; i<MENU_BLINK_COUNT; i++) {
	    paintEntry(menu, sel, False);
	    XSync(dpy, 0);
	    wusleep(MENU_BLINK_DELAY);
	    paintEntry(menu, sel, True);
	    XSync(dpy, 0);
	    wusleep(MENU_BLINK_DELAY);
	}
#endif
	selectEntry(menu, -1);

	if (!menu->flags.buttoned) {
	    wMenuUnmap(menu);
	    move_menus(menu, old_pos_x, old_pos_y);
	}
	closeCascade(menu);
	 
	(*entry->callback)(menu, entry);
    } else {
	if (!menu->flags.buttoned) {
	    wMenuUnmap(menu);
	    move_menus(menu, old_pos_x, old_pos_y);
	}
	selectEntry(menu, -1);
    }

    
    /* returns True if returning from a submenu to a parent menu,
     * False if exiting from menu */
    return False;
}


void
wMenuMapAt(WMenu *menu, int x, int y, int keyboard)
{
    int scr_width = menu->frame->screen_ptr->scr_width;
    int scr_height = menu->frame->screen_ptr->scr_height;

    if (!menu->flags.realized) {
	menu->flags.realized=1;
	wMenuRealize(menu);
    }
    if (!menu->flags.mapped) {
        if (wPreferences.wrap_menus) {
            if (x<0) x = 0;
            if (y<0) y = 0;
            if (x+MENUW(menu) > scr_width)
                x = scr_width - MENUW(menu);
            if (y+MENUH(menu) > scr_height)
                y = scr_height - MENUH(menu);
        }

        XMoveWindow(dpy, menu->frame->core->window, x, y);
        menu->frame_x = x;
        menu->frame_y = y;
        XMapWindow(dpy, menu->frame->core->window);
        wRaiseFrame(menu->frame->core);
        menu->flags.mapped = 1;
    } else {
        selectEntry(menu, 0);
    }
    
    if (keyboard)
	keyboardMenu(menu);
}


void
wMenuMap(WMenu *menu)
{    
    if (!menu->flags.realized) {
	menu->flags.realized=1;
	wMenuRealize(menu);
    }
    if (menu->flags.app_menu && menu->parent==NULL) {
	menu->frame_x = menu->frame->screen_ptr->app_menu_x;
	menu->frame_y = menu->frame->screen_ptr->app_menu_y;
	XMoveWindow(dpy, menu->frame->core->window, menu->frame_x, menu->frame_y);
    }
    XMapWindow(dpy, menu->frame->core->window);
    wRaiseFrame(menu->frame->core);
    menu->flags.mapped = 1;
}


void
wMenuUnmap(WMenu *menu)
{
    int i;

    XUnmapWindow(dpy, menu->frame->core->window);
    if (menu->flags.titled && menu->flags.buttoned) {
	wFrameWindowHideButton(menu->frame, WFF_RIGHT_BUTTON);
    }
    menu->flags.buttoned = 0;
    menu->flags.mapped = 0;
    menu->flags.open_to_left = 0;

    for (i=0; i<menu->cascade_no; i++) {
	if (menu->cascades[i]!=NULL
	    && menu->cascades[i]->flags.mapped 
	    && !menu->cascades[i]->flags.buttoned) {
	    
	    wMenuUnmap(menu->cascades[i]);
	}
    }
    menu->selected_entry = -1;
} 



void 
wMenuPaint(WMenu *menu)
{ 
    int i;
     
    if (!menu->flags.mapped) {
	return;
    }

    /* paint entries */
    for (i=0; i<menu->entry_no; i++) {
	paintEntry(menu, i, i==menu->selected_entry);
    }
}


void
wMenuSetEnabled(WMenu *menu, int index, int enable)
{
    if (index>=menu->entry_no) return;
    menu->entries[index]->flags.enabled=enable;
    paintEntry(menu, index, index==menu->selected_entry);
    paintEntry(menu->brother, index, index==menu->selected_entry);
}


/* ====================================================================== */


static void
editEntry(WMenu *menu, WMenuEntry *entry)
{
    WTextInput *text;
    XEvent event;
    WObjDescriptor *desc;
    char *t;
    int done = 0;
    Window old_focus;
    int old_revert;
    
    menu->flags.editing = 1;

    text = wTextCreate(menu->menu, 1, menu->entry_height * entry->order,
		       menu->menu->width - 2, menu->entry_height - 1);

    wTextPutText(text, entry->text);
    XGetInputFocus(dpy, &old_focus, &old_revert);
    XSetInputFocus(dpy, text->core->window, RevertToNone, CurrentTime);
    
    if (XGrabKeyboard(dpy, text->core->window, True, GrabModeAsync,
		      GrabModeAsync, CurrentTime)!=GrabSuccess) {
	wwarning("could not grab keyboard");
	wTextDestroy(text);
	
	wSetFocusTo(menu->frame->screen_ptr, 
		    menu->frame->screen_ptr->focused_window);
	return;
    }


    while (!done && !text->done) {
	XSync(dpy, 0);
	XAllowEvents(dpy, AsyncKeyboard|AsyncPointer, CurrentTime);
	XSync(dpy, 0);
	WMNextEvent(dpy, &event);

	if (XFindContext(dpy, event.xany.window, wWinContext, 
			 (XPointer *)&desc)==XCNOENT)
	    desc = NULL;
	
	if ((desc != NULL) && (desc->handle_anything != NULL)) {

	    (*desc->handle_anything)(desc, &event);

	} else {	    
	    switch (event.type) {
	     case ButtonPress:
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		done = 1;

	     default:
		WMHandleEvent(&event);
	    }
	}
    }

    XSetInputFocus(dpy, old_focus, old_revert, CurrentTime);

    wSetFocusTo(menu->frame->screen_ptr, 
		menu->frame->screen_ptr->focused_window);


    t = wTextGetText(text);
    /* if !t, the user has canceled editing */
    if (t) {
	if (entry->text)
	    free(entry->text);
	entry->text = wstrdup(t);
	
	menu->flags.realized = 0;
    }
    wTextDestroy(text);
    
    XUngrabKeyboard(dpy, CurrentTime);
    
    if (t && menu->on_edit)
	(*menu->on_edit)(menu, entry);
    
    menu->flags.editing = 0;

    if (!menu->flags.realized)
	wMenuRealize(menu);
}


static void
selectEntry(WMenu *menu, int entry_no)
{
    WMenuEntry *entry;
    WMenu *submenu;
    int old_entry;

    if (menu->entries==NULL)
	return;
    
    if (entry_no >= menu->entry_no)
	return;

    old_entry = menu->selected_entry;
    menu->selected_entry = entry_no;

    if (old_entry!=entry_no) {
	
	/* unselect previous entry */
	if (old_entry>=0) {
	    paintEntry(menu, old_entry, False);
	    entry = menu->entries[old_entry];
	    
	    /* unmap cascade */
	    if (entry->cascade>=0 && menu->cascades) {
		if (!menu->cascades[entry->cascade]->flags.buttoned) {
		    wMenuUnmap(menu->cascades[entry->cascade]);
		}
	    }
	}

	if (entry_no<0) {
	    menu->selected_entry = -1;
	    return;
	}
	entry = menu->entries[entry_no];

	if (entry->cascade>=0 && menu->cascades && entry->flags.enabled) {
	    /* Callback for when the submenu is opened.
	     */
	    submenu = menu->cascades[entry->cascade];
	    if (submenu && submenu->flags.brother)
		submenu = submenu->brother;

	    if (entry->callback) {
		/* Only call the callback if the submenu is not yet mapped.
		 */
		if (menu->flags.brother) {
		    if (!submenu || !submenu->flags.mapped)
			(*entry->callback)(menu->brother, entry);
		} else {
		    if (!submenu || !submenu->flags.buttoned)
			(*entry->callback)(menu, entry);
		}
	    }

	    /* the submenu menu might have changed */
	    submenu = menu->cascades[entry->cascade];
	    
	    /* map cascade */
	    if (!submenu->flags.mapped) {
                int x, y;

                if (!submenu->flags.realized)
                    wMenuRealize(submenu);
                if (wPreferences.wrap_menus) {
                    if (menu->flags.open_to_left)
                        submenu->flags.open_to_left = 1;

                    if (submenu->flags.open_to_left) {
                        x = menu->frame_x - MENUW(submenu);
                        if (x<0) {
                            x = 0;
                            submenu->flags.open_to_left = 0;
                        }
                    } else {
                        x = menu->frame_x + MENUW(menu);

                        if (x + MENUW(submenu)
			    >= menu->frame->screen_ptr->scr_width) {

                            x = menu->frame_x - MENUW(submenu);
                            submenu->flags.open_to_left = 1;
                        }
                    }
                } else {
                    x = menu->frame_x + MENUW(menu);
		}

                if (wPreferences.align_menus) {
                    y = menu->frame_y;
                } else {
                    y = menu->frame_y + menu->entry_height*entry_no;
                    if (menu->flags.titled)
                        y += menu->frame->top_width;
                    if (menu->cascades[entry->cascade]->flags.titled)
                        y -= menu->cascades[entry->cascade]->frame->top_width;
                }

                wMenuMapAt(menu->cascades[entry->cascade], x, y, False);
                menu->cascades[entry->cascade]->parent = menu;
            } else {
                return;
            }
        }
        paintEntry(menu, entry_no, True);
    }
}


static WMenu*
findMenu(WScreen *scr, int *x_ret, int *y_ret)
{
    WMenu *menu;
    WObjDescriptor *desc;
    Window root_ret, win, junk_win;
    int x, y, wx, wy;
    unsigned int mask;
    
    XQueryPointer(dpy, scr->root_win, &root_ret, &win, &x, &y, &wx, &wy,
		  &mask);
    
    if (win==None) return NULL;
    
    if (XFindContext(dpy, win, wWinContext, (XPointer *)&desc)==XCNOENT)
      return NULL;
    
    if (desc->parent_type == WCLASS_MENU) {
	menu = (WMenu*)desc->parent;
	XTranslateCoordinates(dpy, root_ret, menu->menu->window, wx, wy, 
			      x_ret, y_ret, &junk_win);
	return menu;
    }
    return NULL;
}




static void
closeCascade(WMenu *menu)
{
    WMenu *parent=menu->parent;

    if (menu->flags.brother 
	|| (!menu->flags.buttoned 
	    && (!menu->flags.app_menu||menu->parent!=NULL))) {

	selectEntry(menu, -1);
	XSync(dpy, 0);
#if (MENU_BLINK_DELAY > 2)
	wusleep(MENU_BLINK_DELAY/2);
#endif
	wMenuUnmap(menu);
	while (parent!=NULL
	       && (parent->parent!=NULL || !parent->flags.app_menu
		   || parent->flags.brother)
	       && !parent->flags.buttoned) {
	    selectEntry(parent, -1);
	    wMenuUnmap(parent);
	    parent = parent->parent;
	}
	if (parent)
	  selectEntry(parent, -1);
    }
}


static void
closeBrotherCascadesOf(WMenu *menu)
{
    WMenu *tmp;
    int i;
    
    for (i=0; i<menu->cascade_no; i++) {
	if (menu->cascades[i]->flags.brother) {
	    tmp = menu->cascades[i];
	} else {
	    tmp = menu->cascades[i]->brother;
	}
	if (tmp->flags.mapped) {
	    selectEntry(tmp->parent, -1);
	    closeBrotherCascadesOf(tmp);
	    break;
	}
    }
}


#define getEntryAt(menu, x, y)   ((y)<0 ? -1 : (y)/(menu->entry_height))


static WMenu*
parentMenu(WMenu *menu)
{
    WMenu *parent;
    WMenuEntry *entry;

    if (menu->flags.buttoned)
	return menu;

    while (menu->parent && menu->parent->flags.mapped) {
        parent = menu->parent;
        if (parent->selected_entry < 0)
            break;
        entry = parent->entries[parent->selected_entry];
        if (!entry->flags.enabled || entry->cascade<0 || !parent->cascades ||
            parent->cascades[entry->cascade] != menu)
            break;
        menu = parent;
        if (menu->flags.buttoned)
            break;
    }
    
    return menu;
}



/*
 * Will raise the passed menu, if submenu = 0
 * If submenu > 0 will also raise all mapped submenus
 * until the first buttoned one
 * If submenu < 0 will also raise all mapped parent menus
 * until the first buttoned one
 */

static void
raiseMenus(WMenu *menu, int submenus)
{
    WMenu *submenu;
    int i;

    if(!menu) return;

    wRaiseFrame(menu->frame->core);

    if (submenus>0 && menu->selected_entry>=0) {
        i = menu->entries[menu->selected_entry]->cascade;
        if (i>=0 && menu->cascades) {
            submenu = menu->cascades[i];
            if (submenu->flags.mapped && !submenu->flags.buttoned)
                raiseMenus(submenu, submenus);
        }
    }
    if (submenus<0 && !menu->flags.buttoned &&
        menu->parent && menu->parent->flags.mapped)
        raiseMenus(menu->parent, submenus);
}


WMenu*
wMenuUnderPointer(WScreen *screen)
{
    WObjDescriptor *desc;
    Window root_ret, win;
    int dummy;
    unsigned int mask;

    XQueryPointer(dpy, screen->root_win, &root_ret, &win, &dummy, &dummy,
                  &dummy, &dummy, &mask);

    if (win==None) return NULL;

    if (XFindContext(dpy, win, wWinContext, (XPointer *)&desc)==XCNOENT)
        return NULL;

    if (desc->parent_type == WCLASS_MENU)
        return (WMenu *)desc->parent;
    return NULL;
}



#define MIN(a,b)  (((a) > (b)) ? (b) : (a))


static void
getPointerPosition(WScreen *scr, int *x, int *y)
{
    Window root_ret, win;
    int wx, wy;
    unsigned int mask;

    XQueryPointer(dpy, scr->root_win, &root_ret, &win, x, y, &wx, &wy, &mask);
}


static void
getScrollAmount(WMenu *menu, int *hamount, int *vamount)
{
    WScreen *scr = menu->menu->screen_ptr;
    int menuX1 = menu->frame_x;
    int menuY1 = menu->frame_y;
    int menuX2 = menu->frame_x + MENUW(menu);
    int menuY2 = menu->frame_y + MENUH(menu);
    int screenW = scr->scr_width;
    int screenH = scr->scr_height;
    int xroot, yroot;
    
    *hamount = 0;
    *vamount = 0;
    
    getPointerPosition(scr, &xroot, &yroot);


    if (xroot <= 1 && menuX1 < 0) {
	/* scroll to the right */
	*hamount = MIN(MENU_SCROLL_STEP, abs(menuX1));

    } else if (xroot >= screenW-2 && menuX2 > screenW-1) {
	/* scroll to the left */
	*hamount = MIN(MENU_SCROLL_STEP, abs(menuX2-screenW-1));
	
	if (*hamount==0)
	    *hamount = 1;

	*hamount = -*hamount;
    }
    
    if (yroot <= 1 && menuY1 < 0) {
	/* scroll down */
	*vamount = MIN(MENU_SCROLL_STEP, abs(menuY1));
        
    } else if (yroot >= screenH-2 && menuY2 > screenH-1) {
	/* scroll up */
	*vamount = MIN(MENU_SCROLL_STEP, abs(menuY2-screenH-1));
        
	*vamount = -*vamount;
    }
}


static void
dragScrollMenuCallback(void *data)
{
    WMenu *menu = (WMenu*)data;
    WScreen *scr = menu->menu->screen_ptr;
    WMenu *parent = parentMenu(menu);
    int hamount, vamount;
    int x, y;
    int newSelectedEntry;
    
    getScrollAmount(menu, &hamount, &vamount);
    
    
    if (hamount != 0 || vamount != 0) {
	wMenuMove(parent, parent->frame_x + hamount,
		  parent->frame_y + vamount, True);
	if (findMenu(scr, &x, &y)) {
	    newSelectedEntry = getEntryAt(menu, x, y);
	    selectEntry(menu, newSelectedEntry);
	} else {
	    /* Pointer fell outside of menu. If the selected entry is
	     * not a submenu, unselect it */
	    if (menu->selected_entry >= 0 
		&& menu->entries[menu->selected_entry]->cascade<0)
		selectEntry(menu, -1);
	    newSelectedEntry = 0;
	}
	
	/* paranoid check */
	if (newSelectedEntry >= 0) {
	    /* keep scrolling */
	    menu->timer = WMAddTimerHandler(MENU_SCROLL_DELAY, 
					    dragScrollMenuCallback, menu);
	} else {
	    menu->timer = NULL;
	}
    } else {
	/* don't need to scroll anymore */
	menu->timer = NULL;
	if (findMenu(scr, &x, &y)) {
	    newSelectedEntry = getEntryAt(menu, x, y);
	    selectEntry(menu, newSelectedEntry);
	}
    }
}


static void
scrollMenuCallback(void *data)
{
    WMenu *menu = (WMenu*)data;
    WMenu *parent = parentMenu(menu);
    int hamount = 0;		       /* amount to scroll */
    int vamount = 0;

    getScrollAmount(menu, &hamount, &vamount);
    
    if (hamount != 0 || vamount != 0) {
	wMenuMove(parent, parent->frame_x + hamount,
		  parent->frame_y + vamount, True);

	/* keep scrolling */
	menu->timer = WMAddTimerHandler(MENU_SCROLL_DELAY, 
					scrollMenuCallback, menu);
    } else {
	/* don't need to scroll anymore */
	menu->timer = NULL;
    }
}



#define MENU_SCROLL_BORDER   5

static int
isPointNearBoder(WMenu *menu, int x, int y)
{
    int menuX1 = menu->frame_x;
    int menuY1 = menu->frame_y;
    int menuX2 = menu->frame_x + MENUW(menu);
    int menuY2 = menu->frame_y + MENUH(menu);
    int scrXe = menu->menu->screen_ptr->scr_width-1;
    int scrYe = menu->menu->screen_ptr->scr_height-1;
    int flag = 0;
    
    if (x >= menuX1 && x <= menuX2 && (y < MENU_SCROLL_BORDER
				       || y > scrYe-MENU_SCROLL_BORDER))
	flag = 1;
    else if (y >= menuY1 && y <= menuY2 && (x < MENU_SCROLL_BORDER
					    || x > scrXe-MENU_SCROLL_BORDER))
	flag = 1;
	
    return flag;
}


void
wMenuScroll(WMenu *menu, XEvent *event)
{
    WMenu *smenu;
    WMenu *omenu = parentMenu(menu);
    WScreen *scr = menu->frame->screen_ptr;
    int done = 0;
    int jump_back = 0;
    int old_frame_x = omenu->frame_x;
    int old_frame_y = omenu->frame_y;
    XEvent ev;

#ifdef DEBUG
    puts("Entering menu Scroll");
#endif

    if ((/*omenu->flags.buttoned &&*/ !wPreferences.wrap_menus)
        || omenu->flags.app_menu) {
        jump_back = 1;
    }

    if (!wPreferences.wrap_menus)
        raiseMenus(omenu, True);
    else
        raiseMenus(menu, False);

    if (!menu->timer)
	scrollMenuCallback(menu);    

    while(!done) {
        int x, y, on_border, on_x_edge, on_y_edge, on_title;

        WMNextEvent(dpy, &ev);
        switch (ev.type) {
        case EnterNotify:
            WMHandleEvent(&ev);
        case MotionNotify:
            x = (ev.type==MotionNotify) ? ev.xmotion.x_root : ev.xcrossing.x_root;
            y = (ev.type==MotionNotify) ? ev.xmotion.y_root : ev.xcrossing.y_root;

	    /* on_border is != 0 if the pointer is between the menu
	     * and the screen border and is close enough to the border */
	    on_border = isPointNearBoder(menu, x, y);

            smenu = wMenuUnderPointer(scr);
	    	    
            if ((smenu==NULL && !on_border) || (smenu && parentMenu(smenu)!=omenu)) {
                done = 1;
                break;
            }

            on_x_edge = x <= 1 || x >= scr->scr_width - 2;
            on_y_edge = y <= 1 || y >= scr->scr_height - 2;
            on_border = on_x_edge || on_y_edge;

            if (!on_border && !jump_back) {
                done = 1;
                break;
            }

            if (menu->timer && (smenu!=menu || (!on_y_edge && !on_x_edge))) {
                WMDeleteTimerHandler(menu->timer);
                menu->timer = NULL;
            }

            if (smenu != NULL)
                menu = smenu;

	    if (!menu->timer)
		scrollMenuCallback(menu);
            break;
        case ButtonPress:
            /* True if we push on title, or drag the omenu to other position */
            on_title = ev.xbutton.x_root >= omenu->frame_x &&
                ev.xbutton.x_root <= omenu->frame_x + MENUW(omenu) &&
                ev.xbutton.y_root >= omenu->frame_y &&
                ev.xbutton.y_root <= omenu->frame_y + omenu->frame->top_width;
            WMHandleEvent(&ev);
            smenu = wMenuUnderPointer(scr);
            if (smenu == NULL || (smenu && smenu->flags.buttoned && smenu != omenu))
                done = 1;
            else if (smenu==omenu && on_title) {
                jump_back = 0;
                done = 1;
            }
            break;
        case KeyPress:
            done = 1;
        default:
            WMHandleEvent(&ev);
            break;
        }
    }

    if (menu->timer) {
        WMDeleteTimerHandler(menu->timer);
	menu->timer = NULL;
    }

    if (jump_back)
        wMenuMove(omenu, old_frame_x, old_frame_y, True);

#ifdef DEBUG
    puts("Leaving menu Scroll");
#endif
}



static void  
menuExpose(WObjDescriptor *desc, XEvent *event)
{
    wMenuPaint(desc->parent);
}

typedef struct {
    int *delayed_select;
    WMenu *menu;
    WMHandlerID magic;
} delay_data;


static void
delaySelection(void *data)
{
    delay_data *d = (delay_data*)data;
    int x, y, entry_no;
    WMenu *menu;

    d->magic = NULL;

    menu = findMenu(d->menu->menu->screen_ptr, &x, &y);
    if (menu && (d->menu == menu || d->delayed_select)) {
        entry_no = getEntryAt(menu, x, y);
        selectEntry(menu, entry_no);
    }
    if (d->delayed_select)
	*(d->delayed_select) = 0;
}


static void  
menuMouseDown(WObjDescriptor *desc, XEvent *event)
{
    XButtonEvent *bev = &event->xbutton;
    WMenu *menu = desc->parent;
    WMenu *smenu;
    WScreen *scr=menu->frame->screen_ptr;    
    WMenuEntry *entry=NULL;
    XEvent ev;
    int close_on_exit=0;
    int done=0;
    int delayed_select = 0;
    int entry_no;
    int x, y;
    int prevx, prevy;
    int old_frame_x = 0;
    int old_frame_y = 0;
    delay_data d_data = {NULL, NULL, NULL};

    if (!wPreferences.wrap_menus) {
        smenu = parentMenu(menu);
        old_frame_x = smenu->frame_x;
        old_frame_y = smenu->frame_y;
    }
    else if (event->xbutton.window == menu->frame->core->window) {
        /* This is true if the menu was launched with right click on root window */
        delayed_select = 1;
        d_data.delayed_select = &delayed_select;
        d_data.menu = menu;
        d_data.magic = WMAddTimerHandler(wPreferences.dblclick_time,
                                        delaySelection, &d_data);
    }

    wRaiseFrame(menu->frame->core);
    
    close_on_exit = (bev->send_event || menu->flags.brother);

    smenu = findMenu(scr, &x, &y);
    if (!smenu) {
	x = -1;
	y = -1;
    } else {
	menu = smenu;
    }

    if (menu->flags.editing) {
	return;
    }
    entry_no = getEntryAt(menu, x, y);
    if (entry_no>=0) {
	entry = menu->entries[entry_no];

	if (!close_on_exit && (bev->state & ControlMask) && smenu
	    && entry->flags.editable) {
	    editEntry(smenu, entry);
	    return;
	} else if (bev->state & ControlMask) {
	    return;
	}

	if (entry->flags.enabled && entry->cascade>=0 && menu->cascades) {
	    WMenu *submenu = menu->cascades[entry->cascade];
	    /* map cascade */
            if (submenu->flags.mapped && !submenu->flags.buttoned &&
                menu->selected_entry!=entry_no) {
		wMenuUnmap(submenu);
            }
            if (!submenu->flags.mapped && !delayed_select) {
		selectEntry(menu, entry_no);
	    } else if (!submenu->flags.buttoned) {
		selectEntry(menu, -1);
	    }
	    
        } else if (!delayed_select) {
            selectEntry(menu, entry_no);
        }

        if (!wPreferences.wrap_menus && !wPreferences.scrollable_menus) {
	    if (!menu->timer)
		dragScrollMenuCallback(menu);
        }
    }
    
    prevx = bev->x_root;
    prevy = bev->y_root;
    while (!done) {
	int x, y;
	WMMaskEvent(dpy, ExposureMask|ButtonMotionMask|ButtonReleaseMask
		    |ButtonPressMask, &ev);
	switch (ev.type) {
	 case MotionNotify:	    
            smenu = findMenu(scr, &x, &y);

	    if (smenu == NULL) {
		/* moved mouse out of menu */
		
		if (!delayed_select && d_data.magic) {
		    WMDeleteTimerHandler(d_data.magic);
		    d_data.magic = NULL;
		}
		if (menu==NULL
		    || (menu->selected_entry>=0
			&& menu->entries[menu->selected_entry]->cascade>=0)) {
		    prevx = ev.xmotion.x_root;
		    prevy = ev.xmotion.y_root;
		    
		    break;
		}
		selectEntry(menu, -1);
		menu = smenu;
		prevx = ev.xmotion.x_root;
		prevy = ev.xmotion.y_root;
		break;
	    } else if (menu && menu!=smenu
		       && (menu->selected_entry<0
			   || menu->entries[menu->selected_entry]->cascade<0)) {
		selectEntry(menu, -1);

		if (!delayed_select && d_data.magic) {
		    WMDeleteTimerHandler(d_data.magic);
		    d_data.magic = NULL;
		}
	    } else {
		
		/* hysteresis for item selection */

		/* check if the motion was to the side, indicating that 
		 * the user may want to cross to a submenu */
		if (!delayed_select && menu) {
		    int dx;
		    Bool moved_to_submenu;/* moved to direction of submenu */

		    dx = abs(prevx - ev.xmotion.x_root);

		    moved_to_submenu = False;
		    if (dx > 0 /* if moved enough to the side */
			/* maybe a open submenu */
			&& menu->selected_entry>=0 
			/* moving to the right direction */
			&& (wPreferences.align_menus
			    || ev.xmotion.y_root >= prevy)) {
			int index;
			
			index = menu->entries[menu->selected_entry]->cascade;
			if (index>=0) {
			    if (menu->cascades[index]->frame_x>menu->frame_x) {
				if (prevx < ev.xmotion.x_root)
				    moved_to_submenu = True;
			    } else {
				if (prevx > ev.xmotion.x_root)
				    moved_to_submenu = True;
			    }
			}
		    }
		    

		    if (menu != smenu) {
			if (d_data.magic) {
			    WMDeleteTimerHandler(d_data.magic);
			}
			d_data.magic = NULL;
		    } else if (moved_to_submenu) {
			/* while we are moving, postpone the selection */
			if (d_data.magic) {
			    WMDeleteTimerHandler(d_data.magic);
			}
			d_data.delayed_select = NULL;
			d_data.menu = menu;
			d_data.magic = WMAddTimerHandler(MENU_SELECT_DELAY, 
							 delaySelection,
							 &d_data);
			prevx = ev.xmotion.x_root;
			prevy = ev.xmotion.y_root;
			break;
		    } else {
			if (d_data.magic)
			    WMDeleteTimerHandler(d_data.magic);
			d_data.magic = NULL;
		    }
		}
	    }
	    prevx = ev.xmotion.x_root;
	    prevy = ev.xmotion.y_root;
	    if (menu!=smenu) {
		/* pointer crossed menus */
		if (menu && menu->timer) {
		    WMDeleteTimerHandler(menu->timer);
		    menu->timer = NULL;
		}
		if (smenu)
		    dragScrollMenuCallback(smenu);
	    }
	    menu = smenu;
	    if (!menu->timer)
		dragScrollMenuCallback(menu);
	    
            if (!delayed_select) {
                entry_no = getEntryAt(menu, x, y);
                if (entry_no>=0) {
                    entry = menu->entries[entry_no];
                    if (entry->flags.enabled && entry->cascade>=0 &&
                        menu->cascades) {
                        WMenu *submenu = menu->cascades[entry->cascade];
                        if (submenu->flags.mapped && !submenu->flags.buttoned
                            && menu->selected_entry!=entry_no) {
                            wMenuUnmap(submenu);
                        }
                    }
                }
                selectEntry(menu, entry_no);
            }
	    break;

	 case ButtonPress:
	    break;

	 case ButtonRelease:
	    if (ev.xbutton.button == event->xbutton.button)
		done=1;
            break;

	 case Expose: 
            WMHandleEvent(&ev);
            break;
 	}
    }

    if (menu && menu->timer) {
	WMDeleteTimerHandler(menu->timer);
	menu->timer = NULL;
    }
    if (d_data.magic!=NULL)
        WMDeleteTimerHandler(d_data.magic);
    
    if (menu && menu->selected_entry>=0) {
	entry = menu->entries[menu->selected_entry];
	if (entry->callback!=NULL && entry->flags.enabled 
	    && entry->cascade < 0) {
	    /* blink and erase menu selection */
#if (MENU_BLINK_DELAY > 0)
	    int sel = menu->selected_entry;
	    int i;

	    for (i=0; i<MENU_BLINK_COUNT; i++) {
		paintEntry(menu, sel, False);
		XSync(dpy, 0);
		wusleep(MENU_BLINK_DELAY);
		paintEntry(menu, sel, True);
		XSync(dpy, 0);
		wusleep(MENU_BLINK_DELAY);
	    }
#endif
	    /* unmap the menu, it's parents and call the callback */
	    if (!menu->flags.buttoned && 
		(!menu->flags.app_menu||menu->parent!=NULL)) {
		closeCascade(menu);
	    } else {
		selectEntry(menu, -1);
	    }
	    (*entry->callback)(menu, entry);

	    /* If the user double clicks an entry, the entry will
	     * be executed twice, which is not good for things like
	     * the root menu. So, ignore any clicks that were generated 
	     * while the entry was being executed */
	    while (XCheckTypedWindowEvent(dpy, menu->menu->window, 
					  ButtonPress, &ev));
	} else if (entry->callback!=NULL && entry->cascade<0) {
	    selectEntry(menu, -1);
	} else {	    
	    if (entry->cascade>=0 && menu->cascades
		&& menu->cascades[entry->cascade]->flags.brother) {
		selectEntry(menu, -1);
	    }
	}
    }

    if (((WMenu*)desc->parent)->flags.brother || close_on_exit
	|| !smenu)
	closeCascade(desc->parent);

    /* close the cascade windows that should not remain opened */
    closeBrotherCascadesOf(desc->parent);

    if (!wPreferences.wrap_menus)
        wMenuMove(parentMenu(desc->parent), old_frame_x, old_frame_y, True);
}


void
wMenuMove(WMenu *menu, int x, int y, int submenus)
{
    WMenu *submenu;
    int i;
    
    if (!menu) return;
    
    menu->frame_x = x;
    menu->frame_y = y;
    XMoveWindow(dpy, menu->frame->core->window, x, y);
      
    if (submenus>0 && menu->selected_entry>=0) {
	i = menu->entries[menu->selected_entry]->cascade;
	
	if (i>=0 && menu->cascades) {
	    submenu = menu->cascades[i];
	    if (submenu->flags.mapped && !submenu->flags.buttoned)
	      if (wPreferences.align_menus) {
		  wMenuMove(submenu, x + MENUW(menu), y, submenus);
	      } else {
		  wMenuMove(submenu, x+ MENUW(menu),
			    y + submenu->entry_height*menu->selected_entry,
			    submenus);
	      }
	}
    }
    if (submenus<0 && menu->parent!=NULL && menu->parent->flags.mapped &&
	!menu->parent->flags.buttoned) {
	if (wPreferences.align_menus) {
	    wMenuMove(menu->parent, x - MENUW(menu->parent), y, submenus);
	} else {
	    wMenuMove(menu->parent, x - MENUW(menu->parent), menu->frame_y
		      - menu->parent->entry_height*menu->parent->selected_entry,
		      submenus);
	}		  
    }
}


static void
changeMenuLevels(WMenu *menu, int lower)
{
    int i;

    if (!lower) {
	ChangeStackingLevel(menu->frame->core, (!menu->parent ? WMMainMenuLevel
					  : WMSubmenuLevel));
	wRaiseFrame(menu->frame->core);
	menu->flags.lowered = 0;
    } else {
	ChangeStackingLevel(menu->frame->core, WMNormalLevel);
	wLowerFrame(menu->frame->core);
	menu->flags.lowered = 1;
    }
    for (i=0; i<menu->cascade_no; i++) {
	if (menu->cascades[i]
	    && !menu->cascades[i]->flags.buttoned
	    && menu->cascades[i]->flags.lowered!=lower) {
	    changeMenuLevels(menu->cascades[i], lower);
	}
    }
}



static void 
menuTitleDoubleClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WMenu *menu = data;
    int lower;
    
    if (event->xbutton.state & MOD_MASK) {
	if (menu->flags.lowered) {
	    lower = 0;
	} else {
	    lower = 1;
	}
	changeMenuLevels(menu, lower);
    }
}


static void 
menuTitleMouseDown(WCoreWindow *sender, void *data, XEvent *event)
{
    WMenu *menu = data;
    WMenu *tmp;
    XEvent ev;
    int x=menu->frame_x, y=menu->frame_y;
    int dx=event->xbutton.x_root, dy=event->xbutton.y_root;
    int i, lower;
    
#ifdef DEBUG
    printf("Moving menu\n");
#endif
    
    /* can't touch the menu copy */
    if (menu->flags.brother)
	return;
    
    if (event->xbutton.button != Button1 && event->xbutton.button != Button2)
      return;
    if (XGrabPointer(dpy, menu->frame->titlebar->window, False, 
		     ButtonMotionMask|ButtonReleaseMask|ButtonPressMask,
		     GrabModeAsync, GrabModeAsync, None, 
		     wCursor[WCUR_MOVE], CurrentTime)!=GrabSuccess) {
#ifdef DEBUG0
	wwarning("pointer grab failed for menu move\n");
#endif
	return;
    }
    if (event->xbutton.state & MOD_MASK) {
	wLowerFrame(menu->frame->core);
	lower = 1;
    } else {
	wRaiseFrame(menu->frame->core);
	lower = 0;
    }
    tmp = menu;
    
    /* lower/raise all submenus */
    while (1) {
	if (tmp->selected_entry>=0 && tmp->cascades) {
	    tmp = tmp->cascades[tmp->entries[tmp->selected_entry]->cascade];
	    if (!tmp || !tmp->flags.mapped)
	      break;
	    if (lower)
	      wLowerFrame(tmp->frame->core);
	    else
	      wRaiseFrame(tmp->frame->core);
	} else {
	    break;
	}
    }
    /* tear off the menu if it's a root menu or a cascade 
     application menu */
    if (!menu->flags.buttoned && !menu->flags.brother 
	&& (!menu->flags.app_menu||menu->parent!=NULL)) {
	menu->flags.buttoned=1;
	wFrameWindowShowButton(menu->frame, WFF_RIGHT_BUTTON);
	if (menu->parent) {
	    /* turn off selected menu entry in parent menu */
	    selectEntry(menu->parent, -1);

	    /* make parent map the copy in place of the original */
	    for (i=0; i<menu->parent->cascade_no; i++) {
		if (menu->parent->cascades[i] == menu) {
		    menu->parent->cascades[i] = menu->brother;
		    break;
		}
	    }
	}
    }
    
    while(1) {
	WMMaskEvent(dpy, ButtonMotionMask|ButtonReleaseMask|ButtonPressMask
		    |ExposureMask, &ev);
	switch (ev.type) {
	 case MotionNotify:
	    x += ev.xmotion.x_root - dx;
	    y += ev.xmotion.y_root - dy;
	    dx = ev.xmotion.x_root;
	    dy = ev.xmotion.y_root;
	    wMenuMove(menu, x, y, True);
	    break;

	 case ButtonPress:
	    break;

	 case ButtonRelease:
	    if (ev.xbutton.button != event->xbutton.button)
		break;
#ifdef DEBUG
	    printf("End menu move\n");
#endif
	    XUngrabPointer(dpy, CurrentTime);
	    return;
	    
	 default:
	    WMHandleEvent(&ev);
	}
    }
}

/*
 *----------------------------------------------------------------------
 * menuCloseClick--
 * 	Handles mouse click on the close button of menus. The menu is
 * closed when the button is clicked. 
 * 
 * Side effects:
 * 	The closed menu is reinserted at it's parent menus
 * cascade list.
 *---------------------------------------------------------------------- 
 */
static void 
menuCloseClick(WCoreWindow *sender, void *data, XEvent *event)
{
    WMenu *menu = (WMenu*)data;
    WMenu *parent = menu->parent;
    int i;
    
    if (parent) {
	for (i=0; i<parent->cascade_no; i++) {
	    /* find the entry that points to the copy */
	    if (parent->cascades[i] == menu->brother) {
		/* make it point to the original */
		parent->cascades[i] = menu;
		menu->parent = parent;
		break;
	    }
	}
    }
    wMenuUnmap(menu);
}


void
wMenuSaveState(WScreen *scr)
{
    proplist_t menus, key, value;
    int save_menus = 0;
    char buffer[256];

    menus = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    if (scr->root_menu && scr->root_menu->flags.buttoned) {
        sprintf(buffer, "%i,%i", scr->root_menu->frame_x,
                                 scr->root_menu->frame_y);
        key = PLMakeString("RootMenu");
        value = PLMakeString(buffer);
        PLInsertDictionaryEntry(menus, key, value);
        PLRelease(key);
        PLRelease(value);
        save_menus = 1;
    }

    if (scr->switch_menu && scr->switch_menu->flags.buttoned) {
        sprintf(buffer, "%i,%i", scr->switch_menu->frame_x,
                                 scr->switch_menu->frame_y);
        key = PLMakeString("SwitchMenu");
        value = PLMakeString(buffer);
        PLInsertDictionaryEntry(menus, key, value);
        PLRelease(key);
        PLRelease(value);
        save_menus = 1;
    }
    
    if (scr->workspace_menu && scr->workspace_menu->flags.buttoned) {
        sprintf(buffer, "%i,%i", scr->workspace_menu->frame_x,
                                 scr->workspace_menu->frame_y);
        key = PLMakeString("WorkspaceMenu");
        value = PLMakeString(buffer);
        PLInsertDictionaryEntry(menus, key, value);
        PLRelease(key);
        PLRelease(value);
        save_menus = 1;
    }
    
    if (save_menus) {
        key = PLMakeString("Menus");
        PLInsertDictionaryEntry(scr->session_state, key, menus);
        PLRelease(key);
    }
    PLRelease(menus);
}


#define COMPLAIN(key) wwarning(_("bad value in menus state info:%s"), key)


static int
restoreMenu(WScreen *scr, proplist_t menu, int which)
{
    int i, x, y;
    WMenu *pmenu = NULL;
    
    if (!menu)
        return False;
    
    if (!PLIsString(menu)) {
        COMPLAIN("Position");
        return False;
    }

    if (sscanf(PLGetString(menu), "%i,%i", &x, &y)!=2)
        COMPLAIN("Position");

    if (which & WSS_ROOTMENU) {
        OpenRootMenu(scr, x, y, False);
        pmenu = scr->root_menu;
    }
    else if (which & WSS_SWITCHMENU) {
        OpenSwitchMenu(scr, x, y, False);
        pmenu = scr->switch_menu;
    }
    else if (which & WSS_WSMENU) {
        OpenWorkspaceMenu(scr, x, y);
        pmenu = scr->workspace_menu;
        if (pmenu->parent) {
            /* make parent map the copy in place of the original */
            for (i=0; i<pmenu->parent->cascade_no; i++) {
                if (pmenu->parent->cascades[i] == pmenu) {
                    pmenu->parent->cascades[i] = pmenu->brother;
                    break;
                }
            }
        }
    }

    if (pmenu) {
        int width = MENUW(pmenu);
        int height = MENUH(pmenu);
        
        x = (x < -width) ? 0 : x;
        x = (x > scr->scr_width) ? scr->scr_width - width : x;
        y = (y < 0) ? 0 : y;
        y = (y > scr->scr_height) ? scr->scr_height - height : y;
        wMenuMove(pmenu, x, y, True);
        pmenu->flags.buttoned = 1;
        wFrameWindowShowButton(pmenu->frame, WFF_RIGHT_BUTTON);
        return True;
    }
    return False;
}


void
wMenuRestoreState(WScreen *scr)
{
    proplist_t menus, menu, key, rkey, skey, wkey;

    key = PLMakeString("Menus");
    menus = PLGetDictionaryEntry(scr->session_state, key);
    PLRelease(key);

    if (!menus)
        return;

    /* restore menus */

    rkey = PLMakeString("RootMenu");
    skey = PLMakeString("SwitchMenu");
    wkey = PLMakeString("WorkspaceMenu");
    menu = PLGetDictionaryEntry(menus, rkey);
    restoreMenu(scr, menu, WSS_ROOTMENU);
    menu = PLGetDictionaryEntry(menus, skey);
    restoreMenu(scr, menu, WSS_SWITCHMENU);
    menu = PLGetDictionaryEntry(menus, wkey);
    restoreMenu(scr, menu, WSS_WSMENU);

    PLRelease(rkey);
    PLRelease(skey);
    PLRelease(wkey);
}


void
OpenWorkspaceMenu(WScreen *scr, int x, int y)
{
    WMenu *menu, *parent;
    WMenuEntry *entry;

    if (!scr->root_menu) {
        OpenRootMenu(scr, scr->scr_width*2, 0, False);
        wMenuUnmap(scr->root_menu);
    }

    menu = scr->workspace_menu;
    if (menu) {
        if (menu->flags.mapped) {
            if (!menu->flags.buttoned) {
                wMenuUnmap(menu);
                parent = menu->parent;
                if (parent && parent->selected_entry >= 0) {
                    entry = parent->entries[parent->selected_entry];
                    if (parent->cascades[entry->cascade] == menu) {
                        selectEntry(parent, -1);
                        wMenuMapAt(menu, x, y, False);
                    }
                }
            } else {
                wRaiseFrame(menu->frame->core);
		wMenuMapCopyAt(menu, x, y);
            }
        }
        else {
            wMenuMapAt(menu, x, y, False);
        }
    }
}


