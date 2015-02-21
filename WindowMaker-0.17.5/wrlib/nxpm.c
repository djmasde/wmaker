/* nxpm.c - load "normalized" XPM image
 * 
 *  Raster graphics library
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
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

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
#   pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif


#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wraster.h"


/*
 * Restricted support for XPM images.
 * 
 * The images must be in the following "normalized" format:
 *
 *
 * line   	content
 * 1	        signature comment
 * 2		ignored ( normally "static char *xpm[] = {" )
 * 3		"width height color_count chars" where chars is 1 or 2
 * 4	        color definitions. Only c values with #rrggbb or #rrrrggggbbb
 *			format OR None
 * n		data
 *
 * - no comments or blank lines are allowed, except for the signature
 * - all lines must have at most 256 characters
 * - no white spaces allowed at left of each line
 */

#define LINEWIDTH 64

#ifndef USE_XPM


RImage*
RGetImageFromXPMData(RContext *context, char **data)
{
    RImage *image = NULL;
    unsigned char *color_table[4];
    unsigned short *symbol_table;
    unsigned char *r, *g, *b, *a;
    int i, j, k, line = 0;
    int transp;
    unsigned short color;
    int bsize;
    int w, h, ccount, csize;

    if (sscanf(data[line++], "%i %i %i %i", &w, &h, &ccount, &csize)!=4
	|| w <= 0 || h <= 0 || ccount <= 0 || csize <= 0)
	goto bad_format;
    
    if (csize!=1 && csize!=2) 
	goto bad_format;

    color_table[0] = alloca(ccount);
    color_table[1] = alloca(ccount);
    color_table[2] = alloca(ccount);
    color_table[3] = alloca(ccount);
    symbol_table = alloca(ccount * sizeof(unsigned short));
    
    bsize = csize * w + 16;

    if (!color_table[0] || !color_table[1] || !color_table[2] ||
	!color_table[3] || !symbol_table || !bsize) {
	sprintf(RErrorString, "out of memory while converting XPM data");
	alloca(0);
	return NULL;
    }

    transp = 0;
    /* get color table */
    for (i=0; i<ccount; i++) {
	symbol_table[i] = data[line][0];
	if (csize==2)
	    symbol_table[i] |= data[line][1]<<8;

	j = csize;
	while (data[line][j]!='#' && data[line][j]!=0
	       && data[line][j]!='N') j++;

	if (data[line][j]=='#') {
	    unsigned int red, green, blue;
	    
	    k = 0;
	    j++;
	    while (data[line][j+k]!=0) k++;
	    if (k==6) {
		if (sscanf(&(data[line][j]), "%2x%2x%2x", &red, &green, &blue)!=3)
		    goto bad_format;
	    } else if (k==12) {		
		if (sscanf(&(data[line][j]), "%4x%4x%4x", &red, &green, &blue)!=3)
		    goto bad_format;
		red >>= 8;
		green >>= 8;
		blue >>= 8;
	    } else
		goto bad_format;
	    
	    color_table[0][i] = red;
	    color_table[1][i] = green;
	    color_table[2][i] = blue;
	    color_table[3][i] = 255;
	} else if (strncmp(&(data[line][j]), "None", 4)==0) {
	    color_table[3][i] = 0;
	    transp = 1;
	} else {
	    goto bad_format;
	}
	line++;
    }

    image = RCreateImage(w, h, transp);
    if (!image) {
	sprintf(RErrorString, "out of memory while converting XPM data");
	alloca(0);
	return NULL;
    }

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];

    for (i=0; i<h; i++) {
	if (csize==1) {
	    for (j=0; j<w; j++) {
		color = data[line][j];
		
		for (k=0; k<ccount; k++) {
		    if (symbol_table[k] == color)
			break;
		}
		if (k==ccount)
		    k = 0;
		
		*(r++) = color_table[0][k];
		*(g++) = color_table[1][k];
		*(b++) = color_table[2][k];
		if (a)
		    *(a++) = color_table[3][k];
	    }
	} else {
	    for (j=0; j<w*2; j++) {
		color = data[line][j++];
		color |= data[line][j];
		
		for (k=0; k<ccount; k++) {
		    if (symbol_table[k] == color)
			break;
		}	
		if (k==ccount)
		    k = 0;
	
		*(r++) = color_table[0][k];
		*(g++) = color_table[1][k];
		*(b++) = color_table[2][k];
		if (a)
		    *(a++) = color_table[3][k];
	    }
	}
	line++;
    }

#ifdef C_ALLOCA
    alloca(0);
#endif
    return image;

  bad_format:
    sprintf(RErrorString, "XPM data is not in the normalized format");
#ifdef C_ALLOCA
    alloca(0);
#endif
    if (image)
	RDestroyImage(image);
    return NULL;
}



RImage*
RLoadXPM(RContext *context, char *file, int index)
{
    RImage *image = NULL;
    char line[LINEWIDTH+1];
    char *buffer;
    unsigned char *color_table[4];
    unsigned short *symbol_table;
    unsigned char *r, *g, *b, *a;
    int i, j, k;
    int transp;
    unsigned short color;
    int bsize;
    int w, h, ccount, csize;
    FILE *f;
    
    f = fopen(file, "r");
    if (!f) {
	sprintf(RErrorString, "can't open XPM file \"%s\"", file);
	return NULL;
    }
    /* sig */
    if (!fgets(line, LINEWIDTH, f))
	goto bad_file;
    /* declaration */
    if (!fgets(line, LINEWIDTH, f))
	goto bad_file;

    /* data */
    if (!fgets(line, LINEWIDTH, f))
	goto bad_file;
    
    if (line[0]=='/')
	if (!fgets(line, LINEWIDTH, f))
	    goto bad_file;

    if (sscanf(line, "\"%i %i %i %i\"", &w, &h, &ccount, &csize)!=4
	|| w <= 0 || h <= 0 || ccount <= 0 || csize <= 0)
	goto bad_file;
    
    if (csize!=1 && csize!=2) 
	goto bad_format;

    color_table[0] = alloca(ccount);
    color_table[1] = alloca(ccount);
    color_table[2] = alloca(ccount);
    color_table[3] = alloca(ccount);
    symbol_table = alloca(ccount * sizeof(unsigned short));
    
    bsize = csize * w + 16;
    buffer = alloca(bsize);

    if (!color_table[0] || !color_table[1] || !color_table[2] ||
	!color_table[3] || !symbol_table || !bsize) {
	sprintf(RErrorString, "out of memory while loading XPM file \"%s\"",
		file);
	fclose(f);
	alloca(0);
	return NULL;
    }

    transp = 0;
    /* get color table */
    for (i=0; i<ccount; i++) {
	if (!fgets(line, LINEWIDTH, f)) 
	    goto bad_file;
	if (line[0]=='/')
	    if (!fgets(line, LINEWIDTH, f))
		goto bad_file;

	symbol_table[i] = line[1];
	if (csize==2)
	    symbol_table[i] |= line[2]<<8;

	j = csize+1;
	while (line[j]!='#' && line[j]!='"' && line[j]!=0 && line[j]!='N') j++;

	if (line[j]=='#') {
	    unsigned int red, green, blue;
	    
	    k = 0;
	    j++;
	    while (line[j+k]!='"' && line[j+k]!=0) k++;
	    if (k==6) {
		if (sscanf(&(line[j]), "%2x%2x%2x", &red, &green, &blue)!=3)
		    goto bad_format;
	    } else if (k==12) {		
		if (sscanf(&(line[j]), "%4x%4x%4x", &red, &green, &blue)!=3)
		    goto bad_format;
		red >>= 8;
		green >>= 8;
		blue >>= 8;
	    } else
		goto bad_format;
	    
	    color_table[0][i] = red;
	    color_table[1][i] = green;
	    color_table[2][i] = blue;
	    color_table[3][i] = 255;
	} else if (strncmp(&(line[j]), "None", 4)==0) {
	    color_table[3][i] = 0;
	    transp = 1;
	} else {
	    goto bad_format;
	}
    }

    image = RCreateImage(w, h, transp);
    if (!image) {
	sprintf(RErrorString, "out of memory while loading XPM file \"%s\"",
		file);
	fclose(f);
	alloca(0);
	return NULL;
    }

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    a = image->data[3];

    for (i=0; i<h; i++) {
	if (!fgets(buffer, bsize, f))
	    goto bad_file;
	if (buffer[0]=='/')
	    if (!fgets(buffer, bsize, f))
		goto bad_file;
	
	if (csize==1) {
	    for (j=1; j<=w; j++) {
		color = buffer[j];
		
		for (k=0; k<ccount; k++) {
		    if (symbol_table[k] == color)
			break;
		}
		if (k==ccount)
		    k = 0;
		
		*(r++) = color_table[0][k];
		*(g++) = color_table[1][k];
		*(b++) = color_table[2][k];
		if (a)
		    *(a++) = color_table[3][k];
	    }
	} else {
	    for (j=1; j<=w*2; j++) {
		color = buffer[j++];
		color |= buffer[j] << 8;
		
		for (k=0; k<ccount; k++) {
		    if (symbol_table[k] == color)
			break;
		}
		if (k==ccount) {
		    k = 0;
		}
	
		*(r++) = color_table[0][k];
		*(g++) = color_table[1][k];
		*(b++) = color_table[2][k];
		if (a)
		    *(a++) = color_table[3][k];
	    }
	}
    }

    fclose(f);
#ifdef C_ALLOCA
    alloca(0);
#endif
    return image;

  bad_format:
    sprintf(RErrorString, "XPM file \"%s\" is not in the normalized format", file);
    fclose(f);
#ifdef C_ALLOCA
    alloca(0);
#endif
    if (image)
	RDestroyImage(image);
    return NULL;
    
  bad_file:
    sprintf(RErrorString, "bad XPM file \"%s\"", file);
    fclose(f);
#ifdef C_ALLOCA
    alloca(0);
#endif
    if (image)
	RDestroyImage(image);
    return NULL;
}

#endif
