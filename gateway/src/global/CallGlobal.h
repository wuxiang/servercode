#ifndef CALLGLOBAL_H_
#define CALLGLOBAL_H_
#include <set>
#include <string>
#include <exception>

#include <boost/thread/once.hpp>
#include <boost/bind.hpp>
#include "configSingleton.h"
#include "log_module.h"

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

            // init log
            std::vector<std::string>  vLog = ConfigSingleton::instance().get_vector("Log");
            if (!Elephants::CLog::instance().init(vLog)) {
                throw std::runtime_error("init log failed");
            }
        }
};

CallGlobal   INITIALIZER;
#endif

