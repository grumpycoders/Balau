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

Balau::Events::TaskEvent::TaskEvent(Task * taskWaited) : m_taskWaited(taskWaited) {
    m_taskWaited->m_waitedBy.push_back(this);
}

Balau::Events::Timeout::Timeout(ev_tstamp tstamp) {
    set(tstamp);
}

void Balau::Events::Timeout::set(ev_tstamp tstamp) {
    m_evt.set<Timeout, &Timeout::evt_cb>(this);
    m_evt.set(tstamp);
}

void Balau::Events::Timeout::gotOwner(Task * task) {
    m_evt.set(task->getLoop());
    m_evt.start();
}

void Balau::Events::Timeout::evt_cb(ev::timer & w, int revents) {
    doSignal();
}

void Balau::Events::Custom::gotOwner(Task * task) {
    m_loop = task->getLoop();
}
