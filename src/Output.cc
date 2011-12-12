#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "eio.h"
#include "Output.h"
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

Balau::Output::Output(const char * fname, bool truncate) throw (GeneralException) : m_fd(-1), m_size(-1), m_mtime(-1) {
    m_name.set("Output(%s)", fname);
    m_fname = fname;

    Printer::elog(E_OUTPUT, "Opening file %s", fname);

    cbResults_t cbResults;
    eio_req * r = eio_open(fname, O_WRONLY | O_CREAT | (truncate ? O_TRUNC : 0), 0755, 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_open returned a NULL eio_req");
    Task::yield(&cbResults.evt);
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
    Task::yield(&cbStatsResults.evt);
    if (cbStatsResults.result == 0) {
        m_size = cbStatsResults.statdata.st_size;
        m_mtime = cbStatsResults.statdata.st_mtime;
    }
}

void Balau::Output::close() throw (GeneralException) {
    if (m_fd < 0)
        return;
    cbResults_t cbResults;
    eio_req * r = eio_close(m_fd, 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_close returned a NULL eio_req");
    m_fd = -1;
    Task::yield(&cbResults.evt);
    if (cbResults.result < 0) {
        char str[4096];
        strerror_r(cbResults.errorno, str, sizeof(str));
        throw GeneralException(String("Unable to close file ") + m_name + ": " + str);
    } else {
        m_fd = cbResults.result;
    }
}

ssize_t Balau::Output::write(const void * buf, size_t count) throw (GeneralException) {
    cbResults_t cbResults;
    eio_req * r = eio_write(m_fd, const_cast<void *>(buf), count, getWOffset(), 0, eioDone, &cbResults);
    EAssert(r != NULL, "eio_write returned a NULL eio_req");
    Task::yield(&cbResults.evt);
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
