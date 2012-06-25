/**************************************************************************
**
** 	Copyright 2010 Duke Inc.
**
**************************************************************************/

#ifndef _NB_NET_TOOL_H_
#define _NB_NET_TOOL_H_

// Posix header files
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <assert.h>

// C++ header file
#include <string>
#include <vector>

// Boost header file
#include <boost/noncopyable.hpp>

class auto_ifaddrs : private boost::noncopyable
{
public:
    auto_ifaddrs(struct ifaddrs* ifa) : m_ifaddr(ifa)
    { }
    ~auto_ifaddrs()
    {
        if (m_ifaddr != NULL)
        {
            freeifaddrs(m_ifaddr);
            m_ifaddr = NULL;
        }
    }
private:
    struct ifaddrs* m_ifaddr;
};

inline int nb_getnameinfo(sockaddr* sa, std::string& hostname)
{
    char hbuf[NI_MAXHOST] = { 0 };
    socklen_t len = (sa->sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    int ret = getnameinfo(sa, len, hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST);
    if (ret == 0)
    {
        hostname = hbuf;
    }
    return ret;
}

inline bool nb_getifaddrs_v4(std::vector<in_addr_t>& hosts)
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1)
        return false;
    auto_ifaddrs autoifa(ifaddr);

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP))
        {
            struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            hosts.push_back(sin->sin_addr.s_addr);
        }
    }

    return true;
}

inline bool nb_getifaddrs_v4(std::vector<std::string>& hosts)
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) == -1)
        return false;
    auto_ifaddrs autoifa(ifaddr);

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP))
        {
            std::string strhost;
            if (nb_getnameinfo(ifa->ifa_addr, strhost) != 0)
                return false;
            hosts.push_back(strhost);
        }
    }

    return true;
}

// only get the first addr
inline std::string nb_getifaddr_v4()
{
    std::vector<std::string> hosts;
    bool result = nb_getifaddrs_v4(hosts);

    if(result && !hosts.empty())
    {        
        return hosts.front();
    }
    else
    {
        return std::string();
    }    
}


// from header_buf[header_sz], parse the body size(in host bytes order)
inline
size_t parse_header(const unsigned char* header_buf, size_t header_sz)
{
    size_t len = 0;
    for (size_t i = 0; i < header_sz; ++i)
    {
        len |= ((size_t)header_buf[i] << (i*8));
    }
    
    return ntohl(len);
}


// encode bytes(in host bytes order) into header_buf[header_sz] for network transfer
inline
void build_header(size_t bytes, unsigned char* header_buf, size_t header_sz)
{
    size_t len = htonl(bytes);        

    for (size_t i = 0; i < header_sz; ++i)
    {
        header_buf[i] = ( len >> (8*i) ) & 0xff;
    }
}


#endif /* _NB_NET_TOOL_H_ */
