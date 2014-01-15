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
#ifndef __ALSA_H
#define __ALSA_H
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

/* ALSA device */
typedef struct 
{
    char                *name;
    snd_pcm_t           *snd_pcm;
    snd_pcm_uframes_t   frames;
} alsa_dev_t;

alsa_dev_t* alsa_open(char *name);

void alsa_poll_descriptors(alsa_dev_t *alsa_dev, struct pollfd **pfds, int *count);

void alsa_close(alsa_dev_t *alsa_dev);

uint32_t alsa_capture_pcm_buf(alsa_dev_t *alsa_dev, uint8_t **pcm_buf);


#endif /*__ALSA_H*/
