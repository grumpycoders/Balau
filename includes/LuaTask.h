#pragma once

#include <c++11-surrogates.h>
#include <BLua.h>
#include <Task.h>
#include <StacklessTask.h>
#include <Buffer.h>

namespace Balau {

class LuaTask;
class LuaMainTask;

class LuaExecCell {
  public:
      LuaExecCell();
      virtual ~LuaExecCell() { if (m_exception) delete m_exception; }
    void detach() { m_detached = true; }
    void exec(Lua & L);
    void exec(LuaMainTask * mainTask);
    bool gotError() { return m_gotError || m_exception; }
    void throwError() throw (GeneralException);
  protected:
    virtual void run(Lua &) = 0;
    virtual bool needsStack() { return false; }
    void setError() { m_gotError = true; }
  private:
    Events::Async m_event;
    bool m_detached = false;
    bool m_gotError = false;
    bool m_running = false;
    GeneralException * m_exception = NULL;
    friend class LuaTask;

      LuaExecCell(const LuaExecCell &) = delete;
    LuaExecCell & operator=(const LuaExecCell &) = delete;
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
    virtual bool needsStack() { return !m_file.isA<Buffer>(); }
    virtual void run(Lua &);
    IO<Handle> m_file;
};

class LuaTask : public Task {
  public:
      ~LuaTask() { L.weaken(); }
    virtual const char * getName() const { return "LuaTask"; }
  private:
      LuaTask(Lua && __L, LuaExecCell * cell) : L(Move(__L)), m_cell(cell) { if (!cell->needsStack()) setStackless(); }
    virtual void Do();
    Lua L;
    LuaExecCell * m_cell;
    friend class LuaMainTask;
};

class LuaMainTask : public StacklessTask {
  public:
      LuaMainTask();
      ~LuaMainTask();
    void stop();
    virtual const char * getName() const { return "LuaMainTask"; }
    static LuaMainTask * getMainTask(Lua &);
  private:
    void exec(LuaExecCell * cell);
    virtual void Do();
    Lua L;
    TQueue<LuaExecCell> m_queue;
    friend class LuaExecCell;
};

};
