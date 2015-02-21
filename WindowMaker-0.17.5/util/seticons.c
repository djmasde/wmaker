

#include <stdlib.h>
#include <stdio.h>
#include <proplist.h>

#include <string.h>

#include "../src/wconfig.h"

char *ProgName;

char*
defaultsPathForDomain(char *domain)
{
    char path[1024];
    char *gspath, *tmp;

    gspath = getenv("GNUSTEP_USER_ROOT");
    if (gspath) {
	strcpy(path, gspath);
	strcat(path, "/");
    } else {
	char *home;
	
	home = getenv("HOME");
	if (!home) {
	    printf("%s:could not get HOME environment variable!\n", ProgName);
	    exit(0);
	}

	strcpy(path, home);
	strcat(path, "/GNUstep/");
    }
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    tmp = malloc(strlen(path)+2);
    strcpy(tmp, path);
    
    return tmp;
}


int 
main(int argc, char **argv)
{
    proplist_t window_name, icon_key, window_attrs, icon_value;
    proplist_t all_windows, iconset;
    proplist_t keylist;
    int i;
    char *path;

    ProgName = argv[0];
    
    if (argc!=2) {
	printf("Syntax:\n%s <iconset file>\n", argv[0]);
	exit(1);
    }
    
    path = defaultsPathForDomain("WMWindowAttributes");
    
    all_windows = PLGetProplistWithPath(path);
    if (!all_windows) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    iconset = PLGetProplistWithPath(argv[1]);
    if (!iconset) {
	printf("%s:could not load icon set file \"%s\".\n", ProgName, argv[1]);
	exit(1);
    }

    
    keylist = PLGetAllDictionaryKeys(iconset);
    icon_key = PLMakeString("Icon");
    
    for (i=0; i<PLGetNumberOfElements(keylist); i++) {	
	window_name = PLGetArrayElement(keylist, i);
	if (!PLIsString(window_name))
	    continue;

	icon_value = PLGetDictionaryEntry(iconset, window_name);
	if (!icon_value || !PLIsDictionary(icon_value))
	    continue;

	window_attrs = PLGetDictionaryEntry(all_windows, window_name);
	if (window_attrs) {
	    if (PLIsDictionary(window_attrs)) {
		PLMergeDictionaries(window_attrs, icon_value);
	    }
	} else {
	    PLInsertDictionaryEntry(all_windows, window_name, icon_value);
	}
    }
    
    PLSave(all_windows, YES);

    exit(0);
}


