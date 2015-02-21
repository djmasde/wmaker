/* context.c - X context management
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "wraster.h"


static RContextAttributes DEFAULT_CONTEXT_ATTRIBS = {
    RC_UseSharedMemory|RC_RenderMode|RC_ColorsPerChannel, /* flags */
	RM_DITHER, 		       /* render_mode */
	4,			       /* colors_per_channel */
	0, 
	0,
	0,
	0,
	True				   /* use_shared_memory */
};


static XColor*
allocatePseudoColor(RContext *ctx)
{
    XColor *colors;
    XColor avcolors[256];
    int avncolors;
    int i, ncolors, r, g, b;
    int retries;
    int cpc = ctx->attribs->colors_per_channel;

    ncolors = cpc * cpc * cpc;
    
    if ( ncolors > (1<<ctx->depth) ) {
      /* reduce colormap size */
      cpc = ctx->attribs->colors_per_channel = 1<<((int)ctx->depth/3);
      ncolors = cpc * cpc * cpc;
    }

    if (cpc < 2 || ncolors > (1<<ctx->depth)) {
	sprintf(RErrorString, "invalid colormap size %i", cpc);
	return NULL;
    }

    colors = malloc(sizeof(XColor)*ncolors);
    if (!colors) {
	sprintf(RErrorString, "out of memory");
	return NULL;
    }
    i=0;

    if ((ctx->attribs->flags & RC_GammaCorrection) && ctx->attribs->rgamma > 0
	&& ctx->attribs->ggamma > 0 && ctx->attribs->bgamma > 0) {
	double rg, gg, bg;
	double tmp;

	/* do gamma correction */
	rg = 1.0/ctx->attribs->rgamma;
	gg = 1.0/ctx->attribs->ggamma;
	bg = 1.0/ctx->attribs->bgamma;
	for (r=0; r<cpc; r++) {
	    for (g=0; g<cpc; g++) {
		for (b=0; b<cpc; b++) {

		    colors[i].red=(r*0xffff) / (cpc-1);
		    colors[i].green=(g*0xffff) / (cpc-1);
		    colors[i].blue=(b*0xffff) / (cpc-1);
		    colors[i].flags = DoRed|DoGreen|DoBlue;

		    tmp = (double)colors[i].red / 65536.0;
		    colors[i].red = (unsigned short)(65536.0*pow(tmp, rg));

		    tmp = (double)colors[i].green / 65536.0;
		    colors[i].green = (unsigned short)(65536.0*pow(tmp, gg));

		    tmp = (double)colors[i].blue / 65536.0;
		    colors[i].blue = (unsigned short)(65536.0*pow(tmp, bg));

		    i++;
		}
	    }
	}

    } else {
	for (r=0; r<cpc; r++) {
	    for (g=0; g<cpc; g++) {
		for (b=0; b<cpc; b++) {
		    colors[i].red=(r*0xffff) / (cpc-1);
		    colors[i].green=(g*0xffff) / (cpc-1);
		    colors[i].blue=(b*0xffff) / (cpc-1);
		    colors[i].flags = DoRed|DoGreen|DoBlue;
		    i++;
		}
	    }
	}
    }
    /* try to allocate the colors */
    for (i=0; i<ncolors; i++) {
	if (!XAllocColor(ctx->dpy, ctx->cmap, &(colors[i]))) {
	    colors[i].flags = 0; /* failed */
	} else {
	    colors[i].flags = DoRed|DoGreen|DoBlue;	    
	}
    }
    /* try to allocate close values for the colors that couldn't 
     * be allocated before */
    avncolors = (1<<ctx->depth>256 ? 256 : 1<<ctx->depth);
    for (i=0; i<avncolors; i++) avcolors[i].pixel = i;

    XQueryColors(ctx->dpy, ctx->cmap, avcolors, avncolors);

    for (i=0; i<ncolors; i++) {
	if (colors[i].flags==0) {
	    int j;
	    unsigned long cdiff=0xffffffff, diff;
	    unsigned long closest=0;
	    
	    retries = 2;
	    
	    while (retries--) {
		/* find closest color */
		for (j=0; j<avncolors; j++) {
		    r = (colors[i].red - avcolors[i].red)>>8;
		    g = (colors[i].green - avcolors[i].green)>>8;
		    b = (colors[i].blue - avcolors[i].blue)>>8;
		    diff = r*r + g*g + b*b;
		    if (diff<cdiff) {
			cdiff = diff;
			closest = j;
		    }
		}
		/* allocate closest color found */
		colors[i].red = avcolors[closest].red;
		colors[i].green = avcolors[closest].green;
		colors[i].blue = avcolors[closest].blue;
		if (XAllocColor(ctx->dpy, ctx->cmap, &colors[i])) {
		    colors[i].flags = DoRed|DoGreen|DoBlue;
		    break; /* succeeded, don't need to retry */
		}
#ifdef DEBUG
		printf("close color allocation failed. Retrying...\n");
#endif
	    }
	}
    }
    return colors;
}


static XColor*
allocateGrayScale(RContext *ctx)
{
    XColor *colors;
    XColor avcolors[256];
    int avncolors;
    int i, ncolors, r, g, b;
    int retries;
    int cpc = ctx->attribs->colors_per_channel;

    ncolors = cpc * cpc * cpc;
    
    if (ctx->vclass == StaticGray) {
      /* we might as well use all grays */
      ncolors = 1<<ctx->depth;
    } else {
      if ( ncolors > (1<<ctx->depth) ) {
	/* reduce colormap size */
	cpc = ctx->attribs->colors_per_channel = 1<<((int)ctx->depth/3);
	ncolors = cpc * cpc * cpc;
      }
      
      if (cpc < 2 || ncolors > (1<<ctx->depth)) {
	sprintf(RErrorString, "invalid colormap size %i", cpc);
	return NULL;
      }
    }

    if (ncolors>=256 && ctx->vclass==StaticGray) {
        /* don't need dithering for 256 levels of gray in StaticGray visual */
        ctx->attribs->render_mode = RM_MATCH;
    }

    colors = malloc(sizeof(XColor)*ncolors);
    if (!colors) {
	sprintf(RErrorString, "out of memory");
	return False;
    }
    for (i=0; i<ncolors; i++) {
	colors[i].red=(i*0xffff) / (ncolors-1);
	colors[i].green=(i*0xffff) / (ncolors-1);
	colors[i].blue=(i*0xffff) / (ncolors-1);
	colors[i].flags = DoRed|DoGreen|DoBlue;
    }
    /* try to allocate the colors */
    for (i=0; i<ncolors; i++) {
#ifdef DEBUG
        printf("trying:%x,%x,%x\n",colors[i].red,colors[i].green,colors[i].blue);
#endif
	if (!XAllocColor(ctx->dpy, ctx->cmap, &(colors[i]))) {
	    colors[i].flags = 0; /* failed */
#ifdef DEBUG
	    printf("failed:%x,%x,%x\n",colors[i].red,colors[i].green,colors[i].blue);
#endif
	} else {
	    colors[i].flags = DoRed|DoGreen|DoBlue;	    
#ifdef DEBUG
	    printf("success:%x,%x,%x\n",colors[i].red,colors[i].green,colors[i].blue);
#endif
	}
    }
    /* try to allocate close values for the colors that couldn't 
     * be allocated before */
    avncolors = (1<<ctx->depth>256 ? 256 : 1<<ctx->depth);
    for (i=0; i<avncolors; i++) avcolors[i].pixel = i;

    XQueryColors(ctx->dpy, ctx->cmap, avcolors, avncolors);

    for (i=0; i<ncolors; i++) {
	if (colors[i].flags==0) {
	    int j;
	    unsigned long cdiff=0xffffffff, diff;
	    unsigned long closest=0;
	    
	    retries = 2;
	    
	    while (retries--) {
		/* find closest color */
		for (j=0; j<avncolors; j++) {
		    r = (colors[i].red - avcolors[i].red)>>8;
		    g = (colors[i].green - avcolors[i].green)>>8;
		    b = (colors[i].blue - avcolors[i].blue)>>8;
		    diff = r*r + g*g + b*b;
		    if (diff<cdiff) {
			cdiff = diff;
			closest = j;
		    }
		}
		/* allocate closest color found */
#ifdef DEBUG
		printf("best match:%x,%x,%x => %x,%x,%x\n",colors[i].red,colors[i].green,colors[i].blue,avcolors[closest].red,avcolors[closest].green,avcolors[closest].blue);
#endif
		colors[i].red = avcolors[closest].red;
		colors[i].green = avcolors[closest].green;
		colors[i].blue = avcolors[closest].blue;
		if (XAllocColor(ctx->dpy, ctx->cmap, &colors[i])) {
		    colors[i].flags = DoRed|DoGreen|DoBlue;
		    break; /* succeeded, don't need to retry */
		}
#ifdef DEBUG
		printf("close color allocation failed. Retrying...\n");
#endif
	    }
	}
    }
    return colors;
}


static char*
mygetenv(char *var, int scr)
{
    char *p;
    char varname[64];

    if (scr==0) {
	p = getenv(var);
    }
    if (scr!=0 || !p) {
	sprintf(varname, "%s%i", var, scr);
	p = getenv(var);
    }
    return p;
}


static void 
gatherconfig(RContext *context, int screen_n)
{
    char *ptr;

    ptr = mygetenv("WRASTER_GAMMA", screen_n);
    if (ptr) {
	float g1,g2,g3;
	if (sscanf(ptr, "%f/%f/%f", &g1, &g2, &g3)!=3 
	    || g1<=0.0 || g2<=0.0 || g3<=0.0) {
	    printf("wrlib: invalid value(s) for gamma correction \"%s\"\n", 
		   ptr);
	} else {
	    context->attribs->flags |= RC_GammaCorrection;
	    context->attribs->rgamma = g1;
	    context->attribs->ggamma = g2;
	    context->attribs->bgamma = g3;
	}
    }
}


static void
getColormap(RContext *context, int screen_number)
{
    Colormap cmap = None;
    XStandardColormap *cmaps;
    int ncmaps, i;

    if (XGetRGBColormaps(context->dpy, 
			 RootWindow(context->dpy, screen_number), 
			 &cmaps, &ncmaps, XA_RGB_DEFAULT_MAP)) { 
	for (i=0; i<ncmaps; ++i) {
	    if (cmaps[i].visualid == context->visual->visualid) {
		cmap = cmaps[i].colormap;
		break;
	    }
	}
	XFree(cmaps);
    }
    if (cmap == None) {
	XColor color;
	
	cmap = XCreateColormap(context->dpy, 
			       RootWindow(context->dpy, screen_number),
			       context->visual, AllocNone);
	
	color.red = color.green = color.blue = 0;
	XAllocColor(context->dpy, cmap, &color);
	context->black = color.pixel;

	color.red = color.green = color.blue = 0xffff;
	XAllocColor(context->dpy, cmap, &color);
	context->white = color.pixel;
    }
    context->cmap = cmap;
}


static int
count_offset(unsigned long mask) 
{
    int i;
    
    i=0;
    while ((mask & 1)==0) {
	i++;
	mask = mask >> 1;
    }
    return i;
}


RContext*
RCreateContext(Display *dpy, int screen_number, RContextAttributes *attribs)
{
    RContext *context;
    XGCValues gcv;

    
    RErrorString[0]=0;
    context = malloc(sizeof(RContext));
    if (!context) {
	sprintf(RErrorString, "out of memory");
	return NULL;
    }
    memset(context, 0, sizeof(RContext));

    context->dpy = dpy;
    
    context->screen_number = screen_number;
    
    context->attribs = malloc(sizeof(RContextAttributes));
    if (!context->attribs) {
	free(context);
	sprintf(RErrorString, "out of memory");
	return NULL;
    }
    if (!attribs)
	*context->attribs = DEFAULT_CONTEXT_ATTRIBS;
    else
	*context->attribs = *attribs;

    /* get configuration from environment variables */
    gatherconfig(context, screen_number);
    
    if ((context->attribs->flags & RC_VisualID)) {
	XVisualInfo *vinfo, templ;
	int nret;
	    
	templ.screen = screen_number;
	templ.visualid = context->attribs->visualid;
	vinfo = XGetVisualInfo(context->dpy, VisualIDMask|VisualScreenMask,
			       &templ, &nret);

	if (!vinfo || nret==0) {
	    sprintf(RErrorString, "invalid visual id %x\n", 
		    (unsigned int)context->attribs->visualid);
	} else {
	    if (vinfo[0].visual != DefaultVisual(dpy, screen_number)) {
		XSetWindowAttributes attr;
		context->visual = vinfo[0].visual;
		context->depth = vinfo[0].depth;
		context->vclass = vinfo[0].class;

		getColormap(context, screen_number);

                attr.override_redirect = True;
                attr.colormap = context->cmap;
		context->drawable =
                    XCreateWindow(dpy, RootWindow(dpy, screen_number),
                                  1, 1, 1, 1, 0, context->depth,
                                  CopyFromParent, context->visual,
                                  CWColormap|CWOverrideRedirect, &attr);
	    }
	    XFree(vinfo);
	}
    }

    /* use default */
    if (!context->visual) {
	context->visual = DefaultVisual(dpy, screen_number);
	context->depth = DefaultDepth(dpy, screen_number);
	context->cmap = DefaultColormap(dpy, screen_number);
	context->drawable = RootWindow(dpy, screen_number);
	context->black = BlackPixel(dpy, screen_number);
	context->white = WhitePixel(dpy, screen_number);
	context->vclass = context->visual->class; 
    }
    
    gcv.function = GXcopy;
    gcv.graphics_exposures = False;
    context->copy_gc = XCreateGC(dpy, context->drawable, GCFunction
				 |GCGraphicsExposures, &gcv);

    if (context->vclass == PseudoColor || context->vclass == StaticColor) {
	context->colors = allocatePseudoColor(context);
	if (!context->colors) {
	    return NULL;
	}
    } else if (context->vclass == GrayScale || context->vclass == StaticGray) {
	context->colors = allocateGrayScale(context);
	if (!context->colors) {
	    return NULL;
	}
    } else if (context->vclass == TrueColor) {
    	/* calc offsets to create a TrueColor pixel */
	context->red_offset = count_offset(context->visual->red_mask);
	context->green_offset = count_offset(context->visual->green_mask);
	context->blue_offset = count_offset(context->visual->blue_mask);
	/* disable dithering on 24 bits visuals */
	if (context->depth >= 24)
	  context->attribs->render_mode = RM_MATCH;
    }
    
    /* check avaiability of MIT-SHM */
#ifdef XSHM
    if (!(context->attribs->flags & RC_UseSharedMemory)) {
	context->attribs->flags |= RC_UseSharedMemory;
	context->attribs->use_shared_memory = True;
    }

    if (context->attribs->use_shared_memory) {
	if (!XShmQueryExtension(context->dpy)) {
	    context->attribs->use_shared_memory = False;
	}
    } 
#endif

    return context;
}

