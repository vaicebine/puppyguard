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
#ifndef __VIDEO_H
#define __VIDEO_H

#define VIDEO_WIDTH             640
#define VIDEO_HEIGHT            480
#define VIDEO_FPS               5
#define VIDEO_JPEG_FMT_QUALITY  10

#define VIDEO_DEVICE            "/dev/video0"

typedef struct 
{
    uint8_t  enable;
    uint32_t threshold;
    char     *smtp_user;
    char     *smtp_pass;
    char     *smtp_from;
    char     *smtp_to;
    char     *smtp_cc;
    char     *smtp_server;
    uint16_t smtp_port;
    uint8_t  smtp_tls;
} motion_settings_t;

void video_init(void);
void video_clean(void);
int32_t video_get_width(uint32_t *width);
int32_t video_get_height(uint32_t *height);

#endif /*__VIDEO_H*/

