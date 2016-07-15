#ifndef  WORKTHREADS_H_
#define  WORKTHREADS_H_
#include <sys/epoll.h>
#include <pthread.h>

#include <string>
#include <set>

#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "myUtil.h"
#include "globalVariable.h"
#include "serverBuffer.h"

class WorkThreads: public boost::noncopyable {
public:
    WorkThreads(const std::size_t& ids);
    ~WorkThreads();
    bool addFd(int fd);

    bool stop();

private:
    WorkThreads();
    void init();
    void run();

private:
    std::size_t                        m_id;
    boost::thread                      m_thread;
    int                                m_epfd;

    volatile bool                      m_runing;
    std::map<int, ServerBufferPtr>     m_recvBuffer;


    boost::shared_mutex                m_mtx;
    std::set<int>                      m_listenFds;
};

#endif //WORKTHREADS_H_

