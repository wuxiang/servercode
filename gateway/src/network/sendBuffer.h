#ifndef SENDBUFFER_H_
#define SENDBUFFER_H_
#include <stdlib.h>
#include <string.h>

struct  SenderBuffer {
    std::size_t   bufferSize;
    std::size_t   start;
    std::size_t   pos;
    void*         buffer;

    SenderBuffer(const std::size_t& sz): bufferSize(sz), start(0), pos(0) {
        buffer = malloc(sz);
        bzero(buffer, sz);
    }

    SenderBuffer(const SenderBuffer& buf) {
        // mem allocate
        bufferSize = buf.bufferSize;
        start = buf.start;
        pos = buf.pos;
        buffer = realloc(buffer, bufferSize);

        // copy content
        bzero(buffer, bufferSize);
        memcpy(buffer, buf.buffer, pos);
    }

    const SenderBuffer& operator=(const SenderBuffer& buf) {
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

    ~SenderBuffer() {
        if (buffer) {
            delete []((char*)buffer);
        }
    }

    bool moveData() {
        if (0 != start) {
            memmove(buffer, buffer + start, pos - start);
            start = 0;
            pos = pos - start;

            bzero(buffer + pos, bufferSize - pos);
        }

        return true;
    }

private:
    SenderBuffer() {
    }
};

#endif //SENDBUFFER_H_

