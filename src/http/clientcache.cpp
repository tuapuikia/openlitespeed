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
#include "clientcache.h"
#include "clientinfo.h"
#include "util/datetime.h"
#include "httplog.h"
#include <http/iptogeo.h>
#include <socket/gsockaddr.h>
#include <lsiapi/lsiapi.h>

#include <util/accesscontrol.h>
#include <util/accessdef.h>
#include <util/autobuf.h>
#include <util/objpool.h>
#include <util/staticobj.h>

#include <unistd.h>

static StaticObj< ObjPool< ClientInfo > > s_pool;

ClientCache *ClientCache::s_pClients = NULL;

void ClientCache::initObjPool()
{
    s_pool.construct();
}

void ClientCache::clearObjPool()
{
    s_pool()->release();
    s_pool.destruct();
}



ClientCache::ClientCache(int initSize)
    : m_v4(initSize, NULL, NULL)
    , m_v6(initSize, GHash::hfIpv6, GHash::cmpIpv6)
{}

ClientCache::~ClientCache()
{
}

ClientCache::const_iterator ClientCache::find(const struct sockaddr *pAddr)
const
{
    switch (pAddr->sa_family)
    {
    case AF_INET:
        return m_v4.find((void *)(unsigned long)(((sockaddr_in *)
                         pAddr)->sin_addr.s_addr));
    case AF_INET6:
        return m_v6.find((void *)(&((sockaddr_in6 *)pAddr)->sin6_addr));
    default:
        return m_v4.end();
    }
}


void ClientCache::add(ClientInfo *pInfo)
{
    const struct sockaddr *pAddr = pInfo->getAddr();
    switch (pAddr->sa_family)
    {
    case AF_INET:
        m_v4.insert((void *)(unsigned long)(((sockaddr_in *)
                                             pAddr)->sin_addr.s_addr),
                    pInfo);
        break;
    case AF_INET6:
        m_v6.insert((void *)(&((sockaddr_in6 *)pAddr)->sin6_addr), pInfo);
        break;
    }
}

void ClientCache::del(ClientInfo *pInfo)
{
    del(pInfo->getAddr());
}

ClientInfo *ClientCache::del(const struct sockaddr *pAddr)
{
    Cache::iterator iter;
    switch (pAddr->sa_family)
    {
    case AF_INET:
        iter = m_v4.find(
                   (void *)(unsigned long)(((sockaddr_in *)pAddr)->sin_addr.s_addr));
        if (iter != m_v4.end())
        {
            ClientInfo *pInfo = iter.second();
            m_v4.erase(iter);
            return pInfo;
        }
        break;
    case AF_INET6:
        iter = m_v6.find(
                   (void *)(&((sockaddr_in6 *)pAddr)->sin6_addr));
        if (iter != m_v6.end())
        {
            ClientInfo *pInfo = iter.second();
            m_v6.erase(iter);
            return pInfo;
        }
        break;
    }
    return NULL;
}

void ClientCache::recycle(ClientInfo *pInfo)
{
    LsiapiBridge::releaseModuleData(LSI_MODULE_DATA_IP,
                                    pInfo->getModuleData());
    s_pool()->recycle(pInfo);

}

void ClientCache::clean(Cache *pCache)
{
    ClientInfo *pInfo;
    Cache::iterator iter;
    for (iter = pCache->begin(); iter != pCache->end();)
    {
        pInfo = iter.second();
        if (!pInfo)
        {
            Cache::iterator iterDel = iter;
            iter = pCache->next(iter);
            pCache->erase(iterDel);
        }
        else
        {
            if ((pInfo->getConns() == 0)
                && (!pInfo->getOverLimitTime())
                && (DateTime::s_curTime - pInfo->getLastConnTime() > 300))
            {
                s_pool()->recycle(iter.second());
                Cache::iterator iterDel = iter;
                iter = pCache->next(iter);
                pCache->erase(iterDel);
            }
            else
            {
                pInfo->getThrottleCtrl().resetQuotas();
                iter = pCache->next(iter);
            }
        }
    }
    for (ClientList::iterator it = m_toBeRemoved.begin();
         it != m_toBeRemoved.end();)
    {
        if ((*it)->getConns() == 0)
        {
            s_pool()->recycle(*it);
            it = m_toBeRemoved.erase(it);
        }
        else
            ++it;
    }

}

void ClientCache::clean()
{
    clean(&m_v4);
    clean(&m_v6);
}

int ClientCache::writeBlockedIP(AutoBuf *pBuf, Cache *pCache)
{
    ClientInfo *pInfo;
    Cache::iterator iter;
    char achBuf[8192];
    char *p = achBuf;
    char *pEnd = p + sizeof(achBuf);
    int len;
    for (iter = pCache->begin(); iter != pCache->end();
         iter = pCache->next(iter))
    {
        pInfo = iter.second();
        if ((pInfo) && (pInfo->getAccess() == AC_BLOCK))
        {
            memmove(p, pInfo->getAddrString(), pInfo->getAddrStrLen());
            p += pInfo->getAddrStrLen();
            *p++ = ',';
            if (pEnd - p < 1024)
            {
                len = p - achBuf;
                if (pBuf->append(achBuf, len) != len)
                    return LS_FAIL;
                p = achBuf;
            }
        }
    }
    len = p - achBuf;
    if (len > 0)
        if (pBuf->append(achBuf, len) != len)
            return LS_FAIL;
    return 0;
}

int ClientCache::generateBlockedIPReport(int fd)
{
    AutoBuf buf(1024);
    buf.append("BLOCKED_IP: ", 12);
    writeBlockedIP(&buf, &m_v4);
    writeBlockedIP(&buf, &m_v6);
    buf.appendUnsafe('\n');
    write(fd, buf.begin(), buf.size());
    return 0;
}



ClientInfo *ClientCache::newClient(const struct sockaddr *pAddr)
{
    ClientInfo *pInfo = s_pool()->get();
    if (pInfo)
    {
        pInfo->setAddr(pAddr);
        add(pInfo);
    }
    return pInfo;
}

#include <util/accessdef.h>
static int resetQuotas(const void *pKey, void *pData)
{
    ClientInfo *pInfo = ((ClientInfo *)pData);
    if (!pInfo)
        return 0;
    pInfo->setSslNewConn(0);
    pInfo->getThrottleCtrl().resetQuotas();
    time_t tm = pInfo->getOverLimitTime();
    if (tm)
    {
        if (pInfo->getAccess() == AC_BLOCK)
        {
            if (DateTime::s_curTime - tm > ClientInfo::getBanPeriod())
            {
                pInfo->setOverLimitTime(0);
                pInfo->setAccess(AC_ALLOW);
            }
        }
        else if (((int)pInfo->getConns() <=
                  (ClientInfo::getPerClientSoftLimit()>> 1)) &&
                 (DateTime::s_curTime - tm < ClientInfo::getOverLimitGracePeriod()))
            pInfo->setOverLimitTime(0);
    }
    return 0;
}

static int setThrottleLimit(const void *pKey, void *pData)
{
    ClientInfo *pInfo = ((ClientInfo *)pData);
    if (pInfo)
        pInfo->getThrottleCtrl().adjustLimits(ThrottleControl::getDefault());
    return 0;
}

void ClientCache::resetThrottleLimit()
{
    m_v4.for_each(m_v4.begin(), m_v4.end(), setThrottleLimit);
    m_v6.for_each(m_v6.begin(), m_v6.end(), setThrottleLimit);
}


void ClientCache::onTimer()
{
    m_v4.for_each(m_v4.begin(), m_v4.end(), resetQuotas);
    m_v6.for_each(m_v6.begin(), m_v6.end(), resetQuotas);
}

int ClientCache::appendDirtyList(const void *pKey, void *pData,
                                 void *pList)
{
    ((ClientCache::ClientList *)pList)->push_back(pData);
    return 0;
}

void ClientCache::dirtyAll()
{
    m_v4.for_each2(m_v4.begin(), m_v4.end(), appendDirtyList, &m_toBeRemoved);
    m_v4.clear();
    m_v6.for_each2(m_v6.begin(), m_v6.end(), appendDirtyList, &m_toBeRemoved);
    m_v6.clear();
}


ClientInfo *ClientCache::getClientInfo(struct sockaddr *pPeer)
{
    // use this to save client information
    // register TShmClient * pShmClient;

    const_iterator iter = find(pPeer);;
    ClientInfo *pInfo;
    if (!iter)
    {
        //printf( "create new client info!\n" );
        pInfo = newClient(pPeer);
        if (!pInfo)
        {
            ERR_NO_MEM("ClientCache->newClient()");
            return NULL;
        }
        // pShmClient = pInfo->getShmClientInfo();

        // GeoIP lookup
        if (IpToGeo::getIpToGeo() != NULL)
        {
            if (pPeer->sa_family == AF_INET)
            {
                GeoInfo *pGeoInfo = pInfo->allocateGeoInfo();
                if (pGeoInfo)
                {
                    IpToGeo::getIpToGeo()->lookUp(
                        ((struct sockaddr_in *)pPeer)->sin_addr.s_addr, pGeoInfo);
                }
            }
            else if (pPeer->sa_family == AF_INET6)
            {
                GeoInfo *pGeoInfo = pInfo->allocateGeoInfo();
                if (pGeoInfo)
                {
                    IpToGeo::getIpToGeo()->lookUpV6(
                        ((struct sockaddr_in6 *)pPeer)->sin6_addr, pGeoInfo);
                }

            }
        }

        if (AccessControl::getAccessCtrl())
            pInfo->setAccess(AccessControl::getAccessCtrl()->hasAccess(pPeer));
        if (pInfo->getAccess() == AC_TRUST)
            pInfo->getThrottleCtrl().setUnlimited();
        else
            pInfo->getThrottleCtrl().adjustLimits(ThrottleControl::getDefault());
        pInfo->getThrottleCtrl().resetQuotas();

        //perform domain name lookup
        //if ( isDNSLookupEnabled() )
#ifdef  USE_UDNS
        if (HttpServerConfig::getInstance().getDnsLookup())
            Adns::getInstance().getHostByAddr(pInfo, pPeer);
#endif

#ifdef  USE_CARES
        if (HttpServerConfig::getInstance().getDnsLookup())
            Adns::getHostByAddr(pInfo, pPeer);
#endif



    }
    else
    {
        pInfo = iter.second();
        // pShmClient = pInfo->getShmClientInfo();

        if (AccessControl::getAccessCtrl())
            pInfo->setAccess(AccessControl::getAccessCtrl()->hasAccess(pPeer));
        else
            pInfo->setAccess(AC_ALLOW);
    }
    pInfo->hit(DateTime::s_curTime);
    return pInfo;
}


