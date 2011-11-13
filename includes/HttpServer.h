#pragma once

#include <BString.h>
#include <Exceptions.h>

namespace Balau {

class HttpServer {
  public:
      HttpServer() : m_started(false), m_listenerPtr(NULL), m_port(80) { }
      ~HttpServer() { if (!m_started) stop(); }
    void start();
    void stop();
    void setPort(int port) { Assert(!m_started); m_port = port; }
    void setLocal(const char * local) { Assert(!m_started); m_local = local; }
  private:
    bool m_started;
    void * m_listenerPtr;
    int m_port;
    String m_local;
};

};
