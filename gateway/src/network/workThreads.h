#ifndef  WORKTHREADS_H_
#define  WORKTHREADS_H_
#include <sys/epoll.h>
#include <pthread.h>

#include <string>
#include <set>

#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>

class WorkThreads: public boost::noncopyable {
public:
    WorkThreads(const std::size_t& ids);
    ~WorkThreads();
    bool addFd(const int fd);

private:
    WorkThreads();
    void init();
    void run(void* object);

private:
    std::size_t           m_id;
    pthread_t             m_pid;

    boost::shared_mutex   m_mtx;
    int                   m_epfd;
    std::set<int>         m_listenFds;
};

#endif //WORKTHREADS_H_

