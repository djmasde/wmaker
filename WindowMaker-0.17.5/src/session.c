/* session.c - session state handling
 *
 *  Copyright (c) 1998 Dan Pascu
 *
 *  WindowMaker window manager
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
#include <unistd.h>

#include "WindowMaker.h"
#include "screen.h"
#include "window.h"
#include "session.h"
#include "wcore.h"
#include "framewin.h"
#include "workspace.h"
#include "funcs.h"
#include "properties.h"
#include "application.h"
#include "appicon.h"

#include "dock.h"

#include "list.h"

#include <proplist.h>


static proplist_t sApplications = NULL;
static proplist_t sCommand;
static proplist_t sName;
static proplist_t sHost;
static proplist_t sWorkspace;
static proplist_t sShaded;
static proplist_t sMiniaturized;
static proplist_t sHidden;
static proplist_t sGeometry;

static proplist_t sDock;

static proplist_t sYes, sNo;


static void
make_keys()
{
    if (sApplications!=NULL)
        return;

    sApplications = PLMakeString("Applications");
    sCommand = PLMakeString("Command");
    sName = PLMakeString("Name");
    sHost = PLMakeString("Host");
    sWorkspace = PLMakeString("Workspace");
    sShaded = PLMakeString("Shaded");
    sMiniaturized = PLMakeString("Miniaturized");
    sHidden = PLMakeString("Hidden");
    sGeometry = PLMakeString("Geometry");
    sDock = PLMakeString("Dock");

    sYes = PLMakeString("Yes");
    sNo = PLMakeString("No");
}



static int 
getBool(proplist_t value)
{
    char *val;

    if (!PLIsString(value)) {
        return 0;
    }
    if (!(val = PLGetString(value))) {
        return 0;
    }

    if ((val[1]=='\0' && (val[0]=='y' || val[0]=='Y' || val[0]=='T'
                          || val[0]=='t' || val[0]=='1'))
        || (strcasecmp(val, "YES")==0 || strcasecmp(val, "TRUE")==0)) {

        return 1;
    } else if ((val[1]=='\0'
                && (val[0]=='n' || val[0]=='N' || val[0]=='F'
                    || val[0]=='f' || val[0]=='0'))
               || (strcasecmp(val, "NO")==0 || strcasecmp(val, "FALSE")==0)) {

        return 0;
    } else {
        wwarning(_("can't convert \"%s\" to boolean"), val);
        return 0;
    }
}



static proplist_t
makeWindowState(WWindow *wwin, WApplication *wapp)
{
    WScreen *scr = wwin->screen_ptr;
    Window win;
    int argc;
    char **argv;
    char *class, *instance, *command=NULL, buffer[256];
    proplist_t win_state, cmd, name, workspace;
    proplist_t shaded, miniaturized, hidden, geometry;
    proplist_t dock;

    if (wwin->main_window!=None && wwin->main_window!=wwin->client_win)
        win = wwin->main_window;
    else
        win = wwin->client_win;

    if (XGetCommand(dpy, win, &argv, &argc) && argc>0) {
        command = FlattenStringList(argv, argc);
        XFreeStringList(argv);
    }
    if (!command)
        return NULL;

    if (PropGetWMClass(win, &class, &instance)) {
        if (class && instance)
            sprintf(buffer, "%s.%s", instance, class);
        else if (instance)
            sprintf(buffer, "%s", instance);
        else if (class)
            sprintf(buffer, ".%s", class);
        else
            sprintf(buffer, ".");

        name = PLMakeString(buffer);
        cmd = PLMakeString(command);
        /*sprintf(buffer, "%d", wwin->frame->workspace+1);
        workspace = PLMakeString(buffer);*/
        workspace = PLMakeString(scr->workspaces[wwin->frame->workspace]->name);
        shaded = wwin->flags.shaded ? sYes : sNo;
        miniaturized = wwin->flags.miniaturized ? sYes : sNo;
        hidden = wwin->flags.hidden ? sYes : sNo;
        sprintf(buffer, "%ix%i+%i+%i", wwin->client.width, wwin->client.height,
		wwin->frame_x, wwin->frame_y);
        geometry = PLMakeString(buffer);

        win_state = PLMakeDictionaryFromEntries(sName, name,
                                                sCommand, cmd,
                                                sWorkspace, workspace,
                                                sShaded, shaded,
                                                sMiniaturized, miniaturized,
                                                sHidden, hidden,
                                                sGeometry, geometry,
                                                NULL);

        PLRelease(name);
        PLRelease(cmd);
        PLRelease(workspace);
        PLRelease(geometry);
        if (wapp && wapp->app_icon && wapp->app_icon->dock) {
            int i;
            char *name;
            if (wapp->app_icon->dock == scr->dock) {
                name="Dock";
            } else {
                for(i=0; i<scr->workspace_count; i++)
                    if(scr->workspaces[i]->clip == wapp->app_icon->dock)
                        break;
                assert( i < scr->workspace_count);
                /*n = i+1;*/
                name = scr->workspaces[i]->name;
            }
            dock = PLMakeString(name);
            PLInsertDictionaryEntry(win_state, sDock, dock);
            PLRelease(dock);
        }
    } else {
        win_state = NULL;
    }

    if (instance) XFree(instance);
    if (class) XFree(class);
    if (command) free(command);

    return win_state;
}


void
wSessionSaveState(WScreen *scr)
{
    WWindow *wwin = scr->focused_window;
    proplist_t win_info, wks;
    proplist_t list=NULL;
    LinkedList *wapp_list=NULL;


    make_keys();

    if (!scr->session_state) {
        scr->session_state = PLMakeDictionaryFromEntries(NULL, NULL, NULL);
        if (!scr->session_state)
            return;
    }

    list = PLMakeArrayFromElements(NULL);

    while (wwin) {
        WApplication *wapp=wApplicationOf(wwin->main_window);

        if (wwin->transient_for==None && list_find(wapp_list, wapp)==NULL
	    && !wwin->window_flags.dont_save_session) {
            /* A entry for this application was not yet saved. Save one. */
            if ((win_info = makeWindowState(wwin, wapp))!=NULL) {
                list = PLAppendArrayElement(list, win_info);
                PLRelease(win_info);
                /* If we were succesful in saving the info for this window
                 * add the application the window belongs to, to the
                 * application list, so no multiple entries for the same
                 * application are saved.
                 */
                wapp_list = list_cons(wapp, wapp_list);
            }
        }
        wwin = wwin->prev;
    }
    PLRemoveDictionaryEntry(scr->session_state, sApplications);
    PLInsertDictionaryEntry(scr->session_state, sApplications, list);
    PLRelease(list);

    wks = PLMakeString(scr->workspaces[scr->current_workspace]->name);
    PLInsertDictionaryEntry(scr->session_state, sWorkspace, wks);
    PLRelease(wks);

    list_free(wapp_list);
}


void
wSessionClearState(WScreen *scr)
{
    make_keys();

    if (!scr->session_state)
        return;

    PLRemoveDictionaryEntry(scr->session_state, sApplications);
    PLRemoveDictionaryEntry(scr->session_state, sWorkspace);
}


static pid_t
execCommand(WScreen *scr, char *command, char *host)
{
    pid_t pid;
    char **argv;
    int argc;

    ParseCommand(scr, command, &argv, &argc);

    if (argv==NULL) {
        return 0;
    }

    if ((pid=fork())==0) {
        char **args;
        int i;

        args = malloc(sizeof(char*)*(argc+1));
        if (!args)
            exit(111);
        for (i=0; i<argc; i++) {
            args[i] = argv[i];
        }
        args[argc] = NULL;
        execvp(argv[0], args);
        exit(111);
    }
    while (argc > 0)
        free(argv[--argc]);
    free(argv);
    return pid;
}


static WSavedState*
getWindowState(WScreen *scr, proplist_t win_state)
{
    WSavedState *state = wmalloc(sizeof(WSavedState));
    proplist_t value;
    char *tmp;
    int i;

    memset(state, 0, sizeof(WSavedState));
    state->workspace = -1;
    value = PLGetDictionaryEntry(win_state, sWorkspace);
    if (value && PLIsString(value)) {
        tmp = PLGetString(value);
        if (sscanf(tmp, "%i", &state->workspace)!=1) {
            state->workspace = -1;
            for (i=0; i < scr->workspace_count; i++) {
                if (strcmp(scr->workspaces[i]->name, tmp)==0) {
                    state->workspace = i;
                    break;
                }
            }
        } else {
            state->workspace--;
        }
    }
    if ((value = PLGetDictionaryEntry(win_state, sShaded))!=NULL)
        state->shaded = getBool(value);
    if ((value = PLGetDictionaryEntry(win_state, sMiniaturized))!=NULL)
        state->miniaturized = getBool(value);
    if ((value = PLGetDictionaryEntry(win_state, sHidden))!=NULL)
        state->hidden = getBool(value);

    value = PLGetDictionaryEntry(win_state, sGeometry);
    if (value && PLIsString(value)) {
        if (sscanf(PLGetString(value), "%ix%i+%i+%i",
                   &state->w, &state->h, &state->x, &state->y)==4 &&
            (state->w>0 && state->h>0)) {
            state->use_geometry = 1;
        } else if (sscanf(PLGetString(value), "%i,%i,%i,%i",
                   &state->x, &state->y, &state->w, &state->h)==4 &&
            (state->w>0 && state->h>0)) { 
	    /* TODO: remove redundant sscanf() in version 0.20.x */
            state->use_geometry = 1;
        }

    }

    return state;
}


#define SAME(x, y) (((x) && (y) && !strcmp((x), (y))) || (!(x) && !(y)))


void
wSessionRestoreState(WScreen *scr)
{
    WSavedState *state;
    char *instance, *class, *command, *host;
    proplist_t win_info, apps, cmd, value;
    pid_t pid;
    int i, count;
    WDock *dock;
    WAppIcon *btn=NULL;
    int j, n, found;
    char *tmp;

    make_keys();

    if (!scr->session_state)
        return;

    PLSetStringCmpHook(NULL);

    apps = PLGetDictionaryEntry(scr->session_state, sApplications);
    if (!apps)
        return;

    count = PLGetNumberOfElements(apps);
    if (count==0) 
        return;

    for (i=0; i<count; i++) {
        win_info = PLGetArrayElement(apps, i);

        cmd = PLGetDictionaryEntry(win_info, sCommand);
        if (!cmd || !PLIsString(cmd) || !(command = PLGetString(cmd))) {
            continue;
        }

        value = PLGetDictionaryEntry(win_info, sName);
        if (!value)
            continue;

        ParseWindowName(value, &instance, &class, "session");
        if (!instance && !class)
            continue;

        value = PLGetDictionaryEntry(win_info, sHost);
        if (value && PLIsString(value))
            host = PLGetString(value);
        else
            host = NULL;

        state = getWindowState(scr, win_info);

        dock = NULL;
        value = PLGetDictionaryEntry(win_info, sDock);
        if (value && PLIsString(value) && (tmp = PLGetString(value))!=NULL) {
            if (sscanf(tmp, "%i", &n)!=1) {
                if (!strcasecmp(tmp, "DOCK")) {
                    dock = scr->dock;
                } else {
                    for (j=0; j < scr->workspace_count; j++) {
                        if (strcmp(scr->workspaces[j]->name, tmp)==0) {
                            dock = scr->workspaces[j]->clip;
                            break;
                        }
                    }
                }
            } else {
                if (n == 0) {
                    dock = scr->dock;
                } else if (n>0 && n<=scr->workspace_count) {
                    dock = scr->workspaces[n-1]->clip;
                }
            }
        }

        found = 0;
        if (dock!=NULL) {
            for (j=0; j<dock->max_icons; j++) {
                btn = dock->icon_array[j];
                if (btn && SAME(instance, btn->wm_instance) &&
                    SAME(class, btn->wm_class) &&
                    SAME(command, btn->command)) {
                    found = 1;
                    break;
                }
            }
        }

        if (found) {
            wDockLaunchWithState(dock, btn, state);
        } else if ((pid = execCommand(scr, command, host)) > 0) {
            wAddWindowSavedState(instance, class, command, pid, state);
        } else {
            free(state);
        }

        if (instance) free(instance);
        if (class) free(class);
    }
    /* clean up */
    PLSetStringCmpHook(StringCompareHook);
}


void
wSessionRestoreLastWorkspace(WScreen *scr)
{
    proplist_t wks;
    int w, i;
    char *tmp;

    make_keys();

    if (!scr->session_state)
        return;

    PLSetStringCmpHook(NULL);

    wks = PLGetDictionaryEntry(scr->session_state, sWorkspace);
    if (!wks || !PLIsString(wks))
        return;

    tmp = PLGetString(wks);

    /* clean up */
    PLSetStringCmpHook(StringCompareHook);

    if (sscanf(tmp, "%i", &w)!=1) {
        w = -1;
        for (i=0; i < scr->workspace_count; i++) {
            if (strcmp(scr->workspaces[i]->name, tmp)==0) {
                w = i;
                break;
            }
        }
    } else {
        w--;
    }

    if (w!=scr->current_workspace && w<scr->workspace_count) {
        wWorkspaceChange(scr, w);
    }
}


