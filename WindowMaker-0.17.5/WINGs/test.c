/*
 * WINGs test application
 */

#include "WINGs.h"

#include <stdio.h>



/*
 * You need to define this function to link any program to WINGs.
 * This will be called when the application will be terminated because
 * on a fatal error.
 */
void
wAbort()
{
    exit(1);
}

/*
 * ProgName must also be defined, so that the wwarning() & co. functions
 * display the program name in their messsages.
 */
char *ProgName;




Display *dpy;


void
closeAction(WMWidget *self, void *data)
{
    exit(0);
}


void
testOpenFilePanel(WMScreen *scr)
{
    WMOpenPanel *panel;
    
    /* creates the Open File panel */
    panel = WMCreateOpenPanel(scr);
    
    WMRunModalOpenPanelForDirectory(panel, "/usr/local", NULL, NULL);
    
    /* destroy and free the panel */
    WMDestroyFilePanel(panel);
}


void
testScrollView(WMScreen *scr)
{
    WMWindow *win;
    WMScrollView *sview;

    /* creates the top-level window */
    win = WMCreateWindow(scr, "testScroll");
    WMSetWindowTitle(win, "Scrollable View");
    
    WMSetWindowCloseAction(win, closeAction, NULL);
    
    /* set the window size */
    WMResizeWidget(win, 300, 300);
    
    /* creates a scrollable view inside the top-level window */
    sview = WMCreateScrollView(win);
    WMResizeWidget(sview, 200, 200);
    WMMoveWidget(sview, 30, 30);
    WMSetScrollViewRelief(sview, WRSunken);
    WMSetScrollViewHasVerticalScroller(sview, True);
    WMSetScrollViewHasHorizontalScroller(sview, True);
    
    /* make the windows of the widgets be actually created */
    WMRealizeWidget(win);
    
    /* Map all child widgets of the top-level be mapped.
     * You must call this for each container widget (like frames),
     * even if they are childs of the top-level window.
     */
    WMMapSubwidgets(win);

    /* map the top-level window */
    WMMapWidget(win);
}


void
testColorWell(WMScreen *scr)
{
    WMWindow *win;
    WMColorWell *well1, *well2;

    win = WMCreateWindow(scr, "testColor");
    WMResizeWidget(win, 300, 300);

    WMSetWindowCloseAction(win, closeAction, NULL);

    well1 = WMCreateColorWell(win);
    WMResizeWidget(well1, 40, 30);
    WMMoveWidget(well1, 100, 100);
    WMSetColorWellColor(well1, WMCreateRGBColor(scr, 0x8888, 0, 0x1111, True));
    well2 = WMCreateColorWell(win);
    WMResizeWidget(well2, 40, 30);
    WMMoveWidget(well2, 200, 100);
    WMSetColorWellColor(well2, WMCreateRGBColor(scr, 0, 0, 0x8888, True));
    
    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
}

void
testTextField(WMScreen *scr)
{
    WMWindow *win;
    WMTextField *field, *field2;

    win = WMCreateWindow(scr, "testText");
    WMResizeWidget(win, 400, 300);

    WMSetWindowCloseAction(win, closeAction, NULL);    

    field = WMCreateTextField(win);
    WMResizeWidget(field, 200, 20);
    WMMoveWidget(field, 20, 20);

    field2 = WMCreateTextField(win);
    WMResizeWidget(field2, 200, 20);
    WMMoveWidget(field2, 20, 50);
    WMSetTextFieldAlignment(field2, WARight);

    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
    
}


int main(int argc, char **argv)
{
    WMScreen *scr;
    WMPixmap *pixmap;

    /*
     * Open connection to the X display.
     */
    dpy = XOpenDisplay("");
    
    if (!dpy) {
	puts("could not open display");
	exit(1);
    }
    
    /* This is used to disable buffering of X protocol requests. 
     * Do NOT use it unless when debugging. It will cause a major 
     * slowdown in your application
     */
    XSynchronize(dpy, True);
    
    /*
     * Create application descriptor.
     */
    scr = WMCreateScreen(dpy, DefaultScreen(dpy), "Test", &argc, argv);

    /*
     * Loads the logo of the application.
     */
    pixmap = WMCreatePixmapFromFile(scr, "logo.xpm");
    
    /*
     * Makes the logo be used in standard dialog panels.
     */
    WMSetApplicationIconImage(scr, pixmap); WMReleasePixmap(pixmap);
    
    /* 
     * Do some test stuff 
     */
    testScrollView(scr);
    
    /*
     * The main event loop.
     * 
     */
    WMScreenMainLoop(scr);

    return 0;
}
