#ifndef _INCLUDE_THREAD_MUTEX_H
#define _INCLUDE_THREAD_MUTEX_H

#include <pthread.h>

class CThreadMutex
{
public:
    CThreadMutex();
    ~CThreadMutex();
    bool lock();
    void unlock();
    bool try_lock();

private:
    pthread_mutex_t m_cs;
    bool m_bIsValid;
};


#endif		/* _INCLUDE_THREAD_MUTEX_H */

