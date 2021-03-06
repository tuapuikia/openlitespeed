/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#include "../include/ls.h"
#include <lsr/ls_loopbuf.h>

#include <stdlib.h>
#include <memory.h>
//#include <zlib.h>
#include <string.h>


/////////////////////////////////////////////////////////////////////////////
//DEFINE the module name, MUST BE the same as .so file name
//ie.  if MNAME is testmodule, then the .so must be testmodule.so
#define     MNAME       updatetcpout1
/////////////////////////////////////////////////////////////////////////////

lsi_module_t MNAME;
#define MAX_BLOCK_BUFSIZE   8192

typedef struct _MyData
{
    ls_loopbuf_t writeBuf;
} MyData;

int l4release(void *data)
{
    MyData *myData = (MyData *)data;
    g_api->log(NULL, LSI_LOG_DEBUG, "#### updatetcpout1 test %s\n",
               "l4release");

    if (myData)
    {
        ls_loopbuf_d(&myData->writeBuf);
        free(myData);
    }

    return 0;
}


int l4init(lsi_cb_param_t *rec)
{

    MyData *myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME,
                     LSI_MODULE_DATA_L4);
    if (!myData)
    {
        myData = (MyData *) malloc(sizeof(MyData));
        ls_loopbuf(&myData->writeBuf, MAX_BLOCK_BUFSIZE);

        g_api->log(NULL, LSI_LOG_DEBUG, "#### updatetcpout1 test %s\n", "l4init");

        g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_L4,
                               (void *)myData);
    }
    else
        ls_loopbuf_clear(&myData->writeBuf);
    return 0;
}

static int l4send(lsi_cb_param_t *rec)
{
    MyData *myData = NULL;
    char *pBegin;
    struct iovec *iov = (struct iovec *)rec->_param;
    int count = rec->_param_len;
    int total = 0;
    char s[4] = {0};
    int written = 0;
    int i, j;
    struct iovec iovOut;

    g_api->log(NULL, LSI_LOG_DEBUG, "#### updatetcpoutdata test %s\n",
               "tcpsend");
    myData = (MyData *)g_api->get_module_data(rec->_session, &MNAME,
             LSI_MODULE_DATA_L4);
    if (MAX_BLOCK_BUFSIZE > ls_loopbuf_size(&myData->writeBuf))
    {

        for (i = 0; i < count; ++i)
        {
            pBegin = (char *)iov[i].iov_base;

            for (j = 0; j < iov[i].iov_len; ++j)
            {
                sprintf(s, "=%02X", (unsigned char)pBegin[j]);
                ls_loopbuf_append(&myData->writeBuf, s, 3);
                ++ total;
            }
        }
    }

    ls_loopbuf_straight(&myData->writeBuf);
    iovOut.iov_base = ls_loopbuf_begin(&myData->writeBuf);
    iovOut.iov_len = ls_loopbuf_size(&myData->writeBuf);
    written = g_api->stream_writev_next(rec, &iovOut, 1);
    ls_loopbuf_popfront(&myData->writeBuf, written);

    g_api->log(NULL, LSI_LOG_DEBUG,
               "#### updatetcpoutdata test, next caller written %d, return %d, left %d\n",
               written, total, ls_loopbuf_size(&myData->writeBuf));

    int hasData = 1;
    if (ls_loopbuf_size(&myData->writeBuf))
        rec->_flag_out = (void *)&hasData;

    return total;
}

static lsi_serverhook_t serverHooks[] =
{
    {LSI_HKPT_L4_BEGINSESSION, l4init, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_L4_SENDING, l4send, LSI_HOOK_EARLY, LSI_HOOK_FLAG_TRANSFORM | LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int init(lsi_module_t *pModule)
{
    g_api->init_module_data(pModule, l4release, LSI_MODULE_DATA_L4);
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, NULL, NULL, "", serverHooks};
