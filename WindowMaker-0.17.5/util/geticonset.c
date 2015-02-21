

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
    char *path;
    int i;

    
    ProgName = argv[0];
    
    if (argc>2 || (argc==2 && strcmp(argv[1], "-h")==0)) {
	printf("Syntax:\n%s [<iconset file>]\n", argv[0]);
	exit(1);
    }
    
    path = defaultsPathForDomain("WMWindowAttributes");
    
    all_windows = PLGetProplistWithPath(path);
    if (!all_windows) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    iconset = PLMakeDictionaryFromEntries(NULL, NULL, NULL);

    keylist = PLGetAllDictionaryKeys(all_windows);
    icon_key = PLMakeString("Icon");
    
    for (i=0; i<PLGetNumberOfElements(keylist); i++) {
	proplist_t icondic;
	
	window_name = PLGetArrayElement(keylist, i);
	if (!PLIsString(window_name))
	    continue;
		
	window_attrs = PLGetDictionaryEntry(all_windows, window_name);
	if (window_attrs && PLIsDictionary(window_attrs)) {
	    icon_value = PLGetDictionaryEntry(window_attrs, icon_key);
	    if (icon_value) {
		
		icondic = PLMakeDictionaryFromEntries(icon_key, icon_value, 
						      NULL);

		PLInsertDictionaryEntry(iconset, window_name, icondic);
	    }
	}
    }
    

    if (argc==2) {
	proplist_t val;
	val = PLMakeString(argv[1]);
	PLSetFilename(iconset, val);
	PLSave(iconset, NO);
    } else {
	puts(PLGetDescriptionIndent(iconset, 0));
    }
    exit(0);
}


