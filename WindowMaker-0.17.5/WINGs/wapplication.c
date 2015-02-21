

#include "WINGsP.h"

#include <X11/Xutil.h>
#include <ctype.h>

#include "GNUstep.h"


void
WMSetApplicationIconImage(WMScreen *scr, WMPixmap *icon)
{
    if (scr->applicationIcon)
	WMReleasePixmap(scr->applicationIcon);
    
    scr->applicationIcon = WMRetainPixmap(icon);
}


WMPixmap*
WMGetApplicationIconImage(WMScreen *scr)
{
    return scr->applicationIcon;
}


void
WMSetApplicationHasAppIcon(WMScreen *scr, Bool flag)
{
    scr->aflags.hasAppIcon = flag;
}



void
W_InitApplication(WMScreen *scr)
{
    if (!scr->aflags.simpleApplication) {
	Window leader;
	XClassHint classHint;
	char *instance;
	XWMHints *hints;

	leader = XCreateSimpleWindow(scr->display, scr->rootWin, -1, -1,
				     1, 1, 0, 0, 0);

	instance = wstrdup(scr->applicationName);
	instance[0] = tolower(instance[0]);
	classHint.res_name = instance;
	classHint.res_class = scr->applicationName;
	XSetClassHint(scr->display, leader, &classHint);
	free(instance);

	XSetCommand(scr->display, leader, scr->argv, scr->argc);

	hints = XAllocWMHints();

	hints->flags = WindowGroupHint;
	hints->window_group = leader;

	XSetWMHints(scr->display, leader, hints);

	XFree(hints);

	scr->groupLeader = leader;
    }
}


