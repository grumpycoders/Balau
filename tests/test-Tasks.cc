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
        Printer::log(M_STATUS, "xyz");
        customPrinter->setLocal();
        Printer::enable(M_ALL);
        Printer::log(M_DEBUG, "In TestTask::Do()");
    }
};

void MainTask::Do() {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");
    Task * testTask = new TestTask();
    Events::TaskEvent e(testTask);
    waitFor(&e);
    suspend();
    Printer::log(M_STATUS, "Test::Tasks passed.");
    Printer::log(M_DEBUG, "You shouldn't see that message.");
}
