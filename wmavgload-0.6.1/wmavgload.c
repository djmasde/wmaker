/* wmavgload
 * totally hacked from wmload ... see the README file 
 * Anthony Mallet, mallet@laas.fr, November of 97 */

#include <stdio.h>
#include <string.h>

#ifndef linux /* actually only tested on sparc */
#include <sys/param.h>
#include <rpcsvc/rstat.h>

#ifdef SVR4

#include <netdb.h>
#include <sys/systeminfo.h>
#endif /* SVR4 */
#endif /* linux */

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <X11/Xatom.h>

#ifdef MANY_COLORS
#include "./back_full_color.xpm"
#else
#include "./back_less_color.xpm"
#endif
#include "./mask2.xbm"
#include "./mask.xpm"

#define major_VER 0
#define minor_VER 6
#define patch_VER 1

#define MW_EVENTS   (ExposureMask | ButtonPressMask | StructureNotifyMask)

#ifdef linux
#define FALSE 0
#endif

#define SIZE 52-6 /* x-size - 2 * 3 pixels for the bars */ 

/* x & y coordinates inthe pixmap */
#define Shape(num) (ONLYSHAPE ? num-5 : num)

/* load in pixel unit */
#define _(num) ((int)((num)*52.0/scale))

/* convert 16bit color to 8bit */
#define _r(c) ((int)((float)c/256.0))

/* clear enough ? :) */
#define max3(a,b,c) ((a)>(b)?((a)>(c)?(a):(c)):((b)>(c)?(b):(c)))
#define mid3(a,b,c) ((a)>(b)?((b)>(c)?(b):(c)):((a)>(c)?(a):(c)))
#define min3(a,b,c) ((a)>(b)?((b)>(c)?(c):(b)):((a)>(c)?(c):(a)))

/* Global Data storage/structures ********************************************/

int ONLYSHAPE=0; /* default value is noshape */

int updatespeed = 4; /* default */

static char *help_message[] = {
"where options include:",
"    -u <secs>               updatespeed",            
"    -exe <program>          program to start on click",
"    -led <color>            color of the led",
"    -1bar <color>           color of the first bar",
"    -2bar <color>           color of the second bar",
"    -a <color>              color of the scale marks",
"    -position [+|-]x[+|-]y  position of wmavgload",
"    -shape                  without groundplate",
"    -iconic                 start up as icon",
"    -withdrawn              start up withdrawn",
"    -ver                    output version",
NULL
};

int scale;   /* current scale */
float maxload; /* maximum value displayed */
float loads[SIZE]; /* the stored load values (for resizing) */
float loads_2,loads_3; /* for the bars */

#ifndef linux
extern char hostname[MAXHOSTNAMELEN];
extern struct statstime res;
#endif

/* X11 Variables *************************************************************/

Display *dpy;	  /* which DISPLAY */
Window Root;      /* Root window :) */
int screen;
int x_fd;
int d_depth;
XSizeHints mysizehints;
XWMHints mywmhints;
Pixel back_pix, fore_pix;
GC NormalGC;
Window iconwin, win;       /* My home is my window */
char *ProgName;
char *Geometry;

char *LedColor = "#a0a0c5";
char *Bar1Color = "Yellow";
char *Bar2Color = "Orange";
char *ScaleColor = "Red";

char Execute[] = "echo no program has been specified >/dev/console";
char *ERR_colorcells = "not enough free color cells\n";
char *ampers = " &";

/* XPM Structures & Variables ************************************************/

typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
}        XpmIcon;

XpmIcon wmavgload;
XpmIcon visible;
time_t actualtime;
long actualmin;

/* Function definitions ******************************************************/

void GetXPM(void);
Pixel GetColor(char *name);
void RedrawWindow( XpmIcon *v);
void InitLoad();
void InsertLoad();
void Change_Scale();

extern void GetLoad(float *,float*,float *);

/*****************************************************************************/
/* Source Code <--> Function Implementations                                 */
/*****************************************************************************/

void usage()
{
  char **cpp;

  fprintf(stderr,"\nusage:  %s [-options ...] \n", ProgName);
  for (cpp = help_message; *cpp; cpp++) {
    fprintf(stderr, "%s\n", *cpp);
  }
  fprintf(stderr,"\n");
  exit(1);
}

int main(int argc,char *argv[])
{
  int i;
  unsigned int borderwidth ;
  char *display_name = NULL; 
  char *wname = "wmavgload";
  XGCValues gcv;
  unsigned long gcm;
  XEvent Event;
  XTextProperty name;
  XClassHint classHint;
  Pixmap pixmask;
  Geometry = "";
  mywmhints.initial_state = NormalState;

  /* Parse command line options */
  ProgName = argv[0];

  for(i=1;i<argc;i++) {
    char *arg= argv[i];

    if (arg[0] == '-') {
      switch(arg[1]) {
      case 'u':
	if(++i >=argc) usage();
	sscanf(argv[i], "%d", &updatespeed);
	continue;
      case 'e':
	if(++i >=argc) usage();
	strcpy(&Execute[0], argv[i]);
	strcat(&Execute[0], " &");
	continue;
      case 's':
	ONLYSHAPE=1;
	continue;
      case 'p':
	if(++i >=argc) usage();
	Geometry = argv[i];
	continue;
      case 'i':
	mywmhints.initial_state = IconicState;
	continue;
      case 'w':
	mywmhints.initial_state = WithdrawnState;
	continue;
      case 'l':
	if(++i >=argc) usage();
	LedColor = argv[i];
	continue;
      case '1':
	if(++i >=argc) usage();
	Bar1Color = argv[i];
	continue;
      case '2':
	if(++i >=argc) usage();
	Bar2Color = argv[i];
	continue;
      case 'a':
	if(++i >=argc) usage();
	ScaleColor = argv[i];
	continue;
      case 'v':
	fprintf(stdout, "\nwmavgload version: %i.%i.%i\n",
		major_VER, minor_VER, patch_VER);
	if(argc == 2) exit(0);
	continue;
      default:
	usage();
      }
    }
    else
      {
        fprintf(stderr, "\nInvalid argument: %s\n", arg);
        usage();
      }
  }

  /* Open the display */
  if (!(dpy = XOpenDisplay(display_name)))  
    { 
      fprintf(stderr,"wmavgload: can't open display %s\n", 
	      XDisplayName(display_name)); 
      exit (1); 
    } 

  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  d_depth = DefaultDepth(dpy, screen);
  x_fd = XConnectionNumber(dpy);
  
  /* Convert XPM Data to XImage */
  GetXPM();
  
  /* Create a window to hold the banner */
  mysizehints.flags= USSize|USPosition;
  mysizehints.x = 0;
  mysizehints.y = 0;

  back_pix = GetColor("white");
  fore_pix = GetColor("black");

  XWMGeometry(dpy, screen, Geometry, NULL, (borderwidth =1), &mysizehints,
	      &mysizehints.x,&mysizehints.y,&mysizehints.width,&mysizehints.height, &i); 

  mysizehints.width = wmavgload.attributes.width;
  mysizehints.height= wmavgload.attributes.height;

  win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
			    mysizehints.width,mysizehints.height,
			    borderwidth,fore_pix,back_pix);
  iconwin = XCreateSimpleWindow(dpy,win,mysizehints.x,mysizehints.y,
				mysizehints.width,mysizehints.height,
				borderwidth,fore_pix,back_pix);

  /* activate hints */
  XSetWMNormalHints(dpy, win, &mysizehints);
  classHint.res_name =  "wmavgload";
  classHint.res_class = "WMAvgload";
  XSetClassHint(dpy, win, &classHint);

  XSelectInput(dpy,win,MW_EVENTS);
  XSelectInput(dpy,iconwin,MW_EVENTS);
  XSetCommand(dpy,win,argv,argc);
  
  if (XStringListToTextProperty(&wname, 1, &name) ==0) {
    fprintf(stderr, "wmavgload: can't allocate window name\n");
    exit(-1);
  }
  XSetWMName(dpy, win, &name);
  
  /* Create a GC for drawing */
  gcm = GCForeground|GCBackground|GCGraphicsExposures;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  gcv.graphics_exposures = FALSE;
  NormalGC = XCreateGC(dpy, Root, gcm, &gcv);  

  if (ONLYSHAPE) { /* try to make shaped window here */
    pixmask = XCreateBitmapFromData(dpy, win, mask2_bits, mask2_width, 
				    mask2_height);
    XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, pixmask, ShapeSet);
    XShapeCombineMask(dpy, iconwin, ShapeBounding, 0, 0, pixmask, ShapeSet);
  }
  
  mywmhints.icon_window = iconwin;
  mywmhints.icon_x = mysizehints.x;
  mywmhints.icon_y = mysizehints.y;
  mywmhints.window_group = win;
  mywmhints.flags = StateHint | IconWindowHint | IconPositionHint
      | WindowGroupHint;
  XSetWMHints(dpy, win, &mywmhints); 

  XMapWindow(dpy,win);
  InitLoad();
  InsertLoad();
  RedrawWindow(&visible);
  while(1)
    {
      if (actualtime != time(0))
	{
	  actualtime = time(0);
	  
	  if(actualtime % updatespeed == 0)
	    InsertLoad();

	  RedrawWindow(&visible);
	}
      
      /* read a packet */
      while (XPending(dpy))
	{
	  XNextEvent(dpy,&Event);
	  switch(Event.type)
	    {
	    case Expose:
	      if(Event.xexpose.count == 0 )
		RedrawWindow(&visible);
	      break;
	    case ButtonPress:
	      system(Execute);
	      break;
	    case DestroyNotify:
              XFreeGC(dpy, NormalGC);
              XDestroyWindow(dpy, win);
	      XDestroyWindow(dpy, iconwin);
              XCloseDisplay(dpy);
	      exit(0); 
	    default:
	      break;      
	    }
	}
      XFlush(dpy);
#ifdef SYSV
      poll(NULL, (size_t) 0, 50);
#else
      usleep(50000L);			/* 5/100 sec */
#endif
    }
}

/*****************************************************************************/
void nocolor(char *a, char *b)
{
 fprintf(stderr,"wmavgload: can't %s %s\n", a,b);
}

/*****************************************************************************/
/* convert the XPMIcons to XImage */
void GetXPM(void)
{
  static char **alt_xpm;
  XColor col;
  XWindowAttributes attributes;
  int ret;
  static char tempc1[12],tempc2[12],tempc3[12],tempc4[12];
  float colr,colg,colb;

  alt_xpm =ONLYSHAPE ? mask_xpm : back_xpm;

  /* for the colormap */
  XGetWindowAttributes(dpy,Root,&attributes);

  if (!XParseColor (dpy, attributes.colormap, LedColor, &col)) 
      nocolor("parse",LedColor);
  else
  {
     sprintf(tempc1, "Q c #%.2x%.2x%.2x",_r(col.red),_r(col.green),_r(col.blue));
#ifdef MANY_COLORS
     back_xpm[45] = tempc1;
#else
     back_xpm[5] = tempc1;
#endif
  }

  if (!XParseColor (dpy, attributes.colormap, Bar1Color, &col)) 
      nocolor("parse",Bar1Color);
  else
  {
     sprintf(tempc2, "R c #%.2x%.2x%.2x",_r(col.red),_r(col.green),_r(col.blue));
#ifdef MANY_COLORS
     back_xpm[46] = tempc2;
#else
     back_xpm[6] = tempc2;
#endif
  }

  if (!XParseColor (dpy, attributes.colormap, Bar2Color, &col)) 
      nocolor("parse",Bar2Color);
  else
  {
     sprintf(tempc3, "S c #%.2x%.2x%.2x",_r(col.red),_r(col.green),_r(col.blue));
#ifdef MANY_COLORS
     back_xpm[47] = tempc3;
#else
     back_xpm[7] = tempc3;
#endif
  }

  if (!XParseColor (dpy, attributes.colormap, ScaleColor, &col)) 
      nocolor("parse",ScaleColor);
  else
  {
     sprintf(tempc4, "T c #%.2x%.2x%.2x",_r(col.red),_r(col.green),_r(col.blue));
#ifdef MANY_COLORS
     back_xpm[48] = tempc4;
#else
     back_xpm[8] = tempc4;
#endif
  }
     
  wmavgload.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
  ret = XpmCreatePixmapFromData(dpy, Root, alt_xpm, &wmavgload.pixmap, 
				&wmavgload.mask, &wmavgload.attributes);
  if(ret != XpmSuccess)
    {fprintf(stderr, ERR_colorcells);exit(1);}

  visible.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
  ret = XpmCreatePixmapFromData(dpy, Root, back_xpm, &visible.pixmap, 
				&visible.mask, &visible.attributes);
  if(ret != XpmSuccess)
    {fprintf(stderr, ERR_colorcells);exit(1);}

}

/*****************************************************************************/
/* Removes expose events for a specific window from the queue */
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;
  
  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))i++;
  return i;
}

/*****************************************************************************/
/* Draws the icon window */
void RedrawWindow( XpmIcon *v)
{
  flush_expose (iconwin);
  XCopyArea(dpy,v->pixmap,iconwin,NormalGC,
	    0,0,v->attributes.width, v->attributes.height,0,0);
  flush_expose (win);
  XCopyArea(dpy,v->pixmap,win,NormalGC,
	    0,0,v->attributes.width, v->attributes.height,0,0);

}

/*****************************************************************************/
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Root,&attributes);
  color.pixel = 0;
   if (!XParseColor (dpy, attributes.colormap, name, &color)) 
     {
       nocolor("parse",name);
     }
   else if(!XAllocColor (dpy, attributes.colormap, &color)) 
     {
       nocolor("alloc",name);
     }
  return color.pixel;
}

/** True stuff begins here ****************************************************/
void InitLoad()
{
  /* Save the 5 base colors in wmavgload */
  XCopyArea(dpy, visible.pixmap, wmavgload.pixmap, NormalGC,
            6,6,11,52, Shape(6), Shape(6));

  /* Copy the base panel to visible */
  XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	    0,0,mysizehints.width, mysizehints.height, 0 ,0);

  /* Remove the 5 base colors from visible */
  XCopyArea(dpy, visible.pixmap, visible.pixmap, NormalGC,
	    Shape(19),Shape(6),11,52, Shape(6), Shape(6));  

  /* initial scale */
  scale = 1;
  maxload = 0.0;

  /* initial values */
  memset(loads,0,SIZE*sizeof(float));
  loads_2 = 0.0;
  loads_3 = 0.0;

#ifndef linux
#ifndef SVR4
    if (gethostname(hostname, MAXHOSTNAMELEN) != 0) {
        perror("gethostname");
	exit(10);
    }
#else
    if (sysinfo(SI_HOSTNAME, hostname, MAXHOSTNAMELEN) < 0) {
	perror("sysinfo(SI_HOSTNAME)");
	exit(10);
    }
#endif
#endif
}

void InsertLoad()
{
   int i,val,act;

   /* 
    * check out if we are deleting the curent max
    */

   if (loads[0]>=maxload || loads_2>=maxload || loads_3>=maxload)
   {
      maxload = 0.0;
      
      for(i=1;i<SIZE;i++)
	 if (maxload < loads[i]) maxload = loads[i];
   }

   /*
    * get a new value
    */

   memmove(loads,loads+1,(SIZE-1)*sizeof(float));   

   GetLoad(loads+SIZE-1, &loads_2, &loads_3);

   if (maxload < max3(loads[SIZE-1],loads_2,loads_3))
      maxload = max3(loads[SIZE-1],loads_2,loads_3); 

   /*
    * check the scale
    */

  if ((int)maxload+1 != scale) Change_Scale((int)maxload+1);

  /* Move the area */
  XCopyArea(dpy, visible.pixmap, visible.pixmap, NormalGC,
	    Shape(13), Shape(6), 45, 52, Shape(12), Shape(6));

  /* draw the Free Time */

  if(_(loads[SIZE-1]) < 52)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(19), Shape(6), 1, 52-_(loads[SIZE-1]),
	       Shape(57), Shape(6)); 

  /* draw the small value */

  if(_(loads[SIZE-1]) > 0)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(6), Shape(6), 1, _(loads[SIZE-1]),
	       Shape(57), Shape(58-_(loads[SIZE-1])));


  /* draw the medium bar */

  if(_(loads_2) < 52)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(19), Shape(6), 2, 52-_(loads_2),
	       Shape(9), Shape(6)); 
  if(_(loads_2) > 0)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(7), Shape(6), 2, _(loads_2),
	       Shape(9), Shape(58-_(loads_2)));

  /* draw the large bar */

  if(_(loads_3) < 52)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(19), Shape(6), 2, 52-_(loads_3),
	       Shape(6), Shape(6)); 
  if(_(loads_3) > 0)
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(9), Shape(6), 2, _(loads_3),
	       Shape(6), Shape(58-_(loads_3)));

  /* draw the scale for the bars */

  for(i=1;i<scale;i++)
  {
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(11), Shape(6), 1, 1,
	       Shape(57), Shape(58-_(i)));
     XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	       Shape(11), Shape(6), 6, 1,
	       Shape(6), Shape(58-_(i)));
  }
}

/* resizes the current displayed load to fit the new scale */
void Change_Scale(int new_scale)
{
   int i,j;
   
   scale = new_scale;

   /* clear the bars */

   XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
	     Shape(19), Shape(6), 6, 52,
	     Shape(6), Shape(6)); 

   for(i=0;i<SIZE;i++)
   {
      /* draw the Free Time */

      if(_(loads[i]) < 52)
	 XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
		   Shape(19), Shape(6), 1, 52-_(loads[i]),
		   Shape(i+12), Shape(6)); 

      /* draw the small value */

      if(_(loads[i]) > 0)
	 XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
		   Shape(6), Shape(6), 1, _(loads[i]),
		   Shape(i+12), Shape(58-_(loads[i])));

      /* draw the scale */

      for(j=1;j<scale;j++)
	 XCopyArea(dpy, wmavgload.pixmap, visible.pixmap, NormalGC,
		   Shape(11), Shape(6), 1, 1,
		   Shape(i+12), Shape(58-_(j)));
   }
}
