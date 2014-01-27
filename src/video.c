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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <syslog.h>

#include <video.h>
#include <v4l2.h>
#include <codec.h>
#include <http.h>
#include <iofd.h>
#include <cfg.h>
#include <am.h>


static v4l2_dev_t *v4l2_dev = NULL;
static motion_settings_t motion_settings;


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


static void send_motion_alert(time_t time, 
                              uint8_t *jpeg_frame, 
                              uint32_t jpeg_size)
{
    struct tm *tmp = localtime(&time);
    char filename[64];
    char cmd[256];
    int fd = -1;

	strftime(filename, sizeof(filename), "/tmp/%d_%m_%Y_%H_%M_%S.jpg", tmp);

    fd = creat(filename, (S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH));
    if (fd < 0)
        return;
    write(fd, jpeg_frame, jpeg_size);
    close(fd);
    syslog(LOG_WARNING, 
           "Motion detected, send email alert to %s", 
           motion_settings.smtp_to);
    if(fork() == 0)
    {
        snprintf(cmd, 
                 sizeof(cmd),
                 "puppyguard-mail.py %s %s %s %s %s %s %d %s %s",
                 (motion_settings.smtp_user) ? motion_settings.smtp_user : "0", 
                 (motion_settings.smtp_pass) ? motion_settings.smtp_pass : "0",
                 motion_settings.smtp_from,
                 motion_settings.smtp_to,
                 (motion_settings.smtp_cc) ? motion_settings.smtp_cc : "0",  
                 motion_settings.smtp_server, 
                 motion_settings.smtp_port, 
                 (motion_settings.smtp_tls) ? "1" : "0",
                 filename);
        system(cmd);
        exit(0);
    }
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
    static uint8_t motion = 0xff;

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

    if ((http_server->video_clients && (!drop)) ||
        motion_settings.enable)
    {
        if ((video_size = capture_video_frame(&video_frame)) > 0)
        {
            if (motion_settings.enable)
            {
                if (am_motion_detection(video_frame, 
                                        video_size, 
                                        motion_settings.threshold))
                {
                    if (!motion)
                        motion = 0x0f;
                }
                else
                {
                    motion = 0;
                }
            }

            if (http_server->video_clients || (motion == 0x0f))
            {
                codec_jpeg_compress(video_frame,
                                    video_size, 
                                    &jpeg_frame, 
                                    &jpeg_size);
                if (motion == 0x0f)
                {
                    send_motion_alert(t2->tv_sec, jpeg_frame, jpeg_size);
                    motion = 0xff;
                }

                if (http_server->video_clients)
                {
                    http_send_jpeg_frame(jpeg_frame, jpeg_size);
                }
            }
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
    cfg_get_value("motion_enable", &motion_settings.enable, CFG_VALUE_BOOL);
    if (motion_settings.enable)
    {
        cfg_get_value("motion_threshold", 
                      &motion_settings.threshold, 
                      CFG_VALUE_INT32);
        cfg_get_value("motion_smtp_user", 
                      &motion_settings.smtp_user, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_pass", 
                      &motion_settings.smtp_pass, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_from", 
                      &motion_settings.smtp_from, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_to", 
                      &motion_settings.smtp_to, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_cc", 
                      &motion_settings.smtp_cc, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_server", 
                      &motion_settings.smtp_server, 
                      CFG_VALUE_STR);
        cfg_get_value("motion_smtp_port", 
                      &motion_settings.smtp_port, 
                      CFG_VALUE_INT16);
        cfg_get_value("motion_smtp_tls", 
                      &motion_settings.smtp_tls, 
                      CFG_VALUE_BOOL);
    }

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
