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

#include "../wmgeneral/wmgeneral.h"
#include "../wmgeneral/misc.h"

#include "wmtimer.xpm"

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define VERSION 061499

  /********************/
 /* Global Variables */
/********************/

void get_lang();
void BlitString(char *name, int x, int y);
void BlitNum(int num, int x, int y);
void DrawTime(int, int, int);
void DecrementTimer();
void ExecAct();
void usage();

char *ProgName;
char *action;
char wminet_mask_bits[64*64];
int  wminet_mask_width = 64;
int  wminet_mask_height = 64;
int  mode = 0;			// 0 = alarm @ time	1 = countdown timer
int  actdef = 0; 		// 0 = default/bell	1 = command
int  ihr, imin, isec;
int  oldsec = 0;

int main(int argc, char *argv[]) {
	int		i;
	XEvent		Event;
	int	        buttonStatus=-1;
	long		starttime;
	long		curtime;
	long		nexttime;
	struct tm	*time_struct;
	struct tm	old_time_struct;


/* Parse Command Line */

for (i=1; i<argc; i++) {
  char *arg = argv[i];

  if (*arg=='-') {
    switch (arg[1]) {
      case 'a' :
	mode=0;
	break;
      case 'c' :
	mode=1;
	break;
      case 'e' :
	action = argv[i+1]; 
	actdef = 1;
	break;
      case 't' :
	ihr = atoi(strtok(argv[i+1], ":"));
	imin = atoi(strtok(NULL, ":"));
	isec = atoi(strtok(NULL, ":"));
	//if (strcmp(arg+1, "")) {
	break;
      case 'v' :
	fprintf(stderr, "Version: 060199\n");
	exit(0);
	break;
      default:
	usage();
	exit(0);
	break;
    }
  }
  else{
    if (!(strcmp(arg, ":"))) {
      usage();
    }
  }
}

        createXBMfromXPM(wminet_mask_bits, wmtimer_xpm
               , wminet_mask_width, wminet_mask_height);

       openXwindow(argc, argv, wmtimer_xpm, wminet_mask_bits
               , wminet_mask_width, wminet_mask_height);

	//setMaskXY(-64, 0);

        AddMouseRegion(0, 18, 49, 45, 59 ); /* middle button */
        AddMouseRegion(1, 5 , 49, 17, 59 ); /* left button   */
        AddMouseRegion(2, 46, 49, 59, 59 ); /* right button  */
//        AddMouseRegion(3, 2, 2, 58, 33);    /* main area     */
        AddMouseRegion(3, 6, 2, 60, 18);   /*first bar      */
        AddMouseRegion(4, 6, 20, 60, 34);   /*first bar      */
        AddMouseRegion(5, 6, 37, 60, 48);   /*third bar      */





if(mode==0)
  BlitString("Alarm:", 13, 21);
else
  BlitString("Timer:", 13, 21);

BlitNum(ihr, 7, 36);
BlitString(":", 20, 36);
BlitNum(imin, 25, 36);
BlitString(":", 38, 36);
BlitNum(isec, 43, 36);


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
			DrawTime(time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);
			RedrawWindow();
//		}





RedrawWindow();
        // X Events
        while (XPending(display)){
                        XNextEvent(display, &Event);
            switch (Event.type)
            {
                        case Expose:
                                RedrawWindow();
                                break;                                                                  case DestroyNotify:
                                XCloseDisplay(display);                                                         exit(0);
                		break;   
                        case ButtonPress:                                                                       i = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y); 
                                buttonStatus = i;
		                if (buttonStatus == i && buttonStatus >= 0){
               			     switch (buttonStatus){
		                        case 0 :
					     break;
					case 1 :
		                             break;
                    			case 2:
                            		     break;
		                        case 3:
                		            //execCommand(mailclient);
                            		    break;
		                        case 4:
					    break;
                                        }
                                }

                                break;
                        case ButtonRelease:
                                i = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);

                if (buttonStatus == i && buttonStatus >= 0){
                    switch (buttonStatus){
                        case 0 :
                            break;
			case 1 :
//		        BlitNum(hr, 7, 36); 		
                             break;
                        case 2:
                            break;
                        case 3:
                            break;
                        case 4:
                            break;
                        case 5:
                            break;
                                        }
                                }
                                buttonStatus = -1;
                                RedrawWindow();
                                break;
                        }
                }
                usleep(100000L);
        }
}
return 0;
}

  /********************/
 /* DrawTime	     */
/********************/

void DrawTime(int hr, int min, int sec) {
  if(mode==1){
    if(oldsec < sec){
      DecrementTimer();
    }
    oldsec = sec;
  }
  else{
    if(ihr==hr && imin==min && isec==sec){
      ExecAct();
    }
  }

        BlitNum(hr, 7, 5);   		//top
        BlitString(":", 20, 5);
        BlitNum(min, 25, 5);
        BlitString(":", 38, 5);
        BlitNum(sec, 43, 5);
//        BlitNum(hr, 7, 21);		//middle
//        BlitNum(hr, 7, 36); 		//bottom






/*	sprintf(temp, "%02d:%02d:%02d ", hr, min, sec);

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
*/
}



// Blits a string at given co-ordinates
void BlitString(char *name, int x, int y)
{
    int         i;
    int         c;
    int         k;

        k = x;
    for (i=0; name[i]; i++)
    {
        c = toupper(name[i]);
        if (c >= 'A' && c <= 'Z')
        {   // its a letter
                        c -= 'A';
                        copyXPMArea(c * 6, 74, 6, 8, k, y);
                        k += 6;
        }
        else
        {   // its a number or symbol
                        c -= '0';
                        copyXPMArea(c * 6, 64, 6, 8, k, y);
                        k += 6;
                }
       }

}

void BlitNum(int num, int x, int y)
{
    char buf[1024];
    int newx=x;

    if (num > 99)
    {
        newx -= CHAR_WIDTH;
    }

    if (num > 999)
    {
        newx -= CHAR_WIDTH;
    }

    sprintf(buf, "%02i", num);
    BlitString(buf, newx, y);
}

void DecrementTimer(){

  isec--;
  if(isec==-1){
    isec=59;
    imin--;
    if(imin==-1){
      imin=59;
      ihr--;
    }
  }

  BlitNum(ihr, 7, 36);
  BlitString(":", 20, 36);
  BlitNum(imin, 25, 36);
  BlitString(":", 38, 36);
  BlitNum(isec, 43, 36);

  if(isec==0 && imin==0 && ihr==0){
    ExecAct();
  }
}

void ExecAct(){
  if(actdef){
    execCommand(action);
  }
  else{
    execCommand("bell");
    //printf("\07");
  }
  exit(0);
}

void usage(void)
{
fprintf(stderr, "\nwmtimer - Josh King <jking@dwave.net>\n\n");
fprintf(stderr, "usage: -[a|c] -t hh:mm:ss\n\n");
fprintf(stderr, "    -a		alarm mode, wmtimer will beep/exec command\n");
fprintf(stderr, "    		  at specified time\n");
fprintf(stderr, "    -c		countdowntimer mode, wmtimer will beep/exec\n");
fprintf(stderr, "		  command when specified time reaches 0 \n");

fprintf(stderr, "    -e		<command> system bell is default\n");
fprintf(stderr, "    -t		hh:mm:ss\n");

fprintf(stderr, "    -h		this help screen\n");
fprintf(stderr, "    -v		print the version number\n");
fprintf(stderr, "\n");

exit(0);
}



