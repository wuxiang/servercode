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
    fprintf(stderr, "%p\n", b);

    fprintf(stderr, "object: %lu\n", sizeof(class WorkBase));

    fprintf(stderr, "char*: %lu\n", sizeof(char*));
    fprintf(stderr, "char**: %lu\n", sizeof(char**));
    fprintf(stderr, "int*: %lu\n", sizeof(int*));

    Elephants::io_service_manager::instance().run();

    return 0;
}
