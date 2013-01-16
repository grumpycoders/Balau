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
      EAgain(Events::BaseEvent * evt) : GeneralException(), m_evt(evt) { }
    Events::BaseEvent * getEvent() { return m_evt; }
  private:
    Events::BaseEvent * m_evt;
};

class TaskSwitch : public GeneralException {
  public:
      TaskSwitch() : GeneralException() { }
};

class TaskMan;
class Task;

namespace Events {

class Callback {
  public:
      virtual ~Callback() { }
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
      Timeout() { }
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
      TaskEvent(Task * taskWaited = NULL);
      virtual ~TaskEvent();
    void ack();
    void signal();
    Task * taskWaited() { return m_taskWaited; }
    void attachToTask(Task * taskWaited);
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
        SLEEPING,
        STOPPED,
        FAULTED,
        YIELDED,
    };
    static const char * StatusToString(enum Status status) {
        static const char * strs[] = {
            "STARTING",
            "RUNNING",
            "SLEEPING",
            "STOPPED",
            "FAULTED",
            "YIELDED",
        };
        return strs[status];
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
    enum OperationYieldType {
        SIMPLE,
        INTERRUPTIBLE,
        STACKLESS,
    };
    static void operationYield(Events::BaseEvent * evt = NULL, enum OperationYieldType yieldType = SIMPLE) throw (GeneralException);
    TaskMan * getTaskMan() const { return m_taskMan; }
    struct ev_loop * getLoop();
    bool isStackless() { return m_stackless; }
  protected:
    void yield(bool stillRunning = false) throw (GeneralException);
    void yield(Events::BaseEvent * evt) {
        waitFor(evt);
        yield();
    }
    virtual void Do() = 0;
    void waitFor(Events::BaseEvent * event);
    bool setOkayToEAgain(bool enable) {
        if (m_stackless) {
            AAssert(enable, "You can't make a task go not-okay-to-eagain if it's stackless.");
        }
        bool oldValue = m_okayToEAgain;
        m_okayToEAgain = enable;
        return oldValue;
    }
    void setStackless() {
        AAssert(!m_stackless, "Can't set a task to be stackless twice");
        AAssert(m_status == STARTING, "Can't set a task to be stackless after it started. status = %s", StatusToString(m_status));
        m_stackless = true;
        m_okayToEAgain = true;
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
    bool m_okayToEAgain, m_stackless;
};

class QueueBase {
  public:
    bool isEmpty() { ScopeLock sl(m_lock); return !m_front; }
  protected:
      QueueBase() { pthread_cond_init(&m_cond, NULL); }
      ~QueueBase() { while (!isEmpty()) iPop(NULL, false); pthread_cond_destroy(&m_cond); }
    void iPush(void * t, Events::Async * event);
    void * iPop(Events::Async * event, bool wait);

  private:
      QueueBase(const QueueBase &) = delete;
    QueueBase & operator=(const QueueBase &) = delete;
    Lock m_lock;
    struct Cell {
          Cell(void * elem) : m_elem(elem) { }
          Cell(const Cell &) = delete;
        Cell & operator=(const Cell &) = delete;
        Cell * m_next = NULL, * m_prev = NULL;
        void * m_elem;
    };
    Cell * m_front = NULL, * m_back = NULL;
    pthread_cond_t m_cond;
};

template<class T>
class Queue : public QueueBase {
  public:
    void push(T * t) { iPush(t, NULL); }
    T * pop() { return (T *) iPop(NULL, true); }
};

template<class T>
class TQueue : public QueueBase {
  public:
    void push(T * t) { iPush(t, &m_event); }
    T * pop() { return (T *) iPop(&m_event, true); }
  private:
    Events::Async m_event;
};

template<class T>
class CQueue : public QueueBase {
  public:
    void push(T * t) { iPush(t, NULL); }
    T * pop() { return (T *) iPop(NULL, false); }
};

};
