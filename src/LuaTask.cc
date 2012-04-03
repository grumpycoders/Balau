#include "LuaTask.h"
#include "Main.h"
#include "TaskMan.h"

class LuaTaskDummy : public Balau::LuaExecCell {
    virtual void run(Balau::Lua &) { }
};

Balau::LuaMainTask::LuaMainTask() : m_stopping(false) {
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
    m_queue.push(cell);
}

void Balau::LuaMainTask::stop() {
    Atomic::CmpXChgVal(&m_stopping, true, false);
    exec(new LuaTaskDummy());
}

void Balau::LuaMainTask::Do() {
    while (!m_stopping) {
        LuaExecCell * cell;
        while ((cell = m_queue.pop(false))) {
            if (dynamic_cast<LuaTaskDummy *>(cell)) {
                delete cell;
                break;
            }
            TaskMan::createTask(new LuaTask(L.thread(), cell), this);
        }
    }
}

void Balau::LuaTask::Do() {
    m_cell->run(L);
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
        Task::yield(&m_event);
}

void Balau::LuaExecString::run(Lua & L) {
    L.load(m_str);
}

void Balau::LuaExecFile::run(Lua & L) {
    L.load(m_file);
}
