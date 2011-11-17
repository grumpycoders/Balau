#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

class Stopper : public Balau::Task {
    virtual void Do();
    virtual const char * getName();
};

void Stopper::Do() {
    getMyTaskMan()->stopMe();
}

const char * Stopper::getName() {
    return "Stopper";
}

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

namespace Balau {

class TaskScheduler : public Thread, public AtStart, public AtExit {
  public:
      TaskScheduler() : AtStart(100), m_stopping(false) { }
    void registerTask(Task * t);
    virtual void * proc();
    virtual void doStart();
    virtual void doExit();
    void registerTaskMan(TaskMan * t);
    void unregisterTaskMan(TaskMan * t);
    void stopAll();
  private:
    Queue<Task *> m_queue;
    volatile bool m_stopping;
};

};

static Balau::TaskScheduler s_scheduler;

void Balau::TaskScheduler::registerTask(Task * t) {
    Printer::elog(E_TASK, "TaskScheduler::registerTask with t = %p", t);
    m_queue.push(t);
}

void Balau::TaskScheduler::registerTaskMan(TaskMan * t) {
    // meh. We need a round-robin queue system.
}

void Balau::TaskScheduler::unregisterTaskMan(TaskMan * t) {
    // and here, we need to remove that taskman from the round robin queue.
}

void Balau::TaskScheduler::stopAll() {
    m_stopping = true;
    // and finally, we need to crawl the whole list and stop all of them.
    TaskMan * tm = localTaskMan.getGlobal();
    tm->addToPending(new Stopper());
}

void * Balau::TaskScheduler::proc() {
    while (true) {
        Printer::elog(E_TASK, "TaskScheduler waiting for a task to pop");
        Task * t = m_queue.pop();
        if (!t)
            break;
        if (dynamic_cast<Stopper *>(t) || m_stopping)
            break;
        // pick up a task manager here... for now let's take the global one.
        // but we need some sort of round robin across all of the threads, as described above.
        TaskMan * tm = localTaskMan.getGlobal();
        Printer::elog(E_TASK, "TaskScheduler popped task %s at %p; adding to TaskMan %p", t->getName(), t, tm);
        tm->addToPending(t);
        tm->m_evt.send();
    }
    Printer::elog(E_TASK, "TaskScheduler stopping.");
    return NULL;
}

void Balau::TaskScheduler::doStart() {
    threadStart();
}

void Balau::TaskScheduler::doExit() {
    Task * s = NULL;
    m_queue.push(s);
    join();
}

void asyncDummy(ev::async & w, int revents) { }

Balau::TaskMan::TaskMan() : m_stopped(false), m_allowedToSignal(false) {
#ifndef _WIN32
    coro_create(&m_returnContext, 0, 0, 0, 0);
#else
    m_fiber = ConvertThreadToFiber(NULL);
    Assert(m_fiber);
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
}

#ifdef _WIN32
class WinSocketStartup : public Balau::AtStart {
  public:
      WinSocketStartup() : AtStart(5) { }
    virtual void doStart() {
        WSADATA wsaData;
        int r = WSAStartup(MAKEWORD(2, 0), &wsaData);
        Assert(r == 0);
    }
};

static WinSocketStartup wsa;
#endif

Balau::TaskMan * Balau::TaskMan::getDefaultTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    Assert(localTaskMan.getGlobal() != this);
    s_scheduler.unregisterTaskMan(this);
    // probably way more work to do here in order to clean up tasks from that thread
    ev_loop_destroy(m_loop);
}

void Balau::TaskMan::mainLoop() {
    // We need at least one round before bailing :)
    do {
        taskHash_t::iterator iH;
        Task * t;
        bool noWait = false;

        Printer::elog(E_TASK, "TaskMan::mainLoop() with m_tasks.size = %i", m_tasks.size());

        // checking "STARTING" tasks, and running them once; also try to build the status of the noWait boolean.
        for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
            t = *iH;
            if (t->getStatus() == Task::STARTING)
                t->switchTo();
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED))
                noWait = true;
        }

        // probably means we have pending tasks; or none at all, for some reason. Don't wait on it forever.
        if (m_tasks.size() == 0)
            noWait = true;

        if (m_pendingAdd.size() != 0)
            noWait = true;

        // libev's event "loop". We always runs it once though.
        m_allowedToSignal = true;
        Printer::elog(E_TASK, "Going to libev main loop");
        ev_run(m_loop, noWait || m_stopped ? EVRUN_NOWAIT : EVRUN_ONCE);
        Printer::elog(E_TASK, "Getting out of libev main loop");

        // let's check what task got stopped, and signal them
        for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
            t = *iH;
            if (((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) &&
                 (t->m_waitedBy.size() != 0)) {
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
            Printer::elog(E_TASK, "Switching to task %p (%s - %s) that got signaled somehow.", t, t->getName(), ClassName(t).c_str());
            Assert(t->getStatus() == Task::IDLE);
            t->switchTo();
        }
        m_signaledTasks.clear();

        // Adding tasks that were added, maybe from other threads
        while (((m_pendingAdd.size() != 0) || (m_tasks.size() == 0)) && !m_stopped) {
            t = m_pendingAdd.pop();
            Assert(m_tasks.find(t) == m_tasks.end());
            t->setup(this);
            m_tasks.insert(t);
        }

        // Finally, let's destroy tasks that no longer are necessary.
        bool didDelete;
        do {
            didDelete = false;
            for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
                t = *iH;
                if (((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) &&
                     (t->m_waitedBy.size() == 0)) {
                    delete t;
                    m_tasks.erase(iH);
                    didDelete = true;
                    break;
                }
            }
        } while (didDelete);

    } while (!m_stopped);
    Printer::elog(E_TASK, "TaskManager stopping.");
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
    Assert(m_tasks.find(t) != m_tasks.end());
    Assert(m_allowedToSignal);
    m_signaledTasks.insert(t);
}

void Balau::TaskMan::stop() {
    s_scheduler.stopAll();
}
