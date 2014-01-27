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
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <sigfd.h>
#include <iofd.h>
#include <http.h>

static int32_t signal_fd_events_cb(int32_t fd, 
                                   uint16_t revents, 
                                   struct timeval *tv, 
                                   void *priv);

static int32_t signal_fd_events_cb(int32_t fd, 
                                   uint16_t revents, 
                                   struct timeval *tv,
                                   void *priv)
{
    struct signalfd_siginfo fdsi;
    ssize_t size;

    FIX_UNUSED_ARG(revents);
    FIX_UNUSED_ARG(tv);
    FIX_UNUSED_ARG(priv);

    size = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (size != sizeof(struct signalfd_siginfo))
        return 0;

    if (fdsi.ssi_signo == SIGINT)
    {
        return -1;
    }
    else if (fdsi.ssi_signo == SIGQUIT)
    {
        return -1;
    }
    else if (fdsi.ssi_signo == SIGCHLD)
    {
        wait(NULL);
    }

    return 0;
}

void sigfd_init(void)
{
    sigset_t mask;
    int32_t fd = 0;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    fd = signalfd(-1, &mask,  0);

    io_fd_add(fd, IO_FD_SIGNAL, POLLIN, signal_fd_events_cb, NULL);
}


