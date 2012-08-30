#pragma once

#include <c++11-surrogates.h>
#include <BLua.h>
#include <Task.h>

namespace Balau {

class LuaTask;
class LuaMainTask;

class LuaExecCell {
  public:
      LuaExecCell() : m_detached(false) { }
      virtual ~LuaExecCell() { }
    void detach() { m_detached = true; }
    void exec(LuaMainTask * mainTask);
  protected:
    virtual void run(Lua &) = 0;
  private:
    Events::Async m_event;
    bool m_detached;
    friend class LuaTask;
};

class LuaExecString : public LuaExecCell {
  public:
      LuaExecString(const String & str) : m_str(str) { }
  private:
    virtual void run(Lua &);
    String m_str;
};

class LuaTask : public Task {
  public:
      ~LuaTask() { L.weaken(); }
    virtual const char * getName() const { return "LuaTask"; }
  private:
      LuaTask(Lua && __L, LuaExecCell * cell) : L(Move(__L)), m_cell(cell) { }
    virtual void Do();
    Lua L;
    LuaExecCell * m_cell;
    friend class LuaMainTask;
};

class LuaMainTask : public Task {
  public:
      LuaMainTask() : m_stopping(false) { }
      ~LuaMainTask() { L.close(); }
    void stop();
    virtual const char * getName() const { return "LuaMainTask"; }
  private:
    void exec(LuaExecCell * cell);
    virtual void Do();
    Lua L;
    Events::Async m_queueEvent;
    Queue<LuaExecCell> m_queue;
    bool m_stopping;
    friend class LuaExecCell;
};

};
