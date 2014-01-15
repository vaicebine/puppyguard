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
#include <codec.h>
#include <string.h>
#include <audio.h>
#include <syslog.h>
#include <layer3.h>

static shine_t shine;

void codec_mp3_init(void)
{
    shine_config_t config;
    shine_set_config_mpeg_defaults(&config.mpeg);

    config.wave.channels   = AUDIO_CHANNEL_NUM;
    config.wave.samplerate = AUDIO_SAMPLE_RATE;

    config.mpeg.mode = MONO;
    config.mpeg.bitr = MP3_BIT_RATE;

    shine = shine_initialise(&config);
}

void codec_mp3_clean(void)
{
    shine_close(shine);
}

uint32_t codec_mp3_encode_pcm_buf(int16_t *pcm_buf, 
                                  int32_t pcm_size, 
                                  uint8_t **mp3_buf)
{
    int16_t *pcm_chan_buf[2];
    long int mp3_size = 0;
    uint8_t  *mp3_encoded_buf = NULL;

    if (!pcm_size || !pcm_buf)
        return 0;

    pcm_chan_buf[0] = pcm_buf;
    pcm_chan_buf[1] = NULL;

    mp3_encoded_buf = shine_encode_buffer(shine, pcm_chan_buf, &mp3_size);
    if (mp3_size)
    {
        *mp3_buf = malloc(mp3_size);
        memcpy(*mp3_buf, mp3_encoded_buf, mp3_size);
    }

    return mp3_size;
}

