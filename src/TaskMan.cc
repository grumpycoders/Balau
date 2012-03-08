#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

class Stopper : public Balau::Task {
  public:
      Stopper(int code) : m_code(code) { }
  private:
    virtual void Do();
    virtual const char * getName();
    int m_code;
};

void Stopper::Do() {
    getMyTaskMan()->stopMe(m_code);
}

const char * Stopper::getName() {
    return "Stopper";
}

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

static const int TOO_MANY_STACKS = 1024;

namespace Balau {

class TaskScheduler : public GlobalThread {
  public:
      TaskScheduler() : GlobalThread(100), m_stopping(false) { }
    void registerTask(Task * t);
    virtual void * proc();
    virtual void threadExit();
    void registerTaskMan(TaskMan * t);
    void unregisterTaskMan(TaskMan * t);
    void stopAll(int code);
  private:
    Queue<Task> m_queue;
    std::queue<TaskMan *> m_taskManagers;
    Lock m_lock;
    volatile bool m_stopping;
};

};

static Balau::TaskScheduler s_scheduler;

void Balau::TaskScheduler::registerTask(Task * t) {
    Printer::elog(E_TASK, "TaskScheduler::registerTask with t = %p", t);
    m_queue.push(t);
}

void Balau::TaskScheduler::registerTaskMan(TaskMan * t) {
    ScopeLock sl(m_lock);
    m_taskManagers.push(t);
}

void Balau::TaskScheduler::unregisterTaskMan(TaskMan * t) {
    ScopeLock sl(m_lock);
    TaskMan * p = NULL;
    // yes, this is a potentially dangerous operation.
    // But unregistering task managers shouldn't happen that often.
    while (true) {
        p = m_taskManagers.front();
        m_taskManagers.pop();
        if (p == t)
            break;
        m_taskManagers.push(p);
    }
}

void Balau::TaskScheduler::stopAll(int code) {
    m_stopping = true;
    ScopeLock sl(m_lock);
    std::queue<TaskMan *> altQueue;
    TaskMan * tm;
    while (!m_taskManagers.empty()) {
        tm = m_taskManagers.front();
        m_taskManagers.pop();
        altQueue.push(tm);
        tm->addToPending(new Stopper(code));
        tm->m_evt.send();
    }
    while (!altQueue.empty()) {
        tm = altQueue.front();
        altQueue.pop();
        m_taskManagers.push(tm);
    }
}

void * Balau::TaskScheduler::proc() {
    while (true) {
        Printer::elog(E_TASK, "TaskScheduler waiting for a task to pop");
        Task * t = m_queue.pop();
        if (!t)
            break;
        if (dynamic_cast<Stopper *>(t) || m_stopping)
            break;
        m_lock.enter();
        size_t s = m_taskManagers.size();
        if (s == 0)
            break;
        TaskMan * tm = m_taskManagers.front();
        if (s != 1) {
            m_taskManagers.pop();
            m_taskManagers.push(tm);
        }
        m_lock.leave();
        Printer::elog(E_TASK, "TaskScheduler popped task %s at %p; adding to TaskMan %p", t->getName(), t, tm);
        tm->addToPending(t);
        tm->m_evt.send();
    }
    Printer::elog(E_TASK, "TaskScheduler stopping.");
    return NULL;
}

void Balau::TaskScheduler::threadExit() {
    Task * s = NULL;
    m_queue.push(s);
}

void asyncDummy(ev::async & w, int revents) {
    Balau::Printer::elog(Balau::E_TASK, "TaskMan is getting woken up...");
}

Balau::TaskMan::TaskMan() : m_stopped(false), m_allowedToSignal(false), m_stopCode(0) {
#ifndef _WIN32
    coro_create(&m_returnContext, 0, 0, 0, 0);
#else
    m_fiber = ConvertThreadToFiber(NULL);
    RAssert(m_fiber, "ConvertThreadToFiber returned NULL");
#endif
    TaskMan * global = localTaskMan.getGlobal();
    if (!global) {
        localTaskMan.setGlobal(this);
        m_loop = ev_default_loop(EVFLAG_AUTO);
    } else {
        m_loop = ev_loop_new(EVFLAG_AUTO);
    }
    m_evt.set(m_loop);
    m_evt.set<asyncDummy>();
    m_evt.start();
    s_scheduler.registerTaskMan(this);

    m_nStacks = 0;
}

#ifdef _WIN32
class WinSocketStartup : public Balau::AtStart {
  public:
      WinSocketStartup() : AtStart(5) { }
    virtual void doStart() {
        WSADATA wsaData;
        int r = WSAStartup(MAKEWORD(2, 0), &wsaData);
        RAssert(r == 0, "WSAStartup returned %i", r);
    }
};

static WinSocketStartup wsa;
#endif

Balau::TaskMan * Balau::TaskMan::getDefaultTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    AAssert(localTaskMan.getGlobal() != this, "Don't create / delete a TaskMan directly");
    while (m_stacks.size() != 0) {
        free(m_stacks.front());
        m_stacks.pop();
    }
    s_scheduler.unregisterTaskMan(this);
    // probably way more work to do here in order to clean up tasks from that thread
    ev_loop_destroy(m_loop);
}

void * Balau::TaskMan::getStack() {
    void * r = NULL;
    if (m_nStacks == 0) {
        if (Task::needsStacks())
            r = malloc(Task::stackSize());
    } else {
        r = m_stacks.front();
        m_stacks.pop();
        m_nStacks--;
    }
    return r;
}

void Balau::TaskMan::freeStack(void * stack) {
    if (!stack)
        return;
    if (m_nStacks >= TOO_MANY_STACKS) {
        free(stack);
    } else {
        m_stacks.push(stack);
        m_nStacks++;
    }
}

int Balau::TaskMan::mainLoop() {
    taskHash_t starting, stopped, yielded, yielded2;
    taskHash_t::iterator iH;
    Task * t;

    // we start by pushing all of the 'STARTING' tasks into the appropriate queue.
    for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++)
        if (t->getStatus() == Task::STARTING)
            starting.insert(*iH);

    do {
        bool noWait = false;

        Printer::elog(E_TASK, "TaskMan::mainLoop() at %p with m_tasks.size = %li", this, m_tasks.size());

        // checking "STARTING" tasks, and running them once; also try to build the status of the noWait boolean.
        while ((iH = starting.begin()) != starting.end()) {
            t = *iH;
            IAssert(t->getStatus() == Task::STARTING, "Got task at %p in the starting list, but isn't starting.", t);
            t->switchTo();
            IAssert(t->getStatus() != Task::STARTING, "Task at %p got switchedTo, but still is 'STARTING'.", t);
            starting.erase(iH);
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                noWait = true;
                stopped.insert(t);
            }
            if (t->getStatus() == Task::YIELDED) {
                noWait = true;
                yielded.insert(t);
            }
        }

        // if we begin that loop with any pending task, just don't loop, so we can add them immediately.
        if (!m_pendingAdd.isEmpty() || !yielded.empty())
            noWait = true;

        // libev's event "loop". We always runs it once though.
        m_allowedToSignal = true;
        Printer::elog(E_TASK, "TaskMan at %p Going to libev main loop", this);
        ev_run(m_loop, noWait || m_stopped ? EVRUN_NOWAIT : EVRUN_ONCE);
        Printer::elog(E_TASK, "TaskMan at %p Getting out of libev main loop", this);

        // let's check what task got stopped, and signal them
        for (iH = stopped.begin(); iH != stopped.end(); iH++) {
            t = *iH;
            IAssert((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED), "Task %p in stopped list but isn't stopped.", t);
            if (t->m_waitedBy.size() != 0) {
                Task::waitedByList_t::iterator i;
                for (i = t->m_waitedBy.begin(); i != t->m_waitedBy.end(); i++) {
                    Events::TaskEvent * e = *i;
                    e->signal();
                }
            }
        }
        m_allowedToSignal = false;

        // let's check who got signaled, and call them
        for (iH = m_signaledTasks.begin(); iH != m_signaledTasks.end(); iH++) {
            t = *iH;
            Printer::elog(E_TASK, "TaskMan at %p Switching to task %p (%s - %s) that got signaled somehow.", this, t, t->getName(), ClassName(t).c_str());
            IAssert(t->getStatus() == Task::IDLE || t->getStatus() == Task::YIELDED, "We're switching to a non-idle/yielded task at %p... ? status = %i", t, t->getStatus());
            bool wasYielded = t->getStatus() == Task::YIELDED;
            t->switchTo();
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                stopped.insert(t);
                if (wasYielded) {
                    taskHash_t::iterator i = yielded.find(t);
                    IAssert(i != yielded.end(), "Task at %p was yielded, but not in yielded list... ?", t);
                    yielded.erase(i);
                }
            } else if (t->getStatus() == Task::YIELDED) {
                yielded.insert(t);
            }
        }
        m_signaledTasks.clear();

        // now let's make a round of yielded tasks
        for (iH = yielded.begin(); iH != yielded.end(); iH++) {
            t = *iH;
            Printer::elog(E_TASK, "TaskMan at %p Switching to task %p (%s - %s) that was yielded.", this, t, t->getName(), ClassName(t).c_str());
            IAssert(t->getStatus() == Task::YIELDED, "Task %p was in yielded list, but wasn't yielded ?", t);
            t->switchTo();
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                stopped.insert(t);
            } else if (t->getStatus() == Task::YIELDED) {
                yielded2.insert(t);
            }
        }
        yielded = yielded2;
        yielded2.clear();

        // Adding tasks that were added, maybe from other threads
        while (!m_pendingAdd.isEmpty()) {
            Printer::elog(E_TASK, "TaskMan at %p trying to pop a task...", this);
            t = m_pendingAdd.pop();
            Printer::elog(E_TASK, "TaskMan at %p popped task %p...", this, t);
            IAssert(m_tasks.find(t) == m_tasks.end(), "TaskMan got task %p twice... ?", t);
            ev_now_update(m_loop);
            t->setup(this, getStack());
            m_tasks.insert(t);
            starting.insert(t);
        }

        // Finally, let's destroy tasks that no longer are necessary.
        bool didDelete;
        do {
            didDelete = false;
            for (iH = stopped.begin(); iH != stopped.end(); iH++) {
                t = *iH;
                IAssert((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED), "Task %p in stopped list but isn't stopped.", t);
                if (t->m_waitedBy.size() == 0) {
                    freeStack(t->m_stack);
                    stopped.erase(iH);
                    iH = m_tasks.find(t);
                    IAssert(iH != m_tasks.end(), "Task %p in stopped list but not in m_tasks...", t);
                    m_tasks.erase(iH);
                    IAssert(yielded.find(t) == yielded.end(), "Task %p is deleted but is in yielded list... ?", t);
                    delete t;
                    didDelete = true;
                    break;
                }
            }
        } while (didDelete);

    } while (!m_stopped);
    Printer::elog(E_TASK, "TaskManager at %p stopping.", this);
    return m_stopCode;
}

void Balau::TaskMan::registerTask(Balau::Task * t, Balau::Task * stick) {
    if (stick) {
        TaskMan * tm = stick->getMyTaskMan();
        tm->addToPending(t);
        tm->m_evt.send();
    } else {
        s_scheduler.registerTask(t);
    }
}

void Balau::TaskMan::addToPending(Balau::Task * t) {
    m_pendingAdd.push(t);
}

void Balau::TaskMan::signalTask(Task * t) {
    AAssert(m_tasks.find(t) != m_tasks.end(), "Can't signal task %p that I don't own (me = %p)", t, this);
    AAssert(m_allowedToSignal, "I'm not allowed to signal (me = %p)", this);
    m_signaledTasks.insert(t);
}

void Balau::TaskMan::stop(int code) {
    s_scheduler.stopAll(code);
}

class ThreadedTaskMan : public Balau::Thread {
    virtual void * proc() {
        m_taskMan = new Balau::TaskMan();
        m_taskMan->mainLoop();
        return NULL;
    }
    Balau::TaskMan * m_taskMan;
};

Balau::Thread * Balau::TaskMan::createThreadedTaskMan() {
    Thread * r = new ThreadedTaskMan();
    r->threadStart();
    return r;
}
