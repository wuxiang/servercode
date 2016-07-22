#ifndef WORKBASE_H_
#define WORKBASE_H_

class WorkBase {
public:
    WorkBase(): m_id(-1) {
    }

    WorkBase(const std::size_t& id): m_id(id) {
    }

    virtual ~WorkBase() {
    }

    virtual bool addFd(int fd) = 0;
    virtual bool stop() = 0;

protected:
    std::size_t                                            m_id;
};

#endif //WORKBASE_H_

