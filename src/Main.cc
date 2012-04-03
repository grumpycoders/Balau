#include "Main.h"
#include "TaskMan.h"
#include "Printer.h"
#include "AtStartExit.h"

Balau::AtStart * Balau::AtStart::s_head = 0;
Balau::AtExit * Balau::AtExit::s_head = 0;

Balau::AtStart::AtStart(int priority) : m_priority(priority) {
    if (priority < 0)
        return;
    AAssert(!Main::hasMain(), "An AtStart can't be created dynamically");

    AtStart ** ptr = &s_head;

    m_next = 0;

    for (ptr = &s_head; *ptr && (priority > (*ptr)->m_priority); ptr = &((*ptr)->m_next));

    m_next = *ptr;
    *ptr = this;
}

Balau::AtExit::AtExit(int priority) : m_priority(priority) {
    if (priority < 0)
        return;
    AAssert(!Main::hasMain(), "An AtExit can't be created dynamically");

    AtExit ** ptr = &s_head;

    m_next = 0;

    for (ptr = &s_head; *ptr && (priority > (*ptr)->m_priority); ptr = &((*ptr)->m_next));

    m_next = *ptr;
    *ptr = this;
}

Balau::Main * Balau::Main::s_application = NULL;

Balau::MainTask::~MainTask() {
    if (m_stopTaskManOnExit)
        TaskMan::stop(0);
}

const char * Balau::MainTask::getName() const {
    return "Main Task";
}

int Balau::Main::bootstrap(int _argc, char ** _argv) {
    int r = 0;
    m_status = STARTING;

    argc = _argc;
    argv = _argv;
    enve = NULL;

    for (AtStart * ptr = AtStart::s_head; ptr; ptr = ptr->m_next)
        ptr->doStart();

    try {
        m_status = RUNNING;
        TaskMan::createTask(new MainTask());
        r = TaskMan::getDefaultTaskMan()->mainLoop();
        m_status = STOPPING;
    }
    catch (Exit e) {
        m_status = STOPPING;
        Printer::log(M_ERROR, "We shouldn't have gotten an Exit exception here... exitting anyway");
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_ERROR, "%s", str.to_charp());
        r = e.getCode();
    }
    catch (RessourceException e) {
        m_status = STOPPING;
        Printer::log(M_ERROR | M_ALERT, "The application got a ressource problem: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
        r = -1;
    }
    catch (GeneralException e) {
        m_status = STOPPING;
        Printer::log(M_ERROR | M_ALERT, "The application caused an exception: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
        r = -1;
    }
    catch (...) {
        m_status = STOPPING;
        Printer::log(M_ERROR | M_ALERT, "The application caused an unknown exception");
        r = -1;
    }
    m_status = STOPPING;

    for (AtExit * ptr = AtExit::s_head; ptr; ptr = ptr->m_next)
        ptr->doExit();

    m_status = STOPPED;
    return r;
}

extern "C" {

int main(int argc, char ** argv) {
    setlocale(LC_ALL, "");
    Balau::Main mainClass;
    return mainClass.bootstrap(argc, argv);
}

};
