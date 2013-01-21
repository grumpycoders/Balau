#include <Main.h>
#include <Task.h>
#include <TaskMan.h>
#include <StacklessTask.h>
#include <Input.h>

using namespace Balau;

class CustomPrinter : public Printer {
};

static CustomPrinter * customPrinter = NULL;

class TestTask : public Task {
  public:
    virtual const char * getName() const { return "TestTask"; }
  private:
    virtual void Do() {
        customPrinter->setLocal();
        Printer::enable(M_ALL);
        Printer::log(M_DEBUG, "In TestTask::Do()");
    }
};

class TestOperation {
  public:
      TestOperation() : m_count(0), m_completed(false) { }
    void Do() {
        if (m_count++ == 0) {
            m_timeout.set(0.2);
            Task::operationYield(&m_timeout, Task::STACKLESS);
        }
        TAssert(m_timeout.gotSignal());
        m_completed = true;
    }
    bool completed() { return m_completed; }
  private:
    int m_count;
    bool m_completed;
    Events::Timeout m_timeout;
};

class TestStackless : public StacklessTask {
  public:
    virtual const char * getName() const { return "TestStackless"; }
  private:
    virtual void Do() {
        ssize_t r;
        StacklessBegin();
        m_operation = new TestOperation();
        StacklessOperation(m_operation->Do());
        TAssert(m_operation->completed());
        delete m_operation;
        m_handle = new Input("tests/rtest.txt");
        StacklessOperation(m_handle->open());
        StacklessOperation(r = m_handle->read(buf, 25));
        TAssert(r == 10);
        StacklessOperation(m_handle->close());
        StacklessEnd();
    }
    TestOperation * m_operation;
    IO<Input> m_handle;
    char buf[25];
};

static void yieldingFunction() {
    Events::Timeout timeout(0.2);
    Task::operationYield(&timeout);
    TAssert(timeout.gotSignal());
}

void MainTask::Do() {
    customPrinter = new CustomPrinter();
    Printer::log(M_STATUS, "Test::Tasks running.");

    Events::TaskEvent taskEvt;
    Task * testTask = TaskMan::registerTask(new TestTask(), &taskEvt);
    waitFor(&taskEvt);
    TAssert(!taskEvt.gotSignal());
    yield();
    TAssert(taskEvt.gotSignal());
    taskEvt.ack();

    Task * testStackless = TaskMan::registerTask(new TestStackless(), &taskEvt);
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
