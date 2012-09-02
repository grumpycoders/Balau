#include "ZHandle.h"
#include "Task.h"
#include "Async.h"
#include "TaskMan.h"

Balau::ZStream::ZStream(const IO<Handle> & h, int level, header_t header) : m_h(h) {
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
    if (m_h->canWrite())
        finish();
    inflateEnd(&m_zin);
    deflateEnd(&m_zout);
    if (m_in) {
        free(m_in);
        m_in = NULL;
    }
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

namespace {

class AsyncOpInflate : public Balau::AsyncOperation {
  public:
      AsyncOpInflate(z_stream * zin, int * r, Balau::Events::Custom * evt) : m_zin(zin), m_r(r), m_evt(evt) { }
    virtual bool needsMainQueue() { return false; }
    virtual bool needsFinishWorker() { return true; }
    virtual void run() {
        *m_r = inflate(m_zin, Z_SYNC_FLUSH);
    }
    virtual void done() {
        m_evt->doSignal();
        delete this;
    }
  private:
    z_stream * m_zin;
    int * m_r;
    Balau::Events::Custom * m_evt;
};

};

static const int BLOCK_SIZE = 1024;

ssize_t Balau::ZStream::read(void * buf, size_t count) throw (GeneralException) {
    if (m_closed || m_eof)
        return 0;

    AAssert(m_h->canRead(), "Can't call ZStream::read on a non-readable handle.");

    size_t readTotal = 0;
    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    m_zin.next_out = (Bytef *) buf;
    m_zin.avail_out = count;
    if (!m_in) {
        m_zin.next_in = m_in = (uint8_t *) malloc(block_size);
        m_zin.avail_in = 0;
    }
    while ((count != 0) && !m_h->isClosed() && !m_h->isEOF()) {
        if (m_zin.avail_in == 0) {
            m_zin.next_in = m_in;
            size_t r = m_h->read(m_in, block_size);
            if (r <= 0)
                return readTotal;
            m_zin.avail_in = r;
        }
        int r = 0;
        if (m_useAsyncOp) {
            Events::Custom evt;
            createAsyncOp(new AsyncOpInflate(&m_zin, &r, &evt));
            Task::operationYield(&evt);
        } else {
            r = inflate(&m_zin, Z_SYNC_FLUSH);
            Task::operationYield();
        }
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

namespace {

class AsyncOpDeflate : public Balau::AsyncOperation {
  public:
      AsyncOpDeflate(z_stream * zout, int * r, int flush, Balau::Events::Custom * evt) : m_zout(zout), m_r(r), m_flush(flush), m_evt(evt) { }
    virtual bool needsMainQueue() { return false; }
    virtual bool needsFinishWorker() { return true; }
    virtual void run() {
        *m_r = deflate(m_zout, m_flush);
    }
    virtual void done() {
        m_evt->doSignal();
        delete this;
    }
  private:
    z_stream * m_zout;
    int * m_r, m_flush;
    Balau::Events::Custom * m_evt;
};

};

ssize_t Balau::ZStream::write(const void * buf, size_t count) throw (GeneralException) {
    if (m_closed || m_eof)
        return 0;

    AAssert(m_h->canWrite(), "Can't call ZStream::write on a non-writable handle.");

    size_t wroteTotal = 0;
    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    m_zout.next_in = (Bytef *) const_cast<void *>(buf);
    m_zout.avail_in = count;
    void * obuf = m_useAsyncOp ? malloc(block_size) : alloca(block_size);
    while ((count != 0) && !m_h->isClosed()) {
        m_zout.next_out = (Bytef *) obuf;
        m_zout.avail_out = block_size;
        int r = 0;
        if (m_useAsyncOp) {
            Events::Custom evt;
            createAsyncOp(new AsyncOpDeflate(&m_zout, &r, Z_NO_FLUSH, &evt));
            Task::operationYield(&evt);
        } else {
            r = deflate(&m_zout, Z_NO_FLUSH);
            Task::operationYield();
        }
        EAssert(r == Z_OK, "deflate() didn't return Z_OK but %i", r);
        size_t compressed = block_size - m_zout.avail_out;
        if (compressed) {
            size_t w = m_h->forceWrite(obuf, compressed);
            if (m_useAsyncOp)
                free(obuf);
            if (w <= 0)
                return wroteTotal;
        }
        size_t didRead = count - m_zout.avail_in;
        wroteTotal += didRead;
        count -= didRead;
    }
    return wroteTotal;
}

void Balau::ZStream::doFlush(bool finish) {
    AAssert(m_h->canWrite(), "Can't call ZStream::doFlush on a non-writable handle.");

    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    void * buf = m_useAsyncOp ? malloc(block_size) : alloca(block_size);
    m_zout.next_in = NULL;
    m_zout.avail_in = 0;
    int r;
    do {
        m_zout.next_out = (Bytef *) buf;
        m_zout.avail_out = block_size;
        if (m_useAsyncOp) {
            Events::Custom evt;
            createAsyncOp(new AsyncOpDeflate(&m_zout, &r, finish ? Z_FINISH : Z_SYNC_FLUSH, &evt));
            Task::operationYield(&evt);
        } else {
            r = deflate(&m_zout, finish ? Z_FINISH : Z_SYNC_FLUSH);
            Task::operationYield();
        }
        EAssert((r == Z_OK) || ((r == Z_STREAM_END) && finish), "deflate() didn't return Z_OK or Z_STREAM_END, but %i (finish = %s)", r, finish ? "true" : "false");
        size_t compressed = block_size - m_zout.avail_out;
        if (compressed) {
            size_t w = m_h->forceWrite(buf, compressed);
            if (m_useAsyncOp)
                free(buf);
            if (w <= 0)
                return;
        }
    } while (r == Z_OK && finish);
}
