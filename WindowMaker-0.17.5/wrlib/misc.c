/* 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>

#include "wraster.h"

int 
RBevelImage(RImage *image, int bevel_type)
{
    register int i, ofs;
    register int c;
    unsigned char *r, *g, *b;

    if (image->width<3 || image->height<3)
      return False;
      
    if (bevel_type>0) {
	/* raised */
	
	/* top */
	r = image->data[0];
	g = image->data[1];
	b = image->data[2];
	for (i=0; i<image->width * (bevel_type==RBEV_RAISED3 ? 2 : 1); i++) {
	    c = *r;
	    *(r++) = (c<175) ? c+80 : 255;
	    
	    c = *g;
	    *(g++) = (c<175) ? c+80 : 255;
	    
	    c = *b;
	    *(b++) = (c<175) ? c+80 : 255;
	}

	/* left */
	ofs = image->width;
	r = &(image->data[0][ofs]);
	g = &(image->data[1][ofs]);
	b = &(image->data[2][ofs]);
	if (bevel_type==RBEV_RAISED3) {
	    ofs--;
	    for (i=0; i<image->height-1; i++) {
		c = *r;
		*r = (c<175) ? c+80 : 255;
		r++;
		c = *r;
		*r = (c<175) ? c+80 : 255;
		r += ofs;
		
		c = *g;
		*g = (c<175) ? c+80 : 255;
		g++;
		c = *g;
		*g = (c<175) ? c+80 : 255;
		g += ofs;
		
		c = *b;
		*b = (c<175) ? c+80 : 255;
		b++;
		c = *b;
		*b = (c<175) ? c+80 : 255;
		b += ofs;
	    }
	} else {
	    for (i=0; i<image->height-1; i++) {
		c = *r;
		*r = (c<175) ? c+80 : 255;
		r += ofs;
		
		c = *g;
		*g = (c<175) ? c+80 : 255;
		g += ofs;
		
		c = *b;
		*b = (c<175) ? c+80 : 255;
		b += ofs;
	    }
	}

	/* bottom */
	ofs = image->width;
	i = ofs*(image->height-1);   /* last line */
	r = &(image->data[0][i]);
	g = &(image->data[1][i]);
	b = &(image->data[2][i]);
	if (bevel_type==RBEV_RAISED2 || bevel_type==RBEV_RAISED3) {
	    memset(r, 0, ofs);
	    memset(g, 0, ofs);
	    memset(b, 0, ofs);
	    /* go to previous line */
	    r -= ofs; r++;
	    g -= ofs; g++;
	    b -= ofs; b++;
	    ofs--; ofs--;
	}
	for (i=0; i<ofs; i++) {
	    c = *r;
	    *(r++) = (c>40) ? c-40 : 0;
	    
	    c = *g;
	    *(g++) = (c>40) ? c-40 : 0;
	    
	    c = *b;
	    *(b++) = (c>40) ? c-40 : 0;
	}

	/* right */
	ofs = image->width;
	ofs--;
	r = &(image->data[0][ofs]);
	g = &(image->data[1][ofs]);
	b = &(image->data[2][ofs]);
	if (bevel_type==RBEV_RAISED2 || bevel_type==RBEV_RAISED3) {
	    *r = 0; *g = 0; *b = 0;
	    r--; g--; b--;
	    for (i=0; i<image->height-1; i++) {
		c = *r;
		*r = (c>40) ? c-40 : 0;
		*(++r) = 0;
		r += ofs;
		
		c = *g;
		*g = (c>40) ? c-40 : 0;
		*(++g) = 0;
		g += ofs;

		c = *b;
		*b = (c>40) ? c-40 : 0;
		*(++b) = 0;
		b += ofs;
	    }
	} else {
	    ofs++;
	    for (i=0; i<image->height-1; i++) {
		c = *r;
		*r = (c>40) ? c-40 : 0;
		r += ofs;
	    
		c = *g;
		*g = (c>40) ? c-40 : 0;
		g += ofs;

		c = *b;
		*b = (c>40) ? c-40 : 0;
		b += ofs;
	    }
	}
    } else {
	/* sunken */

	/* top */
	r = image->data[0];
	g = image->data[1];
	b = image->data[2];
	for (i=0; i<image->width; i++) {
	    c = *r;
	    *(r++) = (c>40) ? c-40 : 0;
	    
	    c = *g;
	    *(g++) = (c>40) ? c-40 : 0;
	    
	    c = *b;
	    *(b++) = (c>40) ? c-40 : 0;
	}

	/* left */
	ofs = image->width;
	r = &(image->data[0][ofs]);
	g = &(image->data[1][ofs]);
	b = &(image->data[2][ofs]);
	for (i=0; i<image->height-1; i++) {
	    c = *r;
	    *r = (c>40) ? c-40 : 0;
	    r += ofs;
	    
	    c = *g;
	    *g = (c>40) ? c-40 : 0;
	    g += ofs;

	    c = *b;
	    *b = (c>40) ? c-40 : 0;
	    b += ofs;
	}

	/* bottom */
	ofs = image->width;
	i = ofs*(image->height-1);   /* last line */
	r = &(image->data[0][i]);
	g = &(image->data[1][i]);
	b = &(image->data[2][i]);
	for (i=0; i<ofs; i++) {
	    c = *r;
	    *(r++) = (c<175) ? c+80 : 255;
	    
	    c = *g;
	    *(g++) = (c<175) ? c+80 : 255;
	    
	    c = *b;
	    *(b++) = (c<175) ? c+80 : 255;
	}

	/* right */
	ofs = image->width;
	ofs--;
	r = &(image->data[0][ofs]);
	g = &(image->data[1][ofs]);
	b = &(image->data[2][ofs]);
	ofs++;
	for (i=0; i<image->height-1; i++) {
	    c = *r;
	    *r = (c<175) ? c+80 : 255;
	    r += ofs;
	    
	    c = *g;
	    *g = (c<175) ? c+80 : 255;
	    g += ofs;

	    c = *b;
	    *b = (c<175) ? c+80 : 255;
	    b += ofs;
	}
    }
    return True;
}


int 
RClearImage(RImage *image, RColor *color)
{
    int bytes;
    
    bytes = image->width*image->height;

    if (color->alpha==255) {
	memset(image->data[0], color->red, bytes);
	memset(image->data[1], color->green, bytes);
	memset(image->data[2], color->blue, bytes);
	if (image->data[3])
	  memset(image->data[3], 0xff, bytes);
    } else {
	register int i;
	unsigned char *dr, *dg, *db;
	int alpha, nalpha, r, g, b;

	dr = image->data[0];
	dg = image->data[1];
	db = image->data[2];
    
	r = color->red;
	g = color->green;
	b = color->blue;
	alpha = color->alpha;
	nalpha = 255 - alpha;

	for (i=0; i<bytes; i++) {
	    *dr = (((int)*dr * nalpha) + (r * alpha))/256;
	    *dg = (((int)*dg * nalpha) + (g * alpha))/256;
	    *db = (((int)*db * nalpha) + (b * alpha))/256;
	    dr++;    dg++;    db++;
	}
    }

    return True;
}

