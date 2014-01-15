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
#include <string.h>
#include <am.h>

static uint32_t approximate_median(uint8_t *ref_buf, uint8_t *cur_buf, uint32_t size)
{
    uint32_t i = 0;
    int8_t diff;
    uint32_t changes = 0;

    for (i = 0; i < size; i++)
    {
        diff = ref_buf[i] - cur_buf[i];
        if (diff > 0)
        {
            (ref_buf[i])++;
            changes++;
        }
        if (diff < 0)
        {
            (ref_buf[i])--;
            changes++;
        }
    }

    return changes;
}

/* Voice activity detection */
uint8_t am_vad(uint8_t *pcm_buf, uint32_t size, uint32_t threshold)
{
    uint32_t t2_changes = 0;        /* changes for the current buffer */
    static uint32_t t1_changes = 0; /* changes for the previous buffer */
    static uint32_t matches = 0;    /* consecutive times t1_changes and t2_changes match */
    static uint8_t *ref_buf = NULL;

    if (!ref_buf)
    {
        ref_buf = malloc(size);
        memcpy(ref_buf, pcm_buf, size);
    }
    else
    {
        t2_changes = approximate_median(ref_buf, pcm_buf, size);

        if (((t1_changes-threshold) <= t2_changes) &&
            (t2_changes <= (t1_changes+threshold)))
        {
            matches++;

            if (matches >= AM_MATCHES_FOR_SILENCE)
                return 0;
        }
        else
        {
            /* change number differ, we have motion, reset matches counter */
            t1_changes = t2_changes;
            matches = 0;
        }
    }

    return 1;
}

uint8_t am_motion_detection(uint8_t *frame, uint32_t size)
{
    uint32_t t2_changes = 0;        /* changes for the current frame*/
    static uint32_t t1_changes = 0; /* changes for the previous frame*/
    static uint32_t matches = 0;    /* consecutive times t1_changes and t2_changes match */
    static uint8_t *ref_frame = NULL;

    if (!ref_frame)
    {
        ref_frame = malloc(size);
        memcpy(ref_frame, frame, size);
    }
    else
    {

        t2_changes = approximate_median(ref_frame, frame, size);
        if (t2_changes == t1_changes)
        {
            matches++;

            if (matches >= AM_MATCHES_FOR_STATIC)
                return 0;
        }
        else
        {
            /* change number differ, we have motion, reset matches counter */
            t1_changes = t2_changes;
            matches = 0;
        }
    }

    return 1;
}

