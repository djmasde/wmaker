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

/*
 * Environment variables:
 * 
 * WRASTER_GAMMA <rgamma>/<ggamma>/<bgamma>
 * gamma correction value. Must be  greater than 0
 * Only for PseudoColor visuals.
 * 
 * Default:
 * WRASTER_GAMMA 1/1/1
 * 
 * 
 * If you want a specific value for a screen, append the screen number
 * preceded by a hash to the variable name as in
 * WRASTER_GAMMA#1
 * for screen number 1
 */

#ifndef RLRASTER_H_
#define RLRASTER_H_


/* version of the header for the library: 0.4 */
#define WRASTER_HEADER_VERSION	4

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif


#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef XSHM
#include <X11/extensions/XShm.h>
#endif

/* RM_MATCH or RM_DITHER */
#define RC_RenderMode 		(1<<0)

/* number of colors per channel for colormap in PseudoColor mode */
#define RC_ColorsPerChannel	(1<<1)

/* do gamma correction */
#define RC_GammaCorrection	(1<<2)

/* visual id to use */
#define RC_VisualID		(1<<3)

/* shared memory usage */
#define RC_UseSharedMemory	(1<<4)

typedef struct RContextAttributes {
    int flags;
    int render_mode;
    int colors_per_channel;	       /* for PseudoColor */
    float rgamma;		       /* gamma correction for red, */
    float ggamma;		       /* green, */
    float bgamma;		       /* and blue */
    VisualID visualid;		       /* visual ID to use */
    int use_shared_memory;	       /* True of False */
} RContextAttributes;


/*
 * describes a screen in terms of depth, visual, number of colors
 * we can use, if we should do dithering, and what colors to use for
 * dithering.
 */
typedef struct RContext {
    Display *dpy;
    int screen_number;
    Colormap cmap;
    
    RContextAttributes *attribs;

    GC copy_gc;

    Visual *visual;
    int depth;
    Window drawable;		       /* window to pass for XCreatePixmap().*/
				       /* generally = root */
    int vclass;
    
    unsigned long black;
    unsigned long white;
    
    int red_offset;		       /* only used in 24bpp */
    int green_offset;
    int blue_offset;
    
    /* only used for pseudocolor and grayscale */
    int ncolors;		       /* total number of colors we can use */
    XColor *colors;		       /* internal colormap */
} RContext;


/* image display modes */
#define RM_DITHER	0
#define RM_MATCH	1



typedef struct RColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} RColor;


/*
 * internal 24bit+alpha image representation
 */
typedef struct RImage {
    int width, height;		       /* size of the image */
    RColor background;		       /* background color */
    unsigned char *data[4];	       /* image data (R,G,B,A) */
} RImage;


/*
 * internal wrapper for XImage. Used for shm abstraction
 */
typedef struct RXImage {
    XImage *image;
#ifdef XSHM
    XShmSegmentInfo info;
    int is_shared;
#endif
} RXImage;


#define RBEV_SUNKEN	-1
/* 1 pixel wide */
#define RBEV_RAISED	1
/* 1 pixel wide on top/left 2 on bottom/right */
#define RBEV_RAISED2	2
/* 2 pixel width */
#define RBEV_RAISED3	3

#define RGRD_HORIZONTAL	2
#define RGRD_VERTICAL	3
#define RGRD_DIAGONAL	4

/* cpc == colors per channel for the dithering colormap */

/*
 * Xlib contexts
 */
RContext *RCreateContext(Display *dpy, int screen_number,
			 RContextAttributes *attribs);

/*
 * RImage creation
 */
RImage *RCreateImage(int width, int height, int alpha);

RImage *RCreateImageFromXImage(RContext *context, XImage *image, XImage *mask);

RImage *RCreateImageFromDrawable(RContext *context, Drawable drawable,
				 Pixmap mask);

RImage *RLoadImage(RContext *context, char *file, int index);


int RDestroyImage(RImage *image);

RImage *RGetImageFromXPMData(RContext *context, char **data);


/*
 * Area manipulation
 */
RImage *RCloneImage(RImage *image);

RImage *RGetSubImage(RImage *image, int x, int y, int width, int height);

int RCombineImageWithColor(RImage *image, RColor *color);

int RCombineImages(RImage *image, RImage *src);

int ROverlayImages(RImage *image, RImage *src);

int RCombineArea(RImage *image, RImage *src, int sx, int sy, int width,
		 int height, int dx, int dy);

int RCombineImagesWithOpaqueness(RImage *image, RImage *src, int opaqueness);

int RCombineAreaWithOpaqueness(RImage *image, RImage *src, int sx, int sy, 
			       int width, int height, int dx, int dy, 
			       int opaqueness);

RImage *RScaleImage(RImage *image, int new_width, int new_height);

RImage* RMakeTiledImage(RImage *tile, int width, int height);

/*
 * Painting
 */
int RClearImage(RImage *image, RColor *color);

int RBevelImage(RImage *image, int bevel_type);

RImage *RRenderGradient(int width, int height, RColor *from, RColor *to,
			int style);


RImage *RRenderMultiGradient(int width, int height, RColor **colors, 
			     int style);

/*
 * Convertion into X Pixmaps
 */
int RConvertImage(RContext *context, RImage *image, Pixmap *pixmap);

int RConvertImageMask(RContext *context, RImage *image, Pixmap *pixmap,
		      Pixmap *mask, int threshold);


/*
 * misc. utilities
 */
RXImage *RCreateXImage(RContext *context, int depth, int width, int height);

void RDestroyXImage(RContext *context, RXImage *ximage);

void RPutXImage(RContext *context, Drawable d, GC gc, RXImage *ximage, 
		int src_x, int src_y, int dest_x, int dest_y, 
		unsigned int width, unsigned int height);


/****** Global Variables *******/

/*
 * Where error strings are stored
 */
extern char RErrorString[];


#endif
