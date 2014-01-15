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
#include <syslog.h>
#include <audio.h>
#include <alsa.h>

alsa_dev_t* alsa_open(char *name)
{
    alsa_dev_t *alsa_dev = NULL;
    snd_pcm_hw_params_t *params = NULL;
    uint32_t val = 0;
    int32_t rc = 0;
    int32_t dir = 0;

    alsa_dev = malloc(sizeof(alsa_dev_t));
    if (!alsa_dev)
    {
        syslog(LOG_ERR, "Failed to create audio device: Out of memory");
        exit(EXIT_FAILURE);
    }

    /* Open PCM device for recording (capture). */
    rc = snd_pcm_open(&alsa_dev->snd_pcm, name, SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0)
    {
        syslog(LOG_ERR, "Cannot open pcm device: %s", snd_strerror(rc));
        exit(EXIT_FAILURE);
    }

    alsa_dev->name = name;

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(alsa_dev->snd_pcm, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(alsa_dev->snd_pcm, params, 
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(alsa_dev->snd_pcm, params, 
                                 SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(alsa_dev->snd_pcm, params, AUDIO_CHANNEL_NUM);

    /* 44100 bits/second sampling rate (CD quality) */
    val = AUDIO_SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(alsa_dev->snd_pcm, params, &val, &dir);

    alsa_dev->frames = 1152;
    snd_pcm_hw_params_set_period_size_near(alsa_dev->snd_pcm, params, 
                                           &alsa_dev->frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(alsa_dev->snd_pcm, params);
    if (rc < 0)
    {
        syslog(LOG_ERR, "Cannot set hw parameters: %s", snd_strerror(rc));
        exit(EXIT_FAILURE);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &alsa_dev->frames, &dir);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);

    snd_pcm_prepare(alsa_dev->snd_pcm);

    snd_pcm_start(alsa_dev->snd_pcm);

    return alsa_dev;
}

void alsa_poll_descriptors(alsa_dev_t *alsa_dev, struct pollfd **pfds, int *count)
{
    int32_t rc;

    *count = snd_pcm_poll_descriptors_count(alsa_dev->snd_pcm);
    if (*count <= 0)
    {
        syslog(LOG_ERR, "Invalid poll descriptors count");
        exit(EXIT_FAILURE);
    }

    *pfds = malloc(sizeof(struct pollfd) * (*count));

    memset(*pfds, 0, sizeof(struct pollfd) * (*count));

    rc = snd_pcm_poll_descriptors(alsa_dev->snd_pcm, *pfds, *count);
    if (rc < 0)
    {
        syslog(LOG_ERR, "Unable to obtain poll descriptors for capture: %s",
               snd_strerror(rc));
        exit(EXIT_FAILURE);
    }
}

void alsa_close(alsa_dev_t *alsa_dev)
{
    snd_pcm_drain(alsa_dev->snd_pcm);
    snd_pcm_close(alsa_dev->snd_pcm);

    free(alsa_dev);
}

uint32_t alsa_capture_pcm_buf(alsa_dev_t *alsa_dev, uint8_t **pcm_buf)
{
    int32_t rc = 0;
    int32_t pcm_size = 0;

    pcm_size = alsa_dev->frames * (AUDIO_BYTES_PER_SAMPLE*AUDIO_CHANNEL_NUM);

    *pcm_buf = malloc(pcm_size);

    rc = snd_pcm_readi(alsa_dev->snd_pcm, *pcm_buf, alsa_dev->frames);
    if (rc == -EPIPE)
    {
        /* EPIPE means overrun */
        snd_pcm_prepare(alsa_dev->snd_pcm);
        return 0;
    }
    else if (rc < 0)
    {
        return 0;
    }
    else if (rc != (int)alsa_dev->frames)
    {
        return 0;
    }

    return pcm_size;
}

