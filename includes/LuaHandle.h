#pragma once

#include <BLua.h>
#include <Handle.h>
#include <Input.h>

namespace Balau {

class LuaIO {
  public:
      LuaIO(IO<Handle> h) : m_h(h) { }
      LuaIO(const LuaIO & lio) : m_h(lio.m_h) { }
    IO<Handle> getIO() { return m_h; }
    void cleanup() { m_h->close(); }
  private:
    IO<Handle> m_h;
};

class LuaHandleFactory : public LuaObjectFactory {
  public:
      LuaHandleFactory(IO<Handle> h) : m_obj(new LuaIO(h)) { }
    static void pushStatics(Lua & L);
  protected:
      LuaHandleFactory(LuaIO * h) : m_obj(h) { }
    virtual void pushObjectAndMembers(Lua & L) override;
  private:
    LuaIO * m_obj;
};

class LuaInputFactory : public LuaHandleFactory {
  public:
      LuaInputFactory(IO<Input> h) : LuaHandleFactory(h) { }
    static void pushStatics(Lua & L);
  private:
    virtual void pushObjectAndMembers(Lua & L) override;
};

void registerLuaHandle(Lua &);

};
