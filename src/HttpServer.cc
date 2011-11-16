#include <map>

#include "Http.h"
#include "HttpServer.h"
#include "Socket.h"
#include "BStream.h"

static const ev_tstamp s_httpTimeout = 15;

namespace Balau {

class HttpWorker : public Task {
  public:
      HttpWorker(IO<Socket> & io, void * server);
      ~HttpWorker();
  private:
    virtual void Do();
    virtual const char * getName();

    bool handleClient();
    void send400();

    IO<Socket> m_socket;
    IO<BStream> m_strm;
    String m_name;

    uint8_t * m_postData;
};

};

Balau::HttpWorker::HttpWorker(IO<Socket> & io, void * _server) : m_socket(io), m_strm(new BStream(io)), m_postData(NULL) {
    HttpServer * server = (HttpServer *) _server;
    m_name.set("HttpWorker(%s)", m_socket->getName());
    // copy stuff from server, such as port number, root document, base URL, etc...
}

Balau::HttpWorker::~HttpWorker() {
    if (m_postData) {
        free(m_postData);
        m_postData = NULL;
    }
}

void Balau::HttpWorker::send400() {
    static const char str[] =
"HTTP/1.0 400 Bad Request\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"\r\n"
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <title>Bad Request</title>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    The HTTP request you've sent is invalid.\n"
"  </body>\n"
"</html>\n";

    setOkayToEAgain(false);
    m_socket->writeString(str, sizeof(str));
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
    typedef std::map<String, String> HttpHeaders_t;
    HttpHeaders_t HttpHeaders;
    if (m_postData) {
        free(m_postData);
        m_postData = NULL;
    }
    bool persistent = false;

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

            url = line.extract(urlBegin, urlEnd - urlBegin + 1);

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

            String key = line.extract(0, colon);
            String value = line.extract(colon + 1);

            value.trim();

            HttpHeaders[key] = value;
        }
    } while(true);

    if (!gotFirst) {
        send400();
        return false;
    }

    if (method == Http::POST) {
        int lengthStr = 0;
        HttpHeaders_t::iterator i = HttpHeaders.find("Content-Length");

        if (i != HttpHeaders.end())
            lengthStr = i->second.to_int();

        m_postData = (uint8_t *) malloc(lengthStr);

        try {
            m_strm->forceRead(m_postData, lengthStr);
        }
        catch (EAgain) {
            Assert(evtTimeout.gotSignal());
            Balau::Printer::elog(Balau::E_HTTPSERVER, "%s timed out getting request (reading POST values)", m_name.to_charp());
            return false;
        }
    }

    if (HttpVersion == "1.1") {
        HttpHeaders_t::iterator i = HttpHeaders.find("Connection");

        if (i != HttpHeaders.end()) {
            if (i->second != "close") {
                send400();
                return false;
            }
        } else {
            persistent = true;
        }
    }

    // process query; everything should be here now



    // query process finished; wrapping up and exiting.

    return persistent;
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
