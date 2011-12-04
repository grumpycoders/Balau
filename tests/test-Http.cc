#include <Main.h>
#include <HttpServer.h>
#include <TaskMan.h>

#define DAEMON_NAME "Balau/1.0"

using namespace Balau;

class TestAction : public HttpServer::Action {
  public:
      TestAction() : Action(Regexes::any) { }
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool TestAction::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    static const char str[] =
"HTTP/1.1 200 Found\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Content-Length: 266\r\n"
"Server: " DAEMON_NAME "\r\n"
"\r\n"
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
"</html>\n";

    out->forceWrite(str, sizeof(str) - 1);
    return true;
}

Balau::Regex testFailureURL("^/failure.html$");

class TestFailure : public HttpServer::Action {
  public:
      TestFailure() : Action(testFailureURL) { }
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
};

bool TestFailure::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    throw GeneralException("Test...");
}

#define NTHREADS 4

void MainTask::Do() {
    Printer::enable(M_DEBUG);
    Printer::log(M_STATUS, "Test::Http running.");

    Thread * tms[NTHREADS];

    for (int i = 0; i < NTHREADS; i++)
        tms[i] = TaskMan::createThreadedTaskMan();

    HttpServer * s = new HttpServer();
    HttpServer::Action * a = new TestAction();
    HttpServer::Action * f = new TestFailure();
    a->registerMe(s);
    f->registerMe(s);
    s->setPort(8080);
    s->setLocal("localhost");
    s->start();

    yield();

    s->stop();

    for (int i = 0; i < NTHREADS; i++)
        tms[i]->join();

    Printer::log(M_STATUS, "Test::Http passed.");
}
