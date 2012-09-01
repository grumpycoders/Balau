#pragma once

#include <c++11-surrogates.h>
#include <BLua.h>
#include <Task.h>

namespace Balau {

class LuaTask;
class LuaMainTask;

class LuaExecCell {
  public:
      LuaExecCell();
      virtual ~LuaExecCell() { }
    void detach() { m_detached = true; }
    void exec(LuaMainTask * mainTask);
  protected:
    virtual void run(Lua &) = 0;
  private:
    Events::Async m_event;
    bool m_detached = false;
    friend class LuaTask;
};

class LuaExecString : public LuaExecCell {
  public:
      LuaExecString(const String & str) : m_str(str) { }
  private:
    virtual void run(Lua &);
    String m_str;
};

class LuaExecFile : public LuaExecCell {
  public:
      LuaExecFile(IO<Handle> file) : m_file(file) { }
  private:
    virtual void run(Lua &);
    IO<Handle> m_file;
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
      LuaMainTask();
      ~LuaMainTask();
    void stop();
    virtual const char * getName() const { return "LuaMainTask"; }
  private:
    void exec(LuaExecCell * cell);
    virtual void Do();
    Lua L;
    TQueue<LuaExecCell> m_queue;
    friend class LuaExecCell;
};

};
