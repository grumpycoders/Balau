#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "eio.h"
#include "Input.h"
#include "Task.h"

struct cbResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
};

static int eioDone(eio_req * req) {
    cbResults_t * cbResults = (cbResults_t *) req->data;
    cbResults->result = req->result;
    cbResults->errorno = req->errorno;
    cbResults->evt.doSignal();
}

Balau::Input::Input(const char * fname) throw (GeneralException) : m_fd(-1) {
    cbResults_t cbResults;
    m_name.set("Input(%s)", fname);
    eio_req * r = eio_open(fname, O_RDONLY, 0, 0, eioDone, &cbResults);
    Assert(r != 0);
    Task::yield(&cbResults.evt);
    Assert(cbResults.evt.gotSignal());
    if (cbResults.result < 0) {
        char str[4096];
        throw GeneralException(String("Unable to open file ") + m_name + " for reading: " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
    } else {
        m_fd = cbResults.result;
    }
}

void Balau::Input::close() throw (GeneralException) {
    if (m_fd < 0)
        return;
    cbResults_t cbResults;
    eio_req * r = eio_close(m_fd, 0, eioDone, &cbResults);
    Assert(r != 0);
    m_fd = -1;
    Task::yield(&cbResults.evt);
    Assert(cbResults.evt.gotSignal());
    if (cbResults.result < 0) {
        char str[4096];
        strerror_r(cbResults.errorno, str, sizeof(str));
        throw GeneralException(String("Unable to close file ") + m_name + ": " + str);
    } else {
        m_fd = cbResults.result;
    }
}

bool Balau::Input::isClosed() {
    return m_fd < 0;
}

const char * Balau::Input::getName() {
    return m_name.to_charp();
}
