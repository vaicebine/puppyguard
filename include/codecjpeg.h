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
#ifndef __CODEC_JPEG_H
#define __CODEC_JPEG_H
#include <jpeglib.h>

#define OUTPUT_BUF_SIZE  4096

/* Expanded data destination object for memory output */
typedef struct
{
    struct jpeg_destination_mgr pub; /* public fields */

    uint8_t     **outbuffer; /* target buffer */
    uint32_t    *outsize;
    uint8_t     *newbuffer;  /* newly allocated buffer */
    JOCTET      *buffer;     /* start of buffer */
    size_t      bufsize;
} my_buf_destination_mgr;

typedef my_buf_destination_mgr *my_buf_dest_ptr;

typedef struct 
{
    uint32_t width;
    uint32_t height;
    int32_t  quality;
} jpeg_settings_t;

#endif /*__CODEC_JPEG_H*/
