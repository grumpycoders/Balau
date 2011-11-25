#include "BStream.h"
#include "Buffer.h"

static const int s_blockSize = 16 * 1024;

Balau::BStream::BStream(const IO<Handle> & h) : m_h(h), m_buffer((uint8_t *) malloc(s_blockSize)), m_availBytes(0), m_cursor(0), m_passThru(false) {
    Assert(m_h->canRead());
    m_name.set("Stream(%s)", m_h->getName());
    if ((m_h.isA<Buffer>()) || (m_h.isA<BStream>()))
        m_passThru = true;
}

void Balau::BStream::close() throw (Balau::GeneralException) {
    if (!m_detached)
        m_h->close();
    free(m_buffer);
    m_availBytes = 0;
    m_cursor = 0;
}

bool Balau::BStream::isClosed() { return m_closed || m_h->isClosed(); }
bool Balau::BStream::isEOF() { return (m_availBytes == 0) && m_h->isEOF(); }
bool Balau::BStream::canRead() { return true; }
const char * Balau::BStream::getName() { return m_name.to_charp(); }
off_t Balau::BStream::getSize() { return m_h->getSize(); }

ssize_t Balau::BStream::read(void * _buf, size_t count) throw (Balau::GeneralException) {
    if (m_passThru)
        return m_h->read(_buf, count);
    uint8_t * buf = (uint8_t *) _buf;
    size_t copied = 0;
    size_t toCopy = count;

    if (m_availBytes != 0) {
        if (toCopy > m_availBytes)
            toCopy = m_availBytes;
        memcpy(buf, m_buffer + m_cursor, toCopy);
        count -= toCopy;
        m_cursor += toCopy;
        m_availBytes -= toCopy;
        copied = toCopy;
        buf += toCopy;
        toCopy = count;
    }

    if (count == 0)
        return copied;

    if (count >= s_blockSize)
        return m_h->read(buf, count) + copied;

    m_cursor = 0;
    Assert(m_availBytes == 0);
    ssize_t r = m_h->read(m_buffer, s_blockSize);
    Assert(r >= 0);
    m_availBytes = r;

    if (toCopy > m_availBytes)
        toCopy = m_availBytes;
    if (toCopy == 0)
        return 0;
    memcpy(buf, m_buffer, toCopy);
    m_cursor += toCopy;
    m_availBytes -= toCopy;
    copied += toCopy;

    return copied;
}

int Balau::BStream::peekNextByte() {
    m_passThru = false;
    if (m_availBytes == 0) {
        uint8_t b;
        ssize_t r = read(&b, 1);
        if (!r)
            return -1;
        Assert(r == 1);
        Assert(m_cursor > 0);
        Assert(m_availBytes < s_blockSize);
        m_cursor--;
        m_availBytes++;
    }

    return m_buffer[m_cursor];
}

Balau::String Balau::BStream::readString(bool putNL) {
    peekNextByte();
    uint8_t * cr, * lf, * nl;
    String ret;
    size_t chunkSize = 0;

    cr = (uint8_t *) memchr(m_buffer + m_cursor, '\r', m_availBytes);
    lf = (uint8_t *) memchr(m_buffer + m_cursor, '\n', m_availBytes);
    if (cr && lf) {
        nl = cr;
        if (lf < cr)
            nl = lf;
    } else if (!cr) {
        nl = lf;
    } else {
        nl = cr;
    }
    while (!nl) {
        chunkSize = m_availBytes;
        ret += String((const char *) m_buffer + m_cursor, chunkSize);
        m_availBytes -= chunkSize;
        m_cursor += chunkSize;
        if (isClosed() || isEOF())
            return ret;
        peekNextByte();
        Assert(m_cursor == 0);
        cr = (uint8_t *) memchr(m_buffer, '\r', m_availBytes);
        lf = (uint8_t *) memchr(m_buffer, '\n', m_availBytes);
        if (cr && lf) {
            nl = cr;
            if (lf < cr)
                nl = lf;
        } else if (!cr) {
            nl = lf;
        } else {
            nl = cr;
        }
    }

    chunkSize = nl - (m_buffer + m_cursor);
    ret += String((const char *) m_buffer + m_cursor, chunkSize);

    m_availBytes -= chunkSize;
    m_cursor += chunkSize;

    char b;
    read(&b, 1);
    if (putNL)
        ret += String(&b, 1);

    if ((b == '\r') && (peekNextByte() == '\n')) {
        read(&b, 1);
        if (putNL)
            ret += String(&b, 1);
    }

    return ret;
}
