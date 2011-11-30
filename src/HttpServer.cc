#include "Http.h"
#include "HttpServer.h"
#include "Socket.h"
#include "BStream.h"
#include "SimpleMustache.h"
#include "Main.h"

class OutputCheck : public Balau::Handle {
  public:
      OutputCheck(Balau::IO<Balau::Handle> h) : m_h(h), m_wrote(false) { Assert(m_h->canWrite()); m_name.set("OutputCheck(%s)", m_h->getName()); }
    virtual void close() throw (Balau::GeneralException) { m_h->close(); }
    virtual bool isClosed() { return m_h->isClosed(); }
    virtual bool isEOF() { return m_h->isEOF(); }
    virtual bool canWrite() { return true; }
    virtual const char * getName() { return m_name.to_charp(); }
    virtual ssize_t write(const void * buf, size_t count) throw (Balau::GeneralException) {
        if (!count)
            return 0;
        m_wrote = true;
        return m_h->write(buf, count);
    }
    bool wrote() { return m_wrote; }
  private:
    Balau::IO<Balau::Handle> m_h;
    Balau::String m_name;
    bool m_wrote;
};

static const ev_tstamp s_httpTimeout = 5;

namespace Balau {

class HttpWorker : public Task {
  public:
      HttpWorker(IO<Handle> io, void * server);
      ~HttpWorker();
    static void buildErrorTemplate(IO<Handle> h) { m_errorTemplate.setTemplate(h); }
    static void buildErrorTemplate(const char * str, ssize_t s) { m_errorTemplate.setTemplate(str, s); }
    static void buildErrorTemplate(const String & str) { m_errorTemplate.setTemplate(str); }
  private:
    virtual void Do();
    virtual const char * getName();

    bool handleClient();
    void sendError(int error, const char * msg, bool closeConnection, std::vector<String> trace);
    void send400() { std::vector<String> d2; sendError(400, "The HTTP request you've sent is invalid", true, d2); }
    void send404() { std::vector<String> d2; sendError(404, "The HTTP request you've sent didn't match any action on this server.", false, d2); }
    void send500(const char * msg, std::vector<String> trace) { String smsg; smsg.set("The HTTP request you've sent triggered an internal error: `%s'", msg); sendError(500, smsg.to_charp(), true, trace); }
    String httpUnescape(const char * in);
    void readVariables(Http::StringMap & variables, char * str);

    IO<Handle> m_socket;
    IO<BStream> m_strm;
    String m_name;
    HttpServer * m_server;
    String m_serverName;
    static SimpleMustache m_errorTemplate;
};

};

Balau::SimpleMustache Balau::HttpWorker::m_errorTemplate;

class SetDefaultTemplate : public Balau::AtStart {
  public:
      SetDefaultTemplate() : AtStart(0) { }
    virtual void doStart();
    static const Balau::String m_defaultErrorTemplate;
};

static SetDefaultTemplate setDefaultTemplate;

const Balau::String SetDefaultTemplate::m_defaultErrorTemplate(
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
"    <title>{{title}}</title>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    <h1>{{msg}}</h1>\n"
"{{#hasTrace}}\n"
"    <br /><h2>Context:</h2>\n"
"    {{#trace}}<pre>{{line}}</pre>{{/trace}}<br />\n"
"{{/hasTrace}}\n"
"  </body>\n"
"</html>\n"
);

void SetDefaultTemplate::doStart() {
    Balau::HttpWorker::buildErrorTemplate(m_defaultErrorTemplate);
}

Balau::HttpWorker::HttpWorker(IO<Handle> io, void * _server) : m_socket(new WriteOnly(io)), m_strm(new BStream(io)) {
    m_server = (HttpServer *) _server;
    m_name.set("HttpWorker(%s)", m_socket->getName());
    m_serverName = "Balau/1.0";
    // get stuff from server, such as port number, root document, base URL, default 400/404 actions, etc...
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

void Balau::HttpWorker::readVariables(Http::StringMap & variables, char * str) {
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

static const char * getErrorMsg(int httpError) {
    switch (httpError) {
    case 400:
        return "Bad Request";
        break;
    case 403:
        return "Forbidden";
        break;
    case 404:
        return "Not Found";
        break;
    case 500:
    default:
        return "Internal Error";
        break;
    }
}

void Balau::HttpWorker::sendError(int error, const char * msg, bool closeConnection, std::vector<String> trace) {
    SimpleMustache * tpl = &m_errorTemplate;
    const char * errorMsg = getErrorMsg(error);
    Printer::elog(Balau::E_HTTPSERVER, "%s caused a %i error (%s)", m_name.to_charp(), error, errorMsg);
    SimpleMustache::Context ctx;
    String title;
    title.set("Error %i - %s", error, errorMsg);
    ctx["title"] = title;
    ctx["hasTrace"] = !trace.empty();
    ctx["msg"] = msg;
    if (m_socket->isClosed()) return;
    for (std::vector<String>::iterator i = trace.begin(); i != trace.end(); i++)
        ctx["trace"][(ssize_t) 0]["line"] = *i;
    if (closeConnection) {
        String headers;
        headers.set(
"HTTP/1.0 %i %s\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"Server: %s\r\n"
"\r\n", error, errorMsg, m_serverName.to_charp());
        m_socket->forceWrite(headers);
        if (m_socket->isClosed()) return;
        tpl->render(m_socket, &ctx);
    } else {
        IO<Buffer> errorText(new Buffer);
        tpl->render(errorText, &ctx);
        off_t length = errorText->getSize();
        String headers;
        headers.set(
"HTTP/1.1 %i %s\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: keep-alive\r\n"
"Server: %s\r\n"
"Content-Length: %i\r\n"
"\r\n", error, errorMsg, m_serverName.to_charp(), length);
        m_socket->forceWrite(headers);
        if (m_socket->isClosed()) return;
        m_socket->forceWrite(errorText->getBuffer(), length);
    }
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
    Http::StringMap httpHeaders;
    Http::StringMap variables;
    Http::FileList files;
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
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s didn't get a URI after the method", m_name.to_charp());
                send400();
                return false;
            }

            int urlEnd = line.strrchr(' ') - 1;

            if (urlEnd < urlBegin) {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has a misformated URI (or no space after it)", m_name.to_charp());
                send400();
                return false;
            }

            uri = line.extract(urlBegin, urlEnd - urlBegin + 1);

            int httpBegin = urlEnd + 2;

            if ((httpBegin + 5) >= line.strlen()) {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s doesn't have enough characters after the URI", m_name.to_charp());
                send400();
                return false;
            }

            if ((line[httpBegin + 0] == 'H') &&
                (line[httpBegin + 1] == 'T') &&
                (line[httpBegin + 2] == 'T') &&
                (line[httpBegin + 3] == 'P') &&
                (line[httpBegin + 4] == '/')) {
                httpVersion = line.extract(httpBegin + 5);
            } else {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s doesn't have HTTP after the URI", m_name.to_charp());
                send400();
                return false;
            }

            if ((httpVersion != "1.0") && (httpVersion != "1.1")) {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s doesn't have a proper HTTP version", m_name.to_charp());
                send400();
                return false;
            }
        } else {
            // parse HTTP header.
            int colon = line.strchr(':');
            if (colon <= 0) {
                Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has an invalid HTTP header", m_name.to_charp());
                send400();
                return false;
            }

            String key = line.extract(0, colon);
            String value = line.extract(colon + 1);

            httpHeaders[key] = value.trim();
        }
    } while(true);

    if (!gotFirst) {
        Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has nothing in its request", m_name.to_charp());
        send400();
        return false;
    }

    if (httpVersion == "1.1") {
        Http::StringMap::iterator i = httpHeaders.find("Connection");

        if (i != httpHeaders.end()) {
            String conn = i->second;
            String::List connVals = conn.split(',');
            bool gotOne = false;
            for (String::List::iterator j = connVals.begin(); j != connVals.end(); j++) {
                String t = j->trim();
                if ((t == "close") && (!gotOne)) {
                    gotOne = true;
                    persistent = false;
                } else if ((t == "keep-alive") && (!gotOne)) {
                    gotOne = true;
                    persistent = true;
                } else if (t == "TE") {
                    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s got the 'TE' connection marker (which is still unknown)", m_name.to_charp());
                } else {
                    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has an improper Connection HTTP header (%s)", m_name.to_charp(), t.to_charp());
                    send400();
                    return false;
                }
            }
        } else {
            persistent = true;
        }
    }

    if (method == Http::POST) {
        int length = 0;
        Http::StringMap::iterator i;
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
                    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has an improper multipart string (no ;)", m_name.to_charp());
                    send400();
                    return false;
                }
                Http::StringMap t;
                char * b = i->second.extract(sizeof(multipartStr) + 1).do_trim().strdup();
                readVariables(t, b);
                free(b);

                i = t.find("boundary");

                if (i == t.end()) {
                    Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has an improper multipart string (no boundary)", m_name.to_charp());
                    send400();
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

    Http::StringMap::iterator hostIter = httpHeaders.find("host");

    if (hostIter != httpHeaders.end()) {
        if (host != "") {
            Balau::Printer::elog(Balau::E_HTTPSERVER, "%s has a host field, although the URI already has one", m_name.to_charp());
            send400();
            return false;
        }

        host = hostIter->second;
    }

    // process query; everything should be here now

    HttpServer::ActionFound f = m_server->findAction(uri.to_charp(), host.to_charp());
    if (f.first) {
        IO<OutputCheck> out(new OutputCheck(m_socket));
        Http::Request req;
        req.method = method;
        req.host = host;
        req.uri = uri;
        req.variables = variables;
        req.headers = httpHeaders;
        req.files = files;
        req.persistent = persistent;
        try {
            if (!f.first->Do(m_server, req, f.second, out))
                persistent = false;
        }
        catch (GeneralException e) {
            Printer::log(M_ERROR, "%s got an exception while processing its request: `%s'", m_name.to_charp(), e.getMsg());
            std::vector<String> trace = e.getTrace();
            for (std::vector<String>::iterator i = trace.begin(); i != trace.end(); i++) 
                Printer::log(M_DEBUG, "%s", i->to_charp());
            if (!out->wrote())
                send500(e.getMsg(), e.getTrace());
            return false;
        }
        catch (...) {
            Printer::log(M_ERROR, "%s got an unknow exception while processing its request: `%s'", m_name.to_charp());
            if (!out->wrote()) {
                std::vector<String> d;
                send500("unknow exception", d);
            }
            return false;
        }
    } else {
        send404();
    }

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
    m_listenerPtr = createTask(new HttpListener(m_port, m_local.to_charp(), this));
    m_started = true;
}

void Balau::HttpServer::stop() {
    Assert(m_started);
    reinterpret_cast<HttpListener *>(m_listenerPtr)->stop();
    m_started = false;
}

void Balau::HttpServer::registerAction(Action * action) {
    m_actionsLock.enter();
    action->ref();
    m_actions.push_front(action);
    m_actionsLock.leave();
}

void Balau::HttpServer::flushAllActions() {
    m_actionsLock.enter();
    Action * a;
    while (!m_actions.empty()) {
        a = m_actions.front();
        m_actions.pop_front();
        a->unref();
    }
    m_actionsLock.leave();
}

Balau::HttpServer::Action::ActionMatch Balau::HttpServer::Action::matches(const char * uri, const char * host) {
    ActionMatch r;

    r.second = m_host.match(host);
    if (r.second.empty())
        return r;

    r.first = m_regex.match(uri);
    return r;
}

Balau::HttpServer::ActionFound Balau::HttpServer::findAction(const char * uri, const char * host) {
    m_actionsLock.enter();

    ActionList::iterator i;
    ActionFound r;

    for (i = m_actions.begin(); i != m_actions.end(); i++) {
        r.first = *i;
        r.second = r.first->matches(uri, host);
        if (!r.second.first.empty())
            break;
    }

    if (r.second.first.empty())
        r.first = NULL;
    else
        r.first->ref();

    m_actionsLock.leave();

    return r;
}
