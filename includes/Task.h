#pragma once

#include <stdlib.h>
#ifndef _WIN32
#include <coro.h>
#endif
#include <ev++.h>
#include <list>
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
    void reset() {
        // could be potentially changed into a simple return
        AAssert(m_task != NULL, "Can't reset an event that doesn't have a task");
        m_signal = false;
        gotOwner(m_task);
    }
    Task * taskWaiting() { AAssert(m_task, "No task is waiting for that event"); return m_task; }
    void registerOwner(Task * task) {
        if (m_task == task)
            return;
        AAssert(m_task == NULL, "Can't register an event for another task");
        m_task = task;
        gotOwner(task);
    }
  protected:
    virtual void gotOwner(Task * task) { }
  private:
    Callback * m_cb;
    bool m_signal;
    Task * m_task;
};

class Timeout : public BaseEvent {
  public:
      Timeout(ev_tstamp tstamp) { set(tstamp); }
      virtual ~Timeout() { m_evt.stop(); }
    void evt_cb(ev::timer & w, int revents) { doSignal(); }
    void set(ev_tstamp tstamp) { m_evt.set<Timeout, &Timeout::evt_cb>(this); m_evt.set(tstamp); }
  private:
    virtual void gotOwner(Task * task);
    ev::timer m_evt;
};

class TaskEvent : public BaseEvent {
  public:
      TaskEvent(Task * taskWaited);
      virtual ~TaskEvent();
    void ack();
    void signal();
    Task * taskWaited() { return m_taskWaited; }
    void evt_cb(ev::async & w, int revents) { doSignal(); }
  protected:
    virtual void gotOwner(Task * task);
  private:
    Task * m_taskWaited;
    bool m_ack, m_distant;
    ev::async m_evt;
};

class Async : public BaseEvent {
  public:
      Async() { m_evt.set<Async, &Async::evt_cb>(this); }
      virtual ~Async() { m_evt.stop(); }
    void trigger() { m_evt.send(); }
    void evt_cb(ev::async & w, int revents) { doSignal(); }
  protected:
    virtual void gotOwner(Task * task);
  private:
    ev::async m_evt;
};

#ifndef _WIN32
#define CALLBACK
#endif

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
        YIELDED,
    };
      Task();
      virtual ~Task();
    virtual const char * getName() const = 0;
    Status getStatus() const { return m_status; }
    static Task * getCurrentTask();
    static void prepare(Events::BaseEvent * evt) {
        Task * t = getCurrentTask();
        t->waitFor(evt);
    }
    static void yield(Events::BaseEvent * evt, bool interruptible = false) throw (GeneralException);
    TaskMan * getTaskMan() const { return m_taskMan; }
    struct ev_loop * getLoop();
  protected:
    void yield(bool changeStatus = false);
    virtual void Do() = 0;
    void waitFor(Events::BaseEvent * event);
    bool setOkayToEAgain(bool enable) {
        bool oldValue = m_okayToEAgain;
        m_okayToEAgain = enable;
        return oldValue;
    }
  private:
    static size_t stackSize() { return 64 * 1024; }
    void setup(TaskMan * taskMan, void * stack);
    static bool needsStacks();
    void switchTo();
    static void CALLBACK coroutineTrampoline(void *);
    void coroutine();
    void * m_stack;
#ifndef _WIN32
    coro_context m_ctx;
#else
    void * m_fiber;
#endif
    TaskMan * m_taskMan;
    Status m_status;
    void * m_tls;
    friend class TaskMan;
    friend class Events::TaskEvent;
    Lock m_eventLock;
    typedef std::list<Events::TaskEvent *> waitedByList_t;
    waitedByList_t m_waitedBy;
    bool m_okayToEAgain;
};

};
