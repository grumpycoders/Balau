#pragma once

#include <map>
#include <list>

#include <Atomic.h>
#include <BString.h>
#include <BRegex.h>
#include <Exceptions.h>
#include <Threads.h>
#include <Handle.h>
#include <Buffer.h>
#include <Http.h>

namespace Balau {

class HttpWorker;

class HttpServer {
  public:

    class Response {
      public:
          Response(HttpServer * server, Http::Request req, IO<Handle> out) : m_server(server), m_req(req), m_out(out), m_buffer(new Buffer()), m_responseCode(200), m_type("text/html; charset=UTF-8"), m_flushed(false) { }
        void SetResponseCode(int code) { m_responseCode = code; }
        void SetContentType(const String & type) { m_type = type; }
        IO<Buffer> get() { return m_buffer; }
        IO<Buffer> operator->() { return m_buffer; }
        void Flush();
        void AddHeader(const String & line) { m_extraHeaders.push_front(line); }
        void AddHeader(const String & key, const String & val) { AddHeader(key + ": " + val); }
      private:
        HttpServer * m_server;
        Http::Request m_req;
        IO<Handle> m_out;

        IO<Buffer> m_buffer;
        int m_responseCode;
        String m_type;
        std::list<String> m_extraHeaders;
        bool m_flushed;
    };

    class Action {
      public:
          Action(const Regex & regex, const Regex & host = Regexes::any) : m_regex(regex), m_host(host), m_refCount(0) { }
          ~Action() { AAssert(m_refCount == 0, "Don't delete an Action directly"); }
        struct ActionMatch {
            Regex::Captures uri, host;
        };
        ActionMatch matches(const char * uri, const char * host);
        void unref() { if (Atomic::Decrement(&m_refCount) == 0) delete this; }
        void ref() { Atomic::Increment(&m_refCount); }
        void registerMe(HttpServer * server) { server->registerAction(this); }
        virtual bool Do(HttpServer * server, Http::Request & req, ActionMatch & match, IO<Handle> out) throw (GeneralException) = 0;
      private:
        const Regex m_regex, m_host;
        volatile int m_refCount;
    };

      HttpServer() : m_started(false), m_listenerPtr(NULL), m_port(80) { }
      ~HttpServer() { if (!m_started) stop(); }
    void start();
    void stop();
    void setPort(int port) { AAssert(!m_started, "You can't set the HTTP port once the server has started"); m_port = port; }
    void setLocal(const char * local) { AAssert(!m_started, "You can't set the HTTP IP once the server has started"); m_local = local; }
    void registerAction(Action * action);
    void flushAllActions();
    struct ActionFound {
        Action * action;
        Action::ActionMatch matches;
    };
    ActionFound findAction(const char * uri, const char * host);
    String getServerName() { return "Balau/1.0"; }
  private:
    bool m_started;
    void * m_listenerPtr;
    int m_port;
    String m_local;
    typedef std::list<Action *> ActionList;
    ActionList m_actions;
    RWLock m_actionsLock;

    friend class HttpWorker;
};

};
