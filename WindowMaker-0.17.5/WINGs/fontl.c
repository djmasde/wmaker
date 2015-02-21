



#include <stdlib.h>
#include <stdio.h>
#include "WINGs.h"
#include "WUtil.h"

void
wAbort()
{
    exit(0);
}

void show(WMWidget *self, void *data)
{
    char buf[60];
    void *d;
    WMLabel *l = (WMLabel*)data;
    d = WMGetHangedData(self);
    sprintf(buf, "%i -  0x%x - 0%o", (int)d, (int)d, (int)d);
    WMSetLabelText(l, buf);
}

void quit(WMWidget *self, void *data)
{
    exit(0);
}


char *ProgName;

int
main(int argc, char **argv)
{
    Display *dpy;
    WMWindow *win;
    WMScreen *scr;
    WMButton *lab, *l0=NULL;
    WMLabel *pos;
    int x, y, c;
    char buf[20];
    
    ProgName = argv[0];

    dpy = XOpenDisplay("");
    if (!dpy) {
	wfatal("cant open display");
	exit(0);
    }
    
    scr = WMCreateSimpleApplicationScreen(dpy, "Fonts", &argc, argv);
    
    win = WMCreateWindow(scr, "main");
    WMResizeWidget(win, 20*33, 20+20*9);
    WMSetWindowTitle(win, "Font Chars");
    WMSetWindowCloseAction(win, quit, NULL);
    pos = WMCreateLabel(win);
    WMResizeWidget(pos, 20*33, 20);
    WMMoveWidget(pos, 10, 5);

    c = 0;
    for (y=0; y<8; y++) {
	for (x=0; x<32; x++, c++) {
	    lab = WMCreateCustomButton(win, WBBStateLightMask);
	    WMResizeWidget(lab, 20, 20);
	    WMMoveWidget(lab, 10+x*20, 30+y*20);
	    sprintf(buf, "%c", c);
	    WMSetButtonText(lab, buf);
	    WMSetButtonAction(lab, show, pos);
	    WMHangData(lab, (void*)c);
	    if (c>0) {
		WMGroupButtons(l0, lab);
	    } else {
		l0 = lab;
	    }
	}
    }
    WMRealizeWidget(win);
    WMMapSubwidgets(win);
    WMMapWidget(win);
    WMScreenMainLoop(scr);
    return 0;   
}

