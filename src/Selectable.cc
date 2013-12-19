#include <sys/types.h>
#include <unistd.h>
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

#ifndef _WIN32
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
    ioctlsocket(m_fd, FIONBIO, &iMode);
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
        ssize_t r = recv(m_fd, (char *) buf, count, 0);

        if (r >= 0) {
            if (r == 0)
                close();
            return r;
        }

        if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
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
        ssize_t r = send(m_fd, (const char *) buf, count, 0);

        EAssert(r != 0, "send() returned 0 (broken pipe ?)");

        if (r > 0)
            return r;

#ifndef _WIN32
        if (errno == EPIPE) {
            close();
            return 0;
        }
#endif

        if ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)) {
            Task::operationYield(m_evtW, Task::INTERRUPTIBLE);
        } else {
            m_evtW->stop();
            return r;
        }
    } while (spins++ < 2);

    return -1;
}
