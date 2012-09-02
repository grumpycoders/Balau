#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Async.h"
#include "Output.h"
#include "Task.h"
#include "TaskMan.h"
#include "Printer.h"

#ifdef _WIN32
static const char * strerror_r(int errorno, char * buf, size_t bufsize) {
#ifdef _MSVC
    strerror_s(buf, bufsize, errorno);
    return buf;
#else
    return strerror(errorno);
#endif
}
#endif

namespace {

struct cbResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
};

class AsyncOpOpen : public Balau::AsyncOperation {
  public:
      AsyncOpOpen(const char * path, bool truncate, cbResults_t * results) : m_path(path), m_truncate(truncate), m_results(results) { }
    virtual void run() {
        int r = m_results->result = open(m_path, O_WRONLY | O_CREAT | (m_truncate ? O_TRUNC : 0), 0755);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    const char * m_path;
    bool m_truncate;
    cbResults_t * m_results;
};

struct cbStatsResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
    struct stat statdata;
};

class AsyncOpStat : public Balau::AsyncOperation {
  public:
      AsyncOpStat(int fd, cbStatsResults_t * results) : m_fd(fd), m_results(results) { }
    virtual void run() {
        int r = m_results->result = fstat(m_fd, &m_results->statdata);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    int m_fd;
    cbStatsResults_t * m_results;
};

};

Balau::Output::Output(const char * fname, bool truncate) throw (GeneralException) {
    m_name.set("Output(%s)", fname);
    m_fname = fname;

    Printer::elog(E_OUTPUT, "Opening file %s", fname);

    cbResults_t cbResults;
    createAsyncOp(new AsyncOpOpen(fname, truncate, &cbResults));
    Task::operationYield(&cbResults.evt);
    if (cbResults.result < 0) {
        if (cbResults.errorno == ENOENT) {
            throw ENoEnt(fname);
        } else {
            char str[4096];
            throw GeneralException(String("Unable to open file ") + m_name + " for reading: " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
        }
    } else {
        m_fd = cbResults.result;
    }

    cbStatsResults_t cbStatsResults;
    createAsyncOp(new AsyncOpStat(m_fd, &cbStatsResults));
    Task::operationYield(&cbStatsResults.evt);
    if (cbStatsResults.result == 0) {
        m_size = cbStatsResults.statdata.st_size;
        m_mtime = cbStatsResults.statdata.st_mtime;
    }
}

namespace {

class AsyncOpClose : public Balau::AsyncOperation {
  public:
      AsyncOpClose(int fd, cbResults_t * results) : m_fd(fd), m_results(results) { }
    virtual void run() {
        int r = m_results->result = close(m_fd);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    int m_fd;
    cbResults_t * m_results;
};

};

void Balau::Output::close() throw (GeneralException) {
    if (m_fd < 0)
        return;
    cbResults_t cbResults;
    createAsyncOp(new AsyncOpClose(m_fd, &cbResults));
    Task::operationYield(&cbResults.evt);
    m_fd = -1;
    if (cbResults.result < 0) {
        char str[4096];
        strerror_r(cbResults.errorno, str, sizeof(str));
        throw GeneralException(String("Unable to close file ") + m_name + ": " + str);
    }
}

namespace {

struct cbWriteResults_t {
    Balau::Events::Custom evt;
    ssize_t result;
    int errorno;
};

class AsyncOpWrite : public Balau::AsyncOperation {
  public:
      AsyncOpWrite(int fd, const void * buf, size_t count, off_t offset, cbWriteResults_t * results) : m_fd(fd), m_buf(buf), m_count(count), m_offset(offset), m_results(results) { }
    virtual void run() {
        int r = m_results->result = pwrite(m_fd, m_buf, m_count, m_offset);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    int m_fd;
    const void * m_buf;
    size_t m_count;
    off_t m_offset;
    cbWriteResults_t * m_results;
};

};

ssize_t Balau::Output::write(const void * buf, size_t count) throw (GeneralException) {
    cbWriteResults_t cbResults;
    createAsyncOp(new AsyncOpWrite(m_fd, buf, count, getWOffset(), &cbResults));
    Task::operationYield(&cbResults.evt);
    if (cbResults.result > 0) {
        wseek(cbResults.result, SEEK_CUR);
    } else {
        char str[4096];
        throw GeneralException(String("Unable to write file ") + m_name + ": " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
    }
    return cbResults.result;
}

bool Balau::Output::isClosed() {
    return m_fd < 0;
}

const char * Balau::Output::getName() {
    return m_name.to_charp();
}

off_t Balau::Output::getSize() {
    return m_size;
}

time_t Balau::Output::getMTime() {
    return m_mtime;
}

bool Balau::Output::canWrite() {
    return true;
}
