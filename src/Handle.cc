#include <typeinfo>
#include "ev++.h"
#include "eio.h"
#include "Main.h"
#include "TaskMan.h"
#include "Handle.h"

class eioInterface : public Balau::AtStart {
  public:
      eioInterface() : AtStart(100) { }
    void repeatCB(ev::idle & w, int revents);
    void readyCB(ev::async & w, int revents);
    static void wantPoll();
    virtual void doStart();
    ev::idle m_repeat;
    ev::async m_ready;
};

static eioInterface eioIF;

void eioInterface::repeatCB(ev::idle & w, int revents) {
    if (eio_poll() != -1)
        w.stop();
}

void eioInterface::readyCB(ev::async & w, int revents) {
    if (eio_poll() == -1)
        m_repeat.start();
}

void eioInterface::doStart() {
    Balau::TaskMan * taskMan = Balau::TaskMan::getTaskMan();
    Assert(taskMan);
    struct ev_loop * loop = taskMan->getLoop();

    m_repeat.set(loop);
    m_repeat.set<eioInterface, &eioInterface::repeatCB>(this);

    m_ready.set(loop);
    m_ready.set<eioInterface, &eioInterface::readyCB>(this);
    m_ready.start();

    eio_init(wantPoll, NULL);
}

void eioInterface::wantPoll() {
    eioIF.m_ready.send();
}

bool Balau::Handle::canSeek() { return false; }
bool Balau::Handle::canRead() { return false; }
bool Balau::Handle::canWrite() { return false; }
off_t Balau::Handle::getSize() { return -1; }

ssize_t Balau::Handle::read(void * buf, size_t count) throw (GeneralException) {
    if (canRead())
        throw GeneralException(String("Handle ") + getName() + " can read, but read() not implemented (missing in class " + typeid(*this).name() + ")");
    else
        throw GeneralException("Handle can't read");
    return -1;
}

ssize_t Balau::Handle::write(const void * buf, size_t count) throw (GeneralException) {
    if (canWrite())
        throw GeneralException(String("Handle ") + getName() + " can write, but write() not implemented (missing in class " + typeid(this).name() + ")");
    else
        throw GeneralException("Handle can't write");
    return -1;
}

void Balau::Handle::rseek(off_t offset, int whence) throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rseek() not implemented (missing in class " + typeid(this).name() + ")");
    else
        throw GeneralException("Handle can't seek");
}

void Balau::Handle::wseek(off_t offset, int whence) throw (GeneralException) {
    rseek(offset, whence);
}

off_t Balau::Handle::rtell() throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rtell() not implemented (missing in class " + typeid(this).name() + ")");
    else
        throw GeneralException("Handle can't seek");
}

off_t Balau::Handle::wtell() throw (GeneralException) {
    return rtell();
}

bool Balau::SeekableHandle::canSeek() { return true; }

void Balau::SeekableHandle::rseek(off_t offset, int whence) throw (GeneralException) {
    Assert(canRead() || canWrite());
    off_t size;
    if (!canRead())
        wseek(offset, whence);
    switch (whence) {
    case SEEK_SET:
        m_rOffset = offset;
        break;
    case SEEK_CUR:
        m_rOffset += offset;
        break;
    case SEEK_END:
        size = getSize();
        if (getSize() < 0)
            throw GeneralException("Can't seek from end in a Handle you don't know the max size");
        m_rOffset = size + offset;
        break;
    }
    if (m_rOffset < 0)
        m_rOffset = 0;
}

void Balau::SeekableHandle::wseek(off_t offset, int whence) throw (GeneralException) {
    Assert(canRead() || canWrite());
    off_t size;
    if (!canWrite())
        rseek(offset, whence);
    switch (whence) {
    case SEEK_SET:
        m_wOffset = offset;
        break;
    case SEEK_CUR:
        m_wOffset += offset;
        break;
    case SEEK_END:
        size = getSize();
        if (getSize() < 0)
            throw GeneralException("Can't seek from end in a Handle you don't know the max size");
        m_wOffset = size + offset;
        break;
    }
    if (m_wOffset < 0)
        m_wOffset = 0;
}

off_t Balau::SeekableHandle::rtell() throw (GeneralException) {
    Assert(canRead() || canWrite());
    if (!canRead())
        return wtell();
}

off_t Balau::SeekableHandle::wtell() throw (GeneralException) {
    Assert(canRead() || canWrite());
    if (!canWrite())
        return rtell();
}
