#pragma once

#include <map>
#include <list>

#include <Atomic.h>
#include <BString.h>
#include <BRegex.h>
#include <Exceptions.h>
#include <Threads.h>
#include <Handle.h>
#include <Http.h>

namespace Balau {

class HttpWorker;

class HttpServer {
  public:

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
