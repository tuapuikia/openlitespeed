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
#include "cacheentry.h"

#include <string.h>
#include <unistd.h>

CacheEntry::CacheEntry()
    : m_iLastAccess(0)
    , m_iHits(0)
    , m_iMaxStale(0)
    , m_startOffset(0)
    , m_iFdStore(-1)
{
}


CacheEntry::~CacheEntry()
{
    if (m_iFdStore != -1)
        close(m_iFdStore);
}


int CacheEntry::setKey(const CacheHash &hash,
                       const char *pURI, int iURILen,
                       const char *pQS, int iQSLen,
                       const char *pIP, int ipLen,
                       const char *pCookie, int cookieLen)
{
    m_hashKey.init(hash);
    int len = iURILen + ((iQSLen > 0) ? iQSLen + 1 : 0);
    int l;
    if (ipLen > 0)
    {
        len += ipLen + 1;
        if (cookieLen > 0)
            len += cookieLen + 1;
    }

    char *pBuf = m_sKey.prealloc(len + 1);
    if (!pBuf)
        return LS_FAIL;
    memmove(pBuf, pURI, iURILen + 1);
    l = iURILen;
    if (iQSLen > 0)
    {
        pBuf[ l++ ] = '?';
        memmove(pBuf + l , pQS, iQSLen + 1);
        l += iQSLen;
    }
    if (ipLen > 0)
    {
        pBuf[l++] = '@';
        memmove(pBuf + l, pIP, ipLen);
        l += ipLen;
        if (cookieLen > 0)
        {
            pBuf[l++] = '~';
            memmove(pBuf + l , pCookie, cookieLen);
            l += cookieLen;
        }
    }
    m_header.m_iKeyLen = len;
    return 0;
}

int CacheEntry::verifyKey(
    const char *pURI, int iURILen,
    const char *pQS, int iQSLen,
    const char *pIP, int ipLen,
    const char *pCookie, int cookieLen) const
{
    const char *p = m_sKey.c_str();
    if (strncmp(pURI, p, iURILen) != 0)
        return LS_FAIL;
    p += iURILen;
    if (iQSLen > 0)
    {
        if ((*p  != '?') ||
            (memcmp(p + 1, pQS, iQSLen) != 0))
            return LS_FAIL;
        p += iQSLen + 1;
    }

    if (ipLen > 0)
    {
        if ((*p  != '@') ||
            (memcmp(p + 1, pIP, ipLen) != 0))
            return LS_FAIL;
        p += ipLen + 1;
        if (cookieLen > 0)
        {
            if ((*p  != '~') ||
                (memcmp(p + 1, pCookie, cookieLen) != 0))
                return LS_FAIL;
            p += cookieLen + 1;
        }
    }
    if (m_header.m_iKeyLen > p - m_sKey.c_str())
        return LS_FAIL;
    return 0;
}



