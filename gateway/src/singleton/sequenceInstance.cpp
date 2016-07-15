#include "sequenceInstance.h"

SequenceInstance::SequenceInstance(): m_idx(0) {
}

SequenceInstance&  SequenceInstance::instance() {
    static SequenceInstance  instance;
    return instance;
}

uint64_t   SequenceInstance::getSequence() {
    boost::lock_guard<boost::mutex>  lock(m_mtx);
    return m_idx++;
}
