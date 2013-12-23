#pragma once
#ifdef _WIN32
#include <windows.h>
#endif

#include <Handle.h>
#include <TaskMan.h>
#include <Task.h>
#include <StacklessTask.h>
#include <Printer.h>

namespace Balau {

class Selectable : public Handle {
  public:
      ~Selectable();
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();

    bool gotR() { return m_evtR->gotSignal(); }
    bool gotW() { return m_evtW->gotSignal(); }

    class SelectableEvent : public Events::BaseEvent {
      public:
          SelectableEvent(int fd, int evt = ev::READ | ev::WRITE) : m_task(NULL), m_evtType(evt), m_fd(fd) { Printer::elog(E_SELECT, "Got a new SelectableEvent at %p", this); m_evt.set<SelectableEvent, &SelectableEvent::evt_cb>(this); m_evt.set(fd, evt); }
          virtual ~SelectableEvent() { Printer::elog(E_SELECT, "Destroying a SelectableEvent at %p", this); m_evt.stop(); }
        void stop() { Printer::elog(E_SELECT, "Stopping a SelectableEvent at %p", this); resetMaybe(); m_evt.stop(); }
      private:
        void evt_cb(ev::io & w, int revents) { Printer::elog(E_SELECT, "Got a libev callback on a SelectableEvent at %p", this); doSignal(); }
        virtual void gotOwner(Task * task);
        virtual bool relaxed() { return true; }

        ev::io m_evt;
        int m_evtType;
        int m_fd;
        Task * m_task = NULL;
    };

  protected:
      Selectable() { }
    void setFD(int fd) throw (GeneralException);
    void internalClose() { m_fd = -1; }
    int getFD() { return m_fd; }
    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags) = 0;
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags) = 0;

    SelectableEvent * m_evtR = NULL, * m_evtW = NULL;

  private:
    int m_fd = -1;
};

};
