#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Buffer.h"
#include "Task.h"
#include "Printer.h"

static const int s_blockSize = 16 * 1024;

void Balau::Buffer::close() throw (GeneralException) {
    reset();
}

ssize_t Balau::Buffer::read(void * buf, size_t count) throw (GeneralException) {
    off_t cursor = rtell();
    if (cursor >= m_bufSize)
        return 0;
    off_t avail = m_bufSize - cursor;

    if (count > avail)
        count = avail;

    memcpy(buf, m_buffer + cursor, count);

    rseek(cursor + count);

    return count;
}

ssize_t Balau::Buffer::write(const void * buf, size_t count) throw (GeneralException) {
    off_t cursor = wtell();
    off_t end = cursor + count;
    off_t endBlock = (end / s_blockSize) + ((end % s_blockSize) ? 1 : 0);
    off_t oldEndBlock = m_numBlocks;

    if (endBlock > oldEndBlock) {
        m_buffer = (uint8_t *) realloc(m_buffer, endBlock * s_blockSize);
        memset(m_buffer + oldEndBlock * s_blockSize, 0, (endBlock - oldEndBlock) * s_blockSize);
    }

    memcpy(m_buffer + cursor, buf, count);

    wseek(cursor + count);

    return count;
}

void Balau::Buffer::reset() {
    m_buffer = (uint8_t *) realloc(m_buffer, 0);
    m_bufSize = 0;
    m_numBlocks = 0;
    wseek(0);
    rseek(0);
}

bool Balau::Buffer::isClosed() {
    return false;
}

const char * Balau::Buffer::getName() {
    return "Buffer";
}

off_t Balau::Buffer::getSize() {
    return m_bufSize;
}

bool Balau::Buffer::canRead() {
    return true;
}

bool Balau::Buffer::canWrite() {
    return true;
}
