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
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>

#include <iofd.h>
#include <video.h>
#include <audio.h>
#include <sigfd.h>
#include <http.h>
#include <cfg.h>
#include <codec.h>


static void usage(char *name)
{
    fprintf(stderr, "usage: %s [-c file] [-d] [-h]\n"
            "Options:\n"
            "\t-c       :config file\n"
            "\t-d       :daemon mode\n"
            "\t-h       :show usage (this page)\n"
            "Copyright (C) 2013 vaicebine@gmail.com\n\n", name);
}

static void init(char *cfg)
{
    cfg_load(cfg);

    codec_init();

    io_fd_init();

    sigfd_init();

    audio_init();

    video_init();

    http_init();

    cfg_unload();    
}

static void clean(void)
{
    http_clean();

    video_clean();

    audio_clean();

    io_fd_clean();

    codec_clean();
}

static void spin(void)
{
    io_fd_poll();
}

int main(int argc, char **argv)
{
    int32_t opt = 0;
    int32_t daemon = 0;
    char *cfg = SYS_CONF_DIR"/ics/ics.cfg";

    while ((opt = getopt(argc, argv, "c:dh")) != EOF)
    {
        switch (opt)
        {
            case 'c':
                cfg = strdup(optarg);
                break;
            case 'd':
                daemon++;
                break;
            case 'h':
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    openlog(argv[0], LOG_PID | LOG_PERROR, LOG_DAEMON);

    if (daemon)
    {
        switch (fork())
        {
            case  -1:
                exit(EXIT_FAILURE);
            case   0:
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                if (setsid() == -1)
                    exit(EXIT_FAILURE);
                break;
            default :
                return EXIT_SUCCESS;
        }
    }

    init(cfg);

    spin();

    clean();

    return EXIT_SUCCESS;
}

