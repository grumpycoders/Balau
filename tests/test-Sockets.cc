#include <Main.h>
#include <Socket.h>

using namespace Balau;

class Worker : public Task {
  public:
      Worker(IO<Socket> io, void *);
    virtual const char * getName() const;
    virtual void Do();
    IO<Socket> m_io;
    String m_name;
};

Worker::Worker(IO<Socket> io, void *) : m_io(io) {
    m_name = m_io->getName();
    Printer::log(M_STATUS, "Got connection: %s", m_name.to_charp());
}

const char * Worker::getName() const {
    return m_name.to_charp();
}

void Worker::Do() {
    char x, y;

    int r;
    r = m_io->read(&x, 1);
    TAssert(x == 'x');
    TAssert(r == 1);
    y = 'y';
    r = m_io->write(&y, 1);
    TAssert(r == 1);
}

Listener<Worker> * listener;

class Client : public Task {
  public:
    virtual const char * getName() const { return "Test client"; }
    virtual void Do() {
        sleep(0.1);

        char x, y;
        IO<Socket> s(new Socket());
        bool c = s->connect("localhost", 1234);
        TAssert(c);
        x = 'x';
        int r;
        r = s->write(&x, 1);
        TAssert(r == 1);
        r = s->read(&y, 1);
        TAssert(y == 'y');
        TAssert(r == 1);
        listener->stop();
    }
};

void MainTask::Do() {
    Printer::enable(M_ALL);
    Printer::log(M_STATUS, "Test::Sockets running.");

    Events::TaskEvent evtSvr;
    Events::TaskEvent evtCln;

    listener = TaskMan::registerTask(new Listener<Worker>(1234), &evtSvr);
    TaskMan::registerTask(new Client, &evtCln);

    waitFor(&evtSvr);
    waitFor(&evtCln);

    Printer::log(M_STATUS, "Created %s", listener->getName());
    bool svrDone = false, clnDone = false;
    while (!svrDone || !clnDone) {
        yield();
        if (evtSvr.gotSignal()) {
            evtSvr.ack();
            svrDone = true;
        }
        if (evtCln.gotSignal()) {
            evtCln.ack();
            clnDone = true;
        }
    }

    Printer::log(M_STATUS, "Test::Sockets passed.");
}
