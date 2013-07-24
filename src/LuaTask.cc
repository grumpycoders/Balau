#include "LuaTask.h"
#include "Main.h"
#include "TaskMan.h"
#include "Printer.h"

namespace {

class LuaTaskStopper : public Balau::LuaExecCell {
    virtual void run(Balau::Lua &) { }
};

};

Balau::LuaExecCell::LuaExecCell() {
    Printer::elog(E_TASK, "LuaExecCell created at %p", this);
}

Balau::LuaMainTask::LuaMainTask() {
    Printer::elog(E_TASK, "LuaMainTask created at %p", this);
    L.open_base();
    L.open_table();
    L.open_string();
    L.open_math();
    L.open_debug();
    L.open_bit();
    L.open_jit();
}

Balau::LuaMainTask::~LuaMainTask() {
    L.close();
}

void Balau::LuaMainTask::exec(LuaExecCell * cell) {
    Printer::elog(E_TASK, "LuaMainTask at %p is asked to queue Cell %p", this, cell);
    m_queue.push(cell);
}

void Balau::LuaMainTask::stop() {
    Printer::elog(E_TASK, "LuaMainTask at %p asked to stop", this);
    exec(new LuaTaskStopper());
}

void Balau::LuaMainTask::Do() {
    for (;;) {
        LuaExecCell * cell;
        Printer::elog(E_TASK, "LuaMainTask at %p tries to pop an ExecCell", this);
        while ((cell = m_queue.pop())) {
            Printer::elog(E_TASK, "LuaMainTask at %p popped %p", this, cell);
            if (dynamic_cast<LuaTaskStopper *>(cell)) {
                Printer::elog(E_TASK, "LuaMainTask at %p is stopping", this);
                delete cell;
                return;
            }
            TaskMan::registerTask(new LuaTask(L.thread(), cell), this);
        }
    }
}

void Balau::LuaTask::Do() {
    try {
        m_cell->run(L);
    }
    catch (GeneralException e) {
        m_cell->m_exception = new GeneralException(e);
    }
    catch (...) {
        m_cell->setError();
    }
    if (m_cell->m_detached)
        delete m_cell;
    else
        m_cell->m_event.trigger();
}

void Balau::LuaExecCell::exec(LuaMainTask * mainTask) {
    if (!m_detached)
        Task::prepare(&m_event);
    mainTask->exec(this);
    if (!m_detached)
        Task::operationYield(&m_event);
}

void Balau::LuaExecCell::throwError() throw (GeneralException) {
    if (!gotError())
        return;

    if (m_exception) {
        GeneralException copy(*m_exception);
        delete m_exception;
        m_exception = NULL;
        throw copy;
    } else {
        throw GeneralException("Unknown error");
    }
}

void Balau::LuaExecString::run(Lua & L) {
    L.load(m_str);
}

void Balau::LuaExecFile::run(Lua & L) {
    L.load(m_file);
}
