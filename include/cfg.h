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
#ifndef __CFG_H
#define __CFG_H

enum cfg_value_type_e
{
    CFG_VALUE_BOOL = 1,
    CFG_VALUE_STR,
    CFG_VALUE_INT32,
    CFG_VALUE_INT16,
    CFG_VALUE_INT8
};

void cfg_load(char *file);
int32_t cfg_get_value(char *name, void *value, int32_t type);
void cfg_unload(void);

#endif /*__CFG_H*/


