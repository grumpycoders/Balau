#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
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
    ssize_t result;
    int errorno;
    struct stat statdata;
    enum { NONE, OPEN, STAT, CLOSE, READ } type;
};

class AsyncOpOpen : public Balau::AsyncOperation {
  public:
      AsyncOpOpen(const char * path, cbResults_t * results) : m_path(path), m_results(results) { }
    virtual void run() {
#ifdef _MSC_VER
        const ssize_t r = m_results->result = _open(m_path, O_RDONLY | O_BINARY);
#else
        const ssize_t r = m_results->result = open(m_path, O_RDONLY);
#endif
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

class AsyncOpStat : public Balau::AsyncOperation {
  public:
      AsyncOpStat(int fd, cbResults_t * results) : m_fd(fd), m_results(results) { }
    virtual void run() {
        const ssize_t r = m_results->result = fstat(m_fd, &m_results->statdata);
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

Balau::Input::Input(const char * fname) {
    m_name.set("Input(%s)", fname);
    m_fname = fname;
}

bool Balau::Input::isPendingComplete() {
    if (!m_pendingOp)
        return true;
    return reinterpret_cast<cbResults_t *>(m_pendingOp)->evt.gotSignal();
}

void Balau::Input::open() throw (GeneralException) {
    AAssert(isClosed() || m_pendingOp, "Can't open a file twice.");
    Printer::elog(E_INPUT, "Opening file %s", m_fname.to_charp());

    cbResults_t * cbResults;

    if (!m_pendingOp) {
        m_pendingOp = cbResults = new cbResults_t();
        cbResults->type = cbResults_t::NONE;
    } else {
        cbResults = (cbResults_t *) m_pendingOp;
    }

    try {
        switch (cbResults->type) {
        case cbResults_t::NONE:
            cbResults->type = cbResults_t::OPEN;
            createAsyncOp(new AsyncOpOpen(m_fname.to_charp(), cbResults));
            Task::operationYield(&cbResults->evt, Task::INTERRUPTIBLE);
        case cbResults_t::OPEN:
            AAssert(isPendingComplete(), "Don't call open again without checking isPendingComplete.");
            if (cbResults->result < 0) {
                if (cbResults->errorno == ENOENT) {
                    throw ENoEnt(m_fname);
                } else {
                    char str[4096];
                    throw GeneralException(String("Unable to open file ") + m_name + " for reading: " + strerror_r(cbResults->errorno, str, sizeof(str)) + " (err#" + cbResults->errorno + ")");
                }
            } else {
                m_fd = cbResults->result;
            }

            delete cbResults;
            m_pendingOp = cbResults = new cbResults_t();
            cbResults->type = cbResults_t::STAT;
            createAsyncOp(new AsyncOpStat(m_fd, cbResults));
            Task::operationYield(&cbResults->evt, Task::INTERRUPTIBLE);
        case cbResults_t::STAT:
            AAssert(isPendingComplete(), "Don't call open again without checking isPendingComplete.");
            if (cbResults->result == 0) {
                m_size = cbResults->statdata.st_size;
                m_mtime = cbResults->statdata.st_mtime;
            }
            delete cbResults;
            m_pendingOp = NULL;
            break;
        default:
            AAssert(false, "Don't switch operations while one is still not complete.");
        }
    }
    catch (Balau::TaskSwitch) {
        throw;
    }
    catch (Balau::EAgain) {
        throw;
    }
    catch (...) {
        delete cbResults;
        m_pendingOp = NULL;
        throw;
    }
}

namespace {

class AsyncOpClose : public Balau::AsyncOperation {
  public:
      AsyncOpClose(int fd, cbResults_t * results) : m_fd(fd), m_results(results) { }
    virtual void run() {
        const ssize_t r = m_results->result = close(m_fd);
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
    if ((m_fd < 0) && !m_pendingOp)
        return;

    cbResults_t * cbResults;

    if (!m_pendingOp) {
        m_pendingOp = cbResults = new cbResults_t;
        cbResults->type = cbResults_t::NONE;
    } else {
        cbResults = (cbResults_t *) m_pendingOp;
    }

    try {
        switch (cbResults->type) {
        case cbResults_t::NONE:
            cbResults->type = cbResults_t::CLOSE;
            createAsyncOp(new AsyncOpClose(m_fd, cbResults));
            Task::operationYield(&cbResults->evt, Task::INTERRUPTIBLE);
        case cbResults_t::CLOSE:
            m_fd = -1;
            if (cbResults->result < 0) {
                char buf[4096];
                const char * str = strerror_r(cbResults->errorno, buf, sizeof(buf));
                throw GeneralException(String("Unable to close file ") + m_name + ": " + str);
            }
            delete cbResults;
            m_pendingOp = NULL;
            break;
        default:
            AAssert(false, "Don't switch operations while one is still not complete.");
        }
    }
    catch (Balau::TaskSwitch) {
        throw;
    }
    catch (Balau::EAgain) {
        throw;
    }
    catch (...) {
        delete cbResults;
        m_pendingOp = NULL;
        throw;
    }
}

namespace {

class AsyncOpRead : public Balau::AsyncOperation {
  public:
      AsyncOpRead(int fd, void * buf, size_t count, off_t offset, cbResults_t * results) : m_fd(fd), m_buf(buf), m_count(count), m_offset(offset), m_results(results) { }
    virtual void run() {
#ifdef _MSC_VER
        off_t offset = lseek(m_fd, m_offset, SEEK_SET);
        if (offset < 0) {
            m_results->errorno = errno;
            return;
        }
        const ssize_t r = m_results->result = read(m_fd, m_buf, m_count);
#else
        const ssize_t r = m_results->result = pread(m_fd, m_buf, m_count, m_offset);
#endif
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
    cbResults_t * m_results;
};

};

ssize_t Balau::Input::read(void * buf, size_t count) throw (GeneralException) {
    AAssert(!isClosed(), "Can't read a closed file");
    ssize_t result;

    cbResults_t * cbResults;

    if (!m_pendingOp) {
        m_pendingOp = cbResults = new cbResults_t;
        cbResults->type = cbResults_t::NONE;
    } else {
        cbResults = (cbResults_t *) m_pendingOp;
    }

    try {
        switch (cbResults->type) {
        case cbResults_t::NONE:
            cbResults->type = cbResults_t::READ;
            createAsyncOp(new AsyncOpRead(m_fd, buf, count, getROffset(), cbResults));
            Task::operationYield(&cbResults->evt, Task::INTERRUPTIBLE);
        case cbResults_t::READ:
            result = cbResults->result;
            if (result >= 0) {
                rseek(result, SEEK_CUR);
            } else {
                char str[4096];
                throw GeneralException(String("Unable to read file ") + m_name + ": " + strerror_r(cbResults->errorno, str, sizeof(str)) + " (err#" + cbResults->errorno + ")");
            }
            delete cbResults;
            m_pendingOp = NULL;
            return result;
        default:
            AAssert(false, "Don't switch operations while one is still not complete.");
        }
    }
    catch (Balau::TaskSwitch) {
        throw;
    }
    catch (Balau::EAgain) {
        throw;
    }
    catch (...) {
        delete cbResults;
        m_pendingOp = NULL;
        throw;
    }

    IAssert(false, "Shouldn't end up there.");

    return -1;
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
