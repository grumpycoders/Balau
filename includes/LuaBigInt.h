#pragma once

#include <BLua.h>
#include <BigInt.h>

namespace Balau {

class LuaBigIntFactory : public LuaObjectFactory {
  public:
      LuaBigIntFactory(BigInt * b) : m_obj(b) { }
    static void pushStatics(Lua & L);
  protected:
    void pushObjectAndMembers(Lua & L);
  private:
    BigInt * m_obj;
};

void registerLuaBigInt(Lua &);

};
