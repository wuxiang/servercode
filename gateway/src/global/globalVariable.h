#ifndef GLOBALVARIABLE_H_
#define GLOBALVARIABLE_H_
#include <boost/function.hpp>

#include "mailBox.h"
#include "serverBuffer.h"

typedef boost::function<void (MailBox&  mail, ServerBufferPtr&  buf)> ServerCallback;

const int EPOLL_SIZE = 1024;


#endif //GLOBALVARIABLE_H_

