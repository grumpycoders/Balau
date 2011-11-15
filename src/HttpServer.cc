#include <map>

#include "Http.h"
#include "HttpServer.h"
#include "Socket.h"
#include "BStream.h"

static const ev_tstamp s_httpTimeout = 5;

namespace Balau {

class HttpWorker : public Task {
  public:
      HttpWorker(IO<Socket> & io, void * server);
  private:
    virtual void Do();
    virtual const char * getName();

    bool handleClient();
    void send400();

    IO<Socket> m_socket;
    IO<BStream> m_strm;
    String m_name;
};

};

Balau::HttpWorker::HttpWorker(IO<Socket> & io, void * _server) : m_socket(io), m_strm(new BStream(io)) {
    HttpServer * server = (HttpServer *) _server;
    m_name.set("HttpWorker(%s)", m_socket->getName());
}

void Balau::HttpWorker::send400() {
    static const char str[] =
"HTTP/1.0 400 Bad Request\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"\r\n"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\r\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\r\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n"
"  <head>\r\n"
"    <title>Bad Request</title>\r\n"
"  </head>\r\n"
"\r\n"
"  <body>\r\n"
"    The HTTP request you've sent is invalid.\r\n"
"  </body>\r\n"
"</html>\r\n";

    m_socket->write(str, sizeof(str));
    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s had an invalid request", m_name.to_charp());
}

bool Balau::HttpWorker::handleClient() {
    Events::Timeout evtTimeout(s_httpTimeout);
    waitFor(&evtTimeout);
    setOkayToEAgain(true);

    String line;
    bool gotFirst = false;
    int method = -1;
    String url;
    String HttpVersion;
    std::map<String, String> HttpHeaders;

    // read client's request
    do {
        try {
            line = m_strm->readString();
        }
        catch (EAgain) {
            if (evtTimeout.gotSignal()) {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s timed out getting request", m_name.to_charp());
                return false;
            }
            yield();
            continue;
        }
        // until we get a blank line
        if (line == "")
            break;

        if (!gotFirst) {
            gotFirst = true;
            int urlBegin = 0;

            // first line is in the form of METHOD URL HTTP/1.x
            switch(line[0]) {
            case 'G':
                if ((line[1] == 'E') && (line[2] == 'T') && (line[3] == ' ')) {
                    urlBegin = 4;
                    method = Http::GET;
                }
                break;
            case 'P':
                if ((line[1] == 'O') && (line[2] == 'S') && (line[3] == 'T') && (line[4] == ' ')) {
                    urlBegin = 5;
                    method = Http::POST;
                }
                break;
            }
            if (urlBegin == 0) {
                send400();
                return false;
            }

            int urlEnd = line.strrchr(' ') - 1;

            if (urlEnd < urlBegin) {
                send400();
                return false;
            }

            url = line.extract(urlBegin, urlEnd);

            int httpBegin = urlEnd + 2;

            if ((httpBegin + 5) >= line.strlen()) {
                send400();
                return false;
            }

            if ((line[httpBegin + 0] == 'H') &&
                (line[httpBegin + 1] == 'T') &&
                (line[httpBegin + 2] == 'T') &&
                (line[httpBegin + 3] == 'P') &&
                (line[httpBegin + 4] == '/')) {
                HttpVersion = line.extract(httpBegin + 5);
            } else {
                send400();
                return false;
            }

            if ((HttpVersion != "1.0") && (HttpVersion != "1.1")) {
                send400();
                return false;
            }
        } else {
            // parse HTTP header.
            int colon = line.strchr(':');
            if (colon <= 0) {
                send400();
                return false;
            }

            String key = line.extract(0, colon - 1);
            String value = line.extract(colon + 1);

            value.trim();

            HttpHeaders[key] = value;
        }
    } while(true);

    if (!gotFirst) {
        send400();
        return false;
    }

    return true;
}

void Balau::HttpWorker::Do() {
    bool clientStop = false;

    while (!clientStop)
        clientStop = !handleClient() || m_socket->isClosed();
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
