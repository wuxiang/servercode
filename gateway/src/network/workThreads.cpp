#include "workThreads.h"
#include "log_module.h"
#include "configSingleton.h"

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

void WorkThreads::shutdownfd(const int fd) {
    // 关闭套节字
    close(fd);
    
    // 清理关闭套接字的接受缓存
    m_recvBuffer.erase(fd);
    
    // 清理要发送到已关闭套接字的数据
    boost::unique_lock<boost::shared_mutex>  lock(m_mtx);
    m_listenFds.erase(fd);
    m_sendBuffer.erase(fd);
}

void WorkThreads::run() {
    m_runing = true;

    struct epoll_event* events = (struct epoll_event*)calloc(EPOLL_SIZE, sizeof(struct epoll_event));
    int  num = 0;

    while (m_runing) {
        num = epoll_wait(m_epfd, events, EPOLL_SIZE, 500);
        for (int i = 0; i < num; ++i) {
            try {
                if (events[i].events & EPOLLERR 
                    || events[i].events & EPOLLHUP 
                    || (!events[i].events & EPOLLIN && !events[i].events & EPOLLOUT)) {

                    this->shutdownfd(events[i].data.fd);
                    continue;
                } else if (events[i].events & EPOLLIN) {
                    std::map<int, ServerBufferPtr>::iterator it = m_recvBuffer.find(events[i].data.fd);
                    if (it == m_recvBuffer.end()) {
                        m_recvBuffer[events[i].data.fd] = ServerBufferPtr(new (std::nothrow) ServerBuffer(4096));
                    }

                    ServerBuffer&  rcv = *(m_recvBuffer[events[i].data.fd]);

                    // 扩大buffer大小
                    if (0 == rcv.getAvailableSize() && rcv.moveData() && 0 == rcv.getAvailableSize()) {
                        rcv.doubleSize();
                    }

                    // 读取数据
                    int done = 0;
                    std::size_t rs = read(events[i].data.fd, rcv.buffer, rcv.getAvailableSize());
                    if (-1 == rs) {
                        DA_LOG("gateway", Elephants::NOTICE, "fd[%d] error", events[i].data.fd);
                        done = 1;
                    } else if (0 == rs) {
                        DA_LOG("gateway", Elephants::NOTICE, "fd[%d] has been closed", events[i].data.fd);
                    }

                    if (1 == done) {
                        this->shutdownfd(events[i].data.fd);
                    }

                    // 检查拆分数据包
                    uint32_t package = *((uint32_t*)((char*)rcv.buffer + rcv.start));
                    if (package + sizeof(uint16_t) + sizeof(uint32_t) <= rcv.getDataSize()) {
                        // 获取服务ID
                        uint16_t  service = *((uint16_t*)((char*)rcv.buffer + rcv.start + sizeof(uint32_t)));

                        // 打包application请求数据包
                        std::size_t  sz = sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint16_t) + package;
                        ServerBufferPtr   dptr(new (std::nothrow) ServerBuffer(sz));
                        // 1, length
                        uint32_t* pLength = (uint32_t*)((char*)(dptr->buffer));
                        *pLength = package;

                        // 2, sequence id
                        uint64_t* pSeq = (uint64_t*)((char*)(dptr->buffer) + sizeof(uint32_t));
                        *pSeq = SequenceInstance::instance().getSequence();

                        // 3, version
                        uint16_t* pVersion = (uint16_t*)((char*)(dptr->buffer) + sizeof(uint32_t) + sizeof(uint64_t));
                        *pVersion = ConfigSingleton::instance().getValue<uint16_t>(boost::lexical_cast<std::string>(service), std::string("version"));

                        // 4, body数据
                        void* dst = (void*)((char*)(dptr->buffer) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint16_t));
                        void* src = (void*)((char*)rcv.buffer + rcv.start + sizeof(uint32_t) + sizeof(uint16_t));
                        memcpy(dst, src, package);

                        // call application client
                    }
                } else if (events[i].events & EPOLLOUT) {
                    DA_LOG("gateway", Elephants::DEBUG, "write event");
                }
            } catch (...) {
                this->shutdownfd(events[i].data.fd);
            }
        }
    }
}

