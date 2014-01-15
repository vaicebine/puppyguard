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
#ifndef __HTTP_H
#define __HTTP_H
#include <config.h>
#if HAVE_LIBSSL
    #include <openssl/ssl.h>
#endif /*HAVE_LIBSSL*/  
#define HTTP_TIMEOUT        5
#define HTTP_BUF_SIZE       2048
#define HTTP_PORT           8282

#define HTTP_VIDEO_PATH     "/?stream=mjpg"
#define HTTP_AUDIO_PATH     "/?stream=mp3"

enum 
{
    HTTP_CLOSED = 0,
    HTTP_CONNECTED,
    HTTP_VIDEO_STREAMING,
    HTTP_AUDIO_STREAMING
};

typedef struct 
{
    uint8_t             state;
    struct sockaddr_in  saddr;
    int32_t             fd;
#if HAVE_LIBSSL
    SSL                 *ssl;
    int8_t              inbuf[HTTP_BUF_SIZE];
    uint32_t            inlen;
#endif /*HAVE_LIBSSL*/
} http_peer_t;

typedef struct
{
#ifdef HAVE_LIBSSL
    SSL_CTX             *ssl_ctx;
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    const SSL_METHOD    *ssl_method;
#else
    SSL_METHOD *ssl_method;
#endif
    uint8_t             use_ssl;
    char                *certificate;
    char                *private_key;
#endif /*HAVE_LIBSSL*/
    uint8_t             *credentials;
    uint16_t            port;
    int32_t             video_clients;
    int32_t             audio_clients;
} http_server_t;

void http_send_jpeg_frame(uint8_t *frame, uint32_t size);
void http_send_mp3_buf(uint8_t *buf, uint32_t size);
void http_init(void);
void http_clean(void);
const http_server_t* http_get_server(void);


#endif /*__HTTP_H*/

