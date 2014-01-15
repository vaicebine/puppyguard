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
#include <stdlib.h>
#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <video.h>
#include <v4l2.h>
#include <codec.h>
#include <http.h>
#include <iofd.h>
#include <cfg.h>
#include <am.h>


static v4l2_dev_t *v4l2_dev = NULL;

static int32_t video_fd_events_cb(int32_t fd, 
                                  uint16_t revents, 
                                  struct timeval *t2, 
                                  void *priv);
static uint32_t capture_video_frame(uint8_t **frame);
static void drop_video_frame(void);
static int8_t get_video_fps(void);
static int timeval_subtract(struct timeval *result, 
                            struct timeval *x, 
                            struct timeval *y);

static int timeval_subtract(struct timeval *result, 
                            struct timeval *x, 
                            struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec)
    {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000)
    {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

static int32_t video_fd_events_cb(int32_t fd, 
                                  uint16_t revents, 
                                  struct timeval *t2, 
                                  void *priv)
{
    uint8_t *video_frame = NULL;
    uint8_t *jpeg_frame = NULL;
    uint32_t video_size = 0;
    uint32_t jpeg_size = 0;
    uint8_t drop = 0;
    const http_server_t *http_server = http_get_server();
    static struct timeval t1;
    struct timeval delta;


    FIX_UNUSED_ARG(fd);
    FIX_UNUSED_ARG(revents);
    FIX_UNUSED_ARG(priv);

    /* software frame rate */
    if (t1.tv_sec || t1.tv_usec)
    {
        timeval_subtract(&delta, t2, &t1);
        if (((delta.tv_usec/1000) < (1000/get_video_fps())) &&
            !delta.tv_sec)
        {
            drop++;
        }
        else
        {
            t1.tv_sec = t2->tv_sec;
            t1.tv_usec = t2->tv_usec;
        }
    }
    else
    {
        t1.tv_sec = t2->tv_sec;
        t1.tv_usec = t2->tv_usec;
    }

    if (http_server->video_clients && (!drop))
    {
        if ((video_size = capture_video_frame(&video_frame)) > 0)
        {
            codec_jpeg_compress(video_frame,
                                video_size, 
                                &jpeg_frame, 
                                &jpeg_size);

            http_send_jpeg_frame(jpeg_frame, jpeg_size);
        }

        if (jpeg_frame)
            free(jpeg_frame);

        if (video_frame)
            free(video_frame);
    }
    else
    {
        drop_video_frame();
    }

    return 0;
}

static uint32_t capture_video_frame(uint8_t **frame)
{
    if (v4l2_dev == NULL)
        return 0;

    return v4l2_capture_frame(v4l2_dev, frame);
}

static void drop_video_frame(void)
{
    if (v4l2_dev == NULL)
        return;

    v4l2_drop_frame(v4l2_dev);
}

static int8_t get_video_fps(void)
{
    if (v4l2_dev == NULL)
        return -1;

    return v4l2_get_fps(v4l2_dev);
}

void video_init(void)
{
    uint32_t width = VIDEO_WIDTH;
    uint32_t height = VIDEO_HEIGHT; 
    int8_t fps = VIDEO_FPS;
    char *name = VIDEO_DEVICE; 
    uint8_t enable = 0;
    int8_t format = V4L2_RGB_FMT;
    int32_t quality = VIDEO_JPEG_FMT_QUALITY;

    cfg_get_value("video_enable", &enable, CFG_VALUE_BOOL);
    if (!enable)
        return;

    cfg_get_value("video_device", &name, CFG_VALUE_STR);
    cfg_get_value("video_width", &width, CFG_VALUE_INT32);
    cfg_get_value("video_height", &height, CFG_VALUE_INT32);
    cfg_get_value("video_fps", &fps, CFG_VALUE_INT8);
    cfg_get_value("jpeg_quality", &quality, CFG_VALUE_INT32);

    codec_jpeg_init(width, height, quality);

    format = codec_jpeg_get_v4l2_color_format();

    v4l2_dev = v4l2_open(name, width, height, format, fps);

    io_fd_add(v4l2_dev->fd, IO_FD_VIDEO, POLLIN, video_fd_events_cb, NULL);

    v4l2_capture_start(v4l2_dev);
}

void video_clean(void)
{
    if (v4l2_dev == NULL)
        return;

    v4l2_capture_stop(v4l2_dev);

    v4l2_close(v4l2_dev);

    codec_jpeg_clean();
}


int32_t video_get_width(uint32_t *width)
{
    if (v4l2_dev == NULL || width == NULL)
        return -1;

    *width = v4l2_dev->width;

    return 0;
}

int32_t video_get_height(uint32_t *height)
{
    if (v4l2_dev == NULL || height == NULL)
        return -1;

    *height = v4l2_dev->height;

    return 0;
}
