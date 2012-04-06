#include <Main.h>
#include <HttpServer.h>
#include <TaskMan.h>
#include <Socket.h>
#include <SimpleMustache.h>

#define DAEMON_NAME "Balau/1.0"

using namespace Balau;

const char htmlTemplateStr[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
"  <head>\n"
"    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
"    <title>{{title}}</title>\n"
"    <style type=\"text/css\">\n"
"        body { font-family: arial, helvetica, sans-serif; }\n"
"    </style>\n"
"  </head>\n"
"\n"
"  <body>\n"
"    <h1>{{title}}</h1>\n"
"    <h2>{{msg}}</h2>\n"
"  </body>\n"
"</html>\n"
;

class TestHtmlTemplate : public AtStart {
  public:
      TestHtmlTemplate() : AtStart(10), htmlTemplate(m_template) { }
    virtual void doStart() { m_template.setTemplate(htmlTemplateStr); }

    const SimpleMustache & htmlTemplate;
  private:
    SimpleMustache m_template;
};

static TestHtmlTemplate testHtmlTemplate;

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
    SimpleMustache::Context ctx;
    HttpServer::Response response(server, req, out);

    ctx["title"] = "Stop";
    ctx["msg"] = "Server stopping";

    testHtmlTemplate.htmlTemplate.render(response.get(), &ctx);
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
    SimpleMustache::Context ctx;
    HttpServer::Response response(server, req, out);

    ctx["title"] = "Test";
    ctx["msg"] = "This is a test document.";

    testHtmlTemplate.htmlTemplate.render(response.get(), &ctx);
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

    waitFor(&event);

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
