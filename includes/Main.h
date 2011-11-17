#pragma once

#include <Exceptions.h>

namespace Balau {

class AtStart {
  protected:
      AtStart(int priority = 0);
    virtual void doStart() = 0;
  private:
    const int m_priority;
    AtStart * m_next;
    static AtStart * s_head;
    friend class Main;
};

class AtExit {
  protected:
      AtExit(int priority = 0);
    virtual void doExit() = 0;
  private:
    const int m_priority;
    AtExit * m_next;
    static AtExit * s_head;
    friend class Main;
};

class Exit : public GeneralException {
  public:
      Exit(int code = -1) : GeneralException(), m_code(code) { String s; s.set("Application exitting with code = %i", code); setMsg(s.strdup()); }
    int getCode() { return m_code; }
  private:
    int m_code;
};

};

#include <Printer.h>
#include <Task.h>
#include <TaskMan.h>

namespace Balau {

class MainTask : public Task {
  public:
      MainTask() : m_stopTaskManOnExit(true) { }
      virtual ~MainTask() { if (m_stopTaskManOnExit) TaskMan::stop(); }
    virtual const char * getName() { return "Main Task"; }
    virtual void Do();
    void stopTaskManOnExit(bool v) { m_stopTaskManOnExit = v; }
  private:
    bool m_stopTaskManOnExit;
};

class Main {
  public:
    enum Status {
        UNKNOWN = 0,
        STARTING,
        RUNNING,
        STOPPING,
        STOPPED,
    };
      Main() : m_status(UNKNOWN) { Assert(s_application == 0); s_application = this; }
    static Status status() { return s_application->m_status; }
    int bootstrap(int _argc, char ** _argv) {
        int r = 0;
        m_status = STARTING;

        argc = _argc;
        argv = _argv;
        enve = NULL;

        for (AtStart * ptr = AtStart::s_head; ptr; ptr = ptr->m_next)
            ptr->doStart();

        try {
            m_status = RUNNING;
            MainTask * mainTask = createTask(new MainTask());
            TaskMan::getDefaultTaskMan()->mainLoop();
            m_status = STOPPING;
        }
        catch (Exit e) {
            m_status = STOPPING;
            r = e.getCode();
        }
        catch (GeneralException e) {
            m_status = STOPPING;
            Printer::log(M_ERROR | M_ALERT, "The application caused an exception: %s", e.getMsg());
            r = -1;
        }
        catch (...) {
            m_status = STOPPING;
            Printer::log(M_ERROR | M_ALERT, "The application caused an unknown exception");
            r = -1;
        }
        m_status = STOPPING;

        for (AtExit * ptr = AtExit::s_head; ptr; ptr = ptr->m_next)
            ptr->doExit();

        m_status = STOPPED;
        return r;
    }
  protected:
    int argc;
    char ** argv;
    char ** enve;
  private:
    Status m_status;
    static Main * s_application;
};

#define BALAU_STARTUP \
\
class Application : public Balau::Main { \
  public: \
    virtual int startup() throw (Balau::GeneralException); \
}; \
\
extern "C" { \
    int main(int argc, char ** argv) { \
        setlocale(LC_ALL, ""); \
        Balau::Main mainClass; \
        return mainClass.bootstrap(argc, argv); \
    } \
}

};
