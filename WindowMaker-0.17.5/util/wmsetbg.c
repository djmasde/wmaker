

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <string.h>
#include <wraster.h>
#include <pwd.h>
#include <sys/types.h>

#include "../src/wconfig.h"

#include <proplist.h>



#ifdef DEBUG
#include <sys/time.h>
#include <time.h>
#endif

#define WTP_TILE  1
#define WTP_SCALE 2


char *ProgName;



void*
wmalloc(size_t size)
{
    void *ptr;
    ptr = malloc(size);
    if (!ptr) {
	perror(ProgName);
	exit(1);
    }
    return ptr;
}

char*
gethomedir()
{
    char *home = getenv("HOME");
    struct passwd *user;

    if (home)
      return home;
    
    user = getpwuid(getuid());
    if (!user) {
        perror(ProgName);
        return "/";
    }
    if (!user->pw_dir) {
        return "/";
    } else {
        return user->pw_dir;
    }
}



void wAbort()
{
    exit(1);
}


void
print_help()
{
    printf("usage: %s [-options] image\n", ProgName);
    puts("options:");
    puts(" -d		dither image");
    puts(" -m		match  colors");
    puts(" -t		tile   image");
    puts(" -s		scale  image (default)");
    puts(" -u		update WindowMaker domain database");
    puts(" -D <domain>	update <domain> database");
    puts(" -c <cpc>	colors per channel to use");
}


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
        strcpy(path, gethomedir());
        strcat(path, "/GNUstep/");
    }
    strcat(path, DEFAULTS_DIR);
    strcat(path, "/");
    strcat(path, domain);

    tmp = wmalloc(strlen(path)+2);
    strcpy(tmp, path);

    return tmp;
}


char *wstrdup(char *str)
{
	return strcpy(malloc(strlen(str)+1), str);
}


/* Returns an array of pointers to the pixmap paths, doing ~ expansion */
static char**
getPixmapPath(char *domain)
{
    char **ret;
    char *path;
    proplist_t prop, pixmap_path, key, value;
    int count, i;

    path = defaultsPathForDomain(domain);
    if (!path)
        return NULL;

    prop = PLGetProplistWithPath(path);
    if (!prop || !PLIsDictionary(prop))
        return NULL;

    key = PLMakeString("PixmapPath");
    pixmap_path = PLGetDictionaryEntry(prop, key);
    PLRelease(key);
    if (!pixmap_path || !PLIsArray(pixmap_path))
        return NULL;

    count = PLGetNumberOfElements(pixmap_path);
    if (count < 1)
        return NULL;

    ret = wmalloc(sizeof(char*)*(count+1));

    for (i=0; i<count; i++) {
        value = PLGetArrayElement(pixmap_path, i);
        if (!value || !PLIsString(value))
            break;
        ret[i] = wstrdup(PLGetString(value));
        if (ret[i][0]=='~' && ret[i][1]=='/') {
            /* home is statically allocated. Don't free it */
            char *fullpath, *home=gethomedir();

            fullpath = wmalloc(strlen(home)+strlen(ret[i]));
            strcpy(fullpath, home);
            strcat(fullpath, &ret[i][1]);
            free(ret[i]);
            ret[i] = fullpath;
        }
    }
    ret[i] = NULL;
    return ret;
}


int
main(int argc, char **argv)
{
    Display *dpy;
    Window root_win;
    RContextAttributes rattr;
    int screen_number, default_depth, i, style = WTP_SCALE;
    int scr_width, scr_height;
    RContext *rcontext;
    RImage *image, *tmp;
    Pixmap pixmap;
    char *image_name = NULL;
    char *domain = "WindowMaker";
    char *program = "wdwrite";
    int update=0, cpc=4, render_mode=RM_MATCH, obey_user=0;
#ifdef DEBUG
    double t1, t2, total, t;
    struct timeval timev;
#endif


    ProgName = strrchr(argv[0],'/');
    if (!ProgName)
      ProgName = argv[0];
    else
      ProgName++;

    if (argc>1) {
	for (i=1; i<argc; i++) {
            if (strcmp(argv[i], "-s")==0) {
                style = WTP_SCALE;
            } else if (strcmp(argv[i], "-t")==0) {
                style = WTP_TILE;
            } else if (strcmp(argv[i], "-d")==0) {
                render_mode = RM_DITHER;
                obey_user++;
            } else if (strcmp(argv[i], "-m")==0) {
                render_mode = RM_MATCH;
                obey_user++;
            } else if (strcmp(argv[i], "-u")==0) {
                update++;
            } else if (strcmp(argv[i], "-D")==0) {
                update++;
                i++;
                if (i>=argc) {
                    fprintf(stderr, "too few arguments for %s\n", argv[i-1]);
                    exit(0);
                }
                domain = wstrdup(argv[i]);
            } else if (strcmp(argv[i], "-c")==0) {
                i++;
                if (i>=argc) {
		    fprintf(stderr, "too few arguments for %s\n", argv[i-1]);
                    exit(0);
                }
                if (sscanf(argv[i], "%i", &cpc)!=1) {
                    fprintf(stderr, "bad value for colors per channel: \"%s\"\n", argv[i]);
                    exit(0);
                }
            } else if (argv[i][0] != '-') {
                image_name = argv[i];
            } else {
                print_help();
                exit(1);
            }
	}
    }

    if (image_name == NULL) {
        print_help();
        exit(1);
    }
    if (update) {
        char *value = wmalloc(sizeof(image_name) + 30);
        char *tmp=image_name, **paths;
        int i;

        /* should we read PixmapPath from the same file as we write into ? */
        paths = getPixmapPath("WindowMaker");
        if (paths) {
            for(i=0; paths[i]!=NULL; i++) {
                if ((tmp = strstr(image_name, paths[i])) != NULL &&
                    tmp == image_name) {
                    tmp += strlen(paths[i]);
                    while(*tmp=='/') tmp++;
                    break;
                }
            }
        }

        if (!tmp)
            tmp = image_name;

        if (style == WTP_TILE)
            strcpy(value, "(tpixmap, \"");
        else
            strcpy(value, "(spixmap, \"");
        strcat(value, tmp);
        strcat(value, "\", black)");
        execlp(program, program, domain, "WorkspaceBack", value, NULL);
        printf("%s: warning could not run \"%s\"\n", ProgName, program);
        /* Do not exit. At least try to put the image in the background */
    }

    dpy = XOpenDisplay("");
    if (!dpy) {
	puts("Could not open display!");
	exit(1);
    }

    screen_number = DefaultScreen(dpy);
    root_win = RootWindow(dpy, screen_number);
    default_depth = DefaultDepth(dpy, screen_number);
    scr_width = WidthOfScreen(ScreenOfDisplay(dpy, screen_number));
    scr_height = HeightOfScreen(ScreenOfDisplay(dpy, screen_number));

    if (!obey_user && default_depth <= 8)
        render_mode = RM_DITHER;

    rattr.flags = RC_RenderMode | RC_ColorsPerChannel;
    rattr.render_mode = render_mode;
    rattr.colors_per_channel = cpc;

    rcontext = RCreateContext(dpy, screen_number, &rattr);
    if (!rcontext) {
	printf("could not initialize graphics library context: %s\n",
               RErrorString);
	exit(1);
    }

#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    t = t1;
#endif
    image = RLoadImage(rcontext, image_name, 0);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("load image in %f sec\n", total);
#endif

    if (!image) {
        printf("could not load image: %s\n", image_name);
        exit(1);
    }

#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
#endif
    if (style == WTP_SCALE) {
        tmp = RScaleImage(image, scr_width, scr_height);
        if (!tmp) {
            printf("could not scale image: %s\n", image_name);
            exit(1);
        }
        RDestroyImage(image);
        image = tmp;
    }
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("scale image in %f sec\n", total);

    gettimeofday(&timev, NULL);
    t1 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
#endif
    RConvertImage(rcontext, image, &pixmap);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t1;
    printf("convert image to pixmap in %f sec\n", total);
    total = t2 - t;
    printf("total image proccessing in %f sec\n", total);
#endif
    RDestroyImage(image);
    XSetWindowBackgroundPixmap(dpy, root_win, pixmap);
    XClearWindow(dpy, root_win);
    XFlush(dpy);
    XCloseDisplay(dpy);
#ifdef DEBUG
    gettimeofday(&timev, NULL);
    t2 = (double)timev.tv_sec + (((double)timev.tv_usec)/1000000);
    total = t2 - t;
    printf("total proccessing time: %f sec\n", total);
#endif
    exit(0);
}
