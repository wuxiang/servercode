#ifndef APPSVRCLIENT_H_
#define APPSVRCLIENT_H_
#include <string>
#include <set>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "serverBuffer.h"
#include "appBase.h"
#include "globalVariable.h"

class AppSvrClient: public boost::noncopyable, public AppBase {
public:
    AppSvrClient(const uint16_t service, const std::size_t& id);
    virtual ~AppSvrClient();

private:
    std::map<int, ServerBufferPtr>                       m_rcvBuf;

    boost::shared_mutex                                  m_mtx;
    std::set<int>                                        m_listenFds;
    std::map<int, std::map<uint64_t, ServerBufferPtr> >  m_sendBuf;
    std::map<uint64_t, ServerCallback>                   m_callback;
};

#endif //APPSVRCLIENT_H_

