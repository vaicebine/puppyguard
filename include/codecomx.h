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
#ifndef __OMX_H
#define __OMX_H
#include <semaphore.h>
#include <stdlib.h>
#include <ilclient.h>

enum component_index_e
{
    OMX_IMAGE_ENCODE_INDEX = 0,
    OMX_COMPONENT_MAX_INDEX
};

typedef struct 
{
    ILCLIENT_T      *client;
    COMPONENT_T     *component[OMX_COMPONENT_MAX_INDEX];
    sem_t           semaphore[OMX_COMPONENT_MAX_INDEX];
} omx_handle_t;

#endif /*__OMX_H*/
