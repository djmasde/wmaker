/* save.c - save image to file
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 1998 Alfredo K. Kojima
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *  
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>


#include "wraster.h"


extern Bool RSaveXPM(RImage *image, char *filename);


Bool
RSaveImage(RImage *image, char *filename, char *format)
{
    if (strcmp(format, "XPM")!=0) {
	RErrorCode = RERR_BADFORMAT;
	return False;
    }
    return RSaveXPM(image, filename);
}

