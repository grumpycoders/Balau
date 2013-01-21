#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Async.h"
#include "Input.h"
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
      AsyncOpOpen(const char * path, cbResults_t * results) : m_path(path), m_results(results) { }
    virtual void run() {
        int r = m_results->result = open(m_path, O_RDONLY);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    const char * m_path;
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

Balau::Input::Input(const char * fname) {
    m_name.set("Input(%s)", fname);
    m_fname = fname;
}

void Balau::Input::open() throw (GeneralException) {
    Printer::elog(E_INPUT, "Opening file %s", m_fname.to_charp());

    cbResults_t cbResults;
    createAsyncOp(new AsyncOpOpen(m_fname.to_charp(), &cbResults));
    Task::operationYield(&cbResults.evt);
    if (cbResults.result < 0) {
        if (cbResults.errorno == ENOENT) {
            throw ENoEnt(m_fname);
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

void Balau::Input::close() throw (GeneralException) {
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

struct cbReadResults_t {
    Balau::Events::Custom evt;
    ssize_t result;
    int errorno;
};

class AsyncOpRead : public Balau::AsyncOperation {
  public:
      AsyncOpRead(int fd, void * buf, size_t count, off_t offset, cbReadResults_t * results) : m_fd(fd), m_buf(buf), m_count(count), m_offset(offset), m_results(results) { }
    virtual void run() {
        ssize_t r = m_results->result = pread(m_fd, m_buf, m_count, m_offset);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    int m_fd;
    void * m_buf;
    size_t m_count;
    off_t m_offset;
    cbReadResults_t * m_results;
};

};

ssize_t Balau::Input::read(void * buf, size_t count) throw (GeneralException) {
    cbReadResults_t cbResults;
    createAsyncOp(new AsyncOpRead(m_fd, buf, count, getROffset(), &cbResults));
    Task::operationYield(&cbResults.evt);
    if (cbResults.result > 0) {
        rseek(cbResults.result, SEEK_CUR);
    } else {
        char str[4096];
        throw GeneralException(String("Unable to read file ") + m_name + ": " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
    }
    return cbResults.result;
}

bool Balau::Input::isClosed() {
    return m_fd < 0;
}

const char * Balau::Input::getName() {
    return m_name.to_charp();
}

off_t Balau::Input::getSize() {
    return m_size;
}

time_t Balau::Input::getMTime() {
    return m_mtime;
}

bool Balau::Input::canRead() {
    return true;
}
