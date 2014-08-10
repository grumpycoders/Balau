#include "ZHandle.h"
#include "Task.h"
#include "Async.h"
#include "TaskMan.h"

Balau::ZStream::ZStream(IO<Handle> h, int level, header_t header) : Filter(h) {
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
    m_name.set("ZStream(%s)", h->getName());
}

void Balau::ZStream::close() throw (GeneralException) {
    switch (m_phase) {
    case IDLE:
    case WRITING_FINISH:
    case COMPRESSING_FINISH:
    case COMPRESSING_FINISH_IDLE:
        if (getIO()->canWrite())
            finish();
        inflateEnd(&m_zin);
        deflateEnd(&m_zout);
        if (m_buf) {
            free(m_buf);
            m_buf = NULL;
        }
        m_phase = CLOSING;
    case CLOSING:
        Filter::close();
        m_phase = IDLE;
        return;
    default:
        AAssert(false, "Wrong phase");
    }
}

namespace {

class AsyncOpZlib : public Balau::AsyncOperation {
  public:
      AsyncOpZlib(z_stream * z, bool deflate, int flush) : m_z(z), m_deflate(deflate), m_flush(flush) { }
    virtual bool needsMainQueue() { return false; }
    virtual bool needsFinishWorker() { return true; }
    virtual void run() {
        if (m_deflate)
            m_r = deflate(m_z, m_flush);
        else
            m_r = inflate(m_z, Z_SYNC_FLUSH);
    }
    virtual void done() { m_evt.doSignal(); }
    bool gotSignal() { return m_evt.gotSignal(); }
    int getR() { return m_r; }
    void yield() { Balau::Task::operationYield(&m_evt, Balau::Task::INTERRUPTIBLE); }
  private:
    z_stream * m_z;
    int m_r, m_flush;
    bool m_deflate;
    Balau::Events::Custom m_evt;
};

};

bool Balau::ZStream::isPendingComplete() {
    AsyncOpZlib * async = dynamic_cast<AsyncOpZlib *>(m_op);

    switch (m_phase) {
    case READING:
    case WRITING:
    case WRITING_FINISH:
    case CLOSING:
        return getIO()->isPendingComplete();
    case COMPRESSING:
    case DECOMPRESSING:
    case COMPRESSING_FINISH:
        IAssert(async, "Shouldn't not have a cbResults here...");
        return async->gotSignal();
    default:
        return true;
    }
}

static const int BLOCK_SIZE = 1024;

ssize_t Balau::ZStream::read(void * buf, size_t count) throw (GeneralException) {
    if (isClosed() || m_eof)
        return 0;

    AAssert(getIO()->canRead(), "Can't call ZStream::read on a non-readable handle.");

    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    AsyncOpZlib * async = dynamic_cast<AsyncOpZlib *>(m_op);

    switch (m_phase) {
    case IDLE:
        m_total = 0;
        m_count = count;
        m_zin.next_out = (Bytef *) buf;
        m_zin.avail_out = count;
        if (!m_buf) {
            m_zin.next_in = m_buf = (uint8_t *) malloc(block_size);
            m_zin.avail_in = 0;
        }
        while ((m_count != 0) && !getIO()->isClosed() && !getIO()->isEOF()) {
            if (m_zin.avail_in == 0) {
                m_zin.next_in = m_buf;
                m_phase = READING;
    case READING:
                m_status = getIO()->read(m_buf, block_size);
                if (m_status <= 0)
                    return m_total;
                m_zin.avail_in = m_status;
            }
            if (m_useAsyncOp) {
                m_phase = COMPRESSING;
                createAsyncOp(m_op = async = new AsyncOpZlib(&m_zin, false, 0));
                async->yield();
    case COMPRESSING:
                m_status = async->getR();
                delete async;
                m_op = async = NULL;
            } else {
                m_status = inflate(&m_zin, Z_SYNC_FLUSH);
                m_phase = COMPRESSING_IDLE;
                Task::operationYield(NULL, Task::INTERRUPTIBLE);
            }
    case COMPRESSING_IDLE:
            EAssert(m_status == Z_OK || m_status == Z_STREAM_END, "inflate() didn't return Z_OK or Z_STREAM_END but %zi", m_status);
            ssize_t didRead = m_count - m_zin.avail_out;
            m_total += didRead;
            m_count -= didRead;
            if (m_status == Z_STREAM_END) {
                m_eof = true;
                m_phase = IDLE;
                return m_total;
            }
        }
        break;
    default:
        AAssert(false, "Don't call an operation without finishing another.");
    }

    m_phase = IDLE;
    return m_total;
}

ssize_t Balau::ZStream::write(const void * buf, size_t count) throw (GeneralException) {
    if (isClosed() || m_eof)
        return 0;

    AAssert(getIO()->canWrite(), "Can't call ZStream::write on a non-writable handle.");

    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    ssize_t w;
    AsyncOpZlib * async = dynamic_cast<AsyncOpZlib *>(m_op);

    switch (m_phase) {
    case IDLE:
        m_total = 0;
        m_count = count;
        m_zout.next_in = (Bytef *) const_cast<void *>(buf);
        m_zout.avail_in = count;
        if (!m_buf)
            m_buf = (uint8_t *) malloc(block_size);
        while ((m_count != 0) && !getIO()->isClosed()) {
            m_zout.next_out = (Bytef *) m_buf;
            m_zout.avail_out = block_size;
            if (m_useAsyncOp) {
                m_phase = DECOMPRESSING;
                createAsyncOp(m_op = async = new AsyncOpZlib(&m_zout, true, Z_NO_FLUSH));
                async->yield();
    case DECOMPRESSING:
                m_status = async->getR();
                delete async;
                m_op = async = NULL;
            } else {
                m_status = deflate(&m_zout, Z_NO_FLUSH);
                m_phase = DECOMPRESSING_IDLE;
                Task::operationYield(NULL, Task::INTERRUPTIBLE);
            }
    case DECOMPRESSING_IDLE:
            EAssert(m_status == Z_OK, "deflate() didn't return Z_OK but %zi", m_status);
            m_compressed = block_size - m_zout.avail_out;
            m_phase = WRITING;
            m_wptr = m_buf;
            while (m_compressed) {
    case WRITING:
                w = getIO()->write(m_wptr, m_compressed);
                if (w <= 0) {
                    m_phase = IDLE;
                    return m_total;
                }
                m_compressed -= w;
                m_wptr += w;
            }
            size_t didWrite = m_count - m_zout.avail_in;
            m_total += didWrite;
            m_count -= didWrite;
        }
        break;
    default:
        AAssert(false, "Don't call an operation without finishing another.");
    }

    m_phase = IDLE;
    return m_total;
}

void Balau::ZStream::doFlush(bool finish) {
    AAssert(getIO()->canWrite(), "Can't call ZStream::doFlush on a non-writable handle.");

    const int block_size = BLOCK_SIZE * (m_useAsyncOp ? 16 : 1);
    void * buf = m_useAsyncOp ? malloc(block_size) : alloca(block_size);
    AsyncOpZlib * async = dynamic_cast<AsyncOpZlib *>(m_op);
    ssize_t w = 0;

    switch (m_phase) {
    case IDLE:
        m_zout.next_in = NULL;
        m_zout.avail_in = 0;
        do {
            m_zout.next_out = (Bytef *) m_buf;
            m_zout.avail_out = block_size;
            if (m_useAsyncOp) {
                m_phase = COMPRESSING_FINISH;
                createAsyncOp(m_op = async = new AsyncOpZlib(&m_zout, true, finish ? Z_FINISH : Z_SYNC_FLUSH));
                async->yield();
    case COMPRESSING_FINISH:
                m_status = async->getR();
                delete async;
                m_op = async = NULL;
            } else {
                m_status = deflate(&m_zout, finish ? Z_FINISH : Z_SYNC_FLUSH);
                m_phase = COMPRESSING_FINISH_IDLE;
                Task::operationYield(NULL, Task::INTERRUPTIBLE);
            }
    case COMPRESSING_FINISH_IDLE:
            EAssert((m_status == Z_OK) || ((m_status == Z_STREAM_END) && finish), "deflate() didn't return Z_OK or Z_STREAM_END, but %zi (finish = %s)", m_status, finish ? "true" : "false");
            m_compressed = block_size - m_zout.avail_out;
            m_phase = WRITING_FINISH;
            m_wptr = m_buf;
            while (m_compressed) {
    case WRITING_FINISH:
                w = getIO()->write(m_wptr, m_compressed);
                if (w <= 0) {
                    m_phase = IDLE;
                    return;
                }
                m_compressed -= w;
                m_wptr += w;
            }
        } while (m_status == Z_OK && finish);
        break;
    default:
        AAssert(false, "Don't call an operation without finishing another.");
    }

    m_phase = IDLE;
}
