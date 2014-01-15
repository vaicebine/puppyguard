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
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <syslog.h>
#include <errno.h>

#include <iofd.h>

/* io file descriptor */
struct iofd
{
    uint32_t type;          /* io file descriptor type */
    events_cb_t events_cb;  /* io file descriptor event callback */
    void *priv;             /* io file descriptor private data */
    uint32_t timestamp;     /* io file descriptor timestamp */
    uint32_t timeout;       /* io file descriptor timeout in seconds */
    timeout_cb_t timeout_cb;/* io file descriptor timeout callback */
};

/* io file descriptor database */
struct iofddb
{
    struct iofd *iofds;     /* io file descriptor array */
    struct pollfd *pfds;    /* poll file descriptor array */
    int32_t nfds;           /* file descriptor number */
    int32_t maxfds;         /* maximum number of file descriptor */
};

static struct iofddb db;


void io_fd_init(void)
{
    struct rlimit rl;

    /* get max number of fd that can be open */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
        syslog(LOG_ERR, "getrlimit(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* allocate poll fds array */
    db.pfds = malloc(sizeof(struct pollfd) * rl.rlim_cur);
    if (!db.pfds)
    {
        syslog(LOG_ERR, "malloc(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    db.maxfds = rl.rlim_cur;

    memset(db.pfds, 0, (sizeof(struct pollfd) * rl.rlim_cur));

    /* allocate io fds array */
    db.iofds = malloc(sizeof(struct iofd) * rl.rlim_cur);
    if (!db.iofds)
    {
        syslog(LOG_ERR, "malloc(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    memset(db.iofds, 0, (sizeof(struct iofd) * rl.rlim_cur));

    /* no poll fd is open at this time */
    db.nfds = 0; 
}

void io_fd_clean(void)
{
    /* free memory */
    free(db.pfds);
    free(db.iofds);
}

int32_t io_fd_add(int32_t fd,
                  uint32_t type,
                  uint16_t events, 
                  events_cb_t events_cb, 
                  void *priv)
{
    if (fd < 0 || fd >= db.maxfds)
        return -1;

    /* add fd to fd poll array */
    db.pfds[fd].fd = fd;
    db.pfds[fd].events = events;
    db.pfds[fd].revents = 0;

    /* register event callback and store fd private data */
    memset(&db.iofds[fd], 0, sizeof(struct iofd));
    db.iofds[fd].events_cb = events_cb;
    db.iofds[fd].priv = priv;
    db.iofds[fd].type = type;

    if (fd > db.nfds)
        db.nfds = fd;

    return 0;
}

int32_t io_fd_del(int32_t fd)
{
    if (fd < 0 || fd >= db.maxfds)
        return -1;

    /* remove fd from fd poll array */
    db.pfds[fd].fd = 0;
    db.pfds[fd].events = 0;
    db.pfds[fd].revents = 0;

    /* clean event callbacks and private data */
    memset(&db.iofds[fd], 0, sizeof(struct iofd));

    if (fd == db.nfds)
        db.nfds--;

    return 0;
}

int32_t io_fd_set_timeout(int32_t fd, 
                          uint32_t timeout, 
                          timeout_cb_t timeout_cb)
{
    struct timeval tv;

    if (fd < 0 || fd >= db.maxfds)
        return -1;

    gettimeofday(&tv, NULL);
    db.iofds[fd].timestamp = tv.tv_sec;
    db.iofds[fd].timeout = timeout;
    db.iofds[fd].timeout_cb = timeout_cb;

    if (fd > db.nfds)
        db.nfds = fd;

    return 0;
}

void io_fd_poll(void)
{
    int32_t timeout = 1000;
    int32_t fd;
    struct timeval tv;
    int32_t rc;

    while (1)
    {
        poll(db.pfds,  db.nfds + 1, timeout);

        gettimeofday(&tv, NULL);

        /* walk fd poll array and check for events */
        for (fd = 0; fd < db.nfds + 1; fd++)
        {
            /* check if we have timeout set on fd */
            if (db.iofds[fd].timeout)
            {
                /* if timeout expired call the timeout hook */
                if ((tv.tv_sec - db.iofds[fd].timestamp) > db.iofds[fd].timeout)
                {
                    if (db.iofds[fd].timeout_cb != NULL)
                    {
                        db.iofds[fd].timeout_cb(fd, 
                                                db.iofds[fd].type, 
                                                db.iofds[fd].priv);
                    }
                    continue;
                }
            }

            if (db.pfds[fd].revents)
            {
                db.iofds[fd].timestamp = tv.tv_sec; 

                /* call fd event hook */
                if (db.iofds[fd].events_cb != NULL)
                {
                    rc = db.iofds[fd].events_cb(fd, 
                                                db.pfds[fd].revents,
                                                &tv, 
                                                db.iofds[fd].priv);
                    if (rc < 0)
                        return;
                }
            }
        }
    }
}

void io_fd_iter(iter_cb_t iter_cb, void *arg, uint32_t val)
{
    int32_t fd;

    if (!iter_cb)
        return;

    /* fd poll array iterator */
    for (fd = 0; fd < db.nfds + 1; fd++)
    {
        if (db.pfds[fd].fd)
            iter_cb(fd, db.iofds[fd].type, db.iofds[fd].priv, arg, val);
    }
}

