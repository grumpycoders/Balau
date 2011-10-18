#include "Task.h"
#include "TaskMan.h"
#include "Exceptions.h"
#include "Printer.h"
#include "Local.h"

static Balau::LocalTmpl<Balau::Task> localTask;

Balau::Task::Task() {
    size_t size = stackSize();
    m_stack = malloc(size);
    coro_create(&m_ctx, coroutine, this, m_stack, size);

    m_taskMan = TaskMan::getTaskMan();
    m_taskMan->registerTask(this);

    m_tls = g_tlsManager->createTLS();
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    localTask.set(this);
    g_tlsManager->setTLS(oldTLS);

    m_status = STARTING;
    m_okayToEAgain = false;

    Printer::elog(E_TASK, "Created a Task at %p");
}

Balau::Task::~Task() {
    free(m_stack);
    free(m_tls);
}

void Balau::Task::coroutine(void * arg) {
    Task * task = reinterpret_cast<Task *>(arg);
    Assert(task);
    Assert(task->m_status == STARTING);
    try {
        task->m_status = RUNNING;
        task->Do();
        task->m_status = STOPPED;
    }
    catch (GeneralException & e) {
        Printer::log(M_WARNING, "Task %s at %p caused an exception: `%s' - stopping.", task->getName(), task, e.getMsg());
        task->m_status = FAULTED;
    }
    catch (...) {
        Printer::log(M_WARNING, "Task %s at %p caused an unknown exception - stopping.", task->getName(), task);
        task->m_status = FAULTED;
    }
    coro_transfer(&task->m_ctx, &task->m_taskMan->m_returnContext);
}

void Balau::Task::switchTo() {
    Printer::elog(E_TASK, "Switching to task %p - %s", this, getName());
    Assert(m_status == IDLE || m_status == STARTING);
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    coro_transfer(&m_taskMan->m_returnContext, &m_ctx);
    g_tlsManager->setTLS(oldTLS);
    if (m_status == RUNNING)
        m_status = IDLE;
}

void Balau::Task::yield() {
    Printer::elog(E_TASK, "Task %p - %s yielding", this, getName());
    coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
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

Balau::Events::TaskEvent::TaskEvent(Task * taskWaited) : m_taskWaited(taskWaited), m_ack(false) {
    m_taskWaited->m_waitedBy.push_back(this);
}

Balau::Events::TaskEvent::~TaskEvent() {
    if (!m_ack)
        ack();
}

void Balau::Events::TaskEvent::ack() {
    Assert(!m_ack);
    bool deleted = false;
    Task * t = m_taskWaited;
    Task::waitedByList_t::iterator i;
    for (i = t->m_waitedBy.begin(); i != t->m_waitedBy.end(); i++) {
        if (*i == this) {
            t->m_waitedBy.erase(i);
            deleted = true;
            break;
        }
    }
    Printer::elog(E_TASK, "TaskEvent at %p being ack; removing from the 'waited by' list of %p (%s - %s); deleted = %s", this, t, t->getName(), ClassName(t).c_str(), deleted ? "true" : "false");
    Assert(deleted);
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
