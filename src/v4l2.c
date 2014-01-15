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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <syslog.h>
#include <linux/videodev2.h>

#include <v4l2.h>


static void convert_yuv422_to_yuv420(const uint8_t *inbuf, uint8_t *outbuf, 
                                     uint32_t width, uint32_t height)
{
    uint32_t i, j;
    const uint8_t *in;
    const uint8_t *in2;
    uint8_t *out;
    uint8_t *out2;

    /* Write Y plane */
    for (i = 0;i < width * height; ++i)
        outbuf[i] = inbuf[i * 2];

    /* Write UV plane */
    for (j = 0;j < height / 2; ++j)
    {
        in = &(inbuf[j * 2 * width * 2]);
        in2 = &(inbuf[(j * 2 + 1)* width * 2]);
        out = &(outbuf[width * height + j * width / 2]);
        out2 = &(outbuf[width * height * 5 / 4 + j * width / 2]);
        for (i = 0;i < width / 2; ++i)
        {
            out[i] = (in[i * 4 + 1] + in2[i * 4 + 1]) / 2;
            out2[i] = (in[i * 4 + 3] + in2[i * 4 + 3]) / 2;
        }
    }
}

static void convert_yuv422_to_yuv(const uint8_t *inbuf, uint8_t *outbuf, 
                                  uint32_t width, uint32_t height)
{
    uint32_t i, j;
    uint32_t size = (width * height * 2);
    uint8_t y1, u, y2, v;

    for (i = 0, j = 0; i < size; i+=4, j+=6)
    {
        y1 = inbuf[i];
        u  = inbuf[i+1];
        y2 = inbuf[i+2];
        v  = inbuf[i+3];

        outbuf[j]   = y1;
        outbuf[j+1] = u;
        outbuf[j+2] = v;

        outbuf[j+3] = y2;
        outbuf[j+4] = u;
        outbuf[j+5] = v;
    }
}

static void convert_yuv_to_rgb(uint8_t y, uint8_t u, uint8_t v, int32_t *r, 
                               int32_t *g, int32_t *b)
{
    int32_t c, d, e;

    c = y - 16;
    d = u - 128;
    e = v - 128;

    *r = ((298*c + 409*e + 128) >> 8);
    *g = ((298*c - 100*d - 208*e + 128) >> 8); 
    *b = ((298*c + 516*d + 128) >> 8); 

    *r = (*r > 255) ? 255 : (*r < 0) ? 0 : *r;
    *g = (*g > 255) ? 255 : (*g < 0) ? 0 : *g;
    *b = (*b > 255) ? 255 : (*b < 0) ? 0 : *b;
}

static void convert_yuv422_to_rgb(const uint8_t *inbuf, uint8_t *outbuf, 
                                  uint32_t width, uint32_t height)
{
    uint32_t i, j;
    uint32_t size = (width * height * 2);
    uint8_t y1, u, y2, v;
    int32_t r, g, b;

    for (i = 0, j = 0; i < size; i+=4, j+=6)
    {
        y1 = inbuf[i];
        u  = inbuf[i+1];
        y2 = inbuf[i+2];
        v  = inbuf[i+3];

        convert_yuv_to_rgb(y1, u, v, &r, &g, &b);

        outbuf[j]   = r;
        outbuf[j+1] = g;
        outbuf[j+2] = b;

        convert_yuv_to_rgb(y2, u, v, &r, &g, &b);

        outbuf[j+3] = r;
        outbuf[j+4] = g;
        outbuf[j+5] = b;
    }
}

static int32_t xioctl(int32_t fd, int32_t request, void *arg)
{
    int32_t r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

static void init_mmap(v4l2_dev_t *v4l2_dev)
{
    struct v4l2_requestbuffers req;

    V4L2_CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            syslog(LOG_ERR, "%s does not support memory mapping", 
                   v4l2_dev->name);
            exit(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_ERR, "VIDIOC_REQBUFS error %d, %s", 
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (req.count < 2)
    {
        syslog(LOG_ERR, "Insufficient buffer memory on %s",
               v4l2_dev->name);
        exit(EXIT_FAILURE);
    }

    v4l2_dev->buffers = calloc(req.count, sizeof (*v4l2_dev->buffers));

    if (!v4l2_dev->buffers)
    {
        syslog(LOG_ERR, "Out of memory");
        exit(EXIT_FAILURE);
    }

    for (v4l2_dev->n_buffers = 0; v4l2_dev->n_buffers < req.count; ++v4l2_dev->n_buffers)
    {
        struct v4l2_buffer buf;

        V4L2_CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = v4l2_dev->n_buffers;

        if (-1 == xioctl (v4l2_dev->fd, VIDIOC_QUERYBUF, &buf))
        {
            syslog(LOG_ERR, "VIDIOC_QUERYBUF error %d, %s", 
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        v4l2_dev->buffers[v4l2_dev->n_buffers].length = buf.length;
        v4l2_dev->buffers[v4l2_dev->n_buffers].start =
        mmap(NULL /* start anywhere */,
             buf.length,
             PROT_READ | PROT_WRITE /* required */,
             MAP_SHARED /* recommended */,
             v4l2_dev->fd, buf.m.offset);

        if (MAP_FAILED == v4l2_dev->buffers[v4l2_dev->n_buffers].start)
        {
            syslog(LOG_ERR, "mmap() error %d, %s", 
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

v4l2_dev_t* v4l2_open(char *name, uint32_t width, uint32_t height, 
                      int8_t format, int8_t fps)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_streamparm streamparm;
    v4l2_dev_t *v4l2_dev = NULL;
    struct stat st;
    int32_t fd; 
    uint32_t min;

    if (!name || !width || !height)
    {
        syslog(LOG_ERR, "Failed to open device: invalid parameters");
        exit(EXIT_FAILURE);
    }

    v4l2_dev = malloc(sizeof(v4l2_dev_t));
    if (!v4l2_dev)
    {
        syslog(LOG_ERR, "Failed to create device: Out of memory");
        exit(EXIT_FAILURE);
    }

    v4l2_dev->name = name;
    v4l2_dev->width = width;
    v4l2_dev->height = height;
    v4l2_dev->format = format;
    v4l2_dev->fps = fps;

    if (-1 == stat(name, &st))
    {
        syslog(LOG_ERR, "Cannot identify '%s': %d, %s",
               name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        syslog(LOG_ERR, "%s is no device", name);
        exit(EXIT_FAILURE);
    }

    fd = open(name, O_RDWR /* required */ | O_NONBLOCK, 0);
    if (-1 == fd)
    {
        syslog(LOG_ERR, "Cannot open '%s': %d, %s", 
               name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }

    v4l2_dev->fd = fd;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            syslog(LOG_ERR, "%s is no V4L2 device", name);
            exit(EXIT_FAILURE);
        }
        else
        {
            syslog(LOG_ERR, "VIDIOC_QUERYCAP error %d, %s",
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        syslog(LOG_ERR, "%s is no video capture device", name);
        exit(EXIT_FAILURE);
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        syslog(LOG_ERR, "%s does not support streaming i/o", name);
        exit(EXIT_FAILURE);
    }

    V4L2_CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }

    V4L2_CLEAR(fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width; 
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        syslog(LOG_ERR, "VIDIOC_S_FMT error %d, %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    /* Set frame rate */
//	if (cap.capabilities & V4L2_CAP_TIMEPERFRAME)
    {
        memset(&streamparm, 0, sizeof(struct v4l2_streamparm));
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = fps;

        if (-1 == xioctl(fd, VIDIOC_S_PARM, &streamparm))
        {
            syslog(LOG_ERR, "VIDIOC_S_PARM error %d, %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    init_mmap(v4l2_dev);

    return v4l2_dev;
}

void v4l2_capture_start(v4l2_dev_t *v4l2_dev)
{
    enum v4l2_buf_type type;
    uint32_t i;

    for (i = 0; i < v4l2_dev->n_buffers; ++i)
    {
        struct v4l2_buffer buf;

        V4L2_CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        if (-1 == xioctl(v4l2_dev->fd, VIDIOC_QBUF, &buf))
        {
            syslog(LOG_ERR, "VIDIOC_QBUF error %d, %s", 
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_STREAMON, &type))
    {
        syslog(LOG_ERR, "VIDIOC_STREAMON error %d, %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

uint32_t v4l2_capture_frame(v4l2_dev_t *v4l2_dev, uint8_t **frame)
{
    struct v4l2_buffer buf;
    uint32_t size = 0;

    *frame = NULL;

    V4L2_CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_DQBUF, &buf))
    {
        switch (errno)
        {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                syslog(LOG_ERR, "VIDIOC_DQBUF error %d, %s", 
                       errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
    }

    assert(buf.index < v4l2_dev->n_buffers);

    if (V4L2_RGB_FMT == v4l2_dev->format)
    {
        size = V4L2_RGB_FRAME_SIZE(v4l2_dev->width, v4l2_dev->height);
        *frame = malloc(size);
        if (*frame == NULL)
            return 0;

        convert_yuv422_to_rgb(v4l2_dev->buffers[buf.index].start, 
                              *frame, 
                              v4l2_dev->width,
                              v4l2_dev->height);
    }
    else if (V4L2_YUV420_FMT == v4l2_dev->format)
    {
        /* allocate buffer for yuv422 frame format */
        size = V4L2_YUV420_FRAME_SIZE(v4l2_dev->width, v4l2_dev->height);
        *frame = malloc(size);
        if (*frame == NULL)
            return 0;

        /* conversion to yuv420 format */
        convert_yuv422_to_yuv420(v4l2_dev->buffers[buf.index].start, 
                                 *frame, 
                                 v4l2_dev->width,
                                 v4l2_dev->height);
    }
    else if (V4L2_YUV_FMT == v4l2_dev->format)
    {
        /* allocate buffer for yuv422 frame format */
        size = V4L2_YUV_FRAME_SIZE(v4l2_dev->width, v4l2_dev->height);
        *frame = malloc(size);
        if (*frame == NULL)
            return 0;

        /* conversion to yuv420 format */
        convert_yuv422_to_yuv(v4l2_dev->buffers[buf.index].start, 
                              *frame, 
                              v4l2_dev->width,
                              v4l2_dev->height);
    }

    else if (V4L2_YUV422_FMT == v4l2_dev->format)
    {
        /* allocate buffer for yuv422 frame format */
        size = V4L2_YUV422_FRAME_SIZE(v4l2_dev->width, v4l2_dev->height);
        *frame = malloc(size);
        if (*frame == NULL)
            return 0;

        memcpy(*frame, v4l2_dev->buffers[buf.index].start, size);
    }


    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_QBUF, &buf))
    {
        syslog(LOG_ERR, "VIDIOC_QBUF error %d, %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return size;
}

void v4l2_drop_frame(v4l2_dev_t *v4l2_dev)
{
    struct v4l2_buffer buf;

    V4L2_CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_DQBUF, &buf))
    {
        switch (errno)
        {
            case EAGAIN:
                return;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                syslog(LOG_ERR, "VIDIOC_DQBUF error %d, %s", 
                       errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
    }

    assert(buf.index < v4l2_dev->n_buffers);

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_QBUF, &buf))
    {
        syslog(LOG_ERR, "VIDIOC_QBUF error %d, %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void v4l2_capture_stop(v4l2_dev_t *v4l2_dev)
{
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(v4l2_dev->fd, VIDIOC_STREAMOFF, &type))
    {
        syslog(LOG_ERR, "VIDIOC_STREAMOFF error %d, %s", 
               errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void v4l2_close(v4l2_dev_t *v4l2_dev)
{
    uint32_t i;
    struct stat st; 
    int32_t fd;

    for (i = 0; i < v4l2_dev->n_buffers; ++i)
        if (-1 == munmap(v4l2_dev->buffers[i].start, v4l2_dev->buffers[i].length))
        {
            syslog(LOG_ERR, "munmap() error %d, %s", 
                   errno, strerror(errno));
            exit(EXIT_FAILURE);
        }


    if (-1 == stat(v4l2_dev->name, &st))
    {
        syslog(LOG_ERR, "Cannot identify '%s': %d, %s",
               v4l2_dev->name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode))
    {
        syslog(LOG_ERR, "%s is no device", v4l2_dev->name);
        exit(EXIT_FAILURE);
    }

    fd = open(v4l2_dev->name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd)
    {
        syslog(LOG_ERR, "Cannot open '%s': %d, %s",
               v4l2_dev->name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }

    free(v4l2_dev);
}

int8_t v4l2_get_fps(v4l2_dev_t *v4l2_dev)
{
    return v4l2_dev->fps;
}
