#ifndef CALLGLOBAL_H_
#define CALLGLOBAL_H_
#include <set>
#include <string>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include "configSingleton.h"

boost::once_flag once = BOOST_ONCE_INIT;

class CallGlobal {
    public:
        CallGlobal() {
            fprintf(stderr, "callglobal\n");
            boost::call_once(&(CallGlobal::init), once);
        }

    private:
        static void init() {
            // config
            ConfigSingleton::instance();
        }
};

CallGlobal   INITIALIZER;
#endif

