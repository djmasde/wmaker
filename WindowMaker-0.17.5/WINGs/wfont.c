
#include "WINGsP.h"

#include <wraster.h>



#define SYSTEM_FONT "-*-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*"

#define BOLD_SYSTEM_FONT "-*-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*"



WMFont*
WMCreateFont(WMScreen *scrPtr, char *font_name)
{
    WMFont *font;
    Display *display = scrPtr->display;
    
    font = malloc(sizeof(WMFont));
    if (!font)
      return NULL;
    
    font->screen = scrPtr;

    font->font = XLoadQueryFont(display, font_name);
    if (!font->font) {
        wwarning("could not load font %s. Trying fixed", font_name);
        font->font = XLoadQueryFont(display, "fixed");
        if (!font->font) {
            free(font);
            return NULL;
        }
    }
    font->height = font->font->ascent+font->font->descent;
    font->y = font->font->ascent;
    
    font->refCount = 1;
    
    return font;
}


WMFont*
WMRetainFont(WMFont *font)
{
    if (font!=NULL)
	font->refCount++;
    return font;
}


void 
WMReleaseFont(WMFont *font)
{
    font->refCount--;
    if (font->refCount < 1) {
	XFreeFont(font->screen->display, font->font);
	free(font);
    }
}


int
W_TextWidth(WMFont *font, char *text, int length)
{
    return XTextWidth(font->font, text, length); 
}



WMFont*
WMSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;
    
    fontSpec = wmalloc(strlen(SYSTEM_FONT)+16);
	
    sprintf(fontSpec, SYSTEM_FONT, size);

    font = WMCreateFont(scrPtr, fontSpec);
	
    free(fontSpec);
    
    return font;
}


WMFont*
WMBoldSystemFontOfSize(WMScreen *scrPtr, int size)
{
    WMFont *font;
    char *fontSpec;
    
    fontSpec = wmalloc(strlen(BOLD_SYSTEM_FONT)+16);
    
    sprintf(fontSpec, BOLD_SYSTEM_FONT, size);
    
    font = WMCreateFont(scrPtr, fontSpec);
    
    free(fontSpec);
    
    return font;
}


