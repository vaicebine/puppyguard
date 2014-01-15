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
#ifndef __AUDIO_H
#define __AUDIO_H

#define AUDIO_DEVICE            "default"
#define AUDIO_SAMPLE_RATE       44100
#define AUDIO_CHANNEL_NUM       1
#define AUDIO_BYTES_PER_SAMPLE  2

#define VOX_THRESHOLD           3
#define VOX_TIMEOUT             60

typedef struct 
{
    uint8_t  enable;
    uint32_t threshold;
    int32_t  timeout;
} vox_settings_t;

void audio_init(void);
void audio_clean(void);


#endif /*__AUDIO_H*/

