#include "Task.h"
#include "TaskMan.h"
#include "Exceptions.h"
#include "Printer.h"
#include "Local.h"

static Balau::LocalTmpl<Balau::Task> localTask;

Balau::Task::Task() : m_status(STARTING), m_okayToEAgain(false), m_stackless(false) {
    Printer::elog(E_TASK, "Created a Task at %p", this);
    m_tls = Local::createTLS(g_tlsManager->getTLS());
}

bool Balau::Task::needsStacks() {
#ifndef _WIN32
    return true;
#else
    return false;
#endif
}

void Balau::Task::setup(TaskMan * taskMan, void * stack) {
    if (m_stackless) {
        IAssert(!stack, "Since we're stackless, no stack should've been allocated.");
        m_stack = NULL;
    } else {
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
    }

    m_taskMan = taskMan;

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
        IAssert((m_status == STARTING) || (m_stackless && (m_status == RUNNING)), "The Task at %p has a bad status ? m_status = %s, stackless = %s", this, StatusToString(m_status), m_stackless ? "true" : "false");
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
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_ERROR, "%s", str.to_charp());
        TaskMan::stop(-1);
    }
    catch (RessourceException & e) {
        m_status = STOPPED;
        Printer::log(M_ERROR, "Got a ressource exhaustion problem: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
        TaskMan::stop(-1);
    }
    catch (TaskSwitch & e) {
        if (!m_stackless) {
            Printer::log(M_ERROR, "Task %s at %p isn't stackless, but still caused a task switch.", getName(), this);
            const char * details = e.getDetails();
            if (details)
                Printer::log(M_ERROR, "  %s", details);
            auto trace = e.getTrace();
            for (String & str : trace)
                Printer::log(M_DEBUG, "%s", str.to_charp());
            m_status = FAULTED;
        } else {
            Printer::elog(E_TASK, "Stackless task %s at %p is task-switching.", getName(), this);
        }
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
    if (!m_stackless) {
#ifndef _WIN32
        coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
#else
        SwitchToFiber(m_taskMan->m_fiber);
#endif
    }
}

void Balau::Task::switchTo() {
    Printer::elog(E_TASK, "Switching to task %p - %s", this, getName());
    IAssert(m_status == YIELDED || m_status == SLEEPING || m_status == STARTING, "The task at %p isn't either yielded, sleeping or starting... ? m_status = %s", this, StatusToString(m_status));
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    if (m_status == YIELDED || m_status == SLEEPING)
        m_status = RUNNING;
    if (m_stackless) {
        coroutine();
    } else {
#ifndef _WIN32
        coro_transfer(&m_taskMan->m_returnContext, &m_ctx);
#else
        SwitchToFiber(m_fiber);
#endif
    }
    g_tlsManager->setTLS(oldTLS);
    IAssert(m_status != RUNNING, "Task %s at %p is still running... ?", getName(), this);
}

bool Balau::Task::yield(bool stillRunning) {
    Printer::elog(E_TASK, "Task %p - %s yielding", this, getName());
    if (stillRunning)
        m_status = YIELDED;
    else
        m_status = SLEEPING;
    if (m_stackless) {
        return true;
    } else {
#ifndef _WIN32
        coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
#else
        SwitchToFiber(m_taskMan->m_fiber);
#endif
    }
    return false;
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

void Balau::Task::sleep(double timeout) {
    AAssert(!m_stackless && !m_okayToEAgain, "You can only call sleep on simple tasks.");
    Events::Timeout evt(timeout);
    waitFor(&evt);
    yield();
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

Balau::Events::TaskEvent::TaskEvent(Task * taskWaited) : m_taskWaited(NULL), m_ack(false), m_distant(false) {
    if (taskWaited)
        attachToTask(taskWaited);
}

void Balau::Events::TaskEvent::attachToTask(Task * taskWaited) {
    AAssert(!m_taskWaited, "You can't attach a TaskEvent twice.");
    m_ack = false;
    m_taskWaited = taskWaited;
    ScopeLock lock(m_taskWaited->m_eventLock);
    m_taskWaited->m_waitedBy.push_back(this);
}

void Balau::Events::TaskEvent::signal() {
    if (m_distant)
        m_evt.send();
    else
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
    m_taskWaited = NULL;
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

void Balau::Task::operationYield(Events::BaseEvent * evt, enum OperationYieldType yieldType) throw (GeneralException) {
    Task * t = getCurrentTask();
    if (evt)
        t->waitFor(evt);

    if (t->m_stackless) {
        AAssert(yieldType != SIMPLE, "You can't run simple operations from a stackless task.");
    }

    if (yieldType == STACKLESS) {
        AAssert(t->m_okayToEAgain, "You can't run a stackless operation from a non-okay-to-eagain task.");
    }

    bool gotSignal, doEAgain = false;

    do {
        doEAgain = t->yield(evt == NULL);
        static const char * YieldTypeToString[] = {
            "SIMPLE",
            "INTERRUPTIBLE",
            "STACKLESS",
        };
        if (doEAgain)
            Printer::elog(E_TASK, "operation couldn't yield; yieldType = %s; okayToEAgain = %s", YieldTypeToString[yieldType], t->m_okayToEAgain ? "true" : "false");
        else
            Printer::elog(E_TASK, "operation back from yielding; yieldType = %s; okayToEAgain = %s", YieldTypeToString[yieldType], t->m_okayToEAgain ? "true" : "false");
        gotSignal = evt ? evt->gotSignal() : true;
    } while (((yieldType == SIMPLE) || !t->m_okayToEAgain) && !gotSignal);

    if (!evt)
        return;

    if ((yieldType != SIMPLE) && t->m_okayToEAgain && !gotSignal && doEAgain) {
        AAssert(!t->m_cannotEAgain, "task at %p in simple context mode can't EAgain", t);
        Printer::elog(E_TASK, "operation is throwing an EAgain exception with event %p", evt);
        throw EAgain(evt);
    }
}

void Balau::QueueBase::iPush(void * t, Events::Async * event) {
    ScopeLock sl(m_lock);
    Cell * c = new Cell(t);
    c->m_prev = m_back;
    if (m_back)
        m_back->m_next = c;
    else
        m_front = c;
    m_back = c;
    if (event)
        event->trigger();
    else
        pthread_cond_signal(&m_cond);
}

void * Balau::QueueBase::iPop(Events::Async * event, bool wait) {
    ScopeLock sl(m_lock);
    while (!m_front && wait) {
        if (event) {
            Task::prepare(event);
            m_lock.leave();
            Task::operationYield(event, Task::INTERRUPTIBLE);
            m_lock.enter();
            if (event->gotSignal())
                event->reset();
        } else {
            pthread_cond_wait(&m_cond, &m_lock.m_lock);
        }
    }
    Cell * c = m_front;
    if (!c)
        return NULL;
    m_front = c->m_next;
    if (m_front)
        m_front->m_prev = NULL;
    else
        m_back = NULL;
    void * t = c->m_elem;
    delete c;
    return t;
}
