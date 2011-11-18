#pragma once

#include <map>
#include <list>

#include <Atomic.h>
#include <BString.h>
#include <BRegex.h>
#include <Exceptions.h>
#include <Threads.h>
#include <Handle.h>

namespace Balau {

class HttpWorker;

class HttpServer {
  public:

    typedef std::map<String, String> StringMap;
    typedef std::map<String, IO<Handle> > FileList;

    class Action {
      public:
          Action(Regex & regex, Regex & host = Regexes::any) : m_regex(regex), m_host(host), m_refCount(0) { }
          ~Action() { Assert(m_refCount == 0); }
        typedef std::pair<Regex::Captures, Regex::Captures> ActionMatches;
        ActionMatches matches(const char * uri, const char * host);
        void unref() { if (Atomic::Decrement(&m_refCount) == 0) delete this; }
        void ref() { Atomic::Increment(&m_refCount); }
        void registerMe(HttpServer * server) { server->registerAction(this); }
        virtual bool Do(HttpServer * server, ActionMatches & m, IO<Handle> out, StringMap & vars, StringMap & headers, FileList & files) = 0;
      private:
        Regex m_regex, m_host;
        volatile int m_refCount;
    };

      HttpServer() : m_started(false), m_listenerPtr(NULL), m_port(80) { }
      ~HttpServer() { if (!m_started) stop(); }
    void start();
    void stop();
    void setPort(int port) { Assert(!m_started); m_port = port; }
    void setLocal(const char * local) { Assert(!m_started); m_local = local; }
    void registerAction(Action * action);
    void flushAllActions();
    typedef std::pair<Action *, Action::ActionMatches> ActionFound;
    ActionFound findAction(const char * uri, const char * host);
  private:
    bool m_started;
    void * m_listenerPtr;
    int m_port;
    String m_local;
    typedef std::list<Action *> ActionList;
    ActionList m_actions;
    Lock m_actionsLock;

    friend class HttpWorker;
};

};
