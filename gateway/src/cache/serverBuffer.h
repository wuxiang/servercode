#ifndef SERVERBUFFER_H_
#define SERVERBUFFER_H_
#include <stdlib.h>
#include <string.h>

#include <boost/shared_ptr.hpp>

struct  ServerBuffer {
    std::size_t   bufferSize;
    std::size_t   start;
    std::size_t   pos;
    void*         buffer;

    ServerBuffer(const std::size_t& sz): bufferSize(sz), start(0), pos(0) {
        buffer = malloc(sz);
        bzero(buffer, sz);
    }

    ServerBuffer(const ServerBuffer& buf) {
        // mem allocate
        bufferSize = buf.bufferSize;
        start = buf.start;
        pos = buf.pos;
        buffer = realloc(buffer, bufferSize);

        // copy content
        bzero(buffer, bufferSize);
        memcpy(buffer, buf.buffer, pos);
    }

    const ServerBuffer& operator=(const ServerBuffer& buf) {
        if (this != &buf) {
            // mem allocate
            bufferSize = buf.bufferSize;
            start = buf.start;
            pos = buf.pos;
            buffer = realloc(buffer, bufferSize);

            // copy content
            bzero(buffer, bufferSize);
            memcpy(buffer, buf.buffer, pos);
        }

        return *this;
    }

    ~ServerBuffer() {
        if (buffer) {
            delete []((char*)buffer);
        }
    }

    bool doubleSize() {
        return this->resize(2 * this->bufferSize);
    }

    bool resize(const std::size_t bs) {
        std::size_t sz = bufferSize;
        if (bs > bufferSize) {
            sz = (bs / bufferSize  + 1) * bufferSize; 
        }
            
        if (sz > this->bufferSize ) {
            // mem allocate
            void* tBuffer = realloc(buffer, bufferSize);
            if (!tBuffer) {
                return false;
            }
            buffer = tBuffer;
            bufferSize = sz;

            bzero((void*)((char*)buffer + pos), bufferSize - pos);
        }

        return true;
    }

    bool moveData() {
        if (0 != start) {
            memmove(buffer, (void*)((char*)buffer + start), pos - start);
            start = 0;
            pos = pos - start;

            bzero((void*)((char*)buffer + pos), bufferSize - pos);
        }

        return true;
    }

    std::size_t  getAvailableSize() {
        return this->bufferSize - this->pos;
    }

    std::size_t getDataSize() {
        return this->pos - this->start;
    }

private:
    ServerBuffer() {
    }
};

typedef boost::shared_ptr<ServerBuffer>    ServerBufferPtr;

#endif //SERVERBUFFER_H_

