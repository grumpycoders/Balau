#include "HttpServer.h"
#include "Socket.h"
#include "BStream.h"

static const ev_tstamp s_httpTimeout = 5;

namespace Balau {

class HttpWorker : public Task {
  public:
      HttpWorker(IO<Socket> & io, void * server) : m_socket(io), m_parent((HttpServer *) server), m_evtTimeout(s_httpTimeout) { m_name.set("HttpWorker(%s)", m_socket->getName()); }
  private:
    virtual void Do();
    virtual const char * getName();
    IO<Socket> m_socket;
    String m_name;
    HttpServer * m_parent;
    Events::Timeout m_evtTimeout;
};

};

void Balau::HttpWorker::Do() {
    waitFor(&m_evtTimeout);
    setOkayToEAgain(true);

    String line;
    IO<BStream> strm(m_socket);
    bool gotFirst = false;

    do {
        try {
            line = strm->readString();
        }
        catch (EAgain) {
            if (m_evtTimeout.gotSignal()) {
                // do some log
                return;
            }
            yield();
            continue;
        }
        if (line == "")
            break;

        if (!gotFirst) {
            gotFirst = true;
            // parse request method, URL and HTTP version.
        } else {
            // parse HTTP header.
        }
    } while(true);

    if (!gotFirst) {
        // do some log
        return;
    }
}

const char * Balau::HttpWorker::getName() {
    return m_name.to_charp();
}

typedef Balau::Listener<Balau::HttpWorker> HttpListener;

void Balau::HttpServer::start() {
    Assert(!m_started);
    m_listenerPtr = new HttpListener(m_port, m_local.to_charp());
    m_started = true;
}

void Balau::HttpServer::stop() {
    Assert(m_started);
    reinterpret_cast<HttpListener *>(m_listenerPtr)->stop();
    m_started = false;
}
