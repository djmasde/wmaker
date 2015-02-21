/*
 * Stuff that I haven't decided what to do yet.
 */

#include "WINGsP.h"

WMPixmap*
WMCreatePixmapFromXPMData(WMScreen *scrPtr, char **xpmData)
{
    WMPixmap *pixPtr;
    RImage *image;
    Pixmap pixmap, mask;

    
    image = RGetImageFromXPMData(scrPtr->rcontext, xpmData);
    if (!image)
	return NULL;
    
    if (!RConvertImageMask(scrPtr->rcontext, image, &pixmap, &mask, 128)) {
	RDestroyImage(image);
	return NULL;
    }
    
    pixPtr = malloc(sizeof(WMPixmap));
    if (!pixPtr) {
	RDestroyImage(image);
	return NULL;
    }
    pixPtr->screen = scrPtr;
    pixPtr->pixmap = pixmap;
    pixPtr->mask = mask;
    pixPtr->width = image->width;
    pixPtr->height = image->height;
    pixPtr->depth = scrPtr->depth;
    pixPtr->refCount = 1;

    RDestroyImage(image);
    
    return pixPtr;
}
