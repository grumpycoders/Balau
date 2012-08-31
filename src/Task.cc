#include "Task.h"
#include "TaskMan.h"
#include "Exceptions.h"
#include "Printer.h"
#include "Local.h"

static Balau::LocalTmpl<Balau::Task> localTask;

Balau::Task::Task() {
    m_status = STARTING;
    m_okayToEAgain = false;

    Printer::elog(E_TASK, "Created a Task at %p", this);
}

bool Balau::Task::needsStacks() {
#ifndef _WIN32
    return true;
#else
    return false;
#endif
}

void Balau::Task::setup(TaskMan * taskMan, void * stack) {
    size_t size = stackSize();
#ifndef _WIN32
    IAssert(stack, "Can't setup a coroutine without a stack");
    m_stack = stack;
    coro_create(&m_ctx, coroutineTrampoline, this, m_stack, size);
#else
    Assert(!stack, "We shouldn't allocate stacks with Fibers");
    m_stack = NULL;
    m_fiber = CreateFiber(size, coroutineTrampoline, this);
#endif

    m_taskMan = taskMan;

    m_tls = Local::createTLS();
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    localTask.set(this);
    g_tlsManager->setTLS(oldTLS);
}

Balau::Task::~Task() {
    free(m_tls);
}

void Balau::Task::coroutineTrampoline(void * arg) {
    Task * task = reinterpret_cast<Task *>(arg);
    IAssert(task, "We didn't get a task to trampoline from... ?");
    task->coroutine();
}

void Balau::Task::coroutine() {
    try {
        IAssert(m_status == STARTING, "The Task at %p was badly initialized ? m_status = %i", this, m_status);
        m_status = RUNNING;
        Do();
        m_status = STOPPED;
    }
    catch (Exit & e) {
        m_status = STOPPED;
        TaskMan::stop(e.getCode());
    }
    catch (TestException & e) {
        m_status = STOPPED;
        Printer::log(M_ERROR, "Unit test failed: %s", e.getMsg());
        TaskMan::stop(-1);
    }
    catch (RessourceException & e) {
        m_status = STOPPED;
        Printer::log(M_ERROR, "Got a ressource exhaustion problem: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        TaskMan::stop(-1);
    }
    catch (GeneralException & e) {
        Printer::log(M_WARNING, "Task %s at %p caused an exception: `%s' - stopping.", getName(), this, e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_WARNING, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
        m_status = FAULTED;
    }
    catch (...) {
        Printer::log(M_WARNING, "Task %s at %p caused an unknown exception - stopping.", getName(), this);
        m_status = FAULTED;
    }
#ifndef _WIN32
    coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
#else
    SwitchToFiber(m_taskMan->m_fiber);
#endif
}

void Balau::Task::switchTo() {
    Printer::elog(E_TASK, "Switching to task %p - %s", this, getName());
    IAssert(m_status == YIELDED || m_status == IDLE || m_status == STARTING, "The task at %p isn't either yielded, idle or starting... ? m_status = %i", this, m_status);
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    if (m_status == YIELDED || m_status == IDLE)
        m_status = RUNNING;
#ifndef _WIN32
    coro_transfer(&m_taskMan->m_returnContext, &m_ctx);
#else
    SwitchToFiber(m_fiber);
#endif
    g_tlsManager->setTLS(oldTLS);
    if (m_status == RUNNING)
        m_status = IDLE;
}

void Balau::Task::yield(bool changeStatus) {
    Printer::elog(E_TASK, "Task %p - %s yielding", this, getName());
    if (changeStatus)
        m_status = YIELDED;
#ifndef _WIN32
    coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
#else
    SwitchToFiber(m_taskMan->m_fiber);
#endif
}

Balau::Task * Balau::Task::getCurrentTask() {
    return localTask.get();
}

void Balau::Task::waitFor(Balau::Events::BaseEvent * e) {
    Printer::elog(E_TASK, "Task %p - %s waits for event %p (%s)", this, getName(), e, ClassName(e).c_str());
    e->registerOwner(this);
}

struct ev_loop * Balau::Task::getLoop() {
    return getTaskMan()->getLoop();
}

void Balau::Events::BaseEvent::doSignal() {
    Printer::elog(E_TASK, "Event at %p (%s) is signaled with cb %p and task %p", this, ClassName(this).c_str(), m_cb, m_task);
    m_signal = true;
    if (m_cb)
        m_cb->gotEvent(this);
    if (m_task) {
        Printer::elog(E_TASK, "Signaling task %p (%s - %s)", m_task, m_task->getName(), ClassName(m_task).c_str());
        m_task->getTaskMan()->signalTask(m_task);
    }
}

Balau::Events::TaskEvent::TaskEvent(Task * taskWaited) : m_taskWaited(taskWaited), m_ack(false), m_distant(false) {
    ScopeLock lock(m_taskWaited->m_eventLock);
    m_taskWaited->m_waitedBy.push_back(this);
}

void Balau::Events::TaskEvent::signal() {
    if (m_distant)
        m_evt.send();
    doSignal();
}

void Balau::Events::TaskEvent::gotOwner(Task * task) {
    TaskMan * tm = task->getTaskMan();

    m_evt.stop();
    if (tm != m_taskWaited->getTaskMan()) {
        m_evt.set(tm->getLoop());
        m_evt.set<TaskEvent, &TaskEvent::evt_cb>(this);
        m_evt.start();
        m_distant = true;
    } else {
        m_distant = false;
    }
}

Balau::Events::TaskEvent::~TaskEvent() {
    if (!m_ack)
        ack();
    if (m_distant)
        m_evt.stop();
}

void Balau::Events::TaskEvent::ack() {
    AAssert(!m_ack, "You can't ack() a task event twice.");
    bool deleted = false;
    Task * t = m_taskWaited;
    t->m_eventLock.enter();
    for (auto i = t->m_waitedBy.begin(); i != t->m_waitedBy.end(); i++) {
        if (*i == this) {
            t->m_waitedBy.erase(i);
            deleted = true;
            break;
        }
    }
    t->m_eventLock.leave();
    Printer::elog(E_TASK, "TaskEvent at %p being ack; removing from the 'waited by' list of %p (%s - %s); deleted = %s", this, t, t->getName(), ClassName(t).c_str(), deleted ? "true" : "false");
    IAssert(deleted, "We didn't find task %p in the waitedBy lists... ?", this);
    m_ack = true;
    reset();
}

void Balau::Events::Timeout::gotOwner(Task * task) {
    m_evt.set(task->getLoop());
    m_evt.start();
}

void Balau::Events::Async::gotOwner(Task * task) {
    m_evt.set(task->getLoop());
    m_evt.start();
}

void Balau::Events::Custom::gotOwner(Task * task) {
    m_loop = task->getLoop();
}

void Balau::Task::yield(Events::BaseEvent * evt, bool interruptible) throw (GeneralException) {
    Task * t = getCurrentTask();
    if (evt)
        t->waitFor(evt);
    bool gotSignal;

    do {
        t->yield(evt == NULL);
        Printer::elog(E_TASK, "operation back from yielding; interruptible = %s; okayToEAgain = %s", interruptible ? "true" : "false", t->m_okayToEAgain ? "true" : "false");
        gotSignal = evt ? evt->gotSignal() : true;
    } while ((!interruptible || !t->m_okayToEAgain) && !gotSignal);

    if (!evt)
        return;

    if (interruptible && t->m_okayToEAgain && !evt->gotSignal()) {
        Printer::elog(E_TASK, "operation is throwing an exception.");
        throw EAgain(evt);
    }
}
