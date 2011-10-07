#include "Task.h"
#include "TaskMan.h"
#include "Exceptions.h"
#include "Printer.h"
#include "Local.h"

Balau::Task::Task() {
    size_t size = stackSize();
    stack = malloc(size);
    coro_create(&ctx, coroutine, this, stack, size);
    taskMan = TaskMan::getTaskMan();
    tls = tlsManager->createTLS();
    status = STARTING;
}

void Balau::Task::coroutine(void * arg) {
    Task * task = reinterpret_cast<Task *>(arg);
    Assert(task);
    try {
        task->status = RUNNING;
        task->Do();
        task->status = STOPPED;
    }
    catch (GeneralException & e) {
        Printer::log(M_WARNING, "Task %s caused an exception: `%s' - stopping.", task->getName(), e.getMsg());
        task->status = FAULTED;
    }
    catch (...) {
        Printer::log(M_WARNING, "Task %s caused an unknown exception - stopping.", task->getName());
        task->status = FAULTED;
    }
    coro_transfer(&task->ctx, &task->taskMan->returnContext);
}

void Balau::Task::switchTo() {
    status = RUNNING;
    void * oldTLS = tlsManager->getTLS();
    tlsManager->setTLS(tls);
    coro_transfer(&taskMan->returnContext, &ctx);
    tlsManager->setTLS(oldTLS);
    status = IDLE;
}

void Balau::Task::suspend() {
    coro_transfer(&ctx, &taskMan->returnContext);
}
