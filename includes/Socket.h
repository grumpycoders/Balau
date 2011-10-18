#pragma once

#include <netdb.h>
#include <Handle.h>
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
    IO<Socket> accept() throw (GeneralException);
    bool listen();
  private:
      Socket(int fd);
    class SocketEvent : public Events::BaseEvent {
      public:
          SocketEvent(int fd, int evt = EV_READ | EV_WRITE) : m_task(NULL) { Printer::elog(E_SOCKET, "Got a new SocketEvent at %p", this); m_evt.set<SocketEvent, &SocketEvent::evt_cb>(this); m_evt.set(fd, evt); }
          virtual ~SocketEvent() { m_evt.stop(); }
          void stop() { reset(); m_evt.stop(); }
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

template<class Worker>
class Listener : public Task {
  public:
      Listener(int port, const char * local = NULL) : m_stop(false) {
          m_listener.setLocal(local, port);
          m_listener.listen();
          m_name = String(ClassName(this).c_str()) + " - " + m_listener.getName();
          Printer::elog(E_SOCKET, "Created a listener task at %p", this);
      }
    virtual void Do() {
        waitFor(&m_evt);
        setOkayToEAgain(true);
        while (!m_stop) {
            IO<Socket> io;
            try {
                io = m_listener.accept();
            }
            catch (EAgain) {
                Printer::elog(E_SOCKET, "Listener task at %p (%s) got an EAgain - stop = %s", this, ClassName(this).c_str(), m_stop ? "true" : "false");
                if (!m_stop)
                    yield();
                continue;
            }
            new Worker(io);
        }
    }
    void stop() {
        Printer::elog(E_SOCKET, "Listener task at %p (%s) is asked to stop.", this, ClassName(this).c_str());
        m_stop = true;
        m_evt.trigger();
    }
    virtual const char * getName() { return m_name.to_charp(); }
  private:
    Socket m_listener;
    Events::Async m_evt;
    volatile bool m_stop;
    String m_name;
};

};
