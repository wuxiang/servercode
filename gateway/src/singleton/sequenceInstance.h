#ifndef SEQUENCEINSTANCE_H_
#define SEQUENCEINSTANCE_H_
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

class SequenceInstance: public boost::noncopyable {
public:
    static SequenceInstance&  instance();
    uint64_t   getSequence();

private:
    SequenceInstance();

private:
    boost::mutex      m_mtx;
    uint64_t          m_idx;
};

#endif //SEQUENCEINSTANCE_H_

