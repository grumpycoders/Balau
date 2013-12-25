#include "HttpActionStatic.h"
#include "Input.h"
#include "TaskMan.h"
#include "HelperTasks.h"

bool Balau::HttpActionStatic::Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException) {
    HttpServer::Response response(server, req, out);
    String & fname = match.uri[1];
    String extension;

    ssize_t dot = fname.strrchr('.');

    if (dot > 0)
        extension = fname.extract(dot + 1);

    bool error = false;

    if (fname.strstr("/../") > 0)
        error = true;

    IO<Input> file(new Input(m_base + fname));

    if (!error) {
        try {
            file->open();
        }
        catch (ENoEnt & e) {
            error = true;
        }
    }

    if (error) {
        response.get()->writeString("Static file not found.");
        response.SetResponseCode(404);
        response.SetContentType("text/plain");
    }
    else {
        Events::TaskEvent evt;
        Task * copy = TaskMan::registerTask(new CopyTask(file, response.get()), &evt);
        Task::operationYield(&evt);
        file->close();
        response.SetContentType(Http::getContentType(extension));
    }
    response.Flush();
    return true;
}

