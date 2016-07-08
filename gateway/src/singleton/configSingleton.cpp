#include "configSingleton.h"


ConfigSingleton&  ConfigSingleton::instance() {
    static ConfigSingleton  handler;
    return handler;
}

ConfigSingleton::ConfigSingleton(): Config(INI), XMLParse(XML) {
    fprintf(stderr, "ConfigSingleton::ConfigSingleton()\n");
}
