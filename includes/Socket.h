#pragma once

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif
#include <Handle.h>
#include <TaskMan.h>
#include <Task.h>
#include <Printer.h>

namespace Balau {

class Socket : public Handle {
  public:

      Socket() throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName();

    bool setLocal(const char * hostname = NULL, int port = 0);
    bool connect(const char * hostname, int port);
    bool gotR() { return m_evtR->gotSignal(); }
    bool gotW() { return m_evtW->gotSignal(); }
    IO<Socket> accept() throw (GeneralException);
    bool listen();
  private:
      Socket(int fd);
    class SocketEvent : public Events::BaseEvent {
      public:
          SocketEvent(int fd, int evt = ev::READ | ev::WRITE) : m_task(NULL) { Printer::elog(E_SOCKET, "Got a new SocketEvent at %p", this); m_evt.set<SocketEvent, &SocketEvent::evt_cb>(this); m_evt.set(fd, evt); }
          virtual ~SocketEvent() { Printer::elog(E_SOCKET, "Destroying a SocketEvent at %p", this); m_evt.stop(); }
          void stop() { Printer::elog(E_SOCKET, "Stopping a SocketEvent at %p", this); reset(); m_evt.stop(); }
      private:
        void evt_cb(ev::io & w, int revents) { Printer::elog(E_SOCKET, "Got a libev callback on a SocketEvent at %p", this); doSignal(); }
        virtual void gotOwner(Task * task);

        ev::io m_evt;
        Task * m_task;
    };

    int m_fd;
    String m_name;
    bool m_connected;
    bool m_connecting;
    bool m_listening;
    sockaddr_in6 m_localAddr, m_remoteAddr;
    SocketEvent * m_evtR, * m_evtW;
};

class ListenerBase : public Task {
  public:
    virtual void Do();
    void stop();
    virtual const char * getName();
  protected:
      ListenerBase(int port, const char * local, void * opaque);
    virtual void factory(IO<Socket> & io, void * opaque) = 0;
    virtual void setName() = 0;
    String m_name;
    IO<Socket> m_listener;
  private:
    Events::Async m_evt;
    volatile bool m_stop;
    String m_local;
    int m_port;
    void * m_opaque;
};

template<class Worker>
class Listener : public ListenerBase {
  public:
      Listener(int port, const char * local = "", void * opaque = NULL) : ListenerBase(port, local, opaque) { }
  protected:
    virtual void factory(IO<Socket> & io, void * opaque) { createTask(new Worker(io, opaque)); }
    virtual void setName() { m_name = String(ClassName(this).c_str()) + " - " + m_listener->getName(); }
};

};
