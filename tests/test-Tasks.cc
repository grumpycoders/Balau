#include <Main.h>
#include <Task.h>
#include <TaskMan.h>

BALAU_STARTUP;

using namespace Balau;

class CustomPrinter : public Printer {
};

static CustomPrinter * customPrinter = NULL;

class MainTask : public Task {
  public:
    virtual const char * getName() { return "MainTask"; }
  private:
    virtual void Do() {
        customPrinter->setLocal();
        Printer::enable(M_ALL);
        Printer::log(M_DEBUG, "In MainTask::Do()");
    }
};

int Application::startup() throw (Balau::GeneralException) {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");
    Task * mainTask = new MainTask();
    TaskMan::getTaskMan()->mainLoop();
    Printer::log(M_STATUS, "Test::Tasks passed.");
    Printer::log(M_DEBUG, "You shouldn't see that message.");
    return 0;
}
