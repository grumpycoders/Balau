#include <winsock2.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifdef _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include "Selectable.h"
#include "Threads.h"
#include "Printer.h"
#include "Main.h"
#include "Async.h"
#include "Task.h"
#include "TaskMan.h"

#ifdef _WIN32
inline static SOCKET getSocket(int fd) { return _get_osfhandle(fd); }
#else
inline static int getSocket(int fd) { return fd; }

namespace {

class SigpipeBlocker : public Balau::AtStart {
  public:
      SigpipeBlocker() : AtStart(5) { }
    virtual void doStart() {
        struct sigaction new_actn, old_actn;
        new_actn.sa_handler = SIG_IGN;
        sigemptyset(&new_actn.sa_mask);
        new_actn.sa_flags = 0;
        sigaction(SIGPIPE, &new_actn, &old_actn);
    }
};

static SigpipeBlocker sigpipeBlocker;

};
#endif

void Balau::Selectable::SelectableEvent::gotOwner(Task * task) {
    Printer::elog(E_SELECT, "Arming SelectableEvent at %p", this);
    if (!m_task) {
        Printer::elog(E_SELECT, "...with a new task (%p)", task);
    } else if (task == m_task) {
        Printer::elog(E_SELECT, "...with the same task, doing nothing.");
        return;
    } else {
        Printer::elog(E_SELECT, "...with a new task (%p -> %p); stopping first", m_task, task);
        m_evt.stop();
        m_evt.set<SelectableEvent, &SelectableEvent::evt_cb>(this);
        m_evt.set(m_fd, m_evtType);
    }
    m_task = task;
    m_evt.set(task->getLoop());
    m_evt.start();
}

void Balau::Selectable::setFD(int fd) throw (GeneralException) {
    if (m_fd >= 0)
        throw GeneralException("FD already set.");
    m_fd = fd;

    m_evtR = new SelectableEvent(m_fd, ev::READ);
    m_evtW = new SelectableEvent(m_fd, ev::WRITE);
#ifdef _WIN32
    u_long iMode = 1;
    int r = ioctlsocket(_get_osfhandle(m_fd), FIONBIO, &iMode);
    EAssert(r == NO_ERROR, "ioctlsocket FIONBIO failed with error %i", r);
#else
    fcntl(m_fd, F_SETFL, O_NONBLOCK);
#endif
}

Balau::Selectable::~Selectable() {
    m_fd = -1;
    delete m_evtR;
    delete m_evtW;
    m_evtR = m_evtW = NULL;
}

bool Balau::Selectable::isClosed() { return m_fd < 0; }
bool Balau::Selectable::isEOF() { return isClosed(); }

ssize_t Balau::Selectable::read(void * buf, size_t count) throw (GeneralException) {
    if (count == 0)
        return 0;

    AAssert(m_fd >= 0, "You can't call read() on a closed selectable");

    int spins = 0;

    do {
        ssize_t r = recv(getSocket(m_fd), (char *) buf, count, 0);

        if (r >= 0) {
            m_evtR->resetMaybe();
            if (r == 0)
                close();
            return r;
        }

#ifndef _WIN32
        int err = errno;
#else
        int err = WSAGetLastError();
#ifdef _MSC_VER
        if (err == WSAEWOULDBLOCK)
#else
        if (err == WSAWOULDBLOCK)
#endif
            err = EAGAIN;
#endif

        if ((err == EAGAIN) || (err == EINTR) || (err == EWOULDBLOCK)) {
            Task::operationYield(m_evtR, Task::INTERRUPTIBLE);
        } else {
            m_evtR->stop();
            return r;
        }
    } while (spins++ < 2);

    return -1;
}

ssize_t Balau::Selectable::write(const void * buf, size_t count) throw (GeneralException) {
    if (count == 0)
        return 0;

    AAssert(m_fd >= 0, "You can't call write() on a closed selectable");

    int spins = 0;

    do {
        ssize_t r = send(getSocket(m_fd), (const char *) buf, count, 0);

        EAssert(r != 0, "send() returned 0 (broken pipe ?)");

        if (r > 0) {
            m_evtW->resetMaybe();
            return r;
        }

#ifndef _WIN32
        int err = errno;
        if (err == EPIPE) {
            close();
            return 0;
        }
#else
        int err = WSAGetLastError();
#ifdef _MSC_VER
        if (err == WSAEWOULDBLOCK)
#else
        if (err == WSAWOULDBLOCK)
#endif
            err = EAGAIN;
#endif

        if ((err == EAGAIN) || (err == EINTR) || (err == EWOULDBLOCK)) {
            Task::operationYield(m_evtW, Task::INTERRUPTIBLE);
        } else {
            m_evtW->stop();
            return r;
        }
    } while (spins++ < 2);

    return -1;
}
