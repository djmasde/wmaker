/*
 *  WindowMaker window manager
 * 
 *  Copyright (c) 1997, 1998 Alfredo K. Kojima
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#include "wconfig.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <wraster.h>

#include "WindowMaker.h"
#include "wcore.h"
#include "texture.h"
#include "funcs.h"

extern WPreferences wPreferences;


static Pixmap renderTexture(WScreen *scr, int width, int height,
			     WTexture *texture, int rel);


static void bevelImage(RImage *image, int relief);



WTexSolid *
wTextureMakeSolid(WScreen *scr, XColor *color)
{
    WTexSolid *texture;
    int gcm;
    XGCValues gcv;
    unsigned int r, g, b;
        
    texture = wmalloc(sizeof(WTexture));

    texture->type = WTEX_SOLID;
    texture->subtype = 0;

    XAllocColor(dpy, scr->w_colormap, color);
    texture->normal = *color;
    if (color->red==0 && color->blue==0 && color->green == 0) {
	texture->light.red = 0xb6da;
	texture->light.green = 0xb6da;
	texture->light.blue = 0xb6da;
	texture->dim.red = 0x6185;
	texture->dim.green = 0x6185;
	texture->dim.blue = 0x6185;
	texture->dark.red = 0;
	texture->dark.green = 0;
	texture->dark.blue = 0;
    } else {
	r = (color->red * 17)/10;
	g = (color->green * 17)/10;
	b = (color->blue * 17)/10;
	texture->light.red = (r > 0xffff ? 0xffff : r);
	texture->light.green = (g > 0xffff ? 0xffff : g);
	texture->light.blue = (b > 0xffff ? 0xffff : b);
	r = (color->red * 5)/10;
	g = (color->green * 5)/10;
	b = (color->blue * 5)/10;
	texture->dim.red = r;
	texture->dim.green = g;
	texture->dim.blue = b;
	texture->dark.red = 0;
	texture->dark.green = 0;
	texture->dark.blue = 0;
    }
    XAllocColor(dpy, scr->w_colormap, &texture->light);
    XAllocColor(dpy, scr->w_colormap, &texture->dim);
    XAllocColor(dpy, scr->w_colormap, &texture->dark);
    
    gcm = GCForeground|GCBackground|GCGraphicsExposures;
    gcv.graphics_exposures = False;
    
    gcv.background = gcv.foreground = texture->light.pixel;
    texture->light_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);
    
    gcv.background = gcv.foreground = texture->dim.pixel;	
    texture->dim_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);
    
    gcv.background = gcv.foreground = texture->dark.pixel;
    texture->dark_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    gcv.background = gcv.foreground = color->pixel;
    texture->normal_gc = XCreateGC(dpy, scr->w_win, gcm, &gcv);

    return texture;
}


static int
dummyErrorHandler(Display *foo, XErrorEvent *bar)
{
#ifdef DEBUG
    wwarning("your server is buggy. Tell the author if some error related to color occurs");
#endif
    return 0;
}


void
wTextureDestroy(WScreen *scr, WTexture *texture)
{
    int i;
    int count=0;
    unsigned long colors[8];
    
#ifdef DEBUG    
    if (texture==NULL) {
	printf("BUG: trying to free NULL texture\n");
	return;
    }
#endif
    
    /* 
     * some stupid servers don't like white or black being freed...
     */
#define CANFREE(c) (c!=scr->black_pixel && c!=scr->white_pixel)
    switch (texture->any.type) {
     case WTEX_SOLID:
	XFreeGC(dpy, texture->solid.light_gc);
	XFreeGC(dpy, texture->solid.dark_gc);
	XFreeGC(dpy, texture->solid.dim_gc);
	if (CANFREE(texture->solid.light.pixel))
	  colors[count++] = texture->solid.light.pixel;
	if (CANFREE(texture->solid.dim.pixel))
	  colors[count++] = texture->solid.dim.pixel;
	if (CANFREE(texture->solid.dark.pixel))
	  colors[count++] = texture->solid.dark.pixel;
	break;

     case WTEX_PIXMAP:
	RDestroyImage(texture->pixmap.pixmap);
	break;
	
     case WTEX_MHGRADIENT:
     case WTEX_MVGRADIENT:
     case WTEX_MDGRADIENT:
	for (i=0; texture->mgradient.colors[i]!=NULL; i++) {
	    free(texture->mgradient.colors[i]);
	}
	free(texture->mgradient.colors);
	break;
    }
    if (CANFREE(texture->any.color.pixel))
      colors[count++] = texture->any.color.pixel;
    if (count > 0) {
	XErrorHandler oldhandler;
	
	/* ignore error from buggy servers that don't know how
	 * to do reference counting for colors. */
	oldhandler = XSetErrorHandler(dummyErrorHandler);
	XFreeColors(dpy, scr->colormap, colors, count, 0);
	XSync(dpy,0);
	XSetErrorHandler(oldhandler);
    }
    XFreeGC(dpy, texture->any.gc);
    free(texture);
#undef CANFREE    
}



WTexGradient*
wTextureMakeGradient(WScreen *scr, int style, XColor *from, XColor *to)
{
    WTexGradient *texture;
    XGCValues gcv;

    
    texture = wmalloc(sizeof(WTexture));
    memset(texture, 0, sizeof(WTexture));
    texture->type = style;
    texture->subtype = 0;
    
    texture->color1 = *from;
    texture->color2 = *to;

    texture->normal.red = (from->red + to->red)/2;
    texture->normal.green = (from->green + to->green)/2;
    texture->normal.blue = (from->blue + to->blue)/2;

    XAllocColor(dpy, scr->w_colormap, &texture->normal);
    gcv.background = gcv.foreground = texture->normal.pixel;
    gcv.graphics_exposures = False;
    texture->normal_gc = XCreateGC(dpy, scr->w_win, GCForeground|GCBackground
				   |GCGraphicsExposures, &gcv);

    return texture;
}



WTexMGradient*
wTextureMakeMGradient(WScreen *scr, int style, RColor **colors)
{
    WTexMGradient *texture;
    XGCValues gcv;
    int i;

    
    texture = wmalloc(sizeof(WTexture));
    memset(texture, 0, sizeof(WTexture));
    texture->type = style;
    texture->subtype = 0;
    
    i=0;
    while (colors[i]!=NULL) i++;
    i--;
    texture->normal.red = (colors[0]->red<<8);
    texture->normal.green = (colors[0]->green<<8);
    texture->normal.blue =  (colors[0]->blue<<8);
    
    texture->colors = colors;

    XAllocColor(dpy, scr->w_colormap, &texture->normal);
    gcv.background = gcv.foreground = texture->normal.pixel;
    gcv.graphics_exposures = False;
    texture->normal_gc = XCreateGC(dpy, scr->w_win, GCForeground|GCBackground
				   |GCGraphicsExposures, &gcv);

    return texture;
}



WTexPixmap*
wTextureMakePixmap(WScreen *scr, int style, char *pixmap_file, XColor *color)
{
    WTexPixmap *texture;
    XGCValues gcv;
    RImage *image;
    char *file;

    file = FindImage(wPreferences.pixmap_path, pixmap_file);
    if (!file) {
        wwarning(_("image file \"%s\" used as texture could not be found."),
                 pixmap_file);
	return NULL;
    }
    image = RLoadImage(scr->rcontext, file, 0);
    if (!image) {
	wwarning(_("could not load texture pixmap \"%s\":%s"), file,
			RErrorString);
	free(file);
	return NULL;
    }
    free(file);

    texture = wmalloc(sizeof(WTexture));
    memset(texture, 0, sizeof(WTexture));
    texture->type = WTEX_PIXMAP;
    texture->subtype = style;

    texture->normal = *color;

    XAllocColor(dpy, scr->w_colormap, &texture->normal);
    gcv.background = gcv.foreground = texture->normal.pixel;
    gcv.graphics_exposures = False;
    texture->normal_gc = XCreateGC(dpy, scr->w_win, GCForeground|GCBackground
				   |GCGraphicsExposures, &gcv);

    texture->pixmap = image;
    
    return texture;
}


RImage*
wTextureRenderImage(WTexture *texture, int width, int height, int relief)
{
    RImage *image;
    RColor color1, color2;
    int d;
    int subtype;
    
    switch (texture->any.type) {
     case WTEX_SOLID:
	image = RCreateImage(width, height, False);
	
	color1.red = texture->solid.normal.red >> 8;
	color1.green = texture->solid.normal.green >> 8;
	color1.blue = texture->solid.normal.blue >> 8;
        color1.alpha = 255;
        
	RClearImage(image, &color1);
	break;
	
     case WTEX_PIXMAP:
	if (texture->pixmap.subtype == WTP_TILE)
            image = RMakeTiledImage(texture->pixmap.pixmap, width, height);
	else
            image = RScaleImage(texture->pixmap.pixmap, width, height);
	break;
	
     case WTEX_HGRADIENT:
	subtype = RGRD_HORIZONTAL;
	goto render_gradient;
	
     case WTEX_VGRADIENT:
	subtype = RGRD_VERTICAL;
	goto render_gradient;
	
     case WTEX_DGRADIENT:
	subtype = RGRD_DIAGONAL;
     render_gradient:
	color1.red = texture->gradient.color1.red >> 8;
	color1.green = texture->gradient.color1.green >> 8;
	color1.blue = texture->gradient.color1.blue >> 8;
	color2.red = texture->gradient.color2.red >> 8;
	color2.green = texture->gradient.color2.green >> 8;
	color2.blue = texture->gradient.color2.blue >> 8;

	image = RRenderGradient(width, height, &color1, &color2, subtype);
	break;
	
     case WTEX_MHGRADIENT:
	subtype = RGRD_HORIZONTAL;
	goto render_mgradient;

     case WTEX_MVGRADIENT:
	subtype = RGRD_VERTICAL;
	goto render_mgradient;
	
     case WTEX_MDGRADIENT:
	subtype = RGRD_DIAGONAL;
     render_mgradient:
	image = RRenderMultiGradient(width, height, 
				     &(texture->mgradient.colors[1]),
				     subtype);
	break;
	
     default:
	puts("ERROR in wTextureRenderImage()");
	image = NULL;
    }

    if (!image) {
	wwarning(_("could not render texture: %s"), RErrorString);
	return None;
    }


    /* render bevel */
    
    switch (relief) {
    case WREL_ICON:
	d = RBEV_RAISED3;
	break;

     case WREL_RAISED:
	d = RBEV_RAISED2;
	break;
	
     case WREL_SUNKEN:
	d = RBEV_SUNKEN;
	break;
	
     case WREL_FLAT:
	d = 0;
	break;

     case WREL_MENUENTRY:
	d = -WREL_MENUENTRY;
	break;

     default:
	d = 0;
    }
    
    if (d > 0) {
	RBevelImage(image, d);
    } else if (d < 0) {
	bevelImage(image, -d);
    }
    
    return image;
}


/* used only for menu entries */
void
wTextureRender(WScreen *scr, WTexture *texture, Pixmap *data, 
	       int width, int height, int relief)
{
    if (!texture)
      return;
/*    
    switch (texture->any.type) {
     case WTEX_DGRADIENT:
     case WTEX_VGRADIENT:
     case WTEX_HGRADIENT:
     case WTEX_MHGRADIENT:
     case WTEX_MVGRADIENT:
     case WTEX_MDGRADIENT:
     case WTEX_PIXMAP:
	*/
	if (!*data) {
	    *data = renderTexture(scr, width, height, texture, relief);
	}/*
	break;
    }*/
}



static void
bevelImage(RImage *image, int relief)
{
    int width = image->width;
    int height = image->height;
    int x, y, ofs;
    unsigned char *r, *g, *b;

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];
    switch (relief) {
     case WREL_MENUENTRY:
	r++; 	g++;	b++;
	for (x=0; x<(width-2); x++) {
	    *r = (*r<175 ? *r+80 : 255);
	    *g = (*g<175 ? *g+80 : 255);
	    *b = (*b<175 ? *b+80 : 255);	    
	    r++; g++; b++;
	}
	r = image->data[0];
	g = image->data[1];
	b = image->data[2];
	for (y=0; y<height; y++) {
	    ofs = y*width;
	    r[ofs]=0;
	    g[ofs]=0;
	    b[ofs++]=0;
	    r[ofs] = (r[ofs] <175 ? r[ofs]+80 : 255);
	    g[ofs] = (g[ofs] <175 ? g[ofs]+80 : 255);
	    b[ofs] = (b[ofs] <175 ? b[ofs]+80 : 255);
	    ofs = y*width+(width-2);
	    r[ofs] = (r[ofs] >40 ? r[ofs]-40 : 0);
	    g[ofs] = (g[ofs] >40 ? g[ofs]-40 : 0);
	    b[ofs] = (b[ofs] >40 ? b[ofs]-40 : 0);
	    ofs++;
	    r[ofs]=0;
	    g[ofs]=0;
	    b[ofs++]=0;
	}
	r = image->data[0] + (height-2)*width + 1;
	g = image->data[1] + (height-2)*width + 1;
	b = image->data[2] + (height-2)*width + 1;
	for (x=1; x<width; x++) {
	    *r = (*r>40 ? *r-40 : 0);
	    *g = (*g>40 ? *g-40 : 0);
	    *b = (*b>40 ? *b-40 : 0);
	    r++; g++; b++;
	}
	memset(r, 0, width);
	memset(g, 0, width);
	memset(b, 0, width);	
	break;

    }
}
 


static Pixmap
renderTexture(WScreen *scr, int width, int height, WTexture *texture,
	       int rel)
{
    RImage *img;
    Pixmap pix;

    img = wTextureRenderImage(texture, width, height, rel);
    
    if (!img) {
	wwarning(_("could not render texture: %s"), RErrorString);
	return None;
    }
    if (!RConvertImage(scr->rcontext, img, &pix)) {
	wwarning(_("error rendering image:%s"), RErrorString);
    }
    RDestroyImage(img);
    return pix;
}
 

void
wDrawBevel(WCoreWindow *core, WTexSolid *texture, int relief)
{
    GC light, dim, dark;
    XSegment segs[4];

    if (relief==WREL_FLAT) return;

    light = texture->light_gc;
    dim = texture->dim_gc;
    dark = texture->dark_gc;
    switch (relief) {
     case WREL_FLAT:
	return;
     case WREL_MENUENTRY:
     case WREL_RAISED:
     case WREL_ICON:
	segs[0].x1 = 1;
	segs[0].x2 = core->width - 2;
	segs[0].y2 = segs[0].y1 = core->height - 2;
	segs[1].x1 = core->width - 2;
	segs[1].y1 = 1;
	segs[1].x2 = core->width - 2;
	segs[1].y2 = core->height - 2;
	XDrawSegments(dpy, core->window, dim, segs, 2);
	segs[0].x1 = 0;
	segs[0].x2 = core->width - 1;
	segs[0].y2 = segs[0].y1 = core->height - 1;
	segs[1].x1 = segs[1].x2 = core->width - 1;
	segs[1].y1 = 0;
	segs[1].y2 = core->height - 1;
	XDrawSegments(dpy, core->window, dark, segs, 2);
	segs[0].x1 = segs[0].y1 = segs[0].y2 = 0;
	segs[0].x2 = core->width - 2;
	segs[1].x1 = segs[1].y1 = 0;
	segs[1].x2 = 0;
	segs[1].y2 = core->height - 2;
	XDrawSegments(dpy, core->window, light, segs, 2);
	if (relief==WREL_ICON) {
	    segs[0].x1 = segs[0].y1 = segs[0].y2 = 1;
	    segs[0].x2 = core->width - 2;
	    segs[1].x1 = segs[1].y1 = 1;
	    segs[1].x2 = 1;
	    segs[1].y2 = core->height - 2;
	    XDrawSegments(dpy, core->window, light, segs, 2);
	}
	break;
		
     case WREL_SUNKEN:
	segs[0].x1 = 0;
	segs[0].x2 = core->width - 1;
	segs[0].y2 = segs[0].y1 = 0;
	segs[1].x1 = segs[1].x2 = 0;
	segs[1].y1 = 0;
	segs[1].y2 = core->height - 1;
	XDrawSegments(dpy, core->window, dark, segs, 2);
	
	segs[0].x1 = 0;
	segs[0].y1 = segs[0].y2 = core->height - 1;
	segs[0].x2 = core->width - 1;
	segs[1].x2 = segs[1].x1 = core->width - 1;
	segs[1].y1 = 1;
	segs[1].y2 = core->height - 1;
	XDrawSegments(dpy, core->window, light, segs, 2);
	break;	
    }
}

