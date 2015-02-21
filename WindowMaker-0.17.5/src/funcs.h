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

#ifndef WMFUNCS_H_
#define WMFUNCS_H_

#include <sys/types.h>
#include <stdio.h>

#include "window.h"

typedef void (WCallBack)(void *cdata);

typedef void (WDeathHandler)(pid_t pid, unsigned int status, void *cdata);

typedef void* WMagicNumber;

void RestoreDesktop(WScreen *scr);

void Restart(char *manager);

void DispatchEvent(XEvent *event);

void WipeDesktop(WScreen *scr);

void wRootMenuPerformShortcut(XEvent *event);

void wRootMenuBindShortcuts(Window window);

void OpenRootMenu(WScreen *scr, int x, int y, int keyboard);

void OpenSwitchMenu(WScreen *scr, int x, int y, int keyboard);

void OpenWindowMenu(WWindow *wwin, int x, int y, int keyboard);

void OpenWorkspaceMenu(WScreen *scr, int x, int y, int keyboard);

void CloseWindowMenu(WScreen *scr);

void UpdateSwitchMenu(WScreen *scr, WWindow *wwin, int action);

void UpdateSwitchMenuWorkspace(WScreen *scr, int workspace);

WMagicNumber wAddDeathHandler(pid_t pid, WDeathHandler *callback, void *cdata);

WMagicNumber wAddWindowSavedState(char *instance, char *class, char *command,
                                  pid_t pid, WSavedState *state);

WMagicNumber wGetWindowSavedState(Window win);

void wDeleteWindowSavedState(WMagicNumber id);


void InstallColormap(WScreen *screen, struct WWindow *wwin);

void ChangeColormap(struct WWindow *wwin, XEvent *event);

Pixmap LoadIcon(WScreen *scr, char *path, char *mask, int title_height);

void PlaceIcon(WScreen *scr, int *x_ret, int *y_ret);

void PlaceWindow(WWindow *wwin, int *x_ret, int *y_ret);

#ifdef USECPP
char *MakeCPPArgs(char *path);
#endif

char *ExpandOptions(WScreen *scr, char *cmdline);

void ReInstallActiveColormap(void);
void InstallWindowColormaps (WWindow *tmp);
void InstallRootColormap();
void UninstallRootColormap();


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

/* this function is in dock.c */
void ParseCommand(WScreen *scr, char *command, char ***argv, int *argc);

/* This function is in moveres.c. */
void wGetGeometryWindowSize(WScreen *scr, unsigned int *width, 
			    unsigned int *height);

/****** I18N Wrapper for XFetchName,XGetIconName ******/

extern Status MyXFetchName(Display *dpy, Window win, char **winname);
extern Status MyXGetIconName(Display *dpy, Window win, char **iconname);

#ifdef I18N_MB
#define MyTextWidth(x,y,z) XmbTextEscapement(x,y,z)
#else
#define MyTextWidth(x,y,z) XTextWidth(x,y,z)  
#endif


#endif
