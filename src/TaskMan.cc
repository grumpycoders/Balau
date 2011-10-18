#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

Balau::TaskMan::TaskMan() : m_stopped(false), m_allowedToSignal(false) {
    coro_create(&m_returnContext, 0, 0, 0, 0);
    if (!localTaskMan.getGlobal()) {
        localTaskMan.setGlobal(this);
        m_loop = ev_default_loop(EVFLAG_AUTO);
    } else {
        m_loop = ev_loop_new(EVFLAG_AUTO);
    }
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

Balau::TaskMan * Balau::TaskMan::getTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    Assert(localTaskMan.getGlobal() != this);
    ev_loop_destroy(m_loop);
}

void Balau::TaskMan::mainLoop() {
    // We need at least one round before bailing :)
    do {
        taskList_t::iterator iL;
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

        m_pendingLock.enter();
        if (m_pendingAdd.size() != 0)
            noWait = true;
        m_pendingLock.leave();

        // libev's event "loop". We always runs it once though.
        m_allowedToSignal = true;
        Printer::elog(E_TASK, "Going to libev main loop");
        ev_run(m_loop, noWait ? EVRUN_NOWAIT : EVRUN_ONCE);
        Printer::elog(E_TASK, "Getting out of libev main loop");

        // let's check what task got stopped, and signal them
        for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
            t = *iH;
            if (((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) &&
                 (t->m_waitedBy.size() != 0)) {
                Task::waitedByList_t::iterator i;
                for (i = t->m_waitedBy.begin(); i != t->m_waitedBy.end(); i++) {
                    Events::TaskEvent * e = *i;
                    e->doSignal();
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

        m_pendingLock.enter();
        // Adding tasks that were added, maybe from other threads
        for (iL = m_pendingAdd.begin(); iL != m_pendingAdd.end(); iL++) {
            t = *iL;
            Assert(m_tasks.find(t) == m_tasks.end());
            m_tasks.insert(t);
        }
        m_pendingAdd.clear();
        m_pendingLock.leave();

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

    } while (!m_stopped && m_tasks.size() != 0);
}

void Balau::TaskMan::registerTask(Balau::Task * t) {
    m_pendingLock.enter();
    m_pendingAdd.push_back(t);
    m_pendingLock.leave();
}

void Balau::TaskMan::signalTask(Task * t) {
    Assert(m_tasks.find(t) != m_tasks.end());
    Assert(m_allowedToSignal);
    m_signaledTasks.insert(t);
}
