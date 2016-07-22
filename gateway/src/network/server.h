#ifndef  SERVER_H_
#define  SERVER_H_
#include <boost/noncopyable.hpp>
#include "workThreads.h"

class  Server: public boost::noncopyable {
public:
    Server();
    ~Server();
};

#endif //SERVER_H_

