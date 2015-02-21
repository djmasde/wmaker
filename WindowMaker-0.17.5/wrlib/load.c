/* load.c - load image from file
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#ifdef USE_PNG
#include <png.h>
#endif

#include "wraster.h"


typedef struct RCachedImage {
    RImage 	*image;
    char 	*file;
    time_t 	last_modif;	       /* last time file was modified */
    time_t	last_use;	       /* last time image was used */
} RCachedImage;


/*
 * Size of image cache
 */
static int RImageCacheSize = -1;

/*
 * Max. size of image to store in cache
 */
static int RImageCacheMaxImage = -1; /* 0 = any size */

#define IMAGE_CACHE_SIZE	8

#define IMAGE_CACHE_MAX_IMAGE	64*64

static RCachedImage *RImageCache;





#define IM_ERROR	-1
#define IM_UNKNOWN	0
#define IM_XPM		1
#define IM_TIFF 	2
#define IM_PNG		3
#define IM_PPM		4
#define IM_JPEG		5

static int identFile(char *path);

extern RImage *RLoadPPM(RContext *context, char *file_name, int index);

extern RImage *RLoadXPM(RContext *context, char *file, int index);


#ifdef USE_TIFF
extern RImage *RLoadTIFF(RContext *context, char *file, int index);
#endif
#ifdef USE_PNG
extern RImage *RLoadPNG(RContext *context, char *file, int index);
#endif
#ifdef USE_JPEG
extern RImage *RLoadJPEG(RContext *context, char *file_name, int index);
#endif


static void
init_cache()
{
    char *tmp;
    
    tmp = getenv("RIMAGE_CACHE");
    if (!tmp || sscanf(tmp, "%i", &RImageCacheSize)!=1) {
	RImageCacheSize = IMAGE_CACHE_SIZE;
    }
    if (RImageCacheSize<0)
	RImageCacheSize = 0;
    
    tmp = getenv("RIMAGE_CACHE_SIZE");
    if (!tmp || sscanf(tmp, "%i", &RImageCacheMaxImage)!=1) {
	RImageCacheMaxImage = IMAGE_CACHE_MAX_IMAGE;
    }

    if (RImageCacheSize>0) {
	RImageCache = malloc(sizeof(RCachedImage)*RImageCacheSize);
	if (RImageCache==NULL) {
	    printf("wrlib: out of memory for image cache\n");
	    return;
	}
	memset(RImageCache, 0, sizeof(RCachedImage)*RImageCacheSize);
    }
}


RImage*
RLoadImage(RContext *context, char *file, int index)
{
    RImage *image = NULL;
    int i;
    struct stat st;
    
    RErrorString[0] = 0;
    
    if (RImageCacheSize<0) {
	init_cache();
    }
    
    if (RImageCacheSize>0) {
	
	for (i=0; i<RImageCacheSize; i++) {
	    if (RImageCache[i].file
		&& strcmp(file, RImageCache[i].file)==0) {
	
		if (stat(file, &st)==0 
		    && st.st_mtime == RImageCache[i].last_modif) {
		    RImageCache[i].last_use = time(NULL);

		    return RCloneImage(RImageCache[i].image);
		    
		} else {
		    free(RImageCache[i].file);
		    RImageCache[i].file = NULL;
		    RDestroyImage(RImageCache[i].image);
		}
	    }
	}
    }

    switch (identFile(file)) {
     case IM_ERROR:
	sprintf(RErrorString, "error opening file");
	return NULL;

     case IM_UNKNOWN:
	sprintf(RErrorString, "unknown image format");
	return NULL;

     case IM_XPM:
	image = RLoadXPM(context, file, index);
	break;
	
#ifdef USE_TIFF
     case IM_TIFF:
	image = RLoadTIFF(context, file, index);
	break;
#endif /* USE_TIFF */

#ifdef USE_PNG
     case IM_PNG:
	image = RLoadPNG(context, file, index);
	break;
#endif /* USE_PNG */

#ifdef USE_JPEG
     case IM_JPEG:
	image = RLoadJPEG(context, file, index);
	break;
#endif /* USE_JPEG */

     case IM_PPM:
	image = RLoadPPM(context, file, index);
	break;

     default:
	sprintf(RErrorString, "unsupported image format");
	return NULL;
    }
    

    /* store image in cache */
    if (RImageCacheSize>0 && image && 
	(RImageCacheMaxImage==0 
	 || RImageCacheMaxImage >= image->width*image->height)) {
	time_t oldest=time(NULL);
	int oldest_idx = 0;
	int done = 0;
	
	for (i=0; i<RImageCacheSize; i++) {
	    if (!RImageCache[i].file) {
		RImageCache[i].file = malloc(strlen(file)+1);
		strcpy(RImageCache[i].file, file);
		RImageCache[i].image = RCloneImage(image);
		RImageCache[i].last_modif = st.st_mtime;
		RImageCache[i].last_use = time(NULL);
		done = 1;
		break;
	    } else {
		if (oldest > RImageCache[i].last_use) {
		    oldest = RImageCache[i].last_use;
		    oldest_idx = i;
		}
	    }
	}
	
	/* if no slot available, dump least recently used one */
	if (!done) {
	    free(RImageCache[oldest_idx].file);
	    RDestroyImage(RImageCache[oldest_idx].image);
	    RImageCache[oldest_idx].file = malloc(strlen(file)+1);
	    strcpy(RImageCache[oldest_idx].file, file);
	    RImageCache[oldest_idx].image = RCloneImage(image);
	    RImageCache[oldest_idx].last_modif = st.st_mtime;
	    RImageCache[oldest_idx].last_use = time(NULL);
	}
    }

    return image;
}





static int
identFile(char *path)
{
    int fd;
    unsigned char buffer[32];

    if (!path) 
	return IM_ERROR;

    fd = open(path, O_RDONLY);
    if (fd < 0)
	return IM_ERROR;
    if (read(fd, buffer, 32)<1) {
	close(fd);
	return IM_ERROR;
    }
    close(fd);
    
    /* check for XPM */
    if (strncmp((char*)buffer, "/* XPM */", 9)==0) 
	return IM_XPM;
    
    /* check for TIFF */
    if ((buffer[0]=='I' && buffer[1]=='I' && buffer[2]=='*' && buffer[3]==0)
	||(buffer[0]=='M' && buffer[1]=='M' && buffer[2]==0 && buffer[3]=='*'))
	return IM_TIFF;

#ifdef USE_PNG
    /* check for PNG */
    if (png_check_sig(buffer, 8))
	return IM_PNG;
#endif
    
    /* check for PPM */
    if (buffer[0]=='P' && (buffer[1]=='2' || buffer[1]=='3' || buffer[1]=='5'
			   || buffer[1]=='6'))
	return IM_PPM;

    /* check for JPEG */
    if (buffer[0] == 0xff && buffer[1] == 0xd8)
	return IM_JPEG;

    return IM_UNKNOWN;
}



