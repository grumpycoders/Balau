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
    L.open_ffi();
    L.open_lcrypt();
    L.push("LuaMainTask");
    L.push((void *) this);
    L.settable(LUA_REGISTRYINDEX);
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
    LuaExecCell * cell = NULL;
    for (;;) {
        Printer::elog(E_TASK, "LuaMainTask at %p tries to pop an ExecCell", this);
        try {
            cell = m_queue.pop();
        }
        catch (Balau::EAgain &) {
            taskSwitch();
        }
        Printer::elog(E_TASK, "LuaMainTask at %p popped %p", this, cell);
        if (dynamic_cast<LuaTaskStopper *>(cell)) {
            Printer::elog(E_TASK, "LuaMainTask at %p is stopping", this);
            delete cell;
            return;
        }
        TaskMan::registerTask(new LuaTask(L.thread(), cell), this);
    }
}

Balau::LuaMainTask * Balau::LuaMainTask::getMainTask(Lua & L) {
    L.push("LuaMainTask");
    L.gettable(LUA_REGISTRYINDEX);
    LuaMainTask * r = (LuaMainTask *) L.touserdata();
    L.pop();
    return r;
}

void Balau::LuaTask::Do() {
    while(true) {
        try {
            if (L.yielded())
                L.resume();
            else
                m_cell->run(L);
        }
        catch (EAgain &) {
        }
        catch (GeneralException & e) {
            m_cell->m_exception = new GeneralException(e);
        }
        catch (...) {
            m_cell->setError();
        }
        if (L.yielded() && !m_cell->m_exception) {
            yield();
            continue;
        }
        if (m_cell->m_detached)
            delete m_cell;
        else
            m_cell->m_event.trigger();
        break;
    }
}

void Balau::LuaExecCell::exec(LuaMainTask * mainTask) {
    if (m_running)
        return;
    m_running = true;
    if (!m_detached)
        Task::prepare(&m_event);
    mainTask->exec(this);
    if (!m_detached)
        Task::operationYield(&m_event, Task::INTERRUPTIBLE);
}

void Balau::LuaExecCell::exec(Lua & L) {
    exec(LuaMainTask::getMainTask(L));
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
