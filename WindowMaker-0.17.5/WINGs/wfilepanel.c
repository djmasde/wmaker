



#include "WINGsP.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef struct W_FilePanel {
    WMWindow *win;
    
    WMLabel *iconLabel;
    WMLabel *titleLabel;
    
    WMFrame *line;

    WMLabel *nameLabel;
    WMBrowser *browser;
    
    WMButton *okButton;
    WMButton *cancelButton;
    
    WMButton *homeButton;

    WMTextField *fileField;
    
    char **fileTypes;

    struct {
	unsigned int canExit:1;
	unsigned int canceled:1;       /* clicked on cancel */
	unsigned int done:1;
	unsigned int filtered:1;
	unsigned int canChooseFiles:1;
	unsigned int canChooseDirectories:1;
	unsigned int showAllFiles:1;
	unsigned int canFreeFileTypes:1;
    } flags;
} W_FilePanel;


#define PWIDTH		320
#define PHEIGHT 	360

static void listDirectoryOnColumn(WMFilePanel *panel, int column, char *path);
static void browserClick();

static void fillColumn(WMBrowser *bPtr, int column);

static void goHome();

static void buttonClick();


WMFilePanel*
WMCreateOpenPanel(WMScreen *scrPtr)
{
    WMFilePanel *fPtr;
    WMFont *largeFont;

    fPtr = wmalloc(sizeof(WMFilePanel));
    memset(fPtr, 0, sizeof(WMFilePanel));

    fPtr->win = WMCreateWindow(scrPtr, "openPanel");
    WMResizeWidget(fPtr->win, PWIDTH, PHEIGHT);
    
    fPtr->iconLabel = WMCreateLabel(fPtr->win);
    WMResizeWidget(fPtr->iconLabel, 64, 64);
    WMMoveWidget(fPtr->iconLabel, 0, 0);
    WMSetLabelImagePosition(fPtr->iconLabel, WIPImageOnly);
    WMSetLabelImage(fPtr->iconLabel, scrPtr->applicationIcon);
    
    fPtr->titleLabel = WMCreateLabel(fPtr->win);
    WMResizeWidget(fPtr->titleLabel, PWIDTH-64, 64);
    WMMoveWidget(fPtr->titleLabel, 64, 0);
    largeFont = WMBoldSystemFontOfSize(scrPtr, 24);
    WMSetLabelFont(fPtr->titleLabel, largeFont);
    WMReleaseFont(largeFont);
    WMSetLabelText(fPtr->titleLabel, "Open");
    
    fPtr->line = WMCreateFrame(fPtr->win);
    WMMoveWidget(fPtr->line, 0, 64);
    WMResizeWidget(fPtr->line, PWIDTH, 2);
    WMSetFrameRelief(fPtr->line, WRGroove);

    fPtr->browser = WMCreateBrowser(fPtr->win);
    WMSetBrowserFillColumnProc(fPtr->browser, fillColumn);
    WMSetBrowserAction(fPtr->browser, browserClick, fPtr);
    WMMoveWidget(fPtr->browser, 7, 72);
    WMHangData(fPtr->browser, fPtr);

    fPtr->nameLabel = WMCreateLabel(fPtr->win);
    WMMoveWidget(fPtr->nameLabel, 7, 282);
    WMResizeWidget(fPtr->nameLabel, 55, 14);
    WMSetLabelText(fPtr->nameLabel, "Name:");
    
    fPtr->fileField = WMCreateTextField(fPtr->win);
    WMMoveWidget(fPtr->fileField, 60, 278);
    WMResizeWidget(fPtr->fileField, PWIDTH-60-10, 24);
    
    fPtr->okButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->okButton, 230, 325);
    WMResizeWidget(fPtr->okButton, 80, 28);
    WMSetButtonText(fPtr->okButton, "OK");
    WMSetButtonImage(fPtr->okButton, scrPtr->buttonArrow);
    WMSetButtonAltImage(fPtr->okButton, scrPtr->pushedButtonArrow);
    WMSetButtonImagePosition(fPtr->okButton, WIPRight);
    WMSetButtonAction(fPtr->okButton, buttonClick, fPtr);
    
    fPtr->cancelButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->cancelButton, 140, 325);
    WMResizeWidget(fPtr->cancelButton, 80, 28);
    WMSetButtonText(fPtr->cancelButton, "Cancel");
    WMSetButtonAction(fPtr->cancelButton, buttonClick, fPtr);

    fPtr->homeButton = WMCreateCommandButton(fPtr->win);
    WMMoveWidget(fPtr->homeButton, 55, 325);
    WMResizeWidget(fPtr->homeButton, 28, 28);
    WMSetButtonImagePosition(fPtr->homeButton, WIPImageOnly);
    WMSetButtonImage(fPtr->homeButton, scrPtr->homeIcon);
    WMSetButtonAction(fPtr->homeButton, goHome, fPtr);

    WMRealizeWidget(fPtr->win);
    WMMapSubwidgets(fPtr->win);

    WMLoadBrowserColumnZero(fPtr->browser);

    return fPtr;
}


void
WMDestroyFilePanel(WMFilePanel *panel)
{
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    free(panel);
}


int
WMRunModalOpenPanelForDirectory(WMFilePanel *panel, char *path, char *name,
				char **fileTypes)
{
    WMScreen *scr = WMWidgetScreen(panel->win);
    XEvent event;
	
    WMSetFilePanelDirectory(panel, path);

    panel->flags.done = 0;
    panel->fileTypes = fileTypes;

    if (fileTypes)
	panel->flags.filtered = 1;
    panel->fileTypes = fileTypes;

    WMSetBrowserPath(panel->browser, path);
    
    WMMapWidget(panel->win);

    while (!panel->flags.done) {	
	WMNextEvent(scr->display, &event);
	WMHandleEvent(&event);
    }

    WMUnmapWidget(panel->win);

    return (panel->flags.canceled ? False : True);
}


void
WMSetFilePanelDirectory(WMFilePanel *panel, char *path)
{
    WMSetTextFieldText(panel->fileField, path);
    WMSetBrowserPath(panel->browser, path);
}

			
void
WMSetFilePanelCanChooseDirectories(WMFilePanel *panel, int flag)
{
    panel->flags.canChooseDirectories = flag;
}

void
WMSetFilePanelCanChooseFiles(WMFilePanel *panel, int flag)
{
    panel->flags.canChooseFiles = flag;
}


char*
WMGetFilePanelFile(WMFilePanel *panel)
{
    return WMGetTextFieldText(panel->fileField);
}


static char*
get_name_from_path(char *path)
{
    int size;
    
    assert(path!=NULL);
        
    size = strlen(path);

    /* remove trailing / */
    while (size > 0 && path[size-1]=='/')
	size--;
    /* directory was root */
    if (size == 0)
	return wstrdup("/");

    while (size > 0 && path[size-1] != '/')
	size--;
    
    return wstrdup(&(path[size]));
}


static int
filterFileName(WMFilePanel *panel, char *file, Bool isDirectory)
{
    return True;
}


static void
listDirectoryOnColumn(WMFilePanel *panel, int column, char *path)
{
    WMBrowser *bPtr = panel->browser;
    struct dirent *dentry;
    DIR *dir;
    struct stat stat_buf;
    char pbuf[PATH_MAX+16];

    assert(column >= 0);
    assert(path != NULL);

    /* put directory name in the title */
    WMSetBrowserColumnTitle(bPtr, column, get_name_from_path(path));
    
    dir = opendir(path);
    
    if (!dir) {
#ifdef VERBOSE
	printf("WINGs: could not open directory %s\n", path);
#endif
	return;
    }

    /* list contents in the column */
    while ((dentry = readdir(dir))) {	       
	if (strcmp(dentry->d_name, ".")==0 ||
	    strcmp(dentry->d_name, "..")==0)
	    continue;

	strcpy(pbuf, path);
	if (strcmp(path, "/")!=0)
	    strcat(pbuf, "/");
	strcat(pbuf, dentry->d_name);

	if (stat(pbuf, &stat_buf)!=0) {
#ifdef VERBOSE
	    printf("WINGs: could not stat %s\n", pbuf);
#endif
	    continue;
	} else {
	    int isDirectory;

	    isDirectory = S_ISDIR(stat_buf.st_mode);
	    
	    if (filterFileName(panel, dentry->d_name, isDirectory))
		WMAddBrowserItem(bPtr, column, dentry->d_name, isDirectory);
	}
    }

    closedir(dir);
}


static void
fillColumn(WMBrowser *bPtr, int column)
{
    char *path;
    WMFilePanel *panel;
    
    if (column > 0) {
	path = WMGetBrowserPathToColumn(bPtr, column-1);
    } else {
	path = wstrdup("/");
    }

    panel = WMGetHangedData(bPtr);
    listDirectoryOnColumn(panel, column, path);
    free(path);
}



static void 
browserClick(WMBrowser *bPtr, WMFilePanel *panel)
{
    WMSetTextFieldText(panel->fileField, WMGetBrowserPath(bPtr));
}


static void
goHome(WMButton *bPtr, WMFilePanel *panel)
{
    char *home;

    /* home is statically allocated. Don't free it! */
    home = wgethomedir();
    if (!home)
	return;
    
    WMSetFilePanelDirectory(panel, home);
}


static void
buttonClick(WMButton *bPtr, WMFilePanel *panel)
{
    if (bPtr == panel->okButton)
	panel->flags.canceled = 0;
    else
	panel->flags.canceled = 1;

    panel->flags.done = 1;
}
