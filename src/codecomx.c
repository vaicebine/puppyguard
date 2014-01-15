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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <iofd.h>
#include <v4l2.h>
#include <codecomx.h>

#include <bcm_host.h>

static omx_handle_t omx;

static void omx_buffer_fill_done(void *data, COMPONENT_T *comp)
{
    uint32_t i;

    FIX_UNUSED_ARG(data);

    for (i = 0; i < OMX_COMPONENT_MAX_INDEX; i++)
    {
        if (omx.component[i] == comp)
        {
            sem_post(&omx.semaphore[i]);
            break;
        }
    }
}

int8_t codec_jpeg_get_v4l2_color_format(void)
{
    return V4L2_YUV420_FMT;
}

void codec_jpeg_init(uint32_t width, uint32_t height, int32_t quality)
{
    OMX_PARAM_PORTDEFINITIONTYPE def;
    OMX_IMAGE_PARAM_PORTFORMATTYPE fmt;
    OMX_IMAGE_PARAM_QFACTORTYPE qfactor;
    OMX_ERRORTYPE rc;

    if (0 != ilclient_create_component(omx.client, 
                                       &omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                       "image_encode",
                                       ILCLIENT_DISABLE_ALL_PORTS |
                                       ILCLIENT_ENABLE_INPUT_BUFFERS |
                                       ILCLIENT_ENABLE_OUTPUT_BUFFERS))
    {
        syslog(LOG_ERR, "Failed to create image encode component");
        exit(EXIT_FAILURE);
    }

    memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

    def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    def.nVersion.nVersion = OMX_VERSION;
    def.nPortIndex = 340;

    rc = OMX_GetParameter(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), 
                          OMX_IndexParamPortDefinition,
                          &def);
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to get OMX port param definition for port 340, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    def.format.image.nFrameWidth = width;
    def.format.image.nFrameHeight = height;
    def.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    def.format.image.nSliceHeight = height;
    def.format.image.nStride = width;
    def.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
    def.nBufferSize = def.format.image.nStride * def.format.image.nSliceHeight * 3 / 2;
    def.format.image.bFlagErrorConcealment = OMX_FALSE;

    rc = OMX_SetParameter(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]),
                          OMX_IndexParamPortDefinition, 
                          &def);
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to set OMX port param definition for port 340, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    memset(&fmt, 0, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
    fmt.nSize = sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE);
    fmt.nVersion.nVersion = OMX_VERSION;
    fmt.nPortIndex = 341;   
    fmt.eCompressionFormat = OMX_IMAGE_CodingJPEG;

    rc = OMX_SetParameter(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), 
                          OMX_IndexParamImagePortFormat, 
                          &fmt);
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to set OMX port param format for port 341, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    qfactor.nSize = sizeof(OMX_IMAGE_PARAM_QFACTORTYPE);
    qfactor.nVersion.nVersion = OMX_VERSION;
    qfactor.nPortIndex = 341;

    rc = OMX_GetParameter(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), 
                          OMX_IndexParamQFactor, 
                          &qfactor);
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to get OMX port param qfactor for port 341, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    qfactor.nQFactor = quality;

    rc = OMX_SetParameter(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), 
                          OMX_IndexParamQFactor,
                          &qfactor);
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to set OMX port param qfactor for port 341, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    if (0 != ilclient_change_component_state(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                             OMX_StateIdle))
    {
        syslog(LOG_ERR, "Failed to set idle state for image encode component");
        exit(EXIT_FAILURE);
    }

    if (0 != ilclient_enable_port_buffers(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                          340, 
                                          NULL, 
                                          NULL, 
                                          NULL))
    {
        syslog(LOG_ERR, "Failed to enable buffers for port 340");
        exit(EXIT_FAILURE);
    }

    if (0 != ilclient_enable_port_buffers(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                          341, 
                                          NULL, 
                                          NULL, 
                                          NULL))
    {
        syslog(LOG_ERR, "Failed to enable buffers for port 341");
        exit(EXIT_FAILURE);
    }

    if (0 != ilclient_change_component_state(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                             OMX_StateExecuting))
    {
        syslog(LOG_ERR, "Failed to set executing state for image encode component");
        exit(EXIT_FAILURE);
    }
}

void codec_jpeg_clean(void)
{
    ilclient_disable_port_buffers(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                  340, 
                                  NULL, 
                                  NULL, 
                                  NULL);
    ilclient_disable_port_buffers(omx.component[OMX_IMAGE_ENCODE_INDEX], 
                                  341, 
                                  NULL, 
                                  NULL, 
                                  NULL);
}

void codec_jpeg_compress(uint8_t *frame, uint32_t size, uint8_t **outbuf, 
                         uint32_t *outsize)
{
    static OMX_BUFFERHEADERTYPE *obuf = NULL;
    static OMX_BUFFERHEADERTYPE *ibuf = NULL;

    *outsize = 0;

    if (ibuf == NULL)
    {
        ibuf = ilclient_get_input_buffer(omx.component[OMX_IMAGE_ENCODE_INDEX], 340, 1);
        if (ibuf == NULL)
            return;
    }

    memcpy(ibuf->pBuffer, frame, size);
    ibuf->nFilledLen = size;

    if (OMX_ErrorNone != OMX_EmptyThisBuffer(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), ibuf))
        return;

    if (obuf == NULL)
    {
        obuf = ilclient_get_output_buffer(omx.component[OMX_IMAGE_ENCODE_INDEX], 341, 1);
        if (obuf == NULL)
            return;
    }

    if (OMX_ErrorNone != OMX_FillThisBuffer(ILC_GET_HANDLE(omx.component[OMX_IMAGE_ENCODE_INDEX]), obuf))
        return;

    sem_wait(&omx.semaphore[OMX_IMAGE_ENCODE_INDEX]);

    *outbuf = malloc(obuf->nFilledLen);

    memcpy(*outbuf, obuf->pBuffer, obuf->nFilledLen);

    *outsize = obuf->nFilledLen;

    obuf->nFilledLen = 0;
}

void codec_init(void)
{
    OMX_ERRORTYPE rc;
    uint32_t i;

    bcm_host_init();

    omx.client = ilclient_init();
    if (!omx.client)
    {
        syslog(LOG_ERR, "Failed to init ilclient");
        exit(EXIT_FAILURE);
    }

    rc = OMX_Init();
    if (rc != OMX_ErrorNone)
    {
        syslog(LOG_ERR, "Failed to init OMX, error: %x", rc);
        exit(EXIT_FAILURE);
    }

    ilclient_set_fill_buffer_done_callback(omx.client, 
                                           omx_buffer_fill_done, 
                                           NULL);

    for (i = 0; i < OMX_COMPONENT_MAX_INDEX; i++)
        sem_init(&omx.semaphore[i], 0, 0);
}

void codec_clean(void)
{
    uint32_t i;

    ilclient_cleanup_components(omx.component);

    OMX_Deinit();

    ilclient_destroy(omx.client);

    for (i = 0; i < OMX_COMPONENT_MAX_INDEX; i++)
        sem_destroy(&omx.semaphore[i]);
}

