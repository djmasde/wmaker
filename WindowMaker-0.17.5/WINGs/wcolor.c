
#include "WINGsP.h"

#include <wraster.h>

#define LIGHT_STIPPLE_WIDTH 4
#define LIGHT_STIPPLE_HEIGHT 4
static unsigned char LIGHT_STIPPLE_BITS[] = {
   0x05, 0x0a, 0x05, 0x0a};

#define DARK_STIPPLE_WIDTH 4
#define DARK_STIPPLE_HEIGHT 4
static unsigned char DARK_STIPPLE_BITS[] = {
   0x0a, 0x04, 0x0a, 0x01};


/*
 * TODO: make the color creation code return the same WMColor for the
 * same colors.
 * make findCloseColor() find the closest color in the RContext pallette
 * or in the other colors allocated by WINGs.
 */

static WMColor*
findCloseColor(WMScreen *scr, unsigned short red, unsigned short green, 
	       unsigned short blue)
{
    WMColor *color;
    
    return NULL;
    color->color.pixel = 0;
    
    return color;
}



static WMColor*
createRGBColor(WMScreen *scr, unsigned short red, unsigned short green, 
	       unsigned short blue)
{
    WMColor *color;
    XGCValues gcv;
    XColor xcolor;

    xcolor.red = red;
    xcolor.green = green;
    xcolor.blue = blue;
    xcolor.flags = DoRed|DoGreen|DoBlue;
    if (!XAllocColor(scr->display, scr->colormap, &xcolor))
	return NULL;

    color = wmalloc(sizeof(WMColor));

    color->refCount = 1;
    color->color = xcolor;
    color->flags.exact = 1;

    gcv.foreground = color->color.pixel;
    gcv.graphics_exposures = False;
    color->gc = XCreateGC(scr->display, scr->rcontext->drawable, 
			  GCForeground|GCGraphicsExposures, &gcv);

    return color;
}



WMColor*
WMCreateRGBColor(WMScreen *scr, unsigned short red, unsigned short green, 
		 unsigned short blue, Bool exact)
{
    WMColor *color;
    
    if (!exact || !(color=createRGBColor(scr, red, green, blue))) {
	color = findCloseColor(scr, red, green, blue);
    }
    return color;
}


WMColor*
WMRetainColor(WMColor *color)
{
    assert(color!=NULL);
    
    color->refCount++;
    
    return color;
}


void
WMReleaseColor(WMScreen *scr, WMColor *color)
{
    color->refCount--;
    
    if (color->refCount < 1) {
	XFreeColors(scr->display, scr->colormap, &(color->color.pixel), 1, 0);
	XFreeGC(scr->display, color->gc);
	free(color);
    }
}


void
WMPaintColorRectangle(WMScreen *scr, WMColor *color, Drawable d, int x, int y,
		      unsigned int width, unsigned int height)
{
    XFillRectangle(scr->display, d, color->gc, x, y, width, height);
}


WMPixel
WMColorPixel(WMColor *color)
{
    return color->color.pixel;
}


GC
WMColorGC(WMColor *color)
{
    return color->gc;
}


/* "system" colors */
WMColor*
WMWhiteColor(WMScreen *scr)
{
    if (!scr->white) {
	scr->white = WMCreateRGBColor(scr, 0xffff, 0xffff, 0xffff, True);
	if (!scr->white->flags.exact)
	    wwarning("could not allocate %s color", "white");
    }
    return WMRetainColor(scr->white);
}



WMColor*
WMBlackColor(WMScreen *scr)
{
    if (!scr->black) {
	scr->black = WMCreateRGBColor(scr, 0, 0, 0, True);
	if (!scr->black->flags.exact)
	    wwarning("could not allocate %s color", "black");
    }
    return WMRetainColor(scr->black);
}



WMColor*
WMGrayColor(WMScreen *scr)
{
    if (!scr->gray) {
	WMColor *color;
	
	if (scr->depth == 1) {
	    Pixmap stipple;
	    WMColor *white = WMWhiteColor(scr);
	    WMColor *black = WMBlackColor(scr);
	    XGCValues gcv;

	    stipple = XCreateBitmapFromData(scr->display, scr->rootWin, 
					LIGHT_STIPPLE_BITS, LIGHT_STIPPLE_WIDTH,
					LIGHT_STIPPLE_HEIGHT);

	    color = createRGBColor(scr, 0xffff, 0xffff, 0xffff);
	    XFreeGC(scr->display, color->gc);

	    gcv.foreground = white->color.pixel;
	    gcv.background = black->color.pixel;
	    gcv.fill_style = FillStippled;
	    gcv.stipple = stipple;
	    color->gc = XCreateGC(scr->display, scr->rootWin, GCForeground
				  |GCBackground|GCStipple|GCFillStyle
				  |GCGraphicsExposures, &gcv);

	    XFreePixmap(scr->display, stipple);
	    WMReleaseColor(scr, white);
	    WMReleaseColor(scr, black);
	} else {
	    color = WMCreateRGBColor(scr, 0xaeba, 0xaaaa, 0xaeba, True);
	    if (!color->flags.exact)
		wwarning("could not allocate %s color", "gray");
	}
	scr->gray = color;
    }
    return WMRetainColor(scr->gray);
}



WMColor*
WMDarkGrayColor(WMScreen *scr)
{
    if (!scr->darkGray) {
	WMColor *color;
	
	if (scr->depth == 1) {
	    Pixmap stipple;
	    WMColor *white = WMWhiteColor(scr);
	    WMColor *black = WMBlackColor(scr);
	    XGCValues gcv;

	    stipple = XCreateBitmapFromData(scr->display, scr->rootWin, 
					  DARK_STIPPLE_BITS, DARK_STIPPLE_WIDTH,
					  DARK_STIPPLE_HEIGHT);
	
	    color = createRGBColor(scr, 0, 0, 0);
	    XFreeGC(scr->display, color->gc);

	    gcv.foreground = white->color.pixel;
	    gcv.background = black->color.pixel;
	    gcv.fill_style = FillStippled;
	    gcv.stipple = stipple;
	    color->gc = XCreateGC(scr->display, scr->rootWin, GCForeground
				  |GCBackground|GCStipple|GCFillStyle
				  |GCGraphicsExposures, &gcv);

	    XFreePixmap(scr->display, stipple);
	    WMReleaseColor(scr, white);
	    WMReleaseColor(scr, black);
	} else {
	    color = WMCreateRGBColor(scr, 0x5144, 0x5555, 0x5144, True);
	    if (!color->flags.exact)
		wwarning("could not allocate %s color", "dark gray");
	}
	scr->darkGray = color;
    }
    return WMRetainColor(scr->darkGray);
}

