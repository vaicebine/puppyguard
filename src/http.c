/*
 *    Copyright (c) 2013, vaicebine@gmail.com. All rights reserved.
 *    Copyright (c) 2005, Jouni Malinen <j@w1.fi> base64 encoding (RFC1341)
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <syslog.h>
#include <errno.h>

#include <config.h>
#include <video.h>
#include <http.h>
#include <iofd.h>
#include <cfg.h>


static http_server_t server;

static void send_ok_multipart_response(http_peer_t *http_peer);
static void send_ok_html_response(http_peer_t *http_peer);
static void send_ok_audio_response(http_peer_t *http_peer);
static void send_404_html_response(http_peer_t *http_peer);
static ssize_t send_jpeg_frame(http_peer_t *http_peer, uint8_t *frame, uint32_t size);
static void handle_connection(http_peer_t *http_peer);
static void send_jpeg_iter_cb(int32_t fd,
                              uint32_t type,
                              void *priv, 
                              void *frame, 
                              uint32_t size);
static void close_connection(http_peer_t *http_peer);
static int32_t connected_fd_events_cb(int32_t fd, 
                                      uint16_t revents, 
                                      struct timeval *tv, 
                                      void *priv);
static void connected_fd_timeout_cb(int32_t fd, uint32_t type, void *priv);
static int32_t listen_fd_events_cb(int32_t fd, 
                                   uint16_t revents, 
                                   struct timeval *tv, 
                                   void *priv);

const http_server_t* http_get_server(void)
{
    return &server;
}

static ssize_t socket_write(http_peer_t *http_peer, const void *buf, size_t count)
{
#if HAVE_LIBSSL
    if (server.use_ssl)
        return SSL_write(http_peer->ssl, buf, count);
#endif /*HAVE_LIBSSL*/
    return write(http_peer->fd, buf, count);
}

static ssize_t socket_read(http_peer_t *http_peer, uint8_t *buf, size_t count)
{
#if HAVE_LIBSSL
    ssize_t size = 0;

    if (server.use_ssl)
    {
        size = SSL_read(http_peer->ssl, 
                        http_peer->inbuf + http_peer->inlen,
                        HTTP_BUF_SIZE - http_peer->inlen);
        if (size < 0)
            return size;

        if (size == 1)
        {
            buf[0] = http_peer->inbuf[http_peer->inlen];
            http_peer->inlen++;
            if (http_peer->inlen >= HTTP_BUF_SIZE)
            {
                http_peer->inlen = 0;
                return -1;
            }
            return 1;
        }

        http_peer->inlen += size;

        size = (http_peer->inlen > count) ? count : http_peer->inlen;

        memcpy(buf, http_peer->inbuf, size);

        http_peer->inlen = 0;

        return size;
    }
#endif /*HAVE_LIBSSL*/

    return read(http_peer->fd, buf, count);
}

static void send_401_response(http_peer_t *http_peer)
{
    char http_buf[1024];

    snprintf(http_buf, 
             sizeof(http_buf),
             "HTTP/1.0 401 Unauthorized\r\n"
             "WWW-Authenticate: Basic realm=\""PACKAGE_NAME"\"\r\n"
             "Server: "PACKAGE_NAME"/"PACKAGE_VERSION"\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 0\r\n\r\n");
    socket_write(http_peer, http_buf, strlen(http_buf));
}

static void send_ok_multipart_response(http_peer_t *http_peer)
{
    char http_buf[1024];

    snprintf(http_buf, 
             sizeof(http_buf),
             "HTTP/1.0 200 OK\r\n"
             "Connection: close\r\n" 
             "Server: "PACKAGE_NAME"/"PACKAGE_VERSION"\r\n"
             "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
             "Pragma: no-cache\r\n"
             "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
             "Content-Type: multipart/x-mixed-replace; boundary=x-mixed-boundary\r\n\r\n"
             "--x-mixed-boundary\r\n");
    socket_write(http_peer, http_buf, strlen(http_buf));
}

static void send_ok_audio_response(http_peer_t *http_peer)
{
    char http_buf[1024];

    snprintf(http_buf, 
             sizeof(http_buf),
             "HTTP/1.0 200 OK\r\n"
             "Connection: close\r\n" 
             "Server: "PACKAGE_NAME"/"PACKAGE_VERSION"\r\n"
             "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
             "Pragma: no-cache\r\n"
             "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
             "Content-Type: audio/mpeg\r\n\r\n");
    socket_write(http_peer, http_buf, strlen(http_buf));
}

static void send_ok_html_response(http_peer_t *http_peer)
{
    char http_buf[1024];

    snprintf(http_buf, 
             sizeof(http_buf),
             "HTTP/1.0 200 OK\r\n"
             "Server: "PACKAGE_NAME"/"PACKAGE_VERSION"\r\n"
             "Content-Type: text/html\r\n\r\n"
             "<!DOCTYPE HTML>"
             "<html>"
             "<body>"
             "</body>"
             "</html>");

    socket_write(http_peer, http_buf, strlen(http_buf));
}

static void send_404_html_response(http_peer_t *http_peer)
{
    char http_buf[1024];

    snprintf(http_buf, 
             sizeof(http_buf),
             "HTTP/1.0 404 Not Found\r\n"
             "Server: "PACKAGE_NAME"/"PACKAGE_VERSION"\r\n"
             "Content-Type: text/html\r\n\r\n"
             "Not found");
    socket_write(http_peer, http_buf, strlen(http_buf));
}

static ssize_t send_jpeg_frame(http_peer_t *http_peer, uint8_t *frame, uint32_t size)
{
    char http_buf[1024];
    ssize_t len = 0;

    /* send http header */
    snprintf(http_buf, 
             sizeof(http_buf),
             "Content-Type: image/jpeg\r\n"
             "Content-Length: %d\r\n\r\n",
             size);
    len = socket_write(http_peer, http_buf, strlen(http_buf));
    if (len < 0)
        return len;

    /* send jpeg frame */
    len = socket_write(http_peer, frame, size);
    if (len < 0)
        return len;

    /* send mjpeg boundary */
    snprintf(http_buf, 
             sizeof(http_buf),
             "\r\n--x-mixed-boundary\r\n");
    return socket_write(http_peer, http_buf, strlen(http_buf));
}

static uint8_t check_authorization(uint8_t *buf, ssize_t size, uint8_t *basic)
{
    uint8_t *authorization = NULL, *cursor = NULL, *end = buf + size;
    uint32_t length = 0;

    *basic = 0;

    authorization = (uint8_t *) strstr((char*)buf, "Authorization: Basic");
    if (!authorization)
        return 0;

    *basic = 1;

    authorization += 21;

    while (*authorization == ' ')
    {
        authorization++;
        if (authorization >= end)
            return 0;
    }

    cursor = (uint8_t *) strstr((char*)authorization, "\r\n");
    if (!cursor || cursor > end)
        return 0;

    length = cursor - authorization;

    if (length != strlen((char*)server.credentials))
        return 0;

    if (memcmp(server.credentials, 
               authorization, 
               strlen((char*)server.credentials)) == 0)
        return 1;

    return 0;
}

static void handle_connection(http_peer_t *http_peer)
{
    uint8_t buf[HTTP_BUF_SIZE];
    ssize_t size = 0;
    uint8_t basic = 0;

    memset(buf, 0, sizeof(buf));

    if (http_peer->state != HTTP_CONNECTED)
    {
        close_connection(http_peer);
        return;
    }

    size = socket_read(http_peer, buf, sizeof(buf));
    if (size < 0)
    {
        close_connection(http_peer);
        return;
    }

    if (size == 1)
        return;

    if (strstr((char*)buf, "GET "HTTP_VIDEO_PATH))
    {
        if (server.credentials)
        {
            if (0 == check_authorization(buf, size, &basic))
            {
                send_401_response(http_peer);

                if (basic)
                {
                    syslog(LOG_WARNING,
                           "Failed authentication for video stream from %s", 
                           inet_ntoa(http_peer->saddr.sin_addr));
                }

                close_connection(http_peer);
                return;
            }
        }

        http_peer->state = HTTP_VIDEO_STREAMING;
        server.video_clients++;

        send_ok_multipart_response(http_peer);

        syslog(LOG_INFO, "Video stream opened for client %s", 
               inet_ntoa(http_peer->saddr.sin_addr));
        return;
    }
    else if (strstr((char*)buf, "GET "HTTP_AUDIO_PATH))
    {
        if (server.credentials)
        {
            if (0 == check_authorization(buf, size, &basic))
            {
                send_401_response(http_peer);

                if (basic)
                {
                    syslog(LOG_WARNING,
                           "Failed authentication for audio stream from %s", 
                           inet_ntoa(http_peer->saddr.sin_addr));
                }

                close_connection(http_peer);
                return;
            }
        }

        http_peer->state = HTTP_AUDIO_STREAMING;
        server.audio_clients++;

        send_ok_audio_response(http_peer);

        syslog(LOG_INFO, "Audio stream opened for client %s", 
               inet_ntoa(http_peer->saddr.sin_addr));
        return;
    }
    else if ((strstr((char*)buf, "GET / ")))
    {
        send_ok_html_response(http_peer);
    }
    else
    {
        send_404_html_response(http_peer);
    }

    close_connection(http_peer);
}

static void send_jpeg_iter_cb(int32_t fd, uint32_t type, void *priv, void *frame, uint32_t size)
{
    http_peer_t *http_peer = priv;

    if (type != IO_FD_HTTP_CONNECTION)
        return;

    if (!http_peer)
        return;

    FIX_UNUSED_ARG(fd);

    if (http_peer->state == HTTP_VIDEO_STREAMING)
    {
        if (send_jpeg_frame(http_peer, frame, size) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            else
            {
                close_connection(http_peer);
            }
        }
    }
}

void http_send_jpeg_frame(uint8_t *frame, uint32_t size)
{
    io_fd_iter(send_jpeg_iter_cb, frame, size);
}

static void send_mp3_iter_cb(int32_t fd, uint32_t type, void *priv, void *buf, uint32_t size)
{
    http_peer_t *http_peer = priv;

    if (type != IO_FD_HTTP_CONNECTION)
        return;

    if (!http_peer)
        return;

    FIX_UNUSED_ARG(fd);

    if (http_peer->state == HTTP_AUDIO_STREAMING)
    {
        if (socket_write(http_peer, buf, size) < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }
            else
            {
                close_connection(http_peer);
            }
        }
    }
}

void http_send_mp3_buf(uint8_t *buf, uint32_t size)
{
    io_fd_iter(send_mp3_iter_cb, buf, size);
}

static void close_connection(http_peer_t *http_peer)
{
    close(http_peer->fd);

    if (http_peer->state == HTTP_AUDIO_STREAMING)
    {
        server.audio_clients--;
        syslog(LOG_INFO, "Audio stream closed for client %s", 
               inet_ntoa(http_peer->saddr.sin_addr));
    }
    else if (http_peer->state == HTTP_VIDEO_STREAMING)
    {
        server.video_clients--;
        syslog(LOG_INFO, "Video stream closed for client %s", 
               inet_ntoa(http_peer->saddr.sin_addr));
    }

    io_fd_del(http_peer->fd);

#if HAVE_LIBSSL
    if (http_peer->ssl)
        SSL_free(http_peer->ssl);
#endif /*HAVE_LIBSSL*/

    free(http_peer);
}

static int32_t connected_fd_events_cb(int32_t fd,
                                      uint16_t revents, 
                                      struct timeval *tv, 
                                      void *priv)
{
    http_peer_t *http_peer = priv;

    FIX_UNUSED_ARG(revents);
    FIX_UNUSED_ARG(tv);
    FIX_UNUSED_ARG(fd);

    handle_connection(http_peer);

    return 0;
}

static void connected_fd_timeout_cb(int32_t fd, uint32_t type, void *priv)
{
    http_peer_t *http_peer = priv;

    if (!http_peer)
        return;

    FIX_UNUSED_ARG(type);
    FIX_UNUSED_ARG(fd);

    if (http_peer->state == HTTP_CONNECTED)
    {
        close_connection(http_peer);
    }
}

static int32_t listen_fd_events_cb(int32_t fd,
                                   uint16_t revents, 
                                   struct timeval *tv, 
                                   void *priv)
{
    int32_t afd;
    struct sockaddr_in saddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    http_peer_t *http_peer;
    int flags = 0;

    FIX_UNUSED_ARG(revents);
    FIX_UNUSED_ARG(priv);
    FIX_UNUSED_ARG(tv);

    afd = accept(fd, (struct sockaddr*) &saddr, &addrlen);
    if (afd < 0)
        return 0;

    http_peer = malloc(sizeof(http_peer_t));
    if (!http_peer)
    {
        close(afd);
        return 0;
    }

    memset(http_peer, 0, sizeof(http_peer_t));

    http_peer->state = HTTP_CONNECTED;

    memcpy(&http_peer->saddr, &saddr, sizeof(struct sockaddr_in));

    http_peer->fd = afd;

#if HAVE_LIBSSL
    if (server.use_ssl)
    {
        http_peer->ssl = SSL_new(server.ssl_ctx);
        if (http_peer->ssl == NULL)
        {
            free(http_peer);
            return 0;
        }

        if (SSL_set_fd(http_peer->ssl, afd) != 1)
        {
            free(http_peer);
            return 0;
        }

        SSL_accept(http_peer->ssl);
    }
#endif /*HAVE_LIBSSL*/

    flags = fcntl(afd, F_GETFL);
    fcntl(afd, F_SETFL, flags|O_NONBLOCK);

    io_fd_add(afd, 
              IO_FD_HTTP_CONNECTION, 
              POLLIN, 
              connected_fd_events_cb, 
              http_peer);

    io_fd_set_timeout(afd, HTTP_TIMEOUT, connected_fd_timeout_cb);

    return 0;
}

unsigned char* base64_encode(const unsigned char *src, size_t len,
                             size_t *out_len)
{
    const unsigned char base64_table[65] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char *out, *pos;
    const unsigned char *end, *in;
    size_t olen;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72;      /* line feeds */
    olen++;                 /* nul termination */

    if (olen < len)
        return NULL;        /* integer overflow */

    out = malloc(olen);
    if (out == NULL)
        return NULL;

    end = src + len;
    in = src;
    pos = out;

    while (end - in >= 3)
    {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1)
        {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                  (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    *pos = '\0';
    if (out_len)
        *out_len = pos - out;

    return out;
}

void http_init(void)
{
    struct sockaddr_in saddr;
    int32_t on = 1;
    int32_t fd = -1;
    char *credentials = NULL;
    size_t len = 0;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Cannot create http socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    server.port = HTTP_PORT;
    cfg_get_value("http_port", &server.port, CFG_VALUE_INT16);

    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(server.port);

    if (bind(fd, (struct sockaddr*) &saddr, sizeof(struct sockaddr_in)) < 0)
    {
        syslog(LOG_ERR, "Cannot bind on http socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(fd, SOMAXCONN) < 0)
    {
        syslog(LOG_ERR, "Cannot listen on http socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    io_fd_add(fd, IO_FD_HTTP_LISTNER, POLLIN, listen_fd_events_cb, NULL);

    cfg_get_value("http_credentials", &credentials, CFG_VALUE_STR);
    if (credentials != NULL)
    {
        server.credentials = base64_encode((unsigned char*)credentials, 
                                           strlen(credentials), 
                                           &len);
        free(credentials);
    }

#if HAVE_LIBSSL
    cfg_get_value("ssl_enable", &server.use_ssl, CFG_VALUE_BOOL);

    if (server.use_ssl)
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_digests();

        server.ssl_method = TLSv1_server_method();
        server.ssl_ctx = SSL_CTX_new(server.ssl_method);

        cfg_get_value("ssl_certificate", &server.certificate, CFG_VALUE_STR);
        cfg_get_value("ssl_private_key", &server.private_key, CFG_VALUE_STR);

        if (SSL_CTX_use_certificate_file(server.ssl_ctx, 
                                         server.certificate, 
                                         SSL_FILETYPE_PEM) != 1)
        {
            syslog(LOG_ERR, "Cannot use certificate file : %s", server.certificate);
            exit(EXIT_FAILURE);
        }

        if (SSL_CTX_use_PrivateKey_file(server.ssl_ctx, 
                                        server.private_key, 
                                        SSL_FILETYPE_PEM) != 1)
        {
            syslog(LOG_ERR, "Cannot use private key file : %s", server.private_key);
            exit(EXIT_FAILURE);
        }

        if (SSL_CTX_check_private_key(server.ssl_ctx) != 1)
        {
            syslog(LOG_ERR, "Check failed for private key file : %s", server.private_key);
            exit(EXIT_FAILURE);
        }

        if (SSL_CTX_set_session_id_context(server.ssl_ctx, 
                                           (const unsigned char *)PACKAGE_NAME, 
                                           strlen(PACKAGE_NAME)) != 1)
        {
            syslog(LOG_ERR, "Cannot use session id context : %s", PACKAGE_NAME);
            exit(EXIT_FAILURE);
        }

        SSL_CTX_set_session_cache_mode(server.ssl_ctx, SSL_SESS_CACHE_SERVER);
    }

#endif /*HAVE_LIBSSL*/

}

static void clean_http_fd_iter_cb(int32_t fd, uint32_t type, void *priv, void *arg, uint32_t val)
{
    http_peer_t *http_peer = priv;

    FIX_UNUSED_ARG(val);
    FIX_UNUSED_ARG(arg);

    if (type == IO_FD_HTTP_CONNECTION)
    {
        if (http_peer)
            close_connection(http_peer);
    }
    else if (type == IO_FD_HTTP_LISTNER)
    {
        close(fd);
        io_fd_del(fd);
    }
}

void http_clean(void)
{
    io_fd_iter(clean_http_fd_iter_cb, NULL, 0);

    if (server.credentials)
        free(server.credentials);

#if HAVE_LIBSSL
    if (server.certificate)
        free(server.certificate);
    if (server.private_key)
        free(server.private_key);
#endif /*HAVE_LIBSSL*/

}

