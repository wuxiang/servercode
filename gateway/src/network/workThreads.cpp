#include "workThreads.h"

WorkThreads::WorkThreads(const std::size_t& ids): m_id(ids) {
}

WorkThreads::~WorkThreads() {
}

void WorkThreads::init() {
}

bool WorkThreads::addFd(const int fd) {
    return true;
}

void WorkThreads::run(void* object) {
}
