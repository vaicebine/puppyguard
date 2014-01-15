/*
 *    Copyright (c) 2013, vaicebine@gmail.com. All rights reserved.
 *                                                                       
 *   This program is free software; you can redistribute it and/or modify  
 *   it under the terms of the GNU General Public License as published by  
 *   the Free Software Foundation; either version 2 of the License, or     
 *   (at your option) any later version.                                   
 *                                                                         
 *   This program is distributed in the hope that it will be useful,       
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 *   GNU General Public License for more details.                          
 *                                                                         
 *   You should have received a copy of the GNU General Public License     
 *   along with this program; if not, write to the                         
 *   Free Software Foundation, Inc.,                                       
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <codecjpeg.h>
#include <jerror.h>
#include <v4l2.h>
#include <iofd.h>

static jpeg_settings_t jpeg_settings;

METHODDEF(void)
init_buf_destination (j_compress_ptr cinfo)
{
    if (!cinfo)
        return;
    /* no work necessary here */
}

METHODDEF(boolean)
empty_buf_output_buffer (j_compress_ptr cinfo)
{
    size_t nextsize;
    JOCTET * nextbuffer;
    my_buf_dest_ptr dest = (my_buf_dest_ptr) cinfo->dest;

    /* Try to allocate new buffer with double size */
    nextsize = dest->bufsize * 2;
    nextbuffer = (JOCTET *) malloc(nextsize);

    if (nextbuffer == NULL)
        ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);

    memcpy(nextbuffer, dest->buffer, dest->bufsize);

    if (dest->newbuffer != NULL)
        free(dest->newbuffer);

    dest->newbuffer = nextbuffer;

    dest->pub.next_output_byte = nextbuffer + dest->bufsize;
    dest->pub.free_in_buffer = dest->bufsize;

    dest->buffer = nextbuffer;
    dest->bufsize = nextsize;

    return TRUE;
}

METHODDEF(void)
term_buf_destination (j_compress_ptr cinfo)
{
    my_buf_dest_ptr dest = (my_buf_dest_ptr) cinfo->dest;

    *dest->outbuffer = dest->buffer;
    *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}



GLOBAL(void)
jpeg_buf_dest (j_compress_ptr cinfo,
               uint8_t ** outbuffer, uint32_t * outsize)
{
    my_buf_dest_ptr dest;

    if (outbuffer == NULL || outsize == NULL) /* sanity check */
        ERREXIT(cinfo, JERR_BUFFER_SIZE);

    /* The destination object is made permanent so that multiple JPEG images
     * can be written to the same buffer without re-executing jpeg_buf_dest.
     */
    if (cinfo->dest == NULL)
    {    /* first time for this JPEG object? */
        cinfo->dest = (struct jpeg_destination_mgr *)
                      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                                  sizeof(my_buf_destination_mgr));
    }

    dest = (my_buf_dest_ptr) cinfo->dest;
    dest->pub.init_destination = init_buf_destination;
    dest->pub.empty_output_buffer = empty_buf_output_buffer;
    dest->pub.term_destination = term_buf_destination;
    dest->outbuffer = outbuffer;
    dest->outsize = outsize;
    dest->newbuffer = NULL;

    if (*outbuffer == NULL || *outsize == 0)
    {
        /* Allocate initial buffer */
        dest->newbuffer = *outbuffer = (unsigned char *) malloc(OUTPUT_BUF_SIZE);
        if (dest->newbuffer == NULL)
            ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
        *outsize = OUTPUT_BUF_SIZE;
    }

    dest->pub.next_output_byte = dest->buffer = *outbuffer;
    dest->pub.free_in_buffer = dest->bufsize = *outsize;
}

void codec_jpeg_compress(uint8_t *frame, uint32_t size, uint8_t **outbuf,
                         uint32_t *outsize) 
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_ptr[1];
    int32_t row_stride;

    FIX_UNUSED_ARG(size);

    *outbuf = NULL;
    *outsize = 0;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    jpeg_buf_dest(&cinfo, outbuf, (uint32_t *)outsize);

    cinfo.image_width = jpeg_settings.width;
    cinfo.image_height = jpeg_settings.height;
    cinfo.input_components = 3;
//    cinfo.in_color_space = JCS_YCbCr;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality (&cinfo, jpeg_settings.quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = jpeg_settings.width * 3;

    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_ptr[0] = &frame[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
}

int8_t codec_jpeg_get_v4l2_color_format(void)
{
    return V4L2_RGB_FMT;
}

void codec_jpeg_init(uint32_t width, uint32_t height, int32_t quality)
{
    jpeg_settings.width = width;
    jpeg_settings.height = height;
    jpeg_settings.quality = quality;
}

void codec_jpeg_clean(void)
{
    jpeg_settings.width = 0;
    jpeg_settings.height = 0;
    jpeg_settings.quality = 0;
}
