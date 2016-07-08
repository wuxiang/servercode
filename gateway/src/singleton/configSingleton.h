#ifndef CONFIGSINGLETON_H_
#define CONFIGSINGLETON_H_
#include <boost/noncopyable.hpp>

#include "Config.h"
#include "xmlparse.h"

const char* const INI = "config.ini";
const char* const XML = "config.xml";

class ConfigSingleton: public boost::noncopyable, public Config, public XMLParse {
    public:
        static ConfigSingleton&  instance();

    private:
        ConfigSingleton();
};

#endif

