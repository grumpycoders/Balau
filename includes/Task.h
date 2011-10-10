#pragma once

#include <stdlib.h>
#include <coro.h>
#include <ev++.h>
#include <Exceptions.h>
#include <vector>

namespace Balau {

class TaskMan;
class Task;

namespace Events {

class BaseEvent {
  public:
      BaseEvent() : m_signal(false), m_task(NULL) { }
    bool gotSignal() { return m_signal; }
    void doSignal();
    Task * taskWaiting() { Assert(m_task); return m_task; }
    void registerOwner(Task * task) { Assert(m_task == NULL); m_task = task; gotOwner(task); }
  protected:
    virtual void gotOwner(Task * task) { }
  private:
    bool m_signal;
    Task * m_task;
};

class Timeout : public BaseEvent {
  public:
      Timeout(ev_tstamp tstamp);
    void evt_cb(ev::timer & w, int revents);
  private:
    virtual void gotOwner(Task * task);
    ev::timer m_evt;
};

class TaskEvent : public BaseEvent {
  public:
      TaskEvent(Task * taskWaited);
    Task * taskWaited() { return m_taskWaited; }
  private:
    Task * m_taskWaited;
};

};

class Task {
  public:
    enum Status {
        STARTING,
        RUNNING,
        IDLE,
        STOPPED,
        FAULTED,
    };
      Task();
      virtual ~Task();
    virtual const char * getName() = 0;
    Status getStatus() { return m_status; }
    static Task * getCurrentTask();
    TaskMan * getTaskMan() { return m_taskMan; }
  protected:
    void suspend();
    virtual void Do() = 0;
    void waitFor(Events::BaseEvent * event);
  private:
    size_t stackSize() { return 128 * 1024; }
    void switchTo();
    static void coroutine(void *);
    void * m_stack;
    coro_context m_ctx;
    TaskMan * m_taskMan;
    Status m_status;
    void * m_tls;
    friend class TaskMan;
    friend class Events::TaskEvent;
    typedef std::vector<Events::TaskEvent *> waitedByList_t;
    waitedByList_t m_waitedBy;
};

};
