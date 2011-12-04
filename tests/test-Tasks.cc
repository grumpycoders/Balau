#include <Main.h>
#include <Task.h>
#include <TaskMan.h>

using namespace Balau;

class CustomPrinter : public Printer {
};

static CustomPrinter * customPrinter = NULL;

class TestTask : public Task {
  public:
    virtual const char * getName() { return "MainTask"; }
  private:
    virtual void Do() {
        customPrinter->setLocal();
        Printer::enable(M_ALL);
        Printer::log(M_DEBUG, "In TestTask::Do()");
    }
};

static void yieldingFunction() {
    Events::Timeout timeout(0.2);
    Task::yield(&timeout);
    TAssert(timeout.gotSignal());
}

void MainTask::Do() {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");

    Task * testTask = Balau::createTask(new TestTask());
    Events::TaskEvent taskEvt(testTask);
    waitFor(&taskEvt);
    TAssert(!taskEvt.gotSignal());
    yield();
    TAssert(taskEvt.gotSignal());
    taskEvt.ack();

    Events::Timeout timeout(0.1);
    waitFor(&timeout);
    TAssert(!timeout.gotSignal());
    yield();
    TAssert(timeout.gotSignal());

    timeout.set(0.1);
    timeout.reset();
    waitFor(&timeout);
    yieldingFunction();
    TAssert(timeout.gotSignal());

    Printer::log(M_STATUS, "Test::Tasks passed.");
    Printer::log(M_DEBUG, "You shouldn't see that message.");
}
