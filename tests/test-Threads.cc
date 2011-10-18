#include <Main.h>
#include <Threads.h>

BALAU_STARTUP;

using namespace Balau;

class TestThread : public Thread {
  private:
    virtual void * proc();
};

void * TestThread::proc() {
    Printer::log(M_STATUS, "Into a thread");
}

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Threads running.");

    TestThread * t = new TestThread();
    Printer::log(M_STATUS, "Starting thread");
    t->threadStart();
    Printer::log(M_STATUS, "Joining thread");
    t->join();
    delete t;

    Printer::log(M_STATUS, "Test::Threads passed.");
}
