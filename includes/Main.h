#pragma once

#include <Exceptions.h>
#include <Task.h>

namespace Balau {

class Exit : public GeneralException {
  public:
      Exit(int code = -1) : GeneralException(), m_code(code) { String s; s.set("Application exitting with code = %i", code); setMsg(s.strdup()); }
    int getCode() { return m_code; }
  private:
    int m_code;
};

class MainTask : public Task {
  public:
      MainTask(int argc, char ** argv, char ** enve) : argc(argc), argv(argv), enve(enve) { }
      virtual ~MainTask();
    virtual const char * getName() const;
    virtual void Do();
    void stopTaskManOnExit(bool v) { m_stopTaskManOnExit = v; }
  private:
    int argc;
    char ** argv;
    char ** enve;
    bool m_stopTaskManOnExit = true;
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
      Main() : m_status(UNKNOWN) { IAssert(s_application == NULL, "There can't be two main apps"); s_application = this; }
    static Status getStatus() { return s_application->m_status; }
    int bootstrap(int _argc, char ** _argv);
    static bool hasMain() { return s_application; }
  private:
    Status m_status;
    static Main * s_application;
};

};
