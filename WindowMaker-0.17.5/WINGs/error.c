/*
 *  WindowMaker miscelaneous function library
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "../src/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>


#define MAXLINE	1024

extern char *ProgName;

/**************************************************************************
 * Prints a fatal error message with variable arguments and terminates
 * 
 * msg - message to print with optional formatting
 * ... - arguments to use on formatting 
 **************************************************************************/
void 
wfatal(const char *msg, ...)
{
    va_list args;
    char buf[MAXLINE];

    va_start(args, msg);

    vsprintf(buf, msg, args);
    strcat(buf,"\n");
    fflush(stdout);
    fputs(ProgName, stderr);
    fputs(" fatal error: ",stderr);
    fputs(buf, stderr);
    fflush(NULL);

    va_end(args);
}


/*********************************************************************
 * Prints a warning message with variable arguments 
 * 
 * msg - message to print with optional formatting
 * ... - arguments to use on formatting
 *********************************************************************/
void 
wwarning(const char *msg, ...)
{
    va_list args;
    char buf[MAXLINE];
    
    va_start(args, msg);
    
    vsprintf(buf, msg, args);
    strcat(buf,"\n");
    fflush(stdout);
    fputs(ProgName, stderr);
    fputs(" warning: ",stderr);
    fputs(buf, stderr);
    fflush(NULL);
    
    va_end(args);
}


/*********************************************************************
 * Prints a system error message with variable arguments 
 * 
 * msg - message to print with optional formatting
 * ... - arguments to use on formatting
 *********************************************************************/
void 
wsyserror(const char *msg, ...)
{
    va_list args;
    char buf[MAXLINE];
#ifdef HAVE_STRERROR
    int error=errno;
#endif
    va_start(args, msg);
    vsprintf(buf, msg, args);
    fflush(stdout);
    fputs(ProgName, stderr);
    fputs(" error: ", stderr);
    strcat(buf, ": ");
#ifdef HAVE_STRERROR
    strcat(buf, strerror(error));
    strcat(buf,"\n");
    fputs(buf, stderr);
    fflush(NULL);
#else
    perror(buf);
#endif
    va_end(args);
}

