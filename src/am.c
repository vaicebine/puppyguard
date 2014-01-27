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

typedef struct 
{
    uint32_t t1_changes; /* changes for the previous frame*/
    uint32_t matches;    /* consecutive times t1_changes and t2_changes match */
    uint8_t *ref_buf;    /* reference buffer */
} am_context_t;

static am_context_t vad_ctx, motion_detection_ctx;

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

static uint8_t compute_am(am_context_t *ctx, 
                          uint8_t *cur_buf, 
                          uint32_t size, 
                          uint32_t threshold,
                          uint32_t matches)
{
    uint32_t t2_changes = 0;    /* changes for the current buffer */

    if (!ctx->ref_buf)
    {
        ctx->ref_buf = malloc(size);
        memcpy(ctx->ref_buf, cur_buf, size);
    }
    else
    {
        t2_changes = approximate_median(ctx->ref_buf, cur_buf, size);
        if (((ctx->t1_changes - threshold) <= t2_changes) &&
            (t2_changes <= (ctx->t1_changes+threshold)))
        {
            ctx->matches++;
            if (ctx->matches >= matches)
                return 0;
        }
        else
        {
            ctx->matches = 0;
        }
        ctx->t1_changes = t2_changes;
    }

    return 1;

}

/* Voice activity detection */
uint8_t am_vad(uint8_t *pcm_buf, uint32_t size, uint32_t threshold)
{
    return compute_am(&vad_ctx, 
                      pcm_buf, 
                      size, 
                      threshold, 
                      AM_MATCHES_FOR_SILENCE);
}

uint8_t am_motion_detection(uint8_t *frame, uint32_t size, uint32_t threshold)
{
    return compute_am(&motion_detection_ctx, 
                      frame, 
                      size, 
                      threshold, 
                      AM_MATCHES_FOR_STATIC);
}


