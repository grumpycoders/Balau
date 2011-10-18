#pragma once

#include <stdlib.h>
#include <coro.h>
#include <ev++.h>
#include <vector>
#include <Exceptions.h>
#include <Printer.h>

namespace Balau {

namespace Events { class BaseEvent; };

class EAgain : public GeneralException {
  public:
      EAgain(Events::BaseEvent * evt) : GeneralException("Try Again"), m_evt(evt) { }
    Events::BaseEvent * getEvent() { return m_evt; }
  private:
    Events::BaseEvent * m_evt;
};

class TaskMan;
class Task;

namespace Events {

class Callback {
  protected:
    virtual void gotEvent(BaseEvent *) = 0;
    friend class BaseEvent;
};

class BaseEvent {
  public:
      BaseEvent() : m_cb(NULL), m_signal(false), m_task(NULL) { Printer::elog(E_TASK, "Creating event at %p", this); }
      virtual ~BaseEvent() { if (m_cb) delete m_cb; }
    bool gotSignal() { return m_signal; }
    void doSignal();
    void reset() { Assert(m_task != NULL); m_signal = false; gotOwner(m_task); }
    Task * taskWaiting() { Assert(m_task); return m_task; }
    void registerOwner(Task * task) { if (m_task == task) return; Assert(m_task == NULL); m_task = task; gotOwner(task); }
  protected:
    virtual void gotOwner(Task * task) { }
  private:
    Callback * m_cb;
    bool m_signal;
    Task * m_task;
};

class Timeout : public BaseEvent {
  public:
      Timeout(ev_tstamp tstamp);
    void evt_cb(ev::timer & w, int revents);
    void set(ev_tstamp tstamp);
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

class Custom : public BaseEvent {
  public:
    void doSignal() { BaseEvent::doSignal(); ev_break(m_loop, EVBREAK_ALL); }
  protected:
    virtual void gotOwner(Task * task);
  private:
    struct ev_loop * m_loop;
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
    static void prepare(Events::BaseEvent * evt) {
        Task * t = getCurrentTask();
        t->waitFor(evt);
    }
    static void yield(Events::BaseEvent * evt, bool interruptible = false) throw (GeneralException) {
        Task * t = getCurrentTask();
        t->waitFor(evt);

        do {
            t->yield();
            Printer::elog(E_TASK, "operation back from yielding; interruptible = %s; okayToEAgain = %s", interruptible ? "true" : "false", t->m_okayToEAgain ? "true" : "false");
        } while ((!interruptible || !t->m_okayToEAgain) && !evt->gotSignal());
        if (interruptible && t->m_okayToEAgain && !evt->gotSignal()) {
            Printer::elog(E_TASK, "operation is throwing an exception.");
            throw EAgain(evt);
        }
    }
    TaskMan * getTaskMan() { return m_taskMan; }
    struct ev_loop * getLoop();
  protected:
    void yield();
    virtual void Do() = 0;
    void waitFor(Events::BaseEvent * event);
    bool setOkayToEAgain(bool enable) {
        bool oldValue = m_okayToEAgain;
        m_okayToEAgain = enable;
        return oldValue;
    }
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
    bool m_okayToEAgain;
};

};
