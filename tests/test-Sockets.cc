#include <Main.h>
#include <Socket.h>

BALAU_STARTUP;

using namespace Balau;

class Worker : public Task {
  public:
      Worker(IO<Socket> io);
    virtual const char * getName();
    virtual void Do();
    IO<Socket> m_io;
    String m_name;
};

Worker::Worker(IO<Socket> io) : m_io(io) {
    m_name = m_io->getName();
    Printer::log(M_STATUS, "Got connection: %s", m_name.to_charp());
}

const char * Worker::getName() {
    return m_name.to_charp();
}

void Worker::Do() {
    char x, y;

    int r;
    r = m_io->read(&x, 1);
    Assert(x == 'x');
    Assert(r == 1);
    y = 'y';
    r = m_io->write(&y, 1);
    Assert(r == 1);
}

Listener<Worker> * listener;

class Client : public Task {
  public:
    virtual const char * getName() { return "Test client"; }
    virtual void Do() {
        Events::Timeout evt(0.1);
        waitFor(&evt);
        yield();

        char x, y;
        IO<Socket> s(new Socket());
        bool c = s->connect("localhost", 1234);
        Assert(c);
        x = 'x';
        int r;
        r = s->write(&x, 1);
        Assert(r == 1);
        r = s->read(&y, 1);
        Assert(y == 'y');
        Assert(r == 1);
        listener->stop();
    }
};

void MainTask::Do() {
    Printer::enable(M_ALL);
    Printer::log(M_STATUS, "Test::Sockets running.");

    Events::TaskEvent evtSvr(listener = new Listener<Worker>(1234));
    Events::TaskEvent evtCln(new Client);
    Printer::log(M_STATUS, "Created %s", listener->getName());
    waitFor(&evtSvr);
    waitFor(&evtCln);
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
