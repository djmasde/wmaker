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

#ifndef WMFUNCS_H_
#define WMFUNCS_H_

#include <sys/types.h>
#include <stdio.h>

#include "window.h"

typedef void (WCallBack)(void *cdata);

typedef void (WDeathHandler)(pid_t pid, unsigned int status, void *cdata);

void RestoreDesktop(WScreen *scr);

void Exit(int status);

void Restart(char *manager);

void SetupEnvironment(WScreen *scr);

void DispatchEvent(XEvent *event);

void WipeDesktop(WScreen *scr);

Bool wRootMenuPerformShortcut(XEvent *event);

void wRootMenuBindShortcuts(Window window);

void OpenRootMenu(WScreen *scr, int x, int y, int keyboard);

void OpenSwitchMenu(WScreen *scr, int x, int y, int keyboard);

void OpenWindowMenu(WWindow *wwin, int x, int y, int keyboard);

void OpenWorkspaceMenu(WScreen *scr, int x, int y);

void CloseWindowMenu(WScreen *scr);

void UpdateSwitchMenu(WScreen *scr, WWindow *wwin, int action);

void UpdateSwitchMenuWorkspace(WScreen *scr, int workspace);

WMagicNumber wAddDeathHandler(pid_t pid, WDeathHandler *callback, void *cdata);

void wColormapInstallForWindow(WScreen *scr, WWindow *wwin);

void wColormapInstallRoot(WScreen *scr);

void wColormapUninstallRoot(WScreen *scr);

void wColormapAllowClientInstallation(WScreen *scr, Bool starting);

Pixmap LoadIcon(WScreen *scr, char *path, char *mask, int title_height);

void PlaceIcon(WScreen *scr, int *x_ret, int *y_ret);

void PlaceWindow(WWindow *wwin, int *x_ret, int *y_ret,
                 unsigned int width, unsigned int height);

#ifdef USECPP
char *MakeCPPArgs(char *path);
#endif

char *ExpandOptions(WScreen *scr, char *cmdline);

Bool IsDoubleClick(WScreen *scr, XEvent *event);

WWindow *NextFocusWindow(WScreen *scr);
WWindow *PrevFocusWindow(WScreen *scr);

void SlideWindow(Window win, int from_x, int from_y, int to_x, int to_y);

char *ShrinkString(WFont *font, char *string, int width);

char *FindImage(char **paths, char *file);

RImage*wGetImageForWindowName(WScreen *scr, char *winstance, char *wclass);

BOOL StringCompareHook(proplist_t pl1, proplist_t pl2);

int IsEof(FILE * stream);	/* feof that stats pipes */

char *FlattenStringList(char **list, int count);

void ParseWindowName(proplist_t value, char **winstance, char **wclass,
                     char *where);

char *GetShortcutString(char *text);

char *EscapeWM_CLASS(char *name, char *class);

void UnescapeWM_CLASS(char *str, char **name, char **class);

#ifdef NUMLOCK_HACK
void wHackedGrabKey(int keycode, unsigned int modifiers,
		    Window grab_window, Bool owner_events, int pointer_mode,
		    int keyboard_mode);
#endif

void wHackedGrabButton(unsigned int button, unsigned int modifiers, 
		       Window grab_window, Bool owner_events, 
		       unsigned int event_mask, int pointer_mode, 
		       int keyboard_mode, Window confine_to, Cursor cursor);


/* this function is in dock.c */
void ParseCommand(char *command, char ***argv, int *argc);

/* This function is in moveres.c. */
void wGetGeometryWindowSize(WScreen *scr, unsigned int *width, 
			    unsigned int *height);

void ExecExitScript();

/****** I18N Wrapper for XFetchName,XGetIconName ******/

Bool wFetchName(Display *dpy, Window win, char **winname);
Bool wGetIconName(Display *dpy, Window win, char **iconname);

#ifdef I18N_MB
/*void wTextWidth(XFontSet *font, char *text, int length);*/
#define wDrawString(d,f,gc,x,y,text,textlen)	\
	XmbDrawString(dpy, d, (f)->font, gc, (x), (y), text, textlen)

#define wTextWidth(font,text,textlen) XmbTextEscapement(font,text,textlen)

#else /* !I18N_MB */

#define wTextWidth(font,text,textlen) XTextWidth(font,text,textlen)

/*void wTextWidth(XFontStruct *font, char *text, int length);*/
#define wDrawString(d,font,gc,x,y,text,textlen)	\
	XDrawString(dpy, d, gc, (x), (y), text, textlen)

#endif /* !I18N_MB */


#endif
