#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

Balau::TaskMan::TaskMan() : m_stopped(false) {
    coro_create(&m_returnContext, 0, 0, 0, 0);
    if (!localTaskMan.getGlobal()) {
        localTaskMan.setGlobal(this);
        m_loop = ev_default_loop(EVFLAG_AUTO);
    } else {
        m_loop = ev_loop_new(EVFLAG_AUTO);
    }
}

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

        // checking "STARTING" tasks, and running them once
        for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
            t = *iH;
            if (t->getStatus() == Task::STARTING)
                t->switchTo();
        }

        ev_run(m_loop, EVRUN_ONCE);

        // let's check who got signaled, and call them
        for (iH = m_signaledTasks.begin(); iH != m_signaledTasks.end(); iH++) {
            t = *iH;
            t->switchTo();
        }

        m_pendingLock.enter();
        // Adding tasks that were added, maybe from other threads
        for (iL = m_pendingAdd.begin(); iL != m_pendingAdd.end(); iL++) {
            t = *iL;
            Assert(m_tasks.find(t) == m_tasks.end());
            m_tasks.insert(t);
        }
        m_pendingAdd.clear();
        m_pendingLock.leave();

        // Dealing with stopped and faulted tasks.
        // First by signalling the waiters.
        for (iH = m_tasks.begin(); iH != m_tasks.end(); iH++) {
            t = *iH;
            if (((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) &&
                 (t->m_waitedBy.size() != 0)) {
                Task::waitedByList_t::iterator i;
                while ((i = t->m_waitedBy.begin()) != t->m_waitedBy.end()) {
                    Events::TaskEvent * e = *i;
                    e->doSignal();
                    e->taskWaiting()->switchTo();
                    t->m_waitedBy.erase(i);
                }
            }
        }
        m_signaledTasks.clear();

        // Then, by destroying them.
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
    m_signaledTasks.insert(t);
}
