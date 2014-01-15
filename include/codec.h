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
#ifndef __CODEC_H
#define __CODEC_H

#define MP3_BIT_RATE            64

void codec_init(void);
void codec_clean(void);
void codec_mp3_init(void);
void codec_mp3_clean(void);
uint32_t codec_mp3_encode_pcm_buf(int16_t *pcm_buf, 
                                  int32_t pcm_size, 
                                  uint8_t **mp3_buf);

void codec_jpeg_compress(uint8_t *frame, uint32_t size, uint8_t **outbuf,
                         uint32_t *outsize);
void codec_jpeg_init(uint32_t width, uint32_t height, int32_t quality);
void codec_jpeg_clean(void);
int8_t codec_jpeg_get_v4l2_color_format(void);


#endif /*__CODEC_H*/
