/* gradient.c - renders gradients
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <assert.h>

#include "wraster.h"


static RImage *renderHGradient(int width, int height, int r0, int g0, int b0, 
			int rf, int gf, int bf);
static RImage *renderVGradient(int width, int height, int r0, int g0, int b0, 
			int rf, int gf, int bf);
static RImage *renderDGradient(int width, int height, int r0, int g0, int b0, 
			int rf, int gf, int bf);

static RImage *renderMHGradient(int width, int height, RColor **colors,
				int count);
static RImage *renderMVGradient(int width, int height, RColor **colors,
				int count);
static RImage *renderMDGradient(int width, int height, RColor **colors,
				int count);


RImage*
RRenderMultiGradient(int width, int height, RColor **colors, int style)
{
    int count;

    count = 0;
    while (colors[count]!=NULL) count++;

    if (count > 2) {
	switch (style) {
	 case RGRD_HORIZONTAL:
	    return renderMHGradient(width, height, colors, count);
	 case RGRD_VERTICAL:
	    return renderMVGradient(width, height, colors, count);
	 case RGRD_DIAGONAL:
	    return renderMDGradient(width, height, colors, count);
	}
    } else if (count > 1) {
	return RRenderGradient(width, height, colors[0], colors[1], style);
    } else if (count > 0) {
	return RRenderGradient(width, height, colors[0], colors[0], style);
    }
    assert(0);
    return NULL;
}



RImage*
RRenderGradient(int width, int height, RColor *from, RColor *to, int style)
{
    switch (style) {
     case RGRD_HORIZONTAL:
	return renderHGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);
     case RGRD_VERTICAL:
	return renderVGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);

     case RGRD_DIAGONAL:
	return renderDGradient(width, height, from->red, from->green, 
			       from->blue, to->red, to->green, to->blue);
    }
    assert(0);
    return NULL;
}


/*
 *----------------------------------------------------------------------
 * renderHGradient--
 * 	Renders a horizontal linear gradient of the specified size in the
 * RImage format with a border of the specified type. 
 * 
 * Returns:
 * 	A 24bit RImage with the gradient (no alpha channel).
 * 
 * Side effects:
 * 	None
 *---------------------------------------------------------------------- 
 */
static RImage*
renderHGradient(int width, int height, int r0, int g0, int b0, 
		int rf, int gf, int bf)
{    
    int i;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *rp, *gp, *bp;

    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    rp = image->data[0];
    gp = image->data[1];
    bp = image->data[2];

    r = r0 << 16;
    g = g0 << 16;
    b = b0 << 16;
    
    dr = ((rf-r0)<<16)/width;
    dg = ((gf-g0)<<16)/width;
    db = ((bf-b0)<<16)/width;
    /* render the first line */
    for (i=0; i<width; i++) {
	*(rp++) = (unsigned char)(r>>16);
	*(gp++) = (unsigned char)(g>>16);
	*(bp++) = (unsigned char)(b>>16);
	r += dr;
	g += dg;
	b += db;
    }

    
    /* copy the first line to the other lines */
    for (i=1; i<height; i++) {
	memcpy(&(image->data[0][i*width]), image->data[0], width);
	memcpy(&(image->data[1][i*width]), image->data[1], width);
	memcpy(&(image->data[2][i*width]), image->data[2], width);
    }
    return image;
}

/*
 *----------------------------------------------------------------------
 * renderVGradient--
 *      Renders a vertical linear gradient of the specified size in the
 * RImage format with a border of the specified type.
 *
 * Returns:
 *      A 24bit RImage with the gradient (no alpha channel).
 *
 * Side effects:
 *      None
 *----------------------------------------------------------------------
 */
static RImage*
renderVGradient(int width, int height, int r0, int g0, int b0,
		int rf, int gf, int bf)
{
    int i;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *rp, *gp, *bp;

    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }    
    rp = image->data[0];
    gp = image->data[1];
    bp = image->data[2];
    
    r = r0<<16;
    g = g0<<16;
    b = b0<<16;

    dr = ((rf-r0)<<16)/height;
    dg = ((gf-g0)<<16)/height;
    db = ((bf-b0)<<16)/height;

    for (i=0; i<height; i++) {
	memset(rp, (unsigned char)(r>>16), width);
	memset(gp, (unsigned char)(g>>16), width);
	memset(bp, (unsigned char)(b>>16), width);
	rp+=width;
	gp+=width;
	bp+=width;
        r+=dr;
        g+=dg;
        b+=db;
    }
    return image;
}


/*
 *----------------------------------------------------------------------
 * renderDGradient--
 *      Renders a diagonal linear gradient of the specified size in the
 * RImage format with a border of the specified type.
 *
 * Returns:
 *      A 24bit RImage with the gradient (no alpha channel).
 *
 * Side effects:
 *      None
 *----------------------------------------------------------------------
 */
#if 0
/* This version is slower then the one below. It uses more operations,
 * most of them multiplication of floats. Dan.
 */
static RImage*
renderDGradient(int width, int height, int r0, int g0, int b0,
		int rf, int gf, int bf)
{
    int x, y;
    float from_red,from_green,from_blue;
    float to_red,to_green,to_blue;
    float dr,dg,db,dx,dy,w,h,xred,yred,xgreen,ygreen,xblue,yblue;
    RImage *image;
    unsigned char *rp, *gp, *bp;

    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    rp = image->data[0];
    gp = image->data[1];
    bp = image->data[2];
    
    from_red = (float)r0;
    from_green = (float)g0;
    from_blue = (float)b0;
    
    to_red = (float)rf;
    to_green = (float)gf;
    to_blue = (float)bf;
    
    w = (float) width;
    h = (float) height;
    
    dr = (to_red - from_red);
    dg = (to_green - from_green);
    db = (to_blue - from_blue);
    
    for (y=0; y<height; y++) {
	
	dy = y / h;
	
	yred   = dr * dy + from_red;
	ygreen = dg * dy + from_green;
	yblue  = db * dy + from_blue;
	
	for (x=0; x<width; x++) { 
	    dx = x / w;
	    
	    xred   = dr * dx + from_red;
	    xgreen = dg * dx + from_green;
	    xblue  = db * dx + from_blue;
	    
	    *(rp++) = (unsigned char)((xred + yred)/2);
	    *(gp++) = (unsigned char)((xgreen + ygreen)/2);
	    *(bp++) = (unsigned char)((xblue + yblue)/2);
	}
    }
    return image;
}
#endif


static RImage*
renderDGradient(int width, int height, int r0, int g0, int b0,
		int rf, int gf, int bf)
{
    RImage *image, *tmp;
    float a;
    int i, offset;

    if (width == 1)
        return renderVGradient(width, height, r0, g0, b0, rf, gf, bf);
    else if (height == 1)
        return renderHGradient(width, height, r0, g0, b0, rf, gf, bf);

    image = RCreateImage(width, height, False);
    if (!image) {
        return NULL;
    }

    tmp = renderHGradient(2*width-1, 1, r0, g0, b0, rf, gf, bf);
    if (!tmp) {
        RDestroyImage(image);
        return NULL;
    }

    a = ((float)(width - 1))/((float)(height - 1));

    /* copy the first line to the other lines with corresponding offset */
    for (i=0; i<height; i++) {
        offset = (int)(a*i+0.5);
        memcpy(&(image->data[0][i*width]), &(tmp->data[0][offset]), width);
        memcpy(&(image->data[1][i*width]), &(tmp->data[1][offset]), width);
        memcpy(&(image->data[2][i*width]), &(tmp->data[2][offset]), width);
    }
    RDestroyImage(tmp);
    return image;
}


static RImage*
renderMHGradient(int width, int height, RColor **colors, int count)
{
    int i, j, k;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *rp, *gp, *bp;
    int width2;
    
    
    assert(count > 2);
    
    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    rp = image->data[0];
    gp = image->data[1];
    bp = image->data[2];
    
    if (count > width)
	count = width;
    
    if (count > 1)
        width2 = width/(count-1);
    else
        width2 = width;
    
    k = 0;

    r = colors[0]->red << 16;
    g = colors[0]->green << 16;
    b = colors[0]->blue << 16;

    /* render the first line */
    for (i=1; i<count; i++) {
	dr = ((int)(colors[i]->red   - colors[i-1]->red)  <<16)/width2;
	dg = ((int)(colors[i]->green - colors[i-1]->green)<<16)/width2;
	db = ((int)(colors[i]->blue  - colors[i-1]->blue) <<16)/width2;
	for (j=0; j<width2; j++) {
	    *(rp++) = (unsigned char)(r>>16);
	    *(gp++) = (unsigned char)(g>>16);
	    *(bp++) = (unsigned char)(b>>16);
	    r += dr;
	    g += dg;
	    b += db;
	    k++;
	}
	r = colors[i]->red << 16;
	g = colors[i]->green << 16;
	b = colors[i]->blue << 16;
    }
    for (j=k; j<width; j++) {
	*(rp++) = (unsigned char)(r>>16);
	*(gp++) = (unsigned char)(g>>16);
	*(bp++) = (unsigned char)(b>>16);
    }
    
    /* copy the first line to the other lines */
    for (i=1; i<height; i++) {
	memcpy(&(image->data[0][i*width]), image->data[0], width);
	memcpy(&(image->data[1][i*width]), image->data[1], width);
	memcpy(&(image->data[2][i*width]), image->data[2], width);
    }
    return image;
}




static RImage*
renderMVGradient(int width, int height, RColor **colors, int count)
{
    int i, j, k;
    unsigned long r, g, b, dr, dg, db;
    RImage *image;
    unsigned char *rp, *gp, *bp;
    int height2;
    

    assert(count > 2);
    
    image = RCreateImage(width, height, False);
    if (!image) {
	return NULL;
    }
    rp = image->data[0];
    gp = image->data[1];
    bp = image->data[2];
    
    if (count > height)
	count = height;
    
    if (count > 1)
        height2 = height/(count-1);
    else
        height2 = height;
    
    k = 0;

    r = colors[0]->red << 16;
    g = colors[0]->green << 16;
    b = colors[0]->blue << 16;

    for (i=1; i<count; i++) {
	dr = ((int)(colors[i]->red   - colors[i-1]->red)  <<16)/height2;
	dg = ((int)(colors[i]->green - colors[i-1]->green)<<16)/height2;
	db = ((int)(colors[i]->blue  - colors[i-1]->blue) <<16)/height2;
	for (j=0; j<height2; j++) {
	    memset(rp, (unsigned char)(r>>16), width);
	    memset(gp, (unsigned char)(g>>16), width);
	    memset(bp, (unsigned char)(b>>16), width);
	    rp+=width;
	    gp+=width;
	    bp+=width;
	    r += dr;
	    g += dg;
	    b += db;
	    k++;
	}
	r = colors[i]->red << 16;
	g = colors[i]->green << 16;
	b = colors[i]->blue << 16;
    }
    for (j=k; j<height; j++) {
	    memset(rp, (unsigned char)(r>>16), width);
	    memset(gp, (unsigned char)(g>>16), width);
	    memset(bp, (unsigned char)(b>>16), width);
	    rp+=width;
	    gp+=width;
	    bp+=width;
    }
    
    return image;
}


static RImage*
renderMDGradient(int width, int height, RColor **colors, int count)
{
    RImage *image, *tmp;
    float a;
    int i, offset;

    assert(count > 2);

    if (width == 1)
        return renderMVGradient(width, height, colors, count);
    else if (height == 1)
        return renderMHGradient(width, height, colors, count);

    image = RCreateImage(width, height, False);
    if (!image) {
        return NULL;
    }

    if (count > width)
        count = width;
    if (count > height)
        count = height;

    tmp = renderMHGradient(2*width-1, 1, colors, count);
    if (!tmp) {
        RDestroyImage(image);
        return NULL;
    }

    a = ((float)(width - 1))/((float)(height - 1));

    /* copy the first line to the other lines with corresponding offset */
    for (i=0; i<height; i++) {
        offset = (int)(a*i+0.5);
        memcpy(&(image->data[0][i*width]), &(tmp->data[0][offset]), width);
        memcpy(&(image->data[1][i*width]), &(tmp->data[1][offset]), width);
        memcpy(&(image->data[2][i*width]), &(tmp->data[2][offset]), width);
    }
    RDestroyImage(tmp);
    return image;
}
