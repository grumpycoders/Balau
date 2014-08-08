#pragma once

#ifdef _WIN32
#ifndef _MSC_VER
#include <windows.h>
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif
#include <Handle.h>
#include <Selectable.h>
#include <TaskMan.h>
#include <Task.h>
#include <StacklessTask.h>
#include <Printer.h>

namespace Balau {

class Socket : public Selectable {
  public:
      Socket() throw (GeneralException);
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName();

    bool setLocal(const char * hostname = NULL, int port = 0);
    bool connect(const char * hostname, int port);
    IO<Socket> accept() throw (GeneralException);
    bool listen();
    bool resolved() { return m_resolved; }
  private:
      Socket(int fd);

    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags);

    void resolve(const char * hostname);
    void initAddr(sockaddr_in6 & out);
    void resolved(sockaddr_in6 & out);

    String m_name;
    bool m_connected = false;
    bool m_connecting = false;
    bool m_listening = false;
    int m_resolving = 0;
    bool m_resolved = false;
    bool m_resolve4Failed = false;
    bool m_resolve6Failed = false;
    Events::Custom m_resolveEvent;
    struct in_addr m_resolvedAddr4;
    struct in6_addr m_resolvedAddr6;
    sockaddr_in6 m_localAddr, m_remoteAddr;
};

class ListenerBase : public StacklessTask {
  public:
    virtual void Do();
    void stop();
    virtual const char * getName() const;
    bool started() { return m_started; }
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
    int m_port = 0;
    void * m_opaque = NULL;
    bool m_started = false;
};

template<class Worker>
class Listener : public ListenerBase {
  public:
      Listener(int port, const char * local = "", void * opaque = NULL) : ListenerBase(port, local, opaque) { }
  protected:
    virtual void factory(IO<Socket> & io, void * opaque) { TaskMan::registerTask(new Worker(io, opaque)); }
    virtual void setName() { m_name = String(ClassName(this).c_str()) + " - " + m_listener->getName(); }
};

};
