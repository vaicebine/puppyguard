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
#include <syslog.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cfg.h>

static xmlNode *cfg_root = NULL;
static xmlDoc *cfg_doc = NULL;

static void content2value(char *content, void *value, int32_t type);
static int32_t get_value(xmlNode *a_node, char *name, void *value, int32_t type);


static void content2value(char *content, void *value, int32_t type)
{
    switch (type)
    {
        case CFG_VALUE_BOOL:
            *((uint8_t*)value) = (strcmp(content, "true") == 0) ? 1 : 0; 
            break;
        case CFG_VALUE_STR:
            *((char**)value) = strdup(content);
            break;
        case CFG_VALUE_INT32:
            *((int32_t*)value) = strtol(content, (char **) NULL, 10);
            break;
        case CFG_VALUE_INT16:
            *((int16_t*)value) = atoi(content);
            break;
        case CFG_VALUE_INT8:
            *((int8_t*)value) = atoi(content);
            break;
        default:
            break;
    }
}

static int32_t get_value(xmlNode *a_node, char *name, void *value, int32_t type)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            if (strcmp((char*)cur_node->name, name) == 0)
            {
                content2value((char*)xmlNodeGetContent(cur_node), value, type);
                return 0;
            }
        }
        get_value(cur_node->children, name, value, type);
    }

    return -1;
}

int32_t cfg_get_value(char *name, void *value, int32_t type)
{
    return get_value(cfg_root, name, value, type);
}

void cfg_load(char *file)
{
    cfg_doc = xmlReadFile(file, NULL, 0);

    if (cfg_doc == NULL)
    {
        syslog(LOG_ERR, "Cannot parse configuration file : %s", file);
        exit(EXIT_FAILURE);
    }

    cfg_root = xmlDocGetRootElement(cfg_doc);
}

void cfg_unload(void)
{
    xmlFreeDoc(cfg_doc);

    xmlCleanupParser();
}
