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

void MainTask::Do() {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");

    Task * testTask = new TestTask();
    Events::TaskEvent taskEvt(testTask);
    waitFor(&taskEvt);
    Assert(!taskEvt.gotSignal());
    suspend();
    Assert(taskEvt.gotSignal());

    Events::Timeout timeout(0.1);
    waitFor(&timeout);
    Assert(!timeout.gotSignal());
    suspend();
    Assert(timeout.gotSignal());

    Printer::log(M_STATUS, "Test::Tasks passed.");
    Printer::log(M_DEBUG, "You shouldn't see that message.");
}
