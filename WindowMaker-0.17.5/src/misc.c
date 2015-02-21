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
#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <math.h>

#include <wraster.h>

#include "WindowMaker.h"
#include "GNUstep.h"
#include "screen.h"
#include "wcore.h"
#include "window.h"
#include "framewin.h"
#include "funcs.h"
#include "defaults.h"
#include "dialog.h"
#include "xutil.h"
#include "xmodifier.h"

#include "list.h"

/**** global variables *****/

extern char *DisplayName;

extern WPreferences wPreferences;

extern Time LastTimestamp;



#ifdef OFFIX_DND
extern Atom _XA_DND_SELECTION;
#endif


#ifdef USECPP
static void
putdef(char *line, char *name, char *value)
{
    if (!value) {
	wwarning(_("could not define value for %s for cpp"), name);
	return;
    }
    strcat(line, name);
    strcat(line, value);
}



static void
putidef(char *line, char *name, int value)
{
    char tmp[64];
    sprintf(tmp, "%i", value);
    strcat(line, name);
    strcat(line, tmp);
}


static char*
username()
{
    char *tmp;
    
    tmp = getlogin();
    if (!tmp) {
	struct passwd *user;

	user = getpwuid(getuid());
	if (!user) {
	    wsyserror(_("could not get password entry for UID %i"), getuid());
	    return NULL;
	}
	if (!user->pw_name) {
	    return NULL;
	} else {
	    return user->pw_name;
	}
    }
    return tmp;
}
       
char *
MakeCPPArgs(char *path)
{
    int i;
    char buffer[MAXLINE], *buf, *line;
    Visual *visual;
    
    line = wmalloc(MAXLINE);
    *line = 0;
    i=1;
    if ((buf=getenv("HOSTNAME"))!=NULL) {
	if (buf[0]=='(') {
	    wwarning(_("your machine is misconfigured. HOSTNAME is set to %s"),
		     buf);
	} else 
	  putdef(line, " -DHOST=", buf);
    } else if ((buf=getenv("HOST"))!=NULL) {
	if (buf[0]=='(') {
	    wwarning(_("your machine is misconfigured. HOST is set to %s"),
		     buf);
	} else 
	  putdef(line, " -DHOST=", buf);
    }
    buf = username();
    if (buf)
      putdef(line, " -DUSER=", buf);
    putidef(line, " -DUID=", getuid());
    buf = XDisplayName(DisplayString(dpy));
    putdef(line, " -DDISPLAY=", buf);
    putdef(line, " -DWM_VERSION=", VERSION);
    
    visual = DefaultVisual(dpy, DefaultScreen(dpy));
    putidef(line, " -DVISUAL=", visual->class);
    
    putidef(line, " -DDEPTH=", DefaultDepth(dpy, DefaultScreen(dpy)));

    putidef(line, " -DSCR_WIDTH=", WidthOfScreen(DefaultScreenOfDisplay(dpy)));
    putidef(line, " -DSCR_HEIGHT=", 
	    HeightOfScreen(DefaultScreenOfDisplay(dpy)));

    #if 0
    strcpy(buffer, path);    
    buf = strrchr(buffer, '/');
    if (buf) *buf = 0; /* trunc filename */
    putdef(line, " -I", buffer);
    #endif



    /* this should be done just once, but it works this way */
    strcpy(buffer, DEF_CONFIG_PATHS);
    buf = strtok(buffer, ":");

    do {
      char fullpath[MAXLINE];

      if (buf[0]!='~') {
	strcpy(fullpath, buf);
      } else {
	char * wgethomedir();
	/* home is statically allocated. Don't free it! */
	char *home = wgethomedir();
	
	strcpy(fullpath, home);
	strcat(fullpath, &(buf[1]));
      }

      putdef(line, " -I", fullpath);

    } while ((buf = strtok(NULL, ":"))!=NULL);
    
#undef arg
#ifdef DEBUG
    puts("CPP ARGS");
    puts(line);
#endif
    return line;
}
#endif /* USECPP */


#define canFocusShaded(win) (win->frame->workspace==scr->current_workspace \
                             && win->flags.shaded && \
                             !(win->flags.miniaturized || win->flags.hidden))


WWindow*
NextFocusWindow(WScreen *scr)
{
    WWindow *tmp, *wwin, *closest, *min;
    Window d;
    
    if (!(wwin = scr->focused_window))
        return NULL;
    tmp = wwin->prev;
    closest = NULL;
    min = wwin;
    d = 0xffffffff;
    while (tmp) {
        if ((tmp->flags.mapped || canFocusShaded(tmp))
            && tmp->window_flags.focusable &&
            !tmp->window_flags.skip_window_list) {
	    if (min->client_win > tmp->client_win)
	      min = tmp;
	    if (tmp->client_win > wwin->client_win
		&& (!closest
		    || (tmp->client_win - wwin->client_win) < d)) {
		closest = tmp;
		d = tmp->client_win - wwin->client_win;
	    }
	}
	tmp = tmp->prev;
    }
    if (!closest||closest==wwin)
      return min;
    return closest;
}


WWindow*
PrevFocusWindow(WScreen *scr)
{
    WWindow *tmp, *wwin, *closest, *max;
    Window d;
    
    if (!(wwin = scr->focused_window))
        return NULL;
    tmp = wwin->prev;
    closest = NULL;
    max = wwin;
    d = 0xffffffff;
    while (tmp) {
	if ((tmp->flags.mapped || canFocusShaded(tmp))
            && tmp->window_flags.focusable &&
            !tmp->window_flags.skip_window_list) {
	    if (max->client_win < tmp->client_win)
	      max = tmp;
	    if (tmp->client_win < wwin->client_win
		&& (!closest
		    || (wwin->client_win - tmp->client_win) < d)) {
		closest = tmp;
		d = wwin->client_win - tmp->client_win;
	    }
	}
	tmp = tmp->prev;
    }
    if (!closest||closest==wwin)
      return max;
    return closest;
}

#if 1
/*
 *  XFetchName Wrapper
 *
 */

Status MyXFetchName(dpy, win, winname)
Display *dpy;
Window win;
char **winname;
{
    XTextProperty text_prop;
    char **list;
    int num;

	if (XGetWMName(dpy, win, &text_prop) != 0 && text_prop.value
	    && text_prop.nitems > 0) {
		if (text_prop.encoding == XA_STRING)
			*winname = (char *)text_prop.value;
		else {
			text_prop.nitems = strlen((char *)text_prop.value);
			if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >=
				Success && num > 0 && *list) {
				XFree(text_prop.value);
				*winname = wstrdup(*list);
				XFreeStringList(list);
			} else
				*winname = (char *)text_prop.value;
		}
		return 1;
    }
	*winname = NULL;
    return 0;  
}

/*
 *  XGetIconName Wrapper
 *
 */

Status MyXGetIconName(dpy, win, iconname)
Display *dpy;
Window win;
char **iconname;
{
    XTextProperty text_prop;
    char **list;
    int num;
    
    if (XGetWMIconName(dpy, win, &text_prop) != 0 && text_prop.value
	&& text_prop.nitems > 0) {
	if (text_prop.encoding == XA_STRING)
	    *iconname = (char *)text_prop.value;
	else {
	    text_prop.nitems = strlen((char *)text_prop.value);
	    if (XmbTextPropertyToTextList(dpy, &text_prop, &list, &num) >=
		Success && num > 0 && *list) {
		XFree(text_prop.value);
		*iconname = wstrdup(*list);
		XFreeStringList(list);
	    } else
		*iconname = (char *)text_prop.value;
	}
	return 1;
    }
    *iconname = NULL;
    return 0;
}
#endif /* I18N_MB */


static void eatExpose()
{
    XEvent event, foo;
    
    /* compress all expose events into a single one */
    
    if (XCheckMaskEvent(dpy, ExposureMask, &event)) {
	/* ignore other exposure events for this window */
	while (XCheckWindowEvent(dpy, event.xexpose.window, ExposureMask,
			       &foo));
	/* eat exposes for other windows */
	eatExpose();
	
	event.xexpose.count = 0;
	XPutBackEvent(dpy, &event);
    }
}


void
SlideWindow(Window win, int from_x, int from_y, int to_x, int to_y)
{
    float dx, dy, x=from_x, y=from_y, sx, sy, px, py;
    int dx_is_bigger=0;
    /* animation parameters */
    static struct {
	int delay;
	int steps;
	int slowdown;
    } apars[5] = {
	{ICON_SLIDE_DELAY_UF, ICON_SLIDE_STEPS_UF, ICON_SLIDE_SLOWDOWN_UF},
	{ICON_SLIDE_DELAY_F, ICON_SLIDE_STEPS_F, ICON_SLIDE_SLOWDOWN_F},
	{ICON_SLIDE_DELAY_M, ICON_SLIDE_STEPS_M, ICON_SLIDE_SLOWDOWN_M},
	{ICON_SLIDE_DELAY_S, ICON_SLIDE_STEPS_S, ICON_SLIDE_SLOWDOWN_S},
	{ICON_SLIDE_DELAY_U, ICON_SLIDE_STEPS_U, ICON_SLIDE_SLOWDOWN_U}};
    
    

    dx = (float)(to_x-from_x);
    dy = (float)(to_y-from_y);
    sx = (dx == 0 ? 0 : fabs(dx)/dx);
    sy = (dy == 0 ? 0 : fabs(dy)/dy);

    if (fabs(dx) > fabs(dy)) {
        dx_is_bigger = 1;
    }

    if (dx_is_bigger) {
        px = dx / apars[wPreferences.icon_slide_speed].slowdown;
        if (px < apars[wPreferences.icon_slide_speed].steps && px > 0)
            px = apars[wPreferences.icon_slide_speed].steps;
        else if (px > -apars[wPreferences.icon_slide_speed].steps && px < 0)
            px = -apars[wPreferences.icon_slide_speed].steps;
        py = (sx == 0 ? 0 : px*dy/dx);
    } else {
        py = dy / apars[wPreferences.icon_slide_speed].slowdown;
        if (py < apars[wPreferences.icon_slide_speed].steps && py > 0)
            py = apars[wPreferences.icon_slide_speed].steps;
        else if (py > -apars[wPreferences.icon_slide_speed].steps && py < 0)
            py = -apars[wPreferences.icon_slide_speed].steps;
        px = (sy == 0 ? 0 : py*dx/dy);
    }

    while (x != to_x || y != to_y) {
	x += px;
        y += py;
        if ((px<0 && (int)x < to_x) || (px>0 && (int)x > to_x))
            x = (float)to_x;
        if ((py<0 && (int)y < to_y) || (py>0 && (int)y > to_y))
            y = (float)to_y;

        if (dx_is_bigger) {
            px = px * (1.0 - 1/(float)apars[wPreferences.icon_slide_speed].slowdown);
            if (px < apars[wPreferences.icon_slide_speed].steps && px > 0)
                px = apars[wPreferences.icon_slide_speed].steps;
            else if (px > -apars[wPreferences.icon_slide_speed].steps && px < 0)
                px = -apars[wPreferences.icon_slide_speed].steps;
            py = (sx == 0 ? 0 : px*dy/dx);
        } else {
            py = py * (1.0 - 1/(float)apars[wPreferences.icon_slide_speed].slowdown);
            if (py < apars[wPreferences.icon_slide_speed].steps && py > 0)
                py = apars[wPreferences.icon_slide_speed].steps;
            else if (py > -apars[wPreferences.icon_slide_speed].steps && py < 0)
                py = -apars[wPreferences.icon_slide_speed].steps;
            px = (sy == 0 ? 0 : py*dx/dy);
        }

        XMoveWindow(dpy, win, (int)x, (int)y);
        XFlush(dpy);
        if (apars[wPreferences.icon_slide_speed].delay > 0) {
            wmsleep(apars[wPreferences.icon_slide_speed].delay*1000L);
        }
    }
    XMoveWindow(dpy, win, to_x, to_y);

    XSync(dpy, 0);
    /* compress expose events */
    eatExpose();
}


char*
ShrinkString(WFont *font, char *string, int width)
{
#ifndef I18N_MB
    int w, w1=0;
    int p;
    char *pos;
    char *text;
    int p1, p2, t;
#endif

#ifdef I18N_MB
    return wstrdup(string);
#else
    p = strlen(string);
    w = MyTextWidth(font->font, string, p);
    text = wmalloc(strlen(string)+8);
    strcpy(text, string);
    if (w<=width)
      return text;

    pos = strchr(text, ' ');
    if (!pos)
      pos = strchr(text, ':');

    if (pos) {
	*pos = 0;
	p = strlen(text);
	w1=MyTextWidth(font->font, text, p);
	if (w1>width) {
	    w1 = 0;
	    p = 0;
	    *pos = ' ';
	    *text = 0;
	} else {
	    *pos = 0;
	    width -= w1;
	    p++;
	}
	string += p;
	p=strlen(string);
    } else {
	*text=0;
    }
    strcat(text, "...");
    width -= MyTextWidth(font->font, "...", 3);
    
    pos = string;
    p1=0;
    p2=p;
    t = (p2-p1)/2;
    while (p2>p1 && p1!=t) {
	w = MyTextWidth(font->font, &string[p-t], t);
	if (w>width) {
	    p2 = t;
	    t = p1+(p2-p1)/2;
	} else if (w<width) {
	    p1 = t;
	    t = p1+(p2-p1)/2;
	} else 
	  p2=p1=t;
    }
    strcat(text, &string[p-p1]);
    return text;
#endif /* I18N_MB */
}


char*
FindImage(char **paths, char *file)
{
    char *tmp, *path;
    
    tmp = strrchr(file, ':');
    if (tmp) {
	*tmp = 0;
	path = wfindfileinlist(paths, file);
	*tmp = ':';
    }
    if (!tmp || !path) {
	path = wfindfileinlist(paths, file);
    }
    return path;
}


char*
FlattenStringList(char **list, int count)
{
    int i, j;
    char *flat_string, *wspace;

    j = 0;
    for (i=0; i<count; i++) {
        if (list[i]!=NULL && list[i][0]!=0) {
            j += strlen(list[i]);
            if (strpbrk(list[i], " \t"))
                j += 2;
        }
    }
    
    flat_string = malloc(j+count+1);
    if (!flat_string) {
	return NULL;
    }

    strcpy(flat_string, list[0]);
    for (i=1; i<count; i++) {
	if (list[i]!=NULL && list[i][0]!=0) {
            strcat(flat_string, " ");
            wspace = strpbrk(list[i], " \t");
            if (wspace)
                strcat(flat_string, "\"");
	    strcat(flat_string, list[i]);
            if (wspace)
                strcat(flat_string, "\"");
	}
    }
    
    return flat_string;
}



/*
 *----------------------------------------------------------------------
 * ParseCommand --
 * 	Divides a command line into a argv/argc pair.
 *---------------------------------------------------------------------- 
 */
#define PRC_ALPHA	0
#define PRC_BLANK	1
#define PRC_ESCAPE	2
#define PRC_DQUOTE	3
#define PRC_EOS		4
#define PRC_SQUOTE	5

typedef struct {
    short nstate;
    short output;
} DFA;


static DFA mtable[9][6] = {
    {{3,1},{0,0},{4,0},{1,0},{8,0},{6,0}},
    {{1,1},{1,1},{2,0},{3,0},{5,0},{1,1}},
    {{1,1},{1,1},{1,1},{1,1},{5,0},{1,1}},
    {{3,1},{5,0},{4,0},{1,0},{5,0},{6,0}},
    {{3,1},{3,1},{3,1},{3,1},{5,0},{3,1}},
    {{-1,-1},{0,0},{0,0},{0,0},{0,0},{0,0}}, /* final state */
    {{6,1},{6,1},{7,0},{6,1},{5,0},{3,0}},
    {{6,1},{6,1},{6,1},{6,1},{5,0},{6,1}},
    {{-1,-1},{0,0},{0,0},{0,0},{0,0},{0,0}}, /* final state */
};

char*
next_token(char *word, char **next)
{
    char *ptr;
    char *ret, *t;
    int state, ctype;

    t = ret = wmalloc(strlen(word)+1);
    ptr = word;
    
    state = 0;
    *t = 0;
    while (1) {
	if (*ptr==0) 
	    ctype = PRC_EOS;
	else if (*ptr=='\\')
	    ctype = PRC_ESCAPE;
	else if (*ptr=='"')
	    ctype = PRC_DQUOTE;
	else if (*ptr=='\'')
	    ctype = PRC_SQUOTE;
	else if (*ptr==' ' || *ptr=='\t')
	    ctype = PRC_BLANK;
	else
	    ctype = PRC_ALPHA;

	if (mtable[state][ctype].output) {
	    *t = *ptr; t++;
	    *t = 0;
	}
	state = mtable[state][ctype].nstate;
	ptr++;
	if (mtable[state][0].output<0) {
	    break;
	}
    }

    if (*ret==0)
	t = NULL;
    else
	t = wstrdup(ret);

    free(ret);
    
    if (ctype==PRC_EOS)
	*next = NULL;
    else
	*next = ptr;
    
    return t;
}


void
ParseCommand(WScreen *scr, char *command, char ***argv, int *argc)
{
    LinkedList *list = NULL;
    char *token, *line;
    int count, i;

    line = command;
    do {
	token = next_token(line, &line);
	if (token) {	    
	    list = list_cons(token, list);
	}
    } while (token!=NULL && line!=NULL);

    count = list_length(list);
    *argv = wmalloc(sizeof(char*)*count);
    i = count;
    while (list!=NULL) {
	(*argv)[--i] = list->head;
	list_remove_head(&list);
    }
    *argc = count;
}


static void
timeup(void *foo)
{
    *(int*)foo=1;
}

static char*
getselection(WScreen *scr)
{
    XEvent event;
    int timeover=0;
    WMHandlerID *id;
    
#ifdef DEBUG
    puts("getting selection");
#endif
    RequestSelection(dpy, scr->no_focus_win, LastTimestamp);
    /* timeout on 1 sec. */
    id = WMAddTimerHandler(1000, timeup, &timeover);
    while (!timeover) {
	WMNextEvent(dpy, &event);
	if (event.type == SelectionNotify 
	    && event.xany.window==scr->no_focus_win) {
	    WMDeleteTimerHandler(id);
#ifdef DEBUG
	    puts("selection ok");
#endif
	    return GetSelection(dpy, scr->no_focus_win);
	} else {
	    WMHandleEvent(&event);
	}
    }
    wwarning(_("selection timed-out"));
    return NULL;
}


static char*
getuserinput(WScreen *scr, char *line, int *ptr)
{
    char *ret;
    char *tmp;
    int i;
    char buffer[256];
    
    if (line[*ptr]!='(') {
	tmp = _("Program Arguments");
    } else {
	i = 0;
	while (line[*ptr]!=0 && line[*ptr]!=')') {
	    (*ptr)++;
	    if (line[*ptr]!='\\') {
		buffer[i++] = line[*ptr];
	    } else {
		(*ptr)++;
		if (line[*ptr]==0)
		    break;
	    }
	}
	if (i>0)
	    buffer[i-1] = 0;
	tmp = (char*)buffer;
    }

    ret = NULL;
    if (wInputDialog(scr, tmp, _("Enter command arguments:"), &ret)!= WDB_OK)
	return NULL;
    else
	return ret;
}


#ifdef OFFIX_DND
static char*
get_dnd_selection(WScreen *scr)
{
    XTextProperty text_ret;
    int result;
    char **list;
    char *flat_string;
    int count;
    
    result=XGetTextProperty(dpy, scr->root_win, &text_ret, _XA_DND_SELECTION);
    
    if (result==0 || text_ret.value==NULL || text_ret.encoding==None
	|| text_ret.format==0 || text_ret.nitems == 0) {
	wwarning(_("unable to get dropped data from DND drop"));
	return NULL;
    }
    
    XTextPropertyToStringList(&text_ret, &list, &count);
    
    if (!list || count<1) {
	XFree(text_ret.value);
	wwarning(_("error getting dropped data from DND drop"));
	return NULL;
    }

    flat_string = FlattenStringList(list, count);
    if (!flat_string) {
	wwarning(_("out of memory while getting data from DND drop"));
    }
    
    XFreeStringList(list);
    XFree(text_ret.value);
    return flat_string;
}
#endif /* OFFIX_DND */


#define S_NORMAL 0
#define S_ESCAPE 1
#define S_OPTION 2

/* 
 * state    	input   new-state	output
 * NORMAL	%	OPTION		<nil>
 * NORMAL	\	ESCAPE		<nil>
 * NORMAL	etc.	NORMAL		<input>
 * ESCAPE	any	NORMAL		<input>
 * OPTION	s	NORMAL		<selection buffer>
 * OPTION	w	NORMAL		<selected window id>
 * OPTION	a	NORMAL		<input text>
 * OPTION	d	NORMAL		<OffiX DND selection object>
 * OPTION	etc.	NORMAL		%<input>
 */
#define TMPBUFSIZE 64
char*
ExpandOptions(WScreen *scr, char *cmdline)
{
    int ptr, optr, state, len, olen;
    char *out, *nout;
    char *selection=NULL;
    char *user_input=NULL;
#ifdef OFFIX_DND
    char *dropped_thing=NULL;
#endif
    char tmpbuf[TMPBUFSIZE];
    int slen;

    len = strlen(cmdline);
    olen = len+1;
    out = malloc(olen);
    if (!out) {
	wwarning(_("out of memory during expansion of \"%s\""));
	return NULL;
    }
    *out = 0;
    ptr = 0; 			       /* input line pointer */
    optr = 0;			       /* output line pointer */
    state = S_NORMAL;
    while (ptr < len) {
	switch (state) {
	 case S_NORMAL:
	    switch (cmdline[ptr]) {
	     case '\\':
		state = S_ESCAPE;
		break;
	     case '%':
		state = S_OPTION;
		break;
	     default:
		state = S_NORMAL;
		out[optr++]=cmdline[ptr];
		break;
	    }
	    break;
	 case S_ESCAPE:
	    switch (cmdline[ptr]) {
	     case 'n':
		out[optr++]=10;
		break;
		
	     case 'r':
		out[optr++]=13;
		break;
		
	     case 't':
		out[optr++]=9;
		break;
		
	     default:
		out[optr++]=cmdline[ptr];
	    }
	    state = S_NORMAL;
	    break;
	 case S_OPTION:
	    state = S_NORMAL;
	    switch (cmdline[ptr]) {
	     case 'w':
		if (scr->focused_window
		    && scr->focused_window->flags.focused) {
		    sprintf(tmpbuf, "0x%x", 
			    (unsigned int)scr->focused_window->client_win);
		    slen = strlen(tmpbuf);
		    olen += slen;
		    nout = realloc(out,olen);
		    if (!nout) {
			wwarning(_("out of memory during expansion of \"%w\""));
			goto error;
		    }
		    out = nout;
		    strcat(out,tmpbuf);
		    optr+=slen;
		} else {
		    out[optr++]=' ';
		}
		break;
		
	     case 'a':
		ptr++;
		user_input = getuserinput(scr, cmdline, &ptr); 
		if (user_input) {
		    slen = strlen(user_input);
		    olen += slen;
		    nout = realloc(out,olen);
		    if (!nout) {
			wwarning(_("out of memory during expansion of \"%a\""));
			goto error;
		    }
		    out = nout;
		    strcat(out,user_input);
		    optr+=slen;
		}
		break;

#ifdef OFFIX_DND
	     case 'd':
		if (!dropped_thing) {
		    dropped_thing = get_dnd_selection(scr);
		}
		if (!dropped_thing) {
		    scr->flags.dnd_data_convertion_status = 1;
		    goto error;
		}
		slen = strlen(dropped_thing);
		olen += slen;
		nout = realloc(out,olen);
		if (!nout) {
		    wwarning(_("out of memory during expansion of \"%d\""));
		    goto error;
		}
		out = nout;
		strcat(out,dropped_thing);
		optr+=slen;
		break;
#endif /* OFFIX_DND */
		
	     case 's':
		if (!selection) {
		    if (!XGetSelectionOwner(dpy, XA_PRIMARY)) {
			wwarning(_("selection not available"));
			goto error;
		    }
		    selection = getselection(scr);
		}
		if (!selection) {
		    goto error;
		}
		slen = strlen(selection);
		olen += slen;
		nout = realloc(out,olen);
		if (!nout) {
		    wwarning(_("out of memory during expansion of \"%s\""));
		    goto error;
		}
		out = nout;
		strcat(out,selection);
		optr+=slen;
		break;
	     default:
		out[optr++]='%';
		out[optr++]=cmdline[ptr];
	    }
	    break;
	}
	out[optr]=0;
	ptr++;
    }
    if (selection)
      XFree(selection);
    return out;
    
    error:
    free(out);
    if (selection)
      XFree(selection);
    return NULL;
}


/* We don't care for upper/lower case in comparing the keys; so we
   have to define our own comparison function here */
BOOL
StringCompareHook(proplist_t pl1, proplist_t pl2)
{
    char *str1, *str2;

    str1 = PLGetString(pl1);
    str2 = PLGetString(pl2);

    if (strcasecmp(str1, str2)==0)
      return YES;
    else
      return NO;
}


/* feof doesn't seem to work on pipes */
int
IsEof(FILE * stream)
{
    static struct stat stinfo;

    fstat(fileno(stream), &stinfo);
    return ((S_ISFIFO(stinfo.st_dev) && stinfo.st_size == 0) || 
            feof(stream));
}


void
ParseWindowName(proplist_t value, char **winstance, char **wclass, char *where)
{
    char *name;
    char *dot;

    *winstance = *wclass = NULL;

    if (!PLIsString(value)) {
	wwarning(_("bad window name value in %s state info"), where);
	return;
    }

    name = PLGetString(value);
    if (!name || strlen(name)==0) {
	wwarning(_("bad window name value in %s state info"), where);
	return;
    }

    dot = strchr(name, '.');
    if (dot==name) {
	*wclass = wstrdup(&name[1]);
	*winstance = NULL;
    } else if (!dot) {
	*winstance = wstrdup(name);
	*wclass = NULL;
    } else {
	*dot = 0;
	*winstance = wstrdup(name);
	*dot = '.'; /* restore old string */
	*wclass = wstrdup(&dot[1]);
    }
}


static char*
catenate(char *text, char *str)
{
    int i;
    
    if (!text)
	return wstrdup(str);
    else {
	i = strlen(text)+strlen(str)+2;
	text = wrealloc(text, i);
	strcat(text, str);
	return text;
    }
}


static char*
keysymToString(KeySym keysym, unsigned int state)
{
    XKeyEvent kev;
    char *buf = wmalloc(20);
    int count;

    kev.display = dpy;
    kev.type = KeyPress;
    kev.send_event = False;
    kev.window = DefaultRootWindow(dpy);
    kev.root = DefaultRootWindow(dpy);
    kev.same_screen = True;
    kev.subwindow = kev.root;
    kev.serial = 0x12344321;
    kev.time = CurrentTime;
    kev.state = state;
    kev.keycode = XKeysymToKeycode(dpy, keysym);
    count = XLookupString(&kev, buf, 19, NULL, NULL);
    buf[count] = 0;

    return buf;
}


char*
GetShortcutString(char *text)
{
    char *buffer = NULL;
    char *tmp;
    char *k;
    int modmask = 0;
    KeySym ksym;
    
    /* get modifiers */
    while ((k = strchr(text, '+'))!=NULL) {
	int mod;
	
	*k = 0;
	mod = wXModifierFromKey(text);
	if (mod<0) {
	    return wstrdup("bug");
	}
	
	modmask |= mod;

	if (mod!=ControlMask) {
	    buffer = catenate(buffer, text);
	}
	text = k+1;
    }

    if (modmask & ControlMask) {
	buffer = catenate(buffer, "^");
	puts(buffer);
    }
    /* get key */
    ksym = XStringToKeysym(text);
    tmp = keysymToString(ksym, modmask);
    puts(tmp);
    buffer = catenate(buffer, tmp);
    free(tmp);

    return buffer;
}
