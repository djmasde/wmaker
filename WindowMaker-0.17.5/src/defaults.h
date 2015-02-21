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

#ifndef WMDEFAULTS_H_
#define WMDEFAULTS_H_

typedef struct WDDomain {
    char *domain_name;
    proplist_t dictionary;
    char *path;
    time_t timestamp;
} WDDomain;

#if 0
proplist_t wDefaultsInit(int screen_number);
#endif


WDDomain*wDefaultsInitDomain(char *domain, Bool requireDictionary);

void wDefaultsDestroyDomain(WDDomain *domain);

void wReadDefaults(WScreen *scr, proplist_t new_dict);

void wDefaultUpdateIcons(WScreen *scr);

char *wDefaultsPathForDomain(char *domain);

void wReadStaticDefaults(proplist_t dict);

void wDefaultsCheckDomains(void *screen);

void wSaveDefaults(WScreen *scr);

char *wDefaultGetIconFile(WScreen *scr, char *instance, char *class);

RImage*wDefaultGetImage(WScreen *scr, char *winstance, char *wclass);


void wDefaultFillAttributes(WScreen *scr, char *instance, char *class, 
			    WWindowAttributes *attr, Bool useGlobalDefault);

int wDefaultGetStartWorkspace(WScreen *scr, char *instance, char *class);

void wDefaultChangeIcon(WScreen *scr, char *instance, char* class, char *file);

#endif /* WMDEFAULTS_H_ */
