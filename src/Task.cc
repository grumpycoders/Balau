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
}

Balau::Task::~Task() {
    free(m_stack);
    free(m_tls);
}

void Balau::Task::coroutine(void * arg) {
    Task * task = reinterpret_cast<Task *>(arg);
    Assert(task);
    try {
        task->m_status = RUNNING;
        task->Do();
        task->m_status = STOPPED;
    }
    catch (GeneralException & e) {
        Printer::log(M_WARNING, "Task %s caused an exception: `%s' - stopping.", task->getName(), e.getMsg());
        task->m_status = FAULTED;
    }
    catch (...) {
        Printer::log(M_WARNING, "Task %s caused an unknown exception - stopping.", task->getName());
        task->m_status = FAULTED;
    }
    coro_transfer(&task->m_ctx, &task->m_taskMan->m_returnContext);
}

void Balau::Task::switchTo() {
    m_status = RUNNING;
    void * oldTLS = g_tlsManager->getTLS();
    g_tlsManager->setTLS(m_tls);
    coro_transfer(&m_taskMan->m_returnContext, &m_ctx);
    g_tlsManager->setTLS(oldTLS);
    if (m_status == RUNNING)
        m_status = IDLE;
}

void Balau::Task::yield() {
    coro_transfer(&m_ctx, &m_taskMan->m_returnContext);
}

Balau::Task * Balau::Task::getCurrentTask() {
    return localTask.get();
}

void Balau::Task::waitFor(Balau::Events::BaseEvent * e) {
    e->registerOwner(this);
}

void Balau::Events::BaseEvent::doSignal() {
    m_signal = true;
    m_task->getTaskMan()->signalTask(m_task);
}

Balau::Events::TaskEvent::TaskEvent(Task * taskWaited) : m_taskWaited(taskWaited) {
    m_taskWaited->m_waitedBy.push_back(this);
}

Balau::Events::Timeout::Timeout(ev_tstamp tstamp) {
    m_evt.set<Timeout, &Timeout::evt_cb>(this);
    m_evt.set(tstamp);
}

void Balau::Events::Timeout::gotOwner(Task * task) {
    m_evt.set(task->getTaskMan()->getLoop());
    m_evt.start();
}

void Balau::Events::Timeout::evt_cb(ev::timer & w, int revents) {
    doSignal();
}

void Balau::Events::Custom::gotOwner(Task * task) {
    m_loop = task->getTaskMan()->getLoop();
}
