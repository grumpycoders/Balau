#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

Balau::TaskMan::TaskMan() : stopped(false) {
    coro_create(&returnContext, 0, 0, 0, 0);
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
        taskList::iterator i;
        Task * t;

        // lock pending
        // Adding tasks that were added, maybe from other threads
        for (i = pendingAdd.begin(); i != pendingAdd.end(); i++) {
            Assert(tasks.find(*i) == tasks.end());
            tasks.insert(*i);
        }
        pendingAdd.clear();
        // unlock pending

        // checking "STARTING" tasks, and running them once
        for (i = tasks.begin(); i != tasks.end(); i++) {
            t = *i;
            if (t->getStatus() == Task::STARTING) {
                t->switchTo();
            }
        }

        // That's probably where we poll for events

        // checking "STOPPED" tasks, and destroying them
        bool didDelete;
        do {
            didDelete = false;
            for (i = tasks.begin(); i != tasks.end(); i++) {
                t = *i;
                if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                    delete t;
                    tasks.erase(i);
                    didDelete = true;
                    break;
                }
            }
        } while (didDelete);
    } while (!stopped && tasks.size() != 0);
}

void Balau::TaskMan::registerTask(Balau::Task * t) {
    // lock pending
    pendingAdd.insert(t);
    // unlock pending
}
