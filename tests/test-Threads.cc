#include <Main.h>
#include <Threads.h>

using namespace Balau;

volatile bool threadWorked = false;

class TestThread : public Thread {
    virtual void * proc();
};

void * TestThread::proc() {
    Printer::log(M_STATUS, "Into a thread");
    threadWorked = true;
    return NULL;
}

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Threads running.");

    TestThread * t = new TestThread();
    Printer::log(M_STATUS, "Starting thread");
    t->threadStart();
    Printer::log(M_STATUS, "Joining thread");
    t->join();
    Printer::log(M_STATUS, "Deleting thread");
    delete t;

    TAssert(threadWorked);

    Printer::log(M_STATUS, "Test::Threads passed.");
}
