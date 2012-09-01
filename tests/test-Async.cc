#include <Async.h>
#include <Task.h>
#include <TaskMan.h>
#include <Main.h>

using namespace Balau;

class AsyncOpTest : public AsyncOperation {
  public:
      AsyncOpTest(Events::Custom * evt) : m_evt(evt) { }
    virtual void run() {
        Printer::log(M_STATUS, "Async operation running");
        TAssert(!m_ran);
        TAssert(!m_finished);
        TAssert(!m_done);
        m_ran = true;
    }
    virtual void finish() {
        Printer::log(M_STATUS, "Async operation finishing");
        TAssert(m_ran);
        TAssert(!m_finished);
        TAssert(!m_done);
        m_finished = true;
    }
    virtual void done() {
        Printer::log(M_STATUS, "Async operation done");
        TAssert(m_ran);
        TAssert(m_finished);
        TAssert(!m_done);
        m_done = true;
        m_evt->doSignal();
    }
    virtual bool needsFinishWorker() { return true; }

    bool m_ran = false;
    bool m_finished = false;
    bool m_done = false;
    Events::Custom * m_evt;
};

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Async running.");

    Events::Custom evt;
    AsyncOpTest * op = createAsyncOp(new AsyncOpTest(&evt));
    waitFor(&evt);
    TAssert(!evt.gotSignal());
    yield();
    TAssert(evt.gotSignal());
    TAssert(op->m_ran);
    TAssert(op->m_finished);
    TAssert(op->m_done);
    delete op;

    Printer::log(M_STATUS, "Test::Async passed.");
}
