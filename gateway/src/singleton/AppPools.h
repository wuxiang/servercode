#ifndef     APPPOOLS_H_
#define     APPPOOLS_H_
#include <sys/epoll.h>

#include <map>

#include <boost/noncopyable.hpp>
#include "appBase.h"

class AppPools: public boost::noncopyable {
public:
    static AppPools& getInstance();

private:
    AppPools();
    void init();
    void run();

private:
    std::map<__uint64_t, AppBasePtr>   m_apps;
};

#endif //APPPOOLS_H_

