#include "workThreads.h"
#include "log_module.h"

WorkThreads::WorkThreads(const std::size_t& ids): WorkBase(ids), m_epfd(-1), m_runing(false) {
    this->init();
}

WorkThreads::~WorkThreads() {
}

void WorkThreads::init() {
    // init epoll
    m_epfd = epoll_create(EPOLL_SIZE);
    if (m_epfd < 0) {
        throw std::runtime_error("create epoll file description failed");
    }

    if (-1 == MyUtil::set_non_blocking(m_epfd)) {
        throw std::runtime_error("set_non_blocking epoll handler failed");
    }

    // create thread
    m_thread = boost::thread(boost::bind(&WorkThreads::run, this));
    m_thread.detach();
}

bool WorkThreads::addFd(int fd) {
    if (-1 == MyUtil::set_non_blocking(fd)) {
        DA_LOG("gateway", Elephants::WARNING, "[%s: %d] set_non_blocking failed", __FILE__, __LINE__);
        return false;
    }

    struct  epoll_event  event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    event.data.fd = fd;
    if (0 != epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &event)) {
        DA_LOG("gateway", Elephants::WARNING, "[%s: %d] epoll_ctl add client failed", __FILE__, __LINE__);
        return false;
    }

    return true;
}

bool WorkThreads::stop() {
    m_runing = false;
    return m_runing;
}

void  WorkThreads::senderCallback(MailBox&  mail, ServerBufferPtr&  buf) {
}

void WorkThreads::run() {
    m_runing = true;

    struct epoll_event* events = (struct epoll_event*)calloc(EPOLL_SIZE, sizeof(struct epoll_event));
    int  num = 0;

    while (m_runing) {
        num = epoll_wait(m_epfd, events, EPOLL_SIZE, 500);
        for (int i = 0; i < num; ++i) {
            if (events[i].events & EPOLLERR 
                || events[i].events & EPOLLHUP 
                || (!events[i].events & EPOLLIN && !events[i].events & EPOLLOUT)) {
                close(events[i].data.fd);
                m_recvBuffer.erase(events[i].data.fd);
            } else if (events[i].events & EPOLLIN) {
            } else if (events[i].events & EPOLLOUT) {
            }
        }
    }
}

