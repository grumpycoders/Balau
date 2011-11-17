#include <map>

#include "Http.h"
#include "HttpServer.h"
#include "Socket.h"
#include "BStream.h"

static const ev_tstamp s_httpTimeout = 15;
#define DAEMON_NAME "Balau/1.0"

namespace Balau {

typedef std::map<String, String> StringMap;

class HttpWorker : public Task {
  public:
      HttpWorker(IO<Handle> io, void * server);
      ~HttpWorker();
  private:
    virtual void Do();
    virtual const char * getName();

    bool handleClient();
    void send400(Events::BaseEvent * evt);
    String httpUnescape(const char * in);
    void readVariables(StringMap & variables, char * str);

    IO<Handle> m_socket;
    IO<BStream> m_strm;
    String m_name;
};

};

Balau::HttpWorker::HttpWorker(IO<Handle> io, void * _server) : m_socket(new WriteOnly(io)), m_strm(new BStream(io)) {
    HttpServer * server = (HttpServer *) _server;
    m_name.set("HttpWorker(%s)", m_socket->getName());
    // copy stuff from server, such as port number, root document, base URL, etc...
}

Balau::HttpWorker::~HttpWorker() {
}

Balau::String Balau::HttpWorker::httpUnescape(const char * in) {
    String r;
    const char * p;
    char hexa[3];
    char out;

    for (p = in; *p; p++) {
        switch (*p) {
        case '+':
            r += String(" ", 1);
            break;
        case '%':
            hexa[0] = *++p;
            if (!hexa[0])
                return r;
            hexa[1] = *++p;
            if (!hexa[1])
                return r;
            hexa[2] = 0;
            out = strtol(hexa, NULL, 16);
            r += String(&out, 1);
            break;
        default:
            r += String(p, 1);
            break;
        }
    }

    return r;
}

void Balau::HttpWorker::readVariables(StringMap & variables, char * str) {
    char * ampPos;
    do {
        ampPos = strchr(str, '&');
        if (ampPos)
            *ampPos = 0;

        char * val = strchr(str, '=');

        if (val) {
            *val++ = 0;
        }
        String keyStr = httpUnescape(str);
        String valStr = val ? httpUnescape(val) : String("");
        variables[keyStr] = valStr;

        str = ampPos + 1;
    } while (ampPos);
}

void Balau::HttpWorker::send400(Events::BaseEvent * evt) {
    static const char str[] =
"HTTP/1.0 400 Bad Request\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"Server: " DAEMON_NAME "\r\n"
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
    m_socket->forceWrite(str, sizeof(str) - 1, evt);
    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s had an invalid request", m_name.to_charp());
}

bool Balau::HttpWorker::handleClient() {
    Events::Timeout evtTimeout(s_httpTimeout);
    waitFor(&evtTimeout);
    setOkayToEAgain(true);

    String line;
    bool gotFirst = false;
    int method = -1;
    String host;
    String uri;
    String httpVersion;
    StringMap httpHeaders;
    StringMap variables;
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

            // first line is in the form of METHOD URL HTTP/xxx
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
                send400(&evtTimeout);
                return false;
            }

            int urlEnd = line.strrchr(' ') - 1;

            if (urlEnd < urlBegin) {
                send400(&evtTimeout);
                return false;
            }

            uri = line.extract(urlBegin, urlEnd - urlBegin + 1);

            int httpBegin = urlEnd + 2;

            if ((httpBegin + 5) >= line.strlen()) {
                send400(&evtTimeout);
                return false;
            }

            if ((line[httpBegin + 0] == 'H') &&
                (line[httpBegin + 1] == 'T') &&
                (line[httpBegin + 2] == 'T') &&
                (line[httpBegin + 3] == 'P') &&
                (line[httpBegin + 4] == '/')) {
                httpVersion = line.extract(httpBegin + 5);
            } else {
                send400(&evtTimeout);
                return false;
            }

            if ((httpVersion != "1.0") && (httpVersion != "1.1")) {
                send400(&evtTimeout);
                return false;
            }
        } else {
            // parse HTTP header.
            int colon = line.strchr(':');
            if (colon <= 0) {
                send400(&evtTimeout);
                return false;
            }

            String key = line.extract(0, colon);
            String value = line.extract(colon + 1);

            value.trim();

            httpHeaders[key] = value;
        }
    } while(true);

    if (!gotFirst) {
        send400(&evtTimeout);
        return false;
    }

    if (httpVersion == "1.1") {
        StringMap::iterator i = httpHeaders.find("Connection");

        if (i != httpHeaders.end()) {
            if (i->second != "close") {
                send400(&evtTimeout);
                return false;
            }
        } else {
            persistent = true;
        }
    }

    if (method == Http::POST) {
        int length = 0;
        StringMap::iterator i;
        bool multipart = false;
        String boundary;

        i = httpHeaders.find("Content-Length");

        if (i != httpHeaders.end())
            length = i->second.to_int();

        i = httpHeaders.find("Content-Type");

        if (i != httpHeaders.end()) {
            static const String multipartStr = "multipart/form-data";
            if (i->second.extract(multipartStr.strlen()) == multipartStr) {
                if (i->second[multipartStr.strlen() + 1] != ';') {
                    send400(&evtTimeout);
                    return false;
                }
                StringMap t;
                char * b = i->second.extract(sizeof(multipartStr) + 1).trim().strdup();
                readVariables(t, b);
                free(b);

                i = t.find("boundary");

                if (i == t.end()) {
                    send400(&evtTimeout);
                    return false;
                }

                boundary = i->second;
                multipart = true;
            }
        }

        if (multipart) {
            // will handle this horror later...
            Assert(!"multipart/form-data not supported for now");
        } else {
            uint8_t * postData = (uint8_t *) malloc(length);

            try {
                m_strm->forceRead(postData, length);
            }
            catch (EAgain) {
                Assert(evtTimeout.gotSignal());
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s timed out getting request (reading POST values)", m_name.to_charp());
                return false;
            }

            readVariables(variables, (char *) postData);

            free(postData);
        }
    }

    int variablesPos = uri.strchr('?');

    if (variablesPos >= 0) {
        char * variablesStr = uri.strdup(variablesPos + 1);
        uri = httpUnescape(uri.extract(0, variablesPos).to_charp());
        readVariables(variables, variablesStr);
        free(variablesStr);
    } else {
        uri = httpUnescape(uri.to_charp());
    }

    if (uri.extract(0, 7) == "http://") {
        int hostEnd = uri.strchr('/', 7);

        if (hostEnd < 0) {
            host = uri.extract(7);
            uri = "/";
        } else {
            host = uri.extract(7, hostEnd - 7);
            uri = uri.extract(hostEnd + 1);
        }
    }

    StringMap::iterator hostIter = httpHeaders.find("host");

    if (hostIter != httpHeaders.end()) {
        if (host != "") {
            send400(&evtTimeout);
            return false;
        }

        host = hostIter->second;
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
    m_listenerPtr = createTask(new HttpListener(m_port, m_local.to_charp()));
    m_started = true;
}

void Balau::HttpServer::stop() {
    Assert(m_started);
    reinterpret_cast<HttpListener *>(m_listenerPtr)->stop();
    m_started = false;
}
