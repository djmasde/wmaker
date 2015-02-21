/*
	Code based on wmppp/wmifs

	[Orig WMMON comments]

	This code was mainly put together by looking at the
	following programs:

	asclock
		A neat piece of equip, used to display the date
		and time on the screen.
		Comes with every AfterStep installation.

		Source used:
			How do I create a not so solid window?
			How do I open a window?
			How do I use pixmaps?
	
	------------------------------------------------------------

	Author: Martijn Pieterse (pieterse@xs4all.nl)

	This program is distributed under the GPL license.
	(as were asclock and pppstats)

	----
	Changes:
	----
	09/12/1998 (Ronny Haryanto, giant@canada.com)
		* Added internet time (beats) support (see www.swatch.com for info),
		  code mostly borrowed from asclock-itime hack by Karsten Schulz (kaschu@t800.ping.de)
		  http://www.Linux-Systemhaus.de/download/index.html
		* still the same -digital bug, but it's getting even uglier now.. :( 
		* Changed default color to yellow, it just happens that Ronny likes
		  yellow better :)
	17/05/1998 (Antoine Nulle, warp@xs4all.nl)
		* Updated version number and some other minor stuff
	16/05/1998 (Antoine Nulle, warp@xs4all.nl)
		* Added Locale support, based on original diff supplied
		  by Alen Salamun (snowman@hal9000.medinet.si)  
	04/05/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Moved the hands one pixel down.
		* Removed the RedrawWindow out of the main loop
	02/05/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Removed a lot of code that was in the wmgeneral dir.
	02/05/1998 (Antoine Nulle, warp@xs4all.nl)
		* Updated master-xpm, hour dots where a bit 'off'
	30/04/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Added anti-aliased hands
	23/04/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Changed the hand lengths.. again! ;)
		* Zombies were created, so added wait code
	21/04/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Added digital/analog switching support
	18/04/1998 (Martijn Pieterse, pieterse@xs4all.nl)
		* Started this project. 
		* Copied the source from wmmon.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include "wmgeneral.h"
#include "misc.h"

#include "wmtime-master.xpm"
#include "wmtime-mask.xbm"

  /***********/
 /* Defines */
/***********/

#define LEFT_ACTION (NULL)
#define MIDDLE_ACTION (NULL)
#define RIGHT_ACTION (NULL)

#define WMMON_VERSION "1.0b2i1"

  /********************/
 /* Global Variables */
/********************/

char	*ProgName;
int		digital = 0;
char	day_of_week[7][3] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };
char	mon_of_year[12][4] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOW", "DEC" };

/* functions */
void usage(void);
void printversion(void);

void wmtime_routine(int, char **);
void get_lang();

void main(int argc, char *argv[]) {

	int		i;
	

	/* Parse Command Line */

	ProgName = argv[0];
	if (strlen(ProgName) >= 6)
		ProgName += (strlen(ProgName) - 6);
	
	for (i=1; i<argc; i++) {
		char *arg = argv[i];

		if (*arg=='-') {
			switch (arg[1]) {
			case 'd' :
				if (strcmp(arg+1, "display") && strcmp(arg+1, "digital")) {
					usage();
					exit(1);
				}
				if (!strcmp(arg+1, "digital"))
					digital = 1;
				break;
			case 'v' :
				printversion();
				exit(0);
				break;
			default:
				usage();
				exit(0);
				break;
			}
		}
	}
	get_lang();
	wmtime_routine(argc, argv);
}

/************/
/* get_lang */
/************/
void get_lang(){
   FILE *fp;
   int i;
   char temp[5];
   
   fp=fopen("language","r");
   if (fp) {
       for (i=0;i<7;i++){
	   fgets(temp,4,fp);
	   strncpy(day_of_week[i],temp,2);
       };
       for (i=0;i<12;i++){
	   fgets(temp,5,fp);
	   strncpy(mon_of_year[i],temp,3);
       };
   	fclose(fp);
    };
}

/*******************************************************************************\
|* wmtime_routine															   *|
\*******************************************************************************/

char		*left_action = NULL;
char		*right_action = NULL;
char		*middle_action = NULL;

void DrawTime(int, int, int);
void DrawInetTime(int);		/* InetTime hack */
void DrawWijzer(int, int, int);
void DrawDate(int, int, int);

int InetTime(struct tm *);		/* InetTime hack */

void wmtime_routine(int argc, char **argv) {

	rckeys		wmtime_keys[] = {
		{ "left", &left_action },
		{ "right", &right_action },
		{ "middle", &middle_action },
		{ NULL, NULL }
	};

	int			i;
	XEvent		Event;
	int			but_stat = -1;

	struct tm	*time_struct;
	struct tm	old_time_struct;

	long		starttime;
	long		curtime;
	long		nexttime;

	char		temp[128];
	char		*p;

	/* Scan through ~/.wmtimerc for the mouse button actions. */
	if (LEFT_ACTION) left_action = strdup(LEFT_ACTION);
	if (MIDDLE_ACTION) middle_action = strdup(MIDDLE_ACTION);
	if (RIGHT_ACTION) right_action = strdup(RIGHT_ACTION);

	/* Scan throught  the .rc files */
	strcpy(temp, "/etc/wmtimerc");
	parse_rcfile(temp, wmtime_keys);

	p = getenv("HOME");
	strcpy(temp, p);
	strcat(temp, "/.wmtimerc");
	parse_rcfile(temp, wmtime_keys);

	strcpy(temp, "/etc/wmtimerc.fixed");
	parse_rcfile(temp, wmtime_keys);


	openXwindow(argc, argv, wmtime_master_xpm, wmtime_mask_bits, 128, 64);

	copyXPMArea(0, 0, 128, 64, 0, 98);
	if (digital) {
		copyXPMArea(64, 0, 64, 64, 0, 0);
		setMaskXY(-64, 0);
	} else {
		copyXPMArea(0, 0, 64, 64, 64, 0);
		setMaskXY(0, 0);
	}

	/* add mouse region */
	AddMouseRegion(0, 5, 48, 58, 60);
	AddMouseRegion(1, 5, 5, 58, 46);

	starttime = time(0);
	nexttime = starttime + 1;

	curtime = time(0);
	time_struct = localtime(&curtime);

	while (1) {
		curtime = time(0);

		waitpid(0, NULL, WNOHANG);
		
		old_time_struct = *time_struct;
		time_struct = localtime(&curtime);


		if (curtime >= starttime) {
			if (!digital) {
				/* Now to update the seconds */

				DrawWijzer(time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);

				DrawDate(time_struct->tm_wday, time_struct->tm_mday, time_struct->tm_mon);

			} else {

				DrawTime(time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
				
				DrawInetTime(InetTime(time_struct)); 		/* InetTime hack */

				DrawDate(time_struct->tm_wday, time_struct->tm_mday, time_struct->tm_mon);
			}
			RedrawWindow();
		}
		

		while (XPending(display)) {
			XNextEvent(display, &Event);
			switch (Event.type) {
			case Expose:
				RedrawWindow();
				break;
			case DestroyNotify:
				XCloseDisplay(display);
				exit(0);
				break;
			case ButtonPress:
				but_stat = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);
				break;
			case ButtonRelease:
				i = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);
				if (but_stat == i && but_stat >= 0) {
					switch (but_stat) {
					case 0:
						digital = 1-digital;

						if (digital) {
							copyXPMArea(64, 98, 64, 64, 0, 0);
							setMaskXY(-64, 0);
							DrawTime(time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
							DrawInetTime(InetTime(time_struct)); 		/* InetTime hack */
							DrawDate(time_struct->tm_wday, time_struct->tm_mday, time_struct->tm_mon);
						} else {
							copyXPMArea(0, 98, 64, 64, 0, 0);
							setMaskXY(0, 0);
							DrawWijzer(time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
							DrawDate(time_struct->tm_wday, time_struct->tm_mday, time_struct->tm_mon);
						}
						RedrawWindow();
						break;
					case 1:
						switch (Event.xbutton.button) {
						case 1:
							if (left_action)
								execCommand(left_action);
							break;
						case 2:
							if (middle_action)
								execCommand(middle_action);
							break;
						case 3:
							if (right_action)
								execCommand(right_action);
							break;
						}
					}
				}
				break;
			}
		}

		/* Sleep 0.3 seconds */
		usleep(300000L);
	}
}

/*******************************************************************************\
|* DrawTime																	   *|
\*******************************************************************************/

void DrawTime(int hr, int min, int sec) {

	
	char	temp[16];
	char	*p = temp;
	int		i,j,k=6;

	/* 7x13 */

	sprintf(temp, "%02d:%02d:%02d ", hr, min, sec);

	for (i=0; i<3; i++) {
		for (j=0; j<2; j++) {
			copyXPMArea((*p-'0')*7 + 1, 84, 8, 13, k, 6);
			k += 7;
			p++;
		}
		if (*p == ':') {
			copyXPMArea(71, 84, 5, 13, k, 6);
			k += 4;
			p++;
		}
	}
}

/*******************************************************************************\
|* DrawInetTime					InetTime hack								   *|
\*******************************************************************************/

void DrawInetTime(int itime) {

	char	temp[16];
	char	*p = temp;
	int		i,k=35;

	/* 7x13 */

	sprintf(temp, "%03d ", itime);

	for (i=0; i<3; i++) {
		copyXPMArea((*p-'0')*7 + 1, 84, 8, 13, k, 28);
		k += 7;
		p++;
	}
}

/*******************************************************************************\
|* DrawDate																	   *|
\*******************************************************************************/

void DrawDate(int wkday, int dom, int month) {

	
	char	temp[16];
	char	*p = temp;
	int		i,k;

	/* 7x13 */

	sprintf(temp, "%.2s%02d%.3s  ", day_of_week[wkday], dom, mon_of_year[month]);

	k = 5;
	for (i=0; i<2; i++) {
		copyXPMArea((*p-'A')*6, 74, 6, 9, k, 49);
		k += 6;
		p++;
	}
	k = 23;
	for (i=0; i<2; i++) {
		copyXPMArea((*p-'0')*6, 64, 6, 9, k, 49);
		k += 6;
		p++;
	}
	copyXPMArea(61, 64, 4, 9, k, 49);
	k += 4;
	for (i=0; i<3; i++) {
		copyXPMArea((*p-'A')*6, 74, 6, 9, k, 49);
		k += 6;
		p++;
	}
}

/*******************************************************************************\
|* DrawWijzer																   *|
\*******************************************************************************/

void DrawWijzer(int hr, int min, int sec) {

	double		psi;
	int			dx,dy;
	int			x,y;
	int			ddx,ddy;
	int			adder;
	int			k;

	int			i;
	
	hr %= 12;

	copyXPMArea(5+64, 5, 54, 40, 5, 5);

	/**********************************************************************/
	psi = hr * (M_PI / 6.0);
	psi += min * (M_PI / 360);

	dx = floor(sin(psi) * 22 * 0.7 + 0.5);
	dy = floor(-cos(psi) * 16 * 0.7 + 0.5);

	// dx, dy is het punt waar we naar toe moeten.
	// Zoek alle punten die ECHT op de lijn liggen:
	
	ddx = 1;
	ddy = 1;
	if (dx < 0) ddx = -1;
	if (dy < 0) ddy = -1;

	x = 0;
	y = 0;

	if (abs(dx) > abs(dy)) {
		if (dy != 0) 
			adder = abs(dx) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dx); i++) {
			// laat de kleur afhangen van de adder.
			// adder loopt van abs(dx) tot 0

			k = 12 - adder / (abs(dx) / 12.0);
			copyXPMArea(79+k, 67, 1, 1, x + 31, y + 24 - ddy);

			copyXPMArea(79, 67, 1, 1, x + 31, y + 24);

			k = 12-k;
			copyXPMArea(79+k, 67, 1, 1, x + 31, y + 24 + ddy);
				

			x += ddx;

			adder -= abs(dy);
			if (adder < 0) {
				adder += abs(dx);
				y += ddy;
			}
		}
	} else {
		if (dx != 0)
			adder = abs(dy) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dy); i++) {
			k = 12 - adder / (abs(dy) / 12.0);
			copyXPMArea(79+k, 67, 1, 1, x + 31 - ddx, y + 24);

			copyXPMArea(79, 67, 1, 1, x + 31, y + 24);

			k = 12-k;
			copyXPMArea(79+k, 67, 1, 1, x + 31 + ddx, y + 24);
				
			y += ddy;

			adder -= abs(dx);
			if (adder < 0) {
				adder += abs(dy);
				x += ddx;
			}
		}
	}
	/**********************************************************************/
	psi = min * (M_PI / 30.0);
	psi += sec * (M_PI / 1800);

	dx = floor(sin(psi) * 22 * 0.55 + 0.5);
	dy = floor(-cos(psi) * 16 * 0.55 + 0.5);

	// dx, dy is het punt waar we naar toe moeten.
	// Zoek alle punten die ECHT op de lijn liggen:
	
	dx += dx;
	dy += dy;
	
	ddx = 1;
	ddy = 1;
	if (dx < 0) ddx = -1;
	if (dy < 0) ddy = -1;

	x = 0;
	y = 0;

	if (abs(dx) > abs(dy)) {
		if (dy != 0) 
			adder = abs(dx) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dx); i++) {
			// laat de kleur afhangen van de adder.
			// adder loopt van abs(dx) tot 0

			k = 12 - adder / (abs(dx) / 12.0);
			copyXPMArea(79+k, 67, 1, 1, x + 31, y + 24 - ddy);

			copyXPMArea(79, 67, 1, 1, x + 31, y + 24);

			k = 12-k;
			copyXPMArea(79+k, 67, 1, 1, x + 31, y + 24 + ddy);
				

			x += ddx;

			adder -= abs(dy);
			if (adder < 0) {
				adder += abs(dx);
				y += ddy;
			}
		}
	} else {
		if (dx != 0)
			adder = abs(dy) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dy); i++) {
			k = 12 - adder / (abs(dy) / 12.0);
			copyXPMArea(79+k, 67, 1, 1, x + 31 - ddx, y + 24);

			copyXPMArea(79, 67, 1, 1, x + 31, y + 24);

			k = 12-k;
			copyXPMArea(79+k, 67, 1, 1, x + 31 + ddx, y + 24);
				
			y += ddy;

			adder -= abs(dx);
			if (adder < 0) {
				adder += abs(dy);
				x += ddx;
			}
		}
	}
	/**********************************************************************/
	psi = sec * (M_PI / 30.0);

	dx = floor(sin(psi) * 22 * 0.9 + 0.5);
	dy = floor(-cos(psi) * 16 * 0.9 + 0.5);

	// dx, dy is het punt waar we naar toe moeten.
	// Zoek alle punten die ECHT op de lijn liggen:
	
	ddx = 1;
	ddy = 1;
	if (dx < 0) ddx = -1;
	if (dy < 0) ddy = -1;
	
	if (dx == 0) ddx = 0;
	if (dy == 0) ddy = 0;

	x = 0;
	y = 0;


	if (abs(dx) > abs(dy)) {
		if (dy != 0) 
			adder = abs(dx) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dx); i++) {
			// laat de kleur afhangen van de adder.
			// adder loopt van abs(dx) tot 0

			k = 12 - adder / (abs(dx) / 12.0);
			copyXPMArea(79+k, 70, 1, 1, x + 31, y + 24 - ddy);

			k = 12-k;
			copyXPMArea(79+k, 70, 1, 1, x + 31, y + 24);
				

			x += ddx;

			adder -= abs(dy);
			if (adder < 0) {
				adder += abs(dx);
				y += ddy;
			}
		}
	} else {
		if (dx != 0)
			adder = abs(dy) / 2;
		else 
			adder = 0;
		for (i=0; i<abs(dy); i++) {
			k = 12 - adder / (abs(dy) / 12.0);
			copyXPMArea(79+k, 70, 1, 1, x + 31 - ddx, y + 24);

			k = 12-k;
			copyXPMArea(79+k, 70, 1, 1, x + 31, y + 24);
				
			y += ddy;

			adder -= abs(dx);
			if (adder < 0) {
				adder += abs(dy);
				x += ddx;
			}
		}
	}
}

/*******************************************************************************\
|* usage																	   *|
\*******************************************************************************/

void usage(void) {

	fprintf(stderr, "\nwmtime - programming: tijno, (de)bugging & design: warp, web hosting: bobby\n\n");
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t-digital\tdigital clock\n");
	fprintf(stderr, "\t-display <display name>\n");
	fprintf(stderr, "\t-h\tthis screen\n");
	fprintf(stderr, "\t-v\tprint the version number\n");
	fprintf(stderr, "\n");
}

/*******************************************************************************\
|* printversion																   *|
\*******************************************************************************/

void printversion(void) {

	if (!strcmp(ProgName, "wmtime")) {
		fprintf(stderr, "%s\n", WMMON_VERSION);
	}
}

/*******************************************************************************\
|* InetTime		InetTime Hack												   *|
|*	code borrowed from Karsten Schulz's asclock-itime                          *|
\*******************************************************************************/

int InetTime(struct tm *clk)
{
  /* calculate Internet time; BMT (Biel Mean Time) is UTC+1 */
  long iTime=(clk->tm_hour*3600+clk->tm_min*60+clk->tm_sec);
  iTime=iTime+timezone+3600;
  if (clk->tm_isdst)
  	iTime-=3600;
  iTime=(iTime*1000)/86400;

  if (iTime >= 1000) 
    iTime-=1000;
  else
    if (iTime < 0)
	  iTime+=1000;
	  
  return (int)iTime;
}
