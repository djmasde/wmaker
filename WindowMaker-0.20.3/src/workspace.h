/* workspace.c- Workspace management
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

#ifndef WMWORKSPACE_H_
#define WMWORKSPACE_H_



typedef struct WWorkspace {
    char *name;
    struct WDock *clip;
} WWorkspace;

void wWorkspaceMake(WScreen *scr, int count);
int wWorkspaceNew(WScreen *scr);
void wWorkspaceDeleteLast(WScreen *scr);
void wWorkspaceChange(WScreen *scr, int workspace);
void wWorkspaceForceChange(WScreen *scr, int workspace);


WMenu *wWorkspaceMenuMake(WScreen *scr, Bool titled);
void wWorkspaceMenuUpdate(WScreen *scr, WMenu *menu);

void wWorkspaceMenuEdit(WScreen *scr);

void wWorkspaceSaveState(WScreen *scr, proplist_t old_state);
void wWorkspaceRestoreState(WScreen *scr);

void wWorkspaceRename(WScreen *scr, int workspace, char *name);

#endif
