#pragma once

#include <BLua.h>
#include <Handle.h>
#include <Input.h>

namespace Balau {

class LuaHandleFactory : public LuaObjectFactory {
  public:
      LuaHandleFactory(IO<Handle> h) : m_obj(new IO<Handle>(h)) { }
    static void pushStatics(Lua & L);
  protected:
      LuaHandleFactory(IO<Handle> * h) : m_obj(h) { }
    void pushObjectAndMembers(Lua & L);
  private:
    IO<Handle> * m_obj;
};

class LuaInputFactory : public LuaHandleFactory {
  public:
      LuaInputFactory(IO<Input> h) : LuaHandleFactory(new IO<Handle>(h)) { }
    static void pushStatics(Lua & L);
  private:
    void pushObjectAndMembers(Lua & L);
};

void registerLuaHandle(Lua &);

};
