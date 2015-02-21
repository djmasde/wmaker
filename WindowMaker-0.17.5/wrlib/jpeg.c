/* jpeg.c - load JPEG image from file
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


#ifdef USE_JPEG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <jpeglib.h>

#include "wraster.h"


RImage*
RLoadJPEG(RContext *context, char *file_name, int index)
{
    RImage *image = NULL;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    int i, j;
    unsigned char *r, *g, *b;
    JSAMPROW buffer[1];
    FILE *file;
    
    RErrorString[0] = 0;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    file = fopen(file_name, "r");
    if (!file) {
	sprintf(RErrorString, "could not open JPEG file \"%s\"", file_name);
	return NULL;
    }
    jpeg_stdio_src(&cinfo, file);
  
    jpeg_read_header(&cinfo, TRUE);

    buffer[0] = (JSAMPROW)alloca(cinfo.image_width*cinfo.num_components);
    if (!buffer) {
	sprintf(RErrorString, "out of memory");
	goto bye;
    }

    image = RCreateImage(cinfo.image_width, cinfo.image_height, False);
    if (!image) {
	sprintf(RErrorString, "out of memory");
	goto bye;
    }

    cinfo.out_color_space = JCS_RGB;
    cinfo.quantize_colors = FALSE;
    cinfo.do_fancy_upsampling = FALSE;
    cinfo.do_block_smoothing = FALSE;

    jpeg_start_decompress(&cinfo);

    r = image->data[0];
    g = image->data[1];
    b = image->data[2];

    while (cinfo.output_scanline < cinfo.image_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
	for (i=0,j=0; i<cinfo.image_width; i++) {
	    *(r++) = buffer[0][j++];
	    *(g++) = buffer[0][j++];
	    *(b++) = buffer[0][j++];
	}
    }

    jpeg_finish_decompress(&cinfo);

  bye:
    jpeg_destroy_decompress(&cinfo);

    fclose(file);

    return image;
}

#endif /* USE_JPEG */
