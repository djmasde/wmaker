
#include "WINGsP.h"

#include <wraster.h>
#include <ctype.h>


void
W_DrawRelief(W_Screen *scr, Drawable d, int x, int y, unsigned int width,
	     unsigned int height, WMReliefType relief)
{
    Display *dpy = scr->display;
    GC bgc;
    GC wgc;
    GC lgc;
    GC dgc;
    

    switch (relief) {
     case WRSimple:
	XDrawRectangle(scr->display, d, W_GC(scr->black), x, y, width, height);
	return;
	break;
	
     case WRRaised:
	bgc = W_GC(scr->black);
	dgc = W_GC(scr->darkGray);
	wgc = W_GC(scr->white);
	lgc = W_GC(scr->gray);
	break;
	
     case WRSunken:
	wgc = W_GC(scr->darkGray);
	lgc = W_GC(scr->black);
	bgc = W_GC(scr->white);
	dgc = W_GC(scr->gray);
	break;
	
     case WRRidge:
	lgc = bgc = W_GC(scr->darkGray);
	dgc = wgc = W_GC(scr->white);
	break;

     case WRGroove:
	wgc = dgc = W_GC(scr->darkGray);
	lgc = bgc = W_GC(scr->white);
	break;
	
     default:
	return;
    }
    /* top left */
    XDrawLine(dpy, d, wgc, x, y, x+width-1, y);
    if (width > 2 && relief != WRRaised) {
	XDrawLine(dpy, d, lgc, x+1, y+1, x+width-3, y+1);
    }
    
    XDrawLine(dpy, d, wgc, x, y, x, y+height-1);
    if (height > 2 && relief != WRRaised) {
	XDrawLine(dpy, d, lgc, x+1, y+1, x+1, y+height-3);
    }
    
    /* bottom right */
    XDrawLine(dpy, d, bgc, x, y+height-1, x+width-1, y+height-1);
    if (width > 2) {
	XDrawLine(dpy, d, dgc, x+1, y+height-2, x+width-2, y+height-2);
    }

    XDrawLine(dpy, d, bgc, x+width-1, y, x+width-1, y+height-1);
    if (height > 2) {
	XDrawLine(dpy, d, dgc, x+width-2, y+1, x+width-2, y+height-3);
    }
}



WMPixmap*
WMRetainPixmap(WMPixmap *pixmap)
{
    if (pixmap)
	pixmap->refCount++;

    return pixmap;
}


void
WMReleasePixmap(WMPixmap *pixmap)
{
    pixmap->refCount--;

    if (pixmap->refCount<1) {
	XFreePixmap(pixmap->screen->display, pixmap->pixmap);
	if (pixmap->mask)
	    XFreePixmap(pixmap->screen->display, pixmap->mask);
	free(pixmap);
    }
}


WMPixmap*
WMCreatePixmapFromXPixmaps(WMScreen *scrPtr, Pixmap pixmap, Pixmap mask,
			   int width, int height, int depth)
{
    WMPixmap *pixPtr;

    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->pixmap = pixmap;
    pixPtr->mask = mask;
    pixPtr->width = width;
    pixPtr->height = height;
    pixPtr->depth = depth;
    pixPtr->refCount = 1;
    
    return pixPtr;
}




WMPixmap*
WMCreatePixmapFromFile(WMScreen *scrPtr, char *fileName)
{
    WMPixmap *pixPtr;
    RImage *image;
    
    image = RLoadImage(scrPtr->rcontext, fileName, 0);
    if (!image)
	return NULL;

    pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 127);

    RDestroyImage(image);
    
    return pixPtr;
}


WMPixmap*
WMCreatePixmapFromRImage(WMScreen *scrPtr, RImage *image, int threshold)
{
    WMPixmap *pixPtr;
    Pixmap pixmap, mask;

    if (!RConvertImageMask(scrPtr->rcontext, image, &pixmap, &mask, 
			   threshold)) {
	return NULL;
    }
    
    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->pixmap = pixmap;
    pixPtr->mask = mask;
    pixPtr->width = image->width;
    pixPtr->height = image->height;
    pixPtr->depth = scrPtr->depth;
    pixPtr->refCount = 1;

    return pixPtr;    
}


WMPixmap*
WMCreateBlendedPixmapFromFile(WMScreen *scrPtr, char *fileName, RColor *color)
{
    WMPixmap *pixPtr;
    RImage *image;

    
    image = RLoadImage(scrPtr->rcontext, fileName, 0);
    if (!image)
	return NULL;
    
    RCombineImageWithColor(image, color);

    pixPtr = WMCreatePixmapFromRImage(scrPtr, image, 0);
    
    RDestroyImage(image);
    
    return pixPtr;
}


static int
fitText(char *text, WMFont *font, int width, int wrap)
{
    int i, j;
    int w;

    if (text[0]==0)
	return 0;
    i = 0;
    if (wrap) {
	do {
	    i++;
	    w = XTextWidth(font->font, text, i);
	} while (w < width && text[i]!='\n' && text[i]!=0);

	/* keep words complete */
	if (!isspace(text[i])) {
	    j = i;
	    while (j>1 && !isspace(text[j]) && text[j]!=0)
		j--;
	    if (j>1)
		i = j;
	}
    } else {
	while (text[i]!='\n' && text[i]!=0)
	    i++;
    }

    return i;
}


int
W_GetTextHeight(WMFont *font, char *text, int width, int wrap)
{
    char *ptr = text;
    int count;
    int length = strlen(text);
    int h;

    h = 0;
    while (length > 0) {
	count = fitText(ptr, font, width-4, wrap);

	h += font->height;

	if (isspace(ptr[count]))
	    count++;

	ptr += count;
	length -= count;
    }
    return h;
}


void
W_PaintText(W_View *view, Drawable d, WMFont *font,  int x, int y,
	    int width, WMAlignment alignment, GC gc,
	    int wrap, char *text, int length)
{
    char *ptr = text;
    int line_width;
    int line_x;
    int count;

    y += font->y;
    while (length > 0) {
	count = fitText(ptr, font, width-4, wrap);

	line_width = XTextWidth(font->font, ptr, count);

	if (alignment==WALeft)
	    line_x = x + 2;
	else if (alignment==WARight)
	    line_x = x + width - line_width - 2;
	else
	    line_x = x + (width - line_width) / 2;

	XDrawString(view->screen->display, d, gc, line_x, y, ptr, count);

	y += font->height;

	if (isspace(ptr[count]))
	    count++;
	
	ptr += count;
	length -= count;
    }
}




void
W_PaintTextAndImage(W_View *view, int wrap, GC textGC, W_Font *font,
		    WMReliefType relief, char *text, int textHeight,
		    WMAlignment alignment,  W_Pixmap *image, 
		    WMImagePosition position, GC backGC, int ofs)
{
    W_Screen *screen = view->screen;
    int ix, iy;
    int x, y, w, h;
    Drawable d = view->window;

    
#ifdef DOUBLE_BUFFER
    d = XCreatePixmap(screen->display, view->window, 
			   view->size.width, view->size.height, screen->depth);
#endif
    
    /* background */
#ifndef DOUBLE_BUFFER
    if (backGC) {
	XFillRectangle(screen->display, d, backGC,
		       0, 0, view->size.width, view->size.height);
    } else {
	XClearWindow(screen->display, d);
    }
#else
    if (backGC)
	XFillRectangle(screen->display, d, backGC, 0, 0,
		       view->size.width, view->size.height);
    else {
	XSetForeground(screen->display, screen->copyGC, 
		       view->attribs.background_pixel);
	XFillRectangle(screen->display, d, screen->copyGC, 0, 0,
		       view->size.width, view->size.height);
    }
#endif

    
    if (relief == WRFlat) {
	x = 0;
	y = 0;
	w = view->size.width;
	h = view->size.height;
    } else {
	x = 2;
	y = 2;
	w = view->size.width - 4;
	h = view->size.height - 4;	
    }

    /* calc. image alignment */
    if (position!=WIPNoImage && image!=NULL) {
	switch (position) {
	 case WIPOverlaps:
	 case WIPImageOnly:
	    ix = (view->size.width - image->width) / 2;
	    iy = (view->size.height - image->height) / 2;
	    x = 2;
	    y = 0;
	    break;
	    
	 case WIPLeft:
	    ix = x;
	    iy = y + (h - image->height) / 2;
	    x = image->width + 5;
	    y = 0;
	    w -= image->width;
	    break;
	    
	 case WIPRight:
	    ix = w - image->width;
	    iy = y + (h - image->height) / 2;
	    w -= image->width + 5;
	    break;
	    
	 case WIPBelow:
	    ix = (view->size.width - image->width) / 2;
	    iy = h - image->height;
	    y = 0;
	    h -= image->height;
	    break;
	    
	 default:
	 case WIPAbove:
	    ix = (view->size.width - image->width) / 2;
	    iy = y;
	    y = image->height;
	    h -= image->height;
	    break;
	}
	
	ix += ofs;
	iy += ofs;
    
	XSetClipOrigin(screen->display, screen->clipGC, ix, iy);
	XSetClipMask(screen->display, screen->clipGC, image->mask);
	if (image->depth==1)
	    XCopyPlane(screen->display, image->pixmap, d, screen->clipGC,
		       0, 0, image->width, image->height, ix, iy, 1);
	else
	    XCopyArea(screen->display, image->pixmap, d, screen->clipGC, 
		      0, 0, image->width, image->height, ix, iy);
    }

    /* draw text */
    if (position != WIPImageOnly && text!=NULL) {	
	W_PaintText(view, d, font, x+ofs, y+ofs + (h-textHeight)/2, w,
		    alignment, textGC, wrap, text, strlen(text));
    }
    
    
    /* draw relief */
    W_DrawRelief(screen, d, 0, 0, view->size.width, view->size.height, relief);
    
#ifdef DOUBLE_BUFFER
    XCopyArea(screen->display, d, view->window, screen->copyGC, 0, 0,
	      view->size.width, view->size.height, 0, 0);
    XFreePixmap(screen->display, d);
#endif
}



WMSize 
WMGetPixmapSize(WMPixmap *pixmap)
{
    WMSize size;
    
    size.width = pixmap->width;
    size.height = pixmap->height;
    
    return size;
}
