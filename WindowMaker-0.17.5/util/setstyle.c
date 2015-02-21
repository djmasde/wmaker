

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
    proplist_t prop, style;
    char *path;

    ProgName = argv[0];
    
    if (argc!=2) {
	printf("Syntax:\n%s <style file>\n", argv[0]);
	exit(1);
    }
    
    path = defaultsPathForDomain("WindowMaker");
    
    prop = PLGetProplistWithPath(path);
    if (!prop) {
	printf("%s:could not load WindowMaker configuration file \"%s\".\n", 
	       ProgName, path);
	exit(1);
    }

    style = PLGetProplistWithPath(argv[1]);
    if (!style) {
	printf("%s:could not load style file \"%s\".\n", ProgName, argv[1]);
	exit(1);
    }

    PLMergeDictionaries(prop, style);
    
    PLSave(prop, YES);

    exit(0);
}


