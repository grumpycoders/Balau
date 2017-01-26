#include "Main.h"
#include "TaskMan.h"
#include "Printer.h"
#include "AtStartExit.h"
#include "StacklessTask.h"

#include <locale.h>

Balau::AtStart * Balau::AtStart::s_head = 0;
Balau::AtExit * Balau::AtExit::s_head = 0;
Balau::AtStartAsTask * Balau::AtStartAsTask::s_head = 0;
Balau::AtExitAsTask * Balau::AtExitAsTask::s_head = 0;

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

Balau::AtStartAsTask::AtStartAsTask(int priority) : m_priority(priority) {
    if (priority < 0)
        return;
    AAssert(!Main::hasMain(), "An AtStartAsTask can't be created dynamically");

    AtStartAsTask ** ptr = &s_head;

    m_next = 0;

    for (ptr = &s_head; *ptr && (priority > (*ptr)->m_priority); ptr = &((*ptr)->m_next));

    m_next = *ptr;
    *ptr = this;
}

Balau::AtExitAsTask::AtExitAsTask(int priority) : m_priority(priority) {
    if (priority < 0)
        return;
    AAssert(!Main::hasMain(), "An AtExitAsTask can't be created dynamically");

    AtExitAsTask ** ptr = &s_head;

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

namespace Balau {

class BootstrapTask : public StacklessTask {
  public:
      BootstrapTask(int argc, char ** argv, char ** enve) : m_argc(argc), m_argv(argv), m_enve(enve) { }
    virtual void Do() {
        Balau::Task * t;
        try {
            switch (m_state) {
            case 0:
                m_ptrStart = AtStartAsTask::s_head;
                m_ptrExit = AtExitAsTask::s_head;
                m_state = 1;
            case 1:
                if (m_ptrStart) {
                    t = m_ptrStart->createStartTask();
                    if (m_waiting)
                        m_evt.ack();
                    m_evt.attachToTask(t);
                    m_waiting = true;
                    waitFor(&m_evt);
                    TaskMan::registerTask(t);
                    m_ptrStart = m_ptrStart->m_next;
                    yield();
                }
                m_state = 2;
                t = new MainTask(m_argc, m_argv, m_enve);
                if (m_waiting)
                    m_evt.ack();
                m_evt.attachToTask(t);
                waitFor(&m_evt);
                TaskMan::registerTask(t);
                yield();
            case 2:
                if (m_ptrExit) {
                    t = m_ptrExit->createExitTask();
                    m_evt.ack();
                    m_evt.attachToTask(t);
                    waitFor(&m_evt);
                    TaskMan::registerTask(t);
                    m_ptrExit = m_ptrExit->m_next;
                    yield();
                }
            }
        }
        catch (EAgain &) {
            taskSwitch();
        }
    }
  private:
    virtual const char * getName() const { return "BootstrapTask"; }
    int m_argc;
    char ** m_argv;
    char ** m_enve;
    AtStartAsTask * m_ptrStart = NULL;
    AtExitAsTask * m_ptrExit = NULL;
    Events::TaskEvent m_evt;
    bool m_waiting = false;
};

};

int Balau::Main::bootstrap(int argc, char ** argv) {
    int r = 0;
    m_status = STARTING;

    try {
        for (AtStart * ptr = AtStart::s_head; ptr; ptr = ptr->m_next)
            ptr->doStart();

        m_status = RUNNING;
        TaskMan::registerTask(new BootstrapTask(argc, argv, NULL));
        r = TaskMan::getDefaultTaskMan()->mainLoop();
    }
    catch (Exit & e) {
        m_status = STOPPING;
        Printer::log(M_ERROR, "We shouldn't have gotten an Exit exception here... exitting anyway");
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_ERROR, "%s", str.to_charp());
        r = e.getCode();
    }
    catch (RessourceException & e) {
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
    catch (GeneralException & e) {
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

#ifndef BALAU_MAIN
#define BALAU_MAIN main
#endif

int BALAU_MAIN(int argc, char ** argv) {
    setlocale(LC_ALL, "");
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    Balau::Main mainClass;
    return mainClass.bootstrap(argc, argv);
}

};
