#include "ZHandle.h"
#include "Task.h"

Balau::ZStream::ZStream(const IO<Handle> & h, int level, header_t header) : m_h(h), m_detached(false), m_closed(false), m_eof(false), m_in(NULL) {
    m_zin.zalloc = m_zout.zalloc = NULL;
    m_zin.zfree = m_zout.zfree = NULL;
    m_zin.opaque = m_zout.opaque = NULL;
    m_zin.next_in = NULL;
    m_zin.avail_in = 0;
    int window = 0;
    switch (header) {
        case ZLIB: window = 15; break;
        case GZIP: window = 31; break;
        case RAW: window = -15; break;
    }
    int r;
    r = inflateInit2(&m_zin, window);
    EAssert(r == Z_OK, "inflateInit2 returned %i", r);
    r = deflateInit2(&m_zout, level, Z_DEFLATED, window, 9, Z_DEFAULT_STRATEGY);
    EAssert(r == Z_OK, "deflateInit2 returned %i", r);
    m_name.set("ZStream(%s)", m_h->getName());
}

void Balau::ZStream::close() throw (GeneralException) {
    if (m_in) {
        free(m_in);
        m_in = NULL;
    }
    if (m_h->canWrite())
        finish();
    inflateEnd(&m_zin);
    deflateEnd(&m_zout);
    if (!m_detached)
        m_h->close();
    m_closed = true;
}

bool Balau::ZStream::isClosed() {
    return m_closed;
}

bool Balau::ZStream::isEOF() {
    if (m_closed || m_eof)
        return true;
    return m_h->isEOF();
}

bool Balau::ZStream::canRead() {
    return m_h->canRead();
}

bool Balau::ZStream::canWrite() {
    return m_h->canWrite();
}

const char * Balau::ZStream::getName() {
    return m_name.to_charp();
}

static const int BLOCK_SIZE = 1024;

ssize_t Balau::ZStream::read(void * buf, size_t count) throw (GeneralException) {
    if (m_closed || m_eof)
        return 0;

    AAssert(m_h->canRead(), "Can't call ZStream::read on a non-readable handle.");

    size_t readTotal = 0;
    m_zin.next_out = (Bytef *) buf;
    m_zin.avail_out = count;
    if (!m_in) {
        m_zin.next_in = m_in = (uint8_t *) malloc(BLOCK_SIZE);
        m_zin.avail_in = 0;
    }
    while ((count != 0) && !m_h->isClosed() && !m_h->isEOF()) {
        if (m_zin.avail_in == 0) {
            m_zin.next_in = m_in;
            size_t r = m_h->read(m_in, BLOCK_SIZE);
            if (r <= 0)
                return readTotal;
        }
        Task::yield(NULL);
        int r = inflate(&m_zin, Z_SYNC_FLUSH);
        size_t didRead = count - m_zin.avail_out;
        readTotal += didRead;
        count -= didRead;
        if (r == Z_STREAM_END) {
            m_eof = true;
            break;
        }
        EAssert(r == Z_OK, "inflate() didn't return Z_OK but %i", r);
    }
    return readTotal;
}

ssize_t Balau::ZStream::write(const void * buf, size_t count) throw (GeneralException) {
    if (m_closed || m_eof)
        return 0;

    AAssert(m_h->canWrite(), "Can't call ZStream::write on a non-writable handle.");

    size_t wroteTotal = 0;
    m_zout.next_in = (Bytef *) const_cast<void *>(buf);
    m_zout.avail_in = count;
    void * obuf = alloca(BLOCK_SIZE);
    while ((count != 0) && !m_h->isClosed() && !m_h->isEOF()) {
        m_zout.next_out = (Bytef *) obuf;
        m_zout.avail_out = BLOCK_SIZE;
        int r = deflate(&m_zout, Z_NO_FLUSH);
        EAssert(r == Z_OK, "deflate() didn't return Z_OK but %i", r);
        Task::yield(NULL);
        size_t compressed = BLOCK_SIZE - m_zout.avail_out;
        if (compressed) {
            size_t w = m_h->write(obuf, compressed);
            if (w <= 0)
                return wroteTotal;
        }
        wroteTotal += compressed;
        count -= compressed;
    }
    return wroteTotal;
}

void Balau::ZStream::doFlush(bool finish) {
    void * buf = alloca(BLOCK_SIZE);
    m_zout.next_in = NULL;
    m_zout.avail_in = 0;
    int r;
    do {
        m_zout.next_out = (Bytef *) buf;
        m_zout.avail_out = BLOCK_SIZE;
        r = deflate(&m_zout, finish ? Z_FINISH : Z_SYNC_FLUSH);
        size_t compressed = BLOCK_SIZE - m_zout.avail_out;
        if (compressed) {
            size_t w = m_h->write(buf, compressed);
            if (w <= 0)
                return;
        }
    } while (r == Z_OK && finish);
}
