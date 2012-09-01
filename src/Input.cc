#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "eio.h"
#include "Input.h"
#include "Task.h"
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

struct cbResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
};

static int eioDone(eio_req * req) {
    cbResults_t * cbResults = (cbResults_t *) req->data;
    cbResults->result = req->result;
    cbResults->errorno = req->errorno;
    cbResults->evt.doSignal();
    return 0;
}

struct cbStatsResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
    EIO_STRUCT_STAT statdata;
};

static int eioStatsDone(eio_req * req) {
    cbStatsResults_t * cbStatsResults = (cbStatsResults_t *) req->data;
    cbStatsResults->result = req->result;
    cbStatsResults->errorno = req->errorno;
    cbStatsResults->statdata = *(EIO_STRUCT_STAT *) req->ptr2;
    cbStatsResults->evt.doSignal();
    return 0;
}

Balau::Input::Input(const char * fname) throw (GeneralException) {
    m_name.set("Input(%s)", fname);
    m_fname = fname;

    Printer::elog(E_INPUT, "Opening file %s", fname);

    cbResults_t cbResults;
    eio_req * r = eio_open(fname, O_RDONLY, 0, 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_open returned a NULL eio_req");
    Task::operationYield(&cbResults.evt);
    if (cbResults.result < 0) {
        char str[4096];
        if (cbResults.errorno == ENOENT) {
            throw ENoEnt(fname);
        } else {
            throw GeneralException(String("Unable to open file ") + m_name + " for reading: " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
        }
    } else {
        m_fd = cbResults.result;
    }

    cbStatsResults_t cbStatsResults;
    r = eio_fstat(m_fd, 0, eioStatsDone, &cbStatsResults);
    EAssert(r != NULL, "eio_fstat returned a NULL eio_req");
    Task::operationYield(&cbStatsResults.evt);
    if (cbStatsResults.result == 0) {
        m_size = cbStatsResults.statdata.st_size;
        m_mtime = cbStatsResults.statdata.st_mtime;
    }
}

void Balau::Input::close() throw (GeneralException) {
    if (m_fd < 0)
        return;
    cbResults_t cbResults;
    eio_req * r = eio_close(m_fd, 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_close returned a NULL eio_req");
    m_fd = -1;
    Task::operationYield(&cbResults.evt);
    if (cbResults.result < 0) {
        char str[4096];
        strerror_r(cbResults.errorno, str, sizeof(str));
        throw GeneralException(String("Unable to close file ") + m_name + ": " + str);
    } else {
        m_fd = cbResults.result;
    }
}

ssize_t Balau::Input::read(void * buf, size_t count) throw (GeneralException) {
    cbResults_t cbResults;
    eio_req * r = eio_read(m_fd, buf, count, getROffset(), 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_read returned a NULL eio_req");
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
