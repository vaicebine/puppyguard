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
#ifndef __IO_FD_H
#define __IO_FD_H

#define FIX_UNUSED_ARG(x) x = *(&x)

enum 
{
    IO_FD_VIDEO = 1,
    IO_FD_AUDIO,
    IO_FD_SIGNAL,
    IO_FD_HTTP_LISTNER,
    IO_FD_HTTP_CONNECTION,
    IO_FD_SSDP
};

typedef int32_t (*events_cb_t) (int32_t fd, 
                                uint16_t revents, 
                                struct timeval *tv, 
                                void *priv);

typedef void (*timeout_cb_t) (int32_t fd, uint32_t type, void *priv);

typedef void (*iter_cb_t) (int32_t fd, 
                           uint32_t type, 
                           void *priv, 
                           void *arg, 
                           uint32_t val);

void io_fd_init(void);

void io_fd_clean(void);

int32_t io_fd_add(int32_t fd, 
                  uint32_t type, 
                  uint16_t events, 
                  events_cb_t events_cb, 
                  void *priv);

int32_t io_fd_del(int32_t fd);

int32_t io_fd_set_timeout(int32_t fd, 
                          uint32_t timeout, 
                          timeout_cb_t timeout_cb);
void io_fd_poll(void);

void io_fd_iter(iter_cb_t iter_cb, void *arg, uint32_t val);

#endif /*__IO_FD_H*/


