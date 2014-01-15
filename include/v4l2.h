/*
 *    Copyright (c) 2013, vaicebine@gmail.com. All rights reserved.
 *                                                                 
 *    Part of this file is based on the V4L2 video capture example
 *    (http://linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/v4l2spec/) 
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
#ifndef __V4L2_H
#define __V4L2_H

#define V4L2_CLEAR(x) memset(&(x), 0, sizeof (x))
#define V4L2_YUV420_FRAME_SIZE(w,h) ((w*h*3)/2)
#define V4L2_YUV422_FRAME_SIZE(w,h) (w*h*2)
#define V4L2_YUV_FRAME_SIZE(w,h) (w*h*3)
#define V4L2_RGB_FRAME_SIZE(w,h) (w*h*3)

#define V4L2_YUV420_FMT          1
#define V4L2_JPEG_FMT            2
#define V4L2_RGB_FMT             3
#define V4L2_YUV_FMT             4
#define V4L2_YUV422_FMT          5

struct buffer 
{
    void        *start;
    size_t      length;
};

/* Video 4 Linux 2 camera device */
typedef struct 
{
    const char      *name;      /* device name */
    uint32_t        width;      /* width */
    uint32_t        height;     /* height */
    int8_t          fps;        /* frame rate */
    int32_t         fd;         /* device file descriptor */
    struct buffer   *buffers;   /* internall buffer */
    uint32_t        n_buffers;  /* internall buffer num */
    int8_t          format;     /* frame format */
} v4l2_dev_t;

v4l2_dev_t* v4l2_open(char *name, uint32_t width, uint32_t height, 
                      int8_t format, int8_t fps);

void v4l2_capture_start(v4l2_dev_t *v4l2_dev);

uint32_t v4l2_capture_frame(v4l2_dev_t *v4l2_dev, uint8_t **frame);
void v4l2_drop_frame(v4l2_dev_t *v4l2_dev);
int8_t v4l2_get_fps(v4l2_dev_t *v4l2_dev);

void v4l2_capture_stop(v4l2_dev_t *v4l2_dev);

void v4l2_close(v4l2_dev_t *v4l2_dev);

#endif /*__V4L2_H*/


