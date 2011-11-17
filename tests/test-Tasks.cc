#include <Main.h>
#include <Task.h>
#include <TaskMan.h>

BALAU_STARTUP;

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
    Assert(timeout.gotSignal());
}

void MainTask::Do() {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");

    Task * testTask = Balau::createTask(new TestTask());
    Events::TaskEvent taskEvt(testTask);
    waitFor(&taskEvt);
    Assert(!taskEvt.gotSignal());
    yield();
    Assert(taskEvt.gotSignal());
    taskEvt.ack();

    Events::Timeout timeout(0.1);
    waitFor(&timeout);
    Assert(!timeout.gotSignal());
    yield();
    Assert(timeout.gotSignal());

    timeout.set(0.1);
    timeout.reset();
    waitFor(&timeout);
    yieldingFunction();
    Assert(timeout.gotSignal());

    Printer::log(M_STATUS, "Test::Tasks passed.");
    Printer::log(M_DEBUG, "You shouldn't see that message.");
}
