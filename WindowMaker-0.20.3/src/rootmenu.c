/* rootmenu.c- user defined menu
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "WindowMaker.h"
#include "actions.h"
#include "menu.h"
#include "funcs.h" 
#include "dialog.h"
#include "keybind.h"
#include "stacking.h"
#include "workspace.h"
#include "defaults.h"
#include "framewin.h"
#include "session.h"
#include "xmodifier.h"
#include <proplist.h>

#include "list.h"


extern char *Locale;

extern WDDomain *WDRootMenu;

extern wCursor[WCUR_LAST];

extern Time LastTimestamp;

extern WPreferences wPreferences;

extern int wScreenCount;

static WMenu *readMenuPipe(WScreen *scr, char **file_name);
static WMenu *readMenuFile(WScreen *scr, char *file_name);
static WMenu *readMenuDirectory(WScreen *scr, char *title, char **file_name,
				char *command);


typedef struct Shortcut {
    struct Shortcut *next;
    
    int modifier;
    KeyCode keycode;
    WMenuEntry *entry;
    WMenu *menu;
} Shortcut;



static Shortcut *shortcutList = NULL;


/*
 * Syntax:
 * # main menu
 * "Menu Name" MENU
 * 	"Title" EXEC command_to_exec -params
 * 	"Submenu" MENU
 * 		"Title" EXEC command_to_exec -params
 * 	"Submenu" END
 * 	"Workspaces" WORKSPACE_MENU
 * 	"Title" built_in_command
 * 	"Quit" EXIT
 * 	"Quick Quit" EXIT QUICK
 * "Menu Name" END
 * 
 * Commands may be preceded by SHORTCUT key
 * 
 * Built-in commands:
 *
 * INFO_PANEL - shows the Info Panel
 * LEGAL_PANEL - shows the Legal info panel
 * SHUTDOWN [QUICK] - closes the X server [without confirmation]
 * REFRESH - forces the desktop to be repainted
 * EXIT [QUICK] - exit the window manager [without confirmation]
 * EXEC <program> - execute an external program
 * WORKSPACE_MENU - places the workspace submenu
 * ARRANGE_ICONS
 * RESTART [<window manager>] - restarts the window manager
 * SHOW_ALL - unhide all windows on workspace
 * HIDE_OTHERS - hides all windows excep the focused one
 * OPEN_MENU file - read menu data from file which must be a valid menu file.
 * OPEN_MENU /some/dir [/some/other/dir ...] [WITH command -options]
 *              - read menu data from directory(ies) and
 * 		  eventually precede each with a command.
 * OPEN_MENU | command
 *              - opens command and uses its stdout to construct and insert
 *                the resulting menu in current position. The output of
 *                command must be a valid menu description.
 *                The space between '|' and command is optional.
 * SAVE_SESSION - saves the current state of the desktop, which include
 *		  all running applications, all their hints (geometry,
 *		  position on screen, workspace they live on, the dock
 *		  or clip from where they were launched, and
 *		  if minimized, shaded or hidden. Also saves the current
 *		  workspace the user is on. All will be restored on every
 *		  start of windowmaker until another SAVE_SESSION or
 *		  CLEAR_SESSION is used. If SaveSessionOnExit = Yes; in
 *		  WindowMaker domain file, then saving is automatically
 *		  done on every windowmaker exit, overwriting any
 *		  SAVE_SESSION or CLEAR_SESSION (see below).
 * CLEAR_SESSION - clears any previous saved session. This will not have
 *		  any effect if SaveSessionOnExit is True.
 *							     
 */

#define MAX(a,b)	((a)>(b) ? (a) : (b))


#define M_QUICK		1

/* menu commands */

static void
execCommand(WMenu *menu, WMenuEntry *entry)
{
    char *cmdline;
    static char *shell = NULL;

    /*
     * This have a problem: if the shell is tcsh (not sure about others)
     * and ~/.tcshrc have /bin/stty erase ^H somewhere on it, the shell
     * will block and the command will not be executed.
    if (!shell) {
	shell = getenv("SHELL");
	if (!shell)
	    shell = "/bin/sh";
    }
     */
    shell = "/bin/sh";

    cmdline = ExpandOptions(menu->frame->screen_ptr, (char*)entry->clientdata);

    XGrabPointer(dpy, menu->frame->screen_ptr->root_win, True, 0,
		 GrabModeAsync, GrabModeAsync, None, wCursor[WCUR_WAIT],
		 CurrentTime);
    XSync(dpy, 0);

    if (cmdline) {
        /* We should not use ParseCommand() here, because it does not handle
         * correctly '~/' expansion and '>' redirection.
         * While we can do '~/' expansion ourselves, we can do nothing about
         * '>' redirection.
         * So we better let /bin/sh to do that for us. Dan.
	 * Ok. -Alfredo
         */
        if (fork()==0) {

	    SetupEnvironment(menu->frame->screen_ptr);

	    CloseDescriptors();

#ifdef HAVE_SETPGID
            setpgid(0, 0);
#endif
            execl(shell, shell, "-c", cmdline, NULL);
	    wsyserror("could not exec %s -c %s\n", shell, cmdline);
            Exit(-1);
        }
	free(cmdline);
    }
    XUngrabPointer(dpy, CurrentTime);
    XSync(dpy, 0);
}


static void
exitCommand(WMenu *menu, WMenuEntry *entry)
{
    static int inside = 0;

    /* prevent reentrant calls */
    if (inside)
	return;
    inside = 1;

    if ((int)entry->clientdata==M_QUICK
	|| wMessageDialog(menu->frame->screen_ptr, _("Exit"),
			  _("Exit window manager?"), 
			  _("Exit"), _("Cancel"), NULL)==WAPRDefault) {
	int i;
#ifdef DEBUG
	printf("Exiting WindowMaker.\n");
#endif
	for (i=0; i<wScreenCount; i++) {
	    WScreen *scr;

	    scr = wScreenWithNumber(i);
	    if (scr)
		wScreenSaveState(scr);
	}

	RestoreDesktop(NULL);
	
	ExecExitScript();
	Exit(0);
    }
    inside = 0;
}


static void
shutdownCommand(WMenu *menu, WMenuEntry *entry)
{
    static int inside = 0;
    int result;
    int i;

    /* prevent reentrant calls */
    if (inside)
	return;
    inside = 1;

#define R_CANCEL 0
#define R_CLOSE 1
#define R_KILL 2

    
    result = R_CANCEL;
    if ((int)entry->clientdata==M_QUICK)
	result = R_CLOSE;
    else {
#ifdef R6SM
	if (wSessionIsManaged()) {
	    int r;
	    
	    r = wMessageDialog(menu->frame->screen_ptr, 
			       _("Close X session"), 
			       _("Close Window System session?\n"
				 "Kill might close applications with unsaved data."),
			       _("Close"), _("Kill"), _("Cancel"));
	    if (r==WAPRDefault)
		result = R_CLOSE;
	    else if (r==WAPRAlternate)
		result = R_KILL;
	} else
#endif
	{
	    int r;

	    r = wMessageDialog(menu->frame->screen_ptr, 
			       _("Kill X session"), 
			       _("Kill Window System session?\n"
				 "(all applications will be closed)"),
			       _("Kill"), _("Cancel"), NULL);
	    if (r==WAPRDefault)
		result = R_KILL;
	}
    } 
    
    if (result!=R_CANCEL) {
#ifdef R6SM
	if (result == R_CLOSE) {
	    wSessionRequestShutdown();
	} else 
#endif /* R6SM */
	{
	    for (i=0; i<wScreenCount; i++) {
		WScreen *scr;
		
		scr = wScreenWithNumber(i);
		if (scr)
		    wScreenSaveState(scr);
	    }
	    WipeDesktop(NULL);
	    
	    ExecExitScript();
	    Exit(0);
	}
    }
#undef R_CLOSE
#undef R_CANCEL
#undef R_KILL
    inside = 0;
}


static void
restartCommand(WMenu *menu, WMenuEntry *entry)
{
    int i;
    
    for (i=0; i<wScreenCount; i++) {
	WScreen *scr;
	
	scr = wScreenWithNumber(i);
	if (scr)
	    wScreenSaveState(scr);
    }
    RestoreDesktop(NULL);
    Restart((char*)entry->clientdata);
}


static void
refreshCommand(WMenu *menu, WMenuEntry *entry)
{
    wRefreshDesktop(menu->frame->screen_ptr);
}


static void
arrangeIconsCommand(WMenu *menu, WMenuEntry *entry)
{
    wArrangeIcons(menu->frame->screen_ptr, True);
}

static void
showAllCommand(WMenu *menu, WMenuEntry *entry)
{
    wShowAllWindows(menu->frame->screen_ptr);
}

static void
hideOthersCommand(WMenu *menu, WMenuEntry *entry)
{
    wHideOtherApplications(menu->frame->screen_ptr->focused_window);
}


static void
saveSessionCommand(WMenu *menu, WMenuEntry *entry)
{
    wSessionSaveState(menu->frame->screen_ptr);
}


static void
clearSessionCommand(WMenu *menu, WMenuEntry *entry)
{
    wSessionClearState(menu->frame->screen_ptr);
}


static void
infoPanelCommand(WMenu *menu, WMenuEntry *entry)
{
    wShowInfoPanel(menu->frame->screen_ptr);
}


static void
legalPanelCommand(WMenu *menu, WMenuEntry *entry)
{
    wShowLegalPanel(menu->frame->screen_ptr);
}



/********************************************************************/

static void
raiseMenus(WMenu *menu)
{
    int i;
    
    if (menu->flags.mapped) {
	wRaiseFrame(menu->frame->core);
    }
    for (i=0; i<menu->cascade_no; i++) {
	if (menu->cascades[i])
	  raiseMenus(menu->cascades[i]);
    }
}



Bool
wRootMenuPerformShortcut(XEvent *event)
{
    Shortcut *ptr;
    int modifiers;
    int done = 0;
    
    /* ignore CapsLock */
    modifiers = event->xkey.state & ValidModMask;
    
    for (ptr = shortcutList; ptr!=NULL; ptr = ptr->next) {
	if (ptr->keycode==0) 
	    continue;
	
	if (ptr->keycode==event->xkey.keycode && (ptr->modifier==modifiers)) {
	    (*ptr->entry->callback)(ptr->menu, ptr->entry);
	    done = True;
	}
    }
    return done;
}


void
wRootMenuBindShortcuts(Window window)
{
    Shortcut *ptr;
    
    ptr = shortcutList;
    while (ptr) {
	if (ptr->modifier!=AnyModifier) {
	    XGrabKey(dpy, ptr->keycode, ptr->modifier|LockMask,
		     window, True, GrabModeAsync, GrabModeAsync);
#ifdef NUMLOCK_HACK
	    wHackedGrabKey(ptr->keycode, ptr->modifier,
			   window, True, GrabModeAsync, GrabModeAsync);	    
#endif
	}
	XGrabKey(dpy, ptr->keycode, ptr->modifier, window, True, 
		 GrabModeAsync, GrabModeAsync);
	ptr = ptr->next;
    }
}


static void
rebindKeygrabs(WScreen *scr)
{
    WWindow *wwin;
    
    wwin = scr->focused_window;
    
    while (wwin!=NULL) {
	XUngrabKey(dpy, AnyKey, AnyModifier, wwin->frame->core->window);

	if (!wwin->window_flags.no_bind_keys) {
	    wWindowSetKeyGrabs(wwin);
	}
	wwin = wwin->prev;
    }
}


static void
removeShortcutsForMenu(WMenu *menu)
{
    Shortcut *ptr, *tmp;
    Shortcut *newList = NULL;

    ptr = shortcutList;
    while (ptr!=NULL) {
	tmp = ptr->next;
	if (ptr->menu == menu) {
	    free(ptr);
	} else {
	    ptr->next = newList;
	    newList = ptr;
	}
	ptr = tmp;
    }
    shortcutList = newList;
    menu->menu->screen_ptr->flags.root_menu_changed_shortcuts = 1;
}


static Bool
addShortcut(char *file, char *shortcutDefinition, WMenu *menu,
	    WMenuEntry *entry)
{
    Shortcut *ptr;    
    KeySym ksym;
    char *k;
    char buf[128], *b;

    ptr = wmalloc(sizeof(Shortcut));
    
    strcpy(buf, shortcutDefinition);
    b = (char*)buf;

    /* get modifiers */
    ptr->modifier = 0;
    while ((k = strchr(b, '+'))!=NULL) {
	int mod;
	
	*k = 0;
	mod = wXModifierFromKey(b);
	if (mod<0) {
	    wwarning(_("%s:invalid key modifier \"%s\""), file,
		     shortcutDefinition);
	    free(ptr);
	    return False;
	}
	ptr->modifier |= mod;
	
	b = k+1;
    }
    
    /* get key */
    ksym = XStringToKeysym(b);
    
    if (ksym==NoSymbol) {
	wwarning(_("%s:invalid kbd shortcut specification \"%s\" for entry %s"),
		 file, shortcutDefinition, entry->text);
	free(ptr);
	return False;
    }
    
    ptr->keycode = XKeysymToKeycode(dpy, ksym);
    if (ptr->keycode==0) {
	wwarning(_("%s:invalid key in shortcut \"%s\" for entry %s"), file,
		 shortcutDefinition, entry->text);
	free(ptr);
	return False;
    }

    ptr->menu = menu;
    ptr->entry = entry;
    
    ptr->next = shortcutList;
    shortcutList = ptr;
    
    menu->menu->screen_ptr->flags.root_menu_changed_shortcuts = 1;
    
    return True;
}


/*******************************/

static char*
cropline(char *line)
{
    char *end;
    
    if (strlen(line)==0)
	return line;
    
    end = &(line[strlen(line)])-1;
    while (isspace(*line) && *line!=0) line++;
    while (end>line && isspace(*end)) {
	*end=0;
	end--;
    }
    return line;
}


static  char*
next_token(char *line, char **next)
{
    char *tmp, c;
    char *ret;

    *next = NULL;
    while (*line==' ' || *line=='\t') line++;

    tmp = line;

    if (*tmp=='"') {
        tmp++; line++;
        while (*tmp!=0 && *tmp!='"') tmp++;
        if (*tmp!='"') {
            wwarning(_("%s: unmatched '\"' in menu file"), line);
            return NULL;
        }
    } else {
        do {
            if (*tmp=='\\')
                tmp++;

            if (*tmp!=0)
                tmp++;

        } while (*tmp!=0 && *tmp!=' ' && *tmp!='\t');
    }

    c = *tmp;
    *tmp = 0;
    ret = wstrdup(line);
    *tmp = c;

    if (c==0)
        return ret;
    else
        tmp++;

    /* skip blanks */
    while (*tmp==' ' || *tmp=='\t') tmp++;

    if (*tmp!=0)
        *next = tmp;

    return ret;
}


static void
separateCommand(char *line, char ***file, char **command)
{
    char *token, *tmp = line;
    LinkedList *list = NULL;
    int count, i;

    *file = NULL;
    *command = NULL;
    do {
        token = next_token(tmp, &tmp);
        if (token) {
            if (strcmp(token, "WITH")==0) {
                if (tmp!=NULL && *tmp!=0)
                    *command = wstrdup(tmp);
                else
                    wwarning(_("%s: missing command"), line);
                break;
            }
            list = list_cons(token, list);
        }
    } while (token!=NULL && tmp!=NULL);

    count = list_length(list);
    if (count>0) {
        *file = wmalloc(sizeof(char*)*(count+1));
        i = count;
        (*file)[count] = NULL;
        while (list!=NULL) {
            (*file)[--i] = list->head;
            list_remove_head(&list);
        }
    }
}


static void
constructMenu(WMenu *menu, WMenuEntry *entry)
{
    WMenu *submenu;
    struct stat stat_buf;
    char **path;
    char *cmd;
    char *lpath = NULL;
    int i, first=-1;
    time_t last=0;
    
    separateCommand((char*)entry->clientdata, &path, &cmd);
    if (!path || *path==NULL || **path==0) {
	wwarning(_("invalid OPEN_MENU specification: %s"),
		 (char*)entry->clientdata);
	return;
    }
    
    if (path[0][0]=='|') {
        /* pipe menu */
	
        if (!menu->cascades[entry->cascade] || 
            menu->cascades[entry->cascade]->timestamp == 0) { 
            /* parse pipe */

	    submenu = readMenuPipe(menu->frame->screen_ptr, path);

            /* there's no automatic reloading */
            if(submenu != NULL)
                submenu->timestamp = 1;
        } else { 
            submenu = NULL; 
        }
  
    } else {
        i=0;
        while(path[i] != NULL) {
	    char *tmp;

	    tmp = wexpandpath(path[i]);
	    free(path[i]);
	    path[i] = tmp;

            if (Locale) {
                lpath = wmalloc(strlen(path[i])+32);

                strcpy(lpath, path[i]);
                strcat(lpath, ".");
                strcat(lpath, Locale);
                if (stat(lpath, &stat_buf)<0) {
                    int i;
                    i = strlen(Locale);
                    if (i>2) {
                        lpath[strlen(lpath)-(i-2)]=0;
                        if (stat(lpath, &stat_buf)==0) {
                            free(path[i]);
                            path[i] = lpath;
                            lpath = NULL;
                        }
                    }
                } else {
                    free(path[i]);
                    path[i] = lpath;
                    lpath = NULL;
                }
            }

            if (lpath) {
                free(lpath);
                lpath = NULL;
            }

            if (stat(path[i], &stat_buf)==0) {
                if (last < stat_buf.st_mtime)
                    last = stat_buf.st_mtime;
                if (first<0)
                    first=i;
            } else {
                wsyserror(_("%s:could not stat menu"), path[i]);
                /*goto finish;*/
            }

            i++;
        }

        if (first < 0) {
            wsyserror(_("%s:could not stat menu :%s"), "OPEN_MENU",
                      (char*)entry->clientdata);
            goto finish;
        }
        stat(path[first], &stat_buf);
	if (!menu->cascades[entry->cascade]
	    || menu->cascades[entry->cascade]->timestamp < last) {

	    if (S_ISDIR(stat_buf.st_mode)) {
		/* menu directory */
		submenu = readMenuDirectory(menu->frame->screen_ptr,
                                            entry->text, &path[first], cmd);
		if (submenu)
		    submenu->timestamp = last;
	    } else if (S_ISREG(stat_buf.st_mode)) {
		/* menu file */

		if (cmd || path[1])
		    wwarning(_("too many parameters in OPEN_MENU: %s"), 
			     (char*)entry->clientdata);

		submenu = readMenuFile(menu->frame->screen_ptr, path[first]);
		if (submenu)
		    submenu->timestamp = stat_buf.st_mtime;
	    } else {
		submenu = NULL;
	    }
	} else {
	    submenu = NULL;
	}
    }

    if (submenu) {
	wMenuEntryRemoveCascade(menu, entry);
	wMenuEntrySetCascade(menu, entry, submenu);
    }

finish:
    i = 0;
    while (path[i]!=NULL)
        free(path[i++]);
    free(path);
    if (cmd)
	free(cmd);
}


static WMenuEntry*
addWorkspaceMenu(WScreen *scr, WMenu *menu, char *title)
{
    WMenu *wsmenu;
    WMenuEntry *entry;

    if (scr->flags.added_workspace_menu) {
	wwarning(_("There are more than one WORKSPACE_MENU commands in the applications menu. Only one is allowed."));
	return NULL;
    } else {
	scr->flags.added_workspace_menu = 1;
	
	wsmenu = wWorkspaceMenuMake(scr, True);
	scr->workspace_menu = wsmenu;
	entry=wMenuAddCallback(menu, title, NULL, NULL);
	wMenuEntrySetCascade(menu, entry, wsmenu);
	
	wWorkspaceMenuUpdate(scr, wsmenu);
    }
    return entry;
}


static WMenuEntry*
addMenuEntry(WMenu *menu, char *title, char *shortcut, char *command,
	     char *params, char *file_name)
{
    WScreen *scr;
    WMenuEntry *entry = NULL;
    Bool shortcutOk = False;

    if (!menu)
	return NULL;
    scr = menu->frame->screen_ptr;
    if (strcmp(command, "OPEN_MENU")==0) {
	if (!params) {
	    wwarning(_("%s:missing parameter for menu command \"%s\""),
		     file_name, command);
	} else {
	    WMenu *dummy;
	    char *path;
	    
            path = wfindfile(DEF_CONFIG_PATHS, params);
	    if (!path) {
		path = wstrdup(params);
	    }
	    dummy = wMenuCreate(scr, title, False);
	    dummy->on_destroy = removeShortcutsForMenu;
	    entry = wMenuAddCallback(menu, title, constructMenu, path);
	    entry->free_cdata = free;
	    wMenuEntrySetCascade(menu, entry, dummy);
	}
    } else if (strcmp(command, "EXEC")==0) {
	if (!params)
	    wwarning(_("%s:missing parameter for menu command \"%s\""),
		     file_name, command);
	else {
	    entry = wMenuAddCallback(menu, title, execCommand, wstrdup(params));
	    entry->free_cdata = free;
	    shortcutOk = True;
	}
    } else if (strcmp(command, "EXIT")==0) {
	
	if (params && strcmp(params, "QUICK")==0)
	    entry = wMenuAddCallback(menu, title, exitCommand, (void*)M_QUICK);
	else
	    entry = wMenuAddCallback(menu, title, exitCommand, NULL);

	shortcutOk = True;
    } else if (strcmp(command, "SHUTDOWN")==0) {
	    
	if (params && strcmp(params, "QUICK")==0)
	    entry = wMenuAddCallback(menu, title, shutdownCommand, 
				     (void*)M_QUICK);
	else
	    entry = wMenuAddCallback(menu, title, shutdownCommand, NULL);
	
	shortcutOk = True;
    } else if (strcmp(command, "REFRESH")==0) {
	entry = wMenuAddCallback(menu, title, refreshCommand, NULL);
	
	shortcutOk = True;
    } else if (strcmp(command, "WORKSPACE_MENU")==0) {
	entry = addWorkspaceMenu(scr, menu, title);
	
	shortcutOk = True;
    } else if (strcmp(command, "ARRANGE_ICONS")==0) {
	entry = wMenuAddCallback(menu, title, arrangeIconsCommand, NULL);
	
	shortcutOk = True;
    } else if (strcmp(command, "HIDE_OTHERS")==0) {
	entry = wMenuAddCallback(menu, title, hideOthersCommand, NULL);
	
	shortcutOk = True;
    } else if (strcmp(command, "SHOW_ALL")==0) {
	entry = wMenuAddCallback(menu, title, showAllCommand, NULL);
	
	shortcutOk = True;
    } else if (strcmp(command, "RESTART")==0) {
	entry = wMenuAddCallback(menu, title, restartCommand, 
				 params ? wstrdup(params) : NULL);
	entry->free_cdata = free;
	shortcutOk = True;
    } else if (strcmp(command, "SAVE_SESSION")==0) {
        entry = wMenuAddCallback(menu, title, saveSessionCommand, NULL);
        
	shortcutOk = True;
    } else if (strcmp(command, "CLEAR_SESSION")==0) {
        entry = wMenuAddCallback(menu, title, clearSessionCommand, NULL);
	shortcutOk = True;
    } else if (strcmp(command, "INFO_PANEL")==0) {
	entry = wMenuAddCallback(menu, title, infoPanelCommand, NULL);
	shortcutOk = True;
    } else if (strcmp(command, "LEGAL_PANEL")==0) {
	entry = wMenuAddCallback(menu, title, legalPanelCommand, NULL);
	shortcutOk = True;
    } else {
	wwarning(_("%s:unknown command \"%s\" in menu config."), file_name,
		 command);
	
	return NULL;
    }
    
    if (shortcut && entry) {
	if (!shortcutOk) {
	    wwarning(_("%s:can't add shortcut for entry \"%s\""), file_name,
		     title);
	} else {
	    if (addShortcut(file_name, shortcut, menu, entry)) {
	      
		entry->rtext = GetShortcutString(shortcut);
		 /*
		entry->rtext = wstrdup(shortcut);
		  */
	    }
	}
    }

    return entry;
}



/*******************   Menu Configuration From File   *******************/

static void
separateline(char *line, char *title, char *command, char *parameter,
	     char *shortcut)
{
    int l, i;
    
    l = strlen(line);

    *title = 0;
    *command = 0;
    *parameter = 0;
    *shortcut = 0;
    /* get the title */
    while (isspace(*line) && (*line!=0)) line++;
    if (*line=='"') {
	line++;
	i=0;
	while (line[i]!='"' && (line[i]!=0)) i++;
	if (line[i]!='"')
	  return;
    } else {
	i=0;
	while (!isspace(line[i]) && (line[i]!=0)) i++;
    }
    strncpy(title, line, i);
    title[i++]=0;
    line+=i;

    /* get the command or shortcut keyword */
    while (isspace(*line) && (*line!=0)) line++;
    if (*line==0)
      return;
    i=0;
    while (!isspace(line[i]) && (line[i]!=0)) i++;
    strncpy(command, line, i);
    command[i++]=0;
    line+=i;

    if (strcmp(command, "SHORTCUT")==0) {
	/* get the shortcut key */
	while (isspace(*line) && (*line!=0)) line++;
	if (*line=='"') {
	    line++;
	    i=0;
	    while (line[i]!='"' && (line[i]!=0)) i++;
	    if (line[i]!='"')
		return;
	} else {
	    i=0;
	    while (!isspace(line[i]) && (line[i]!=0)) i++;
	}
	strncpy(shortcut, line, i);
	shortcut[i++]=0;
	line+=i;
	
	*command=0;
	
	/* get the command */
	while (isspace(*line) && (*line!=0)) line++;
	if (*line==0)
	    return;
	i=0;
	while (!isspace(line[i]) && (line[i]!=0)) i++;
	strncpy(command, line, i);
	command[i++]=0;
	line+=i;
    }

    /* get the parameters */
    while (isspace(*line) && (*line!=0)) line++;
    if (*line==0)
      return;
    
    if (*line=='"') {
	line++;
	l = 0;
	while (line[l]!=0 && line[l]!='"') {
	    parameter[l] = line[l];
	    l++;
	}
	parameter[l] = 0;
	return;
    }
    
    l = strlen(line);
    while (isspace(line[l]) && (l>0)) l--;
    strncpy(parameter, line, l);
    parameter[l]=0;
}


static WMenu*
parseCascade(WScreen *scr, WMenu *menu, FILE *file, char *file_name)
{
    char linebuf[MAXLINE];
    char elinebuf[MAXLINE];
    char title[MAXLINE];
    char command[MAXLINE];
    char shortcut[MAXLINE];
    char params[MAXLINE];
    char *line;
    
    while (!IsEof(file)) {
	int lsize, ok;
	
	ok = 0;
	fgets(linebuf, MAXLINE, file);
	line = cropline(linebuf);
	lsize = strlen(line);
	do {
	    if (line[lsize-1]=='\\') {
		char *line2;
		int lsize2;
		fgets(elinebuf, MAXLINE, file);
		line2=cropline(elinebuf);
		lsize2=strlen(line2);
		if (lsize2+lsize>MAXLINE) {
		    wwarning(_("%s:maximal line size exceeded in menu config: %s"),
			     file_name, line);
		    ok=2;
		} else {
		    line[lsize-1]=0;
		    lsize+=lsize2-1;
		    strcat(line, line2);
		}
	    } else {
		ok=1;
	    }
	} while (!ok && !IsEof(file));
	if (ok==2)
	  continue;
	
	if (line[0]==0 || line[0]=='#' || (line[0]=='/' && line[1]=='/')) 
	  continue;

	
	separateline(line, title, command, params, shortcut);
	
	if (!command[0]) {
	    wwarning(_("%s:missing command in menu config: %s"), file_name,
		     line);
	    goto error;
	}

	if (strcasecmp(command, "MENU")==0) {
	    WMenu *cascade;
	    
	    /* start submenu */

	    cascade = wMenuCreate(scr, title, False);
	    cascade->on_destroy = removeShortcutsForMenu;
	    if (parseCascade(scr, cascade, file, file_name)==NULL) {
		wMenuDestroy(cascade, True);
	    } else {
		wMenuEntrySetCascade(menu, 
				     wMenuAddCallback(menu, title, NULL, NULL),
				     cascade);
	    }
	} else if (strcasecmp(command, "END")==0) {
	    /* end of menu */
	    return menu;
	    
	} else {
	    /* normal items */
	    addMenuEntry(menu, title, shortcut[0] ? shortcut : NULL, command, 
			 params[0] ? params : NULL, file_name);
	}
    }

    wwarning(_("%s:syntax error in menu file:END declaration missing"),
	     file_name);
    return menu;

    error:
    return menu;
}


static WMenu*
readMenuFile(WScreen *scr, char *file_name)
{
    WMenu *menu=NULL;
    FILE *file = NULL;
    char linebuf[MAXLINE];
    char title[MAXLINE];
    char shortcut[MAXLINE];
    char command[MAXLINE];
    char params[MAXLINE];
    char *line;
#ifdef USECPP
    char *args;
    int cpp = 0;
#endif

#ifdef USECPP
    if (!wPreferences.flags.nocpp) {
	args = MakeCPPArgs(file_name);
	if (!args) {
	    wwarning(_("could not make arguments for menu file preprocessor"));
	} else {
	    sprintf(command, "%s %s %s", CPP_PATH, args, file_name);
	    free(args);
	    file = popen(command, "r");
	    if (!file) {
		wsyserror(_("%s:could not open/preprocess menu file"), 
			  file_name);
	    } else {
		cpp = 1;
	    }
	}
    }
#endif /* USECPP */

    if (!file) {
	file = fopen(file_name, "r");
	if (!file) {
	    wsyserror(_("%s:could not open menu file"), file_name);
	    return NULL;
	}
    }
    
    while (!IsEof(file)) {
	if (!fgets(linebuf, MAXLINE, file))
	    break;
	line = cropline(linebuf);
	if (line[0]==0 || line[0]=='#' || (line[0]=='/' && line[1]=='/')) 
	  continue;

	separateline(line, title, command, params, shortcut);
	
	if (!command[0]) {
	    wwarning(_("%s:missing command in menu config: %s"), file_name,
		     line);
	    break;
	}
	if (strcasecmp(command, "MENU")==0) {
	    menu = wMenuCreate(scr, title, True);
	    menu->on_destroy = removeShortcutsForMenu;
	    if (!parseCascade(scr, menu, file, file_name)) {
		wMenuDestroy(menu, True);
	    }
	    break;
	} else {
	    wwarning(_("%s:invalid menu file. MENU command is missing"), 
		     file_name);
	    break;
	}
    }
    
#ifdef CPP
    if (cpp) {
	if (pclose(file)==-1) {
	    wsyserror(_("error reading preprocessed menu data"));
	}
    } else {
	fclose(file);
    }
#else
    fclose(file);
#endif
    
    return menu;
}

/************    Menu Configuration From Pipe      *************/

static WMenu*
readMenuPipe(WScreen *scr, char **file_name) 
{
    WMenu *menu=NULL;
    FILE *file = NULL;
    char linebuf[MAXLINE];
    char title[MAXLINE];
    char command[MAXLINE];
    char params[MAXLINE];
    char shortcut[MAXLINE];
    char *line;
    char * filename;
    char flat_file[MAXLINE];
    int i;
#ifdef USECPP
    char *args;
    int cpp = 0;
#endif

    flat_file[0] = '\0';

    for(i = 0 ; file_name[i] != NULL ; i++) {
	strcat(flat_file, file_name[i]);
	strcat(flat_file, " ");
    }
    filename = flat_file+1;

#ifdef USECPP
    if (!wPreferences.flags.nocpp) {
       args = MakeCPPArgs(filename);
       if (!args) {
           wwarning(_("could not make arguments for menu file preprocessor"));
       } else {
           sprintf(command, "%s | %s %s", filename, CPP_PATH, args);

           free(args);
           file = popen(command, "r");
           if (!file) {
               wsyserror(_("%s:could not open/preprocess menu file"), filename);
           } else {
               cpp = 1;
           }
       }
    }

#endif /* USECPP */

    if (!file) {
       file = popen(filename, "r");

       if (!file) {
           wsyserror(_("%s:could not open menu file"), filename);
           return NULL;
       }
    }

    while (!IsEof(file)) {
	if (!fgets(linebuf, MAXLINE, file))
	    break;
	line = cropline(linebuf);
	if (line[0]==0 || line[0]=='#' || (line[0]=='/' && line[1]=='/')) 
	    continue;

	separateline(line, title, command, params, shortcut);

	if (!command[0]) {
	    wwarning(_("%s:missing command in menu config: %s"), file_name,
		     line);
	    break;
	}
	if (strcasecmp(command, "MENU")==0) {
	    menu = wMenuCreate(scr, title, True);
	    menu->on_destroy = removeShortcutsForMenu;
	    if (!parseCascade(scr, menu, file, filename)) {
		wMenuDestroy(menu, True);
	    }
	    break;
	} else {
	    wwarning(_("%s:no title given for the root menu"), filename);
	    break;
	}
    }

    pclose(file);

    return menu;
}



typedef struct {
    char *name;
    int index;
} dir_data;


static int
myCompare(dir_data *d1, dir_data *d2)
{
    return strcmp(d1->name, d2->name);
}


/************  Menu Configuration From Directory   *************/
static WMenu*
readMenuDirectory(WScreen *scr, char *title, char **path, char *command)
{
    DIR *dir;
    struct dirent *dentry;
    struct stat stat_buf;
    WMenu *menu=NULL;
    char *buffer;
    LinkedList *dirs = NULL, *files = NULL;
    int length, i, have_space=0;
    dir_data *data;

    i=0;
    while (path[i]!=NULL) {
        dir = opendir(path[i]);
        if (!dir) {
            i++;
            continue;
        }

        while ((dentry = readdir(dir))) {

            if (strcmp(dentry->d_name, ".")==0 ||
                strcmp(dentry->d_name, "..")==0)
                continue;

	    if (dentry->d_name[0] == '.')
		continue;

            buffer = wmalloc(strlen(path[i])+strlen(dentry->d_name)+4);
            if (!buffer) {
                wsyserror(_("out of memory while constructing directory menu %s"),
                         path[i]);
                break;
            }

            strcpy(buffer, path[i]);
            strcat(buffer, "/");
            strcat(buffer, dentry->d_name);

            if (stat(buffer, &stat_buf)!=0) {
                wsyserror(_("%s:could not stat file \"%s\" in menu directory"),
                          path[i], dentry->d_name);
            } else {
                data = (dir_data*) wmalloc(sizeof(dir_data));
                data->name = wstrdup(dentry->d_name);
                data->index = i;
                if (S_ISDIR(stat_buf.st_mode)) {
                    /* access always returns success for user root */
                    if (access(buffer, X_OK)==0) {
                        /* Directory is accesible. Add to directory list */
                        list_insert_sorted(data, &dirs, (int(*)())myCompare);
                        data = NULL;
                    }
                } else if (S_ISREG(stat_buf.st_mode)) {
                    /* Hack because access always returns X_OK success for user root */
#define S_IXANY (S_IXUSR | S_IXGRP | S_IXOTH)
                    if ((command!=NULL && access(buffer, R_OK)==0) ||
                        (command==NULL && access(buffer, X_OK)==0 &&
                         (stat_buf.st_mode & S_IXANY))) {
                        list_insert_sorted(data, &files, (int(*)())myCompare);
                        data = NULL;
                    }
                }
                if (data!=NULL) {
                    if (data->name)
                        free(data->name);
                    free(data);
                    data = NULL;
                }
            }
            free(buffer);
        }

        closedir(dir);
        i++;
    }

    if (!dirs && !files)
        return NULL;

    menu = wMenuCreate(scr, title, False);
    menu->on_destroy = removeShortcutsForMenu;
    
    while (dirs != NULL) {
        /* New directory. Use same OPEN_MENU command that was used
         * for the current directory. */
        dir_data *d = (dir_data*)dirs->head;

        length = strlen(path[d->index])+strlen(d->name)+6;
        if (command)
            length += strlen(command) + 6;
        buffer = wmalloc(length);
        if (!buffer) {
            wsyserror(_("out of memory while constructing directory menu %s"),
                     path[d->index]);
            break;
        }

        have_space = strchr(path[d->index], ' ')!=NULL ||
                     strchr(d->name, ' ')!=NULL;
        if (have_space) {
            buffer[0] = '"';
            buffer[1] = 0;
            strcat(buffer, path[d->index]);
        } else {
            strcpy(buffer, path[d->index]);
        }
        strcat(buffer, "/");
        strcat(buffer, d->name);
        if (have_space)
            strcat(buffer, "\"");
        if (command) {
            strcat(buffer, " WITH ");
            strcat(buffer, command);
        }

        addMenuEntry(menu, d->name, NULL, "OPEN_MENU", buffer, path[d->index]);

        free(buffer);
        if (dirs->head) {
            if (d->name)
                free(d->name);
            free(dirs->head);
        }
        list_remove_head(&dirs);
    }

    while (files != NULL) {
        /* executable: add as entry */
        dir_data *f = (dir_data*) files->head;;

        length = strlen(path[f->index])+strlen(f->name)+6;
        if (command)
            length += strlen(command);

        buffer = wmalloc(length);
        if (!buffer) {
            wsyserror(_("out of memory while constructing directory menu %s"),
                     path[f->index]);
            break;
        }

        have_space = strchr(path[f->index], ' ')!=NULL ||
                     strchr(f->name, ' ')!=NULL;
        if (command!=NULL) {
            strcpy(buffer, command);
            strcat(buffer, " ");
            if (have_space)
                strcat(buffer, "\"");
            strcat(buffer, path[f->index]);
        } else {
            if (have_space) {
                buffer[0] = '"';
                buffer[1] = 0;
                strcat(buffer, path[f->index]);
            } else {
                strcpy(buffer, path[f->index]);
            }
        }
        strcat(buffer, "/");
        strcat(buffer, f->name);
        if (have_space)
            strcat(buffer, "\"");

        addMenuEntry(menu, f->name, NULL, "EXEC", buffer, path[f->index]);

        free(buffer);
        if (files->head) {
            if (f->name)
                free(f->name);
            free(files->head);
        }
        list_remove_head(&files);
    }

    return menu;
}


/************  Menu Configuration From WMRootMenu   *************/

static WMenu*
makeDefaultMenu(WScreen *scr)
{
    WMenu *menu=NULL;
    
    menu = wMenuCreate(scr, _("Commands"), True);
    wMenuAddCallback(menu, "XTerm", execCommand, "xterm");
    wMenuAddCallback(menu, _("Exit..."), exitCommand, NULL);
    return menu;
}





/*
 *---------------------------------------------------------------------- 
 * configureMenu--
 * 	Reads root menu configuration from defaults database.
 * 
 *---------------------------------------------------------------------- 
 */
static WMenu*
configureMenu(WScreen *scr, proplist_t definition)
{
    WMenu *menu = NULL;
    proplist_t elem;
    int i, count;
    proplist_t title, command, params;
    char *tmp, *mtitle;

    
    if (PLIsString(definition)) {
	struct stat stat_buf;
	char *path = NULL;
	Bool menu_is_default = False;

	/* menu definition is a string. Probably a path, so parse the file */
	
	tmp = wexpandpath(PLGetString(definition));

	if (Locale) {
	    path = wmalloc(strlen(tmp)+32);

	    strcpy(path, tmp);
	    strcat(path, ".");
	    strcat(path, Locale);

	    /* look for menu.xy */
	    if (stat(path, &stat_buf)<0) {		
		int i;
		i = strlen(Locale);
		if (i>2) {
		    path[strlen(path)-(i-2)]=0;
		    /* look for menu.xy_zw */
		    if (stat(path, &stat_buf)<0) {
			free(path);
			/* If did not find any localized menus, try
			 * only menu. This can also mean that
			 * the path in WMRootMenu was already the
			 * path for the localized menu (eg: menu = "menu.ab")
			 */
			path = NULL;
		    }
		} else {
		    free(path);
		    path = NULL;
		}
	    }
	}

	if (!path)
	    path = wfindfile(DEF_CONFIG_PATHS, tmp);

	if (!path) {
	    path = wfindfile(DEF_CONFIG_PATHS, DEF_MENU_FILE);
	    menu_is_default = True;
	}

	if (!path) {
	    wsyserror(_("could not find menu file \"%s\" referenced in WMRootMenu"),
		     tmp);
	    free(tmp);
	    return NULL;
	}
	
	if (stat(path, &stat_buf)<0) {
	    wsyserror(_("could not access menu \"%s\" referenced in WMRootMenu"), path);
	    free(path);
	    free(tmp);
	    return NULL;
	}

	if (!scr->root_menu || stat_buf.st_mtime > scr->root_menu->timestamp
	    /* if the pointer in WMRootMenu has changed */
	    || WDRootMenu->timestamp > scr->root_menu->timestamp) {
	    
	    if (menu_is_default) {
		wwarning(_("using default menu file \"%s\" as the menu referenced in WMRootMenu could not be found "),
			 path);
	    }

	    menu = readMenuFile(scr, path);
	    if (menu)
		menu->timestamp = MAX(stat_buf.st_mtime, WDRootMenu->timestamp);
	} else {
	    menu = NULL;
	}
	free(path);
	free(tmp);

	return menu;
    }

    count = PLGetNumberOfElements(definition);
    if (count==0)
	return NULL;

    elem = PLGetArrayElement(definition, 0);
    if (!PLIsString(elem)) {
	tmp = PLGetDescription(elem);
	wwarning(_("%s:format error in root menu configuration \"%s\""), 
		 "WMRootMenu", tmp);
	free(tmp);
	return NULL;
    }
    mtitle = PLGetString(elem);
    
    menu = wMenuCreate(scr, mtitle, False);
    menu->on_destroy = removeShortcutsForMenu;

    for (i=1; i<count; i++) {
	elem = PLGetArrayElement(definition, i);

	if (!PLIsArray(elem) || PLGetNumberOfElements(elem) < 2)
	    goto error;
	
	if (PLIsArray(PLGetArrayElement(elem,1))) {
	    WMenu *submenu;
	    WMenuEntry *mentry;
	    
	    /* submenu */
	    submenu = configureMenu(scr, elem);
	    if (submenu) {
		mentry = wMenuAddCallback(menu, submenu->frame->title, NULL, 
					  NULL);
		wMenuEntrySetCascade(menu, mentry, submenu);
	    }
	} else {
	    int idx = 0;
	    char *shortcut;
	    /* normal entry */

	    title = PLGetArrayElement(elem, idx++);
	    shortcut = PLGetArrayElement(elem, idx++);
	    if (strcmp(PLGetString(shortcut), "SHORTCUT")==0) {
		shortcut = PLGetArrayElement(elem, idx++);
		command = PLGetArrayElement(elem, idx++);
	    } else {
		command = shortcut;
		shortcut = NULL;
	    }
	    params = PLGetArrayElement(elem, idx++);

	    if (!title || !command)
		goto error;
	    
	    addMenuEntry(menu, PLGetString(title), 
			 shortcut ? PLGetString(shortcut) : NULL, 
			 PLGetString(command),
			 params ? PLGetString(params) : NULL, "WMRootMenu");
	}
	continue;

      error:
	tmp = PLGetDescription(elem);
	wwarning(_("%s:format error in root menu configuration \"%s\""), 
		 "WMRootMenu", tmp);
	free(tmp);
    }
    
    return menu;
}








/*
 *---------------------------------------------------------------------- 
 * OpenRootMenu--
 * 	Opens the root menu, parsing the menu configuration from the
 * defaults database.
 *	If the menu is already mapped and is not sticked to the
 * root window, it will be unmapped.
 * 
 * Side effects:
 * 	The menu may be remade.
 * 
 * Notes:
 * Construction of OPEN_MENU entries are delayed to the moment the
 * user map's them.
 *---------------------------------------------------------------------- 
 */
void
OpenRootMenu(WScreen *scr, int x, int y, int keyboard)
{
    WMenu *menu=NULL;
    proplist_t definition;
    /*
    static proplist_t domain=NULL;
    
    if (!domain) {
	domain = PLMakeString("WMRootMenu");
    }
     */
    
    scr->flags.root_menu_changed_shortcuts = 0;
    scr->flags.added_workspace_menu = 0;

    if (scr->root_menu && scr->root_menu->flags.mapped) {
	menu = scr->root_menu;
	if (!menu->flags.buttoned) {
	    wMenuUnmap(menu);
	} else {
	    wRaiseFrame(menu->frame->core);
	    
	    if (keyboard)
		wMenuMapAt(menu, 0, 0, True);
	    else
		wMenuMapCopyAt(menu, x-menu->frame->core->width/2, 
			       y-menu->frame->top_width/2);
	}
	return;
    }
    
    
    definition = WDRootMenu->dictionary;

/*
    definition = PLGetDomain(domain);
 */
    if (definition) {
	if (PLIsArray(definition)) {
	    if (!scr->root_menu 
		|| WDRootMenu->timestamp > scr->root_menu->timestamp) {
		menu = configureMenu(scr, definition);
		if (menu)
		    menu->timestamp = WDRootMenu->timestamp;

	    } else
		menu = NULL;
	} else {
	    menu = configureMenu(scr, definition);
	}
    }

    if (!menu) {
	/* menu hasn't changed or could not be read */
	if (!scr->root_menu) {
	    menu = makeDefaultMenu(scr);
	    scr->root_menu = menu;
	}
	menu = scr->root_menu;
    } else {
	/* new root menu */
	if (scr->root_menu)
	    wMenuDestroy(scr->root_menu, True);
	scr->root_menu = menu;
    }
    if (menu) {
	wMenuMapAt(menu, x-menu->frame->core->width/2, y-menu->frame->top_width/2,
		   keyboard);
    }
    
    if (scr->flags.root_menu_changed_shortcuts)
	rebindKeygrabs(scr);
}

