#pragma once

#include <zlib.h>
#include <Handle.h>
#include <Async.h>

namespace Balau {

class ZStream : public Filter {
  public:
    typedef enum {
        ZLIB,
        GZIP,
        RAW,
    } header_t;
      ZStream(IO<Handle> h, int level = Z_BEST_COMPRESSION, header_t header = ZLIB);
    virtual void close() throw (GeneralException) override;
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException) override;
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) override;
    virtual bool isEOF() override { return m_phase == CLOSING || m_eof || Filter::isEOF(); }
    virtual bool isClosed() override { return m_phase == CLOSING || Filter::isClosed(); }
    virtual const char * getName() override { return m_name.to_charp(); }
    void flush() { doFlush(false); }
    void setUseAsyncOp(bool useAsyncOp) { m_useAsyncOp = useAsyncOp; }
    virtual bool isPendingComplete() override;
    void finish() { doFlush(true); }
    void doFlush(bool finish);
  private:
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
        WRITING_FINISH,
        COMPRESSING_FINISH,
        COMPRESSING_FINISH_IDLE,
        CLOSING,
    } m_phase = IDLE;
    size_t m_total, m_count, m_compressed;
    AsyncOperation * m_op = NULL;
    ssize_t m_status = IDLE;
    bool m_useAsyncOp = true, m_eof = false;
};

};
