#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Buffer.h"
#include "Task.h"
#include "Printer.h"

static const int s_blockSize = 16 * 1024;

Balau::Buffer::~Buffer() {
    if (!m_fromConst)
        free(m_buffer);
}

void Balau::Buffer::close() throw (GeneralException) {
    reset();
}

ssize_t Balau::Buffer::read(void * buf, size_t count) throw (GeneralException) {
    off64_t cursor = rtell();
    if (cursor >= m_bufSize)
        return 0;
    off64_t avail = m_bufSize - cursor;

    if (count > avail)
        count = avail;

    memcpy(buf, m_buffer + cursor, count);

    rseek(cursor + count);

    return count;
}

ssize_t Balau::Buffer::write(const void * buf, size_t count) throw (GeneralException) {
    if (m_fromConst)
        throw GeneralException("Buffer is read only and can't be written to.");

    off64_t cursor = wtell();
    off64_t end = cursor + count;
    off64_t endBlock = (end / s_blockSize) + ((end % s_blockSize) ? 1 : 0);
    off64_t oldEndBlock = m_numBlocks;

    if (endBlock > oldEndBlock) {
        m_buffer = (uint8_t *) realloc(m_buffer, endBlock * s_blockSize);
        memset(m_buffer + oldEndBlock * s_blockSize, 0, (endBlock - oldEndBlock) * s_blockSize);
        m_numBlocks = endBlock;
    }

    memcpy(m_buffer + cursor, buf, count);

    wseek(cursor + count);

    if (m_bufSize < end)
        m_bufSize = end;

    return count;
}

void Balau::Buffer::reset() {
    if (!m_fromConst) {
        m_buffer = (uint8_t *)realloc(m_buffer, 0);
        m_bufSize = 0;
    }
    m_numBlocks = 0;
    wseek(0);
    rseek(0);
}

void Balau::Buffer::clear() {
    reset();
    m_fromConst = false;
    m_buffer = NULL;
    m_bufSize = 0;
}

void Balau::Buffer::borrow(const uint8_t * buffer, size_t s) {
    clear();
    m_fromConst = true;
    m_buffer = const_cast<uint8_t *>(buffer);
    m_bufSize = s;
}

bool Balau::Buffer::isClosed() { return false; }
bool Balau::Buffer::isEOF() { return rtell() == m_bufSize; }
const char * Balau::Buffer::getName() { return "Buffer"; }
off64_t Balau::Buffer::getSize() { return m_bufSize; }
bool Balau::Buffer::canRead() { return true; }
bool Balau::Buffer::canWrite() { return !m_fromConst; }
