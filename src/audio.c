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
#include <stdlib.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <math.h>
#include <config.h>

#include <audio.h>
#include <http.h>
#include <iofd.h>
#include <alsa.h>
#include <codec.h>
#include <cfg.h>
#include <am.h>


static alsa_dev_t *alsa_dev = NULL;
static vox_settings_t vox_settings;

static uint32_t capture_pcm_buf(uint8_t **pcm_buf);
static uint32_t pcm2mp3(uint8_t *pcm_buf, 
                        int32_t pcm_size, 
                        uint8_t **mp3_buf);
static int32_t audio_fd_events_cb(int32_t fd, 
                                  uint16_t revents,
                                  struct timeval *tv,
                                  void *priv);


static uint32_t capture_pcm_buf(uint8_t **pcm_buf)
{
    if (!alsa_dev)
        return 0;

    return alsa_capture_pcm_buf(alsa_dev, pcm_buf);
}

static uint32_t pcm2mp3(uint8_t *pcm_buf, 
                        int32_t pcm_size, 
                        uint8_t **mp3_buf)
{
    return codec_mp3_encode_pcm_buf((int16_t*)pcm_buf, 
                                    (pcm_size/AUDIO_BYTES_PER_SAMPLE),
                                    mp3_buf);
}

static int32_t audio_fd_events_cb(int32_t fd, 
                                  uint16_t revents,
                                  struct timeval *tv,
                                  void *priv)
{
    const http_server_t *http_server = http_get_server();
    uint8_t *pcm_buf = NULL;
    uint8_t *mp3_buf = NULL;
    uint32_t size = 0;
    static struct timeval vad_tv;

    FIX_UNUSED_ARG(fd);
    FIX_UNUSED_ARG(revents);
    FIX_UNUSED_ARG(priv);

    size = capture_pcm_buf(&pcm_buf);
    if (size)
    {
        if (http_server->audio_clients)
        {
            /* Voice Operated eXchange */
            if (vox_settings.enable)
            {
                if (!am_vad(pcm_buf, size, vox_settings.threshold))
                {
                    /* we don't have voice activity */
                    if (!vad_tv.tv_sec)
                    {
                        /* remove noise */
                        memset(pcm_buf, 0, size);
                    }
                    else if ((tv->tv_sec - vad_tv.tv_sec) >= vox_settings.timeout)
                    {
                        /* vox have timeout, we can remove noise and reset
                         * voice activity timestamp
                         */
                        memset(pcm_buf, 0, size);
                        vad_tv.tv_sec = 0;
                    }
                }
                else
                {
                    if (!vad_tv.tv_sec)
                    {
                        /* we have voice activity, save vad timestamp */
                        vad_tv.tv_sec = tv->tv_sec;
                    }
                }
            }

            size = pcm2mp3(pcm_buf, size, &mp3_buf);
            if (size)
                http_send_mp3_buf(mp3_buf, size);
        }
    }

    if (pcm_buf)
        free(pcm_buf);

    if (mp3_buf)
        free(mp3_buf);

    return 0;
}

void audio_init(void)
{
    struct pollfd *pfds = NULL;
    int32_t i, nfds;
    uint8_t enable = 0;
    char *name = AUDIO_DEVICE;
    vox_settings.enable = 0;
    vox_settings.threshold = VOX_THRESHOLD;
    vox_settings.timeout = VOX_TIMEOUT;

    cfg_get_value("audio_enable", &enable, CFG_VALUE_BOOL);
    if (!enable)
        return;

    cfg_get_value("audio_device", &name, CFG_VALUE_STR);
    cfg_get_value("vox_enable", &vox_settings.enable, CFG_VALUE_BOOL);
    cfg_get_value("vox_threshold", &vox_settings.threshold, CFG_VALUE_INT32);
    cfg_get_value("vox_timeout", &vox_settings.timeout, CFG_VALUE_INT32);

    alsa_dev = alsa_open(name);

    alsa_poll_descriptors(alsa_dev, &pfds, &nfds);

    for (i = 0; i < nfds; i++)
    {
        if (pfds[i].fd)
            io_fd_add(pfds[i].fd, 
                      IO_FD_AUDIO, 
                      pfds[i].events, 
                      audio_fd_events_cb, 
                      NULL);
    }

    if (pfds && nfds)
        free(pfds);

    codec_mp3_init();
}

void audio_clean(void)
{
    alsa_close(alsa_dev);
    codec_mp3_clean();
}

