#include <Main.h>
#include <HttpServer.h>
#include <TaskMan.h>
#include <Socket.h>

#define DAEMON_NAME "Balau/1.0"

using namespace Balau;

static Regex stopURL("/stop$");

class StopAction : public HttpServer::Action {
  public:
      StopAction(Events::Async & event, bool & stop) : Action(stopURL), m_event(event), m_stop(stop) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
    Events::Async & m_event;
    bool & m_stop;
};

bool StopAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    m_stop = true;
    m_event.trigger();

    HttpServer::Response response(server, req, out);
    response->writeString(
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <title>Stop</title>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    Server stopping.\n"
"  </body>\n"
"</html>\n");

    response.Flush();
    return true;
}

class TestAction : public HttpServer::Action {
  public:
      TestAction() : Action(Regexes::any) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool TestAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);
    response->writeString(
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <title>Test</title>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    This is a test document.\n"
"  </body>\n"
"</html>\n");

    response.Flush();
    return true;
}

static Regex testFailureURL("^/failure.html$");

class TestFailure : public HttpServer::Action {
  public:
      TestFailure() : Action(testFailureURL) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool TestFailure::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    throw GeneralException("Test...");
}

class Stopper : public Task {
    virtual const char * getName() const { return "ServerStopper"; }
    virtual void Do();
};

void Stopper::Do() {
    IO<Socket> s(new Socket());
    bool c = s->connect("localhost", 8080);
    TAssert(c);
    s->writeString("GET /stop HTTP/1.0\r\n\r\n");
}

static const int NTHREADS = 4;

void MainTask::Do() {
    Printer::enable(M_DEBUG);
    Printer::log(M_STATUS, "Test::Http running.");

    TaskMan::TaskManThread * tms[NTHREADS];

    for (int i = 0; i < NTHREADS; i++)
        tms[i] = TaskMan::createThreadedTaskMan();

    Events::Async event;
    bool stop = false;

    HttpServer * s = new HttpServer();
    s->registerAction(new TestAction());
    s->registerAction(new TestFailure());
    s->registerAction(new StopAction(event, stop));
    s->setPort(8080);
    s->setLocal("localhost");
    s->start();

    Events::Timeout timeout(1);
    waitFor(&timeout);
    yield();

    waitFor(&event);

    Events::TaskEvent stopperEvent;
    Task * stopper = TaskMan::registerTask(new Stopper, &stopperEvent);
    waitFor(&stopperEvent);

    bool gotEvent = false, gotStopperEvent = false;
    int count = 0;

    while (!gotEvent || !gotStopperEvent) {
        count++;
        yield();
        if (event.gotSignal()) {
            TAssert(!gotEvent);
            gotEvent = true;
        }
        if (stopperEvent.gotSignal()) {
            TAssert(!gotStopperEvent);
            gotStopperEvent = true;
            stopperEvent.ack();
        }
    }
    TAssert(count <= 2);

    TAssert(stop);
    Printer::log(M_STATUS, "Test::Http is stopping.");

    s->stop();

    for (int i = 0; i < NTHREADS; i++)
        TaskMan::stopThreadedTaskMan(tms[i]);

    Printer::log(M_STATUS, "Test::Http passed.");
}
