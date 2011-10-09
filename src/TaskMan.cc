#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

Balau::TaskMan::TaskMan() : m_stopped(false) {
    coro_create(&m_returnContext, 0, 0, 0, 0);
    if (!localTaskMan.getGlobal())
        localTaskMan.setGlobal(this);
}

Balau::TaskMan * Balau::TaskMan::getTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    Assert(localTaskMan.getGlobal() != this);
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
            if (t->getStatus() == Task::STARTING) {
                t->switchTo();
            }
        }

        // That's probably where we poll for events

        // lock pending
        // Adding tasks that were added, maybe from other threads
        for (iL = m_pendingAdd.begin(); iL != m_pendingAdd.end(); iL++) {
            t = *iL;
            Assert(m_tasks.find(t) == m_tasks.end());
            m_tasks.insert(t);
        }
        m_pendingAdd.clear();
        // unlock pending

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
    // lock pending
    m_pendingAdd.push_back(t);
    // unlock pending
}
