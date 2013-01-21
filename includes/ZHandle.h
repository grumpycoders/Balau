#pragma once

#include <zlib.h>
#include <Handle.h>
#include <Async.h>

namespace Balau {

class ZStream : public Handle {
  public:
    typedef enum {
        ZLIB,
        GZIP,
        RAW,
    } header_t;
      ZStream(const IO<Handle> & h, int level = Z_BEST_COMPRESSION, header_t header = ZLIB);
    virtual void close() throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName();
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    void detach() { m_detached = true; }
    void flush() { doFlush(false); }
    void setUseAsyncOp(bool useAsyncOp) { m_useAsyncOp = useAsyncOp; }
    virtual bool isPendingComplete();
    void finish() { doFlush(true); }
    void doFlush(bool finish);
  private:
    IO<Handle> m_h;
    z_stream m_zin, m_zout;
    String m_name;
    uint8_t * m_buf = NULL;
    uint8_t * m_wptr;
    enum {
        IDLE,
        READING,
        WRITING,
        COMPRESSING,
        COMPRESSING_IDLE,
        DECOMPRESSING,
        DECOMPRESSING_IDLE,
        CLOSING,
    } m_phase = IDLE;
    size_t m_total, m_count, m_compressed;
    AsyncOperation * m_op = NULL;
    ssize_t m_status;
    bool m_detached = false, m_closed = false, m_eof = false, m_useAsyncOp = true;
};

};
