#include <stdio.h>
#include <iostream>

#include "io_service_manager.h"
#include "configSingleton.h"
#include "server.h"

void signalFunc(const int sig) {
}

int main(int argc, char* argv[]) {
    // 初始化asio线程
    Elephants::io_service_manager::instance().init(ConfigSingleton::instance().getValue<std::size_t>("gateway", "asio"));

    // server
    Server   gateway;

    char*  a = new char[20];
    void* b = (void*)a;
    fprintf(stderr, "%p", b);

    Elephants::io_service_manager::instance().run();

    return 0;
}
