#include <Main.h>
#include <BLua.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Lua running.");

    Lua L;

    // yeah, they really should be the same thing.
    TAssert(sizeof(L) == sizeof(lua_State *));

    L.open_base();
    L.open_table();
    L.open_string();
    L.open_math();
    L.open_debug();
    L.open_bit();
    L.open_jit();

    TAssert(L.gettop() == 0);
    L.load("return 42");
    TAssert(L.gettop() == 1);
    int r = L.tonumber();
    TAssert(r == 42);
    L.pop();
    TAssert(L.gettop() == 0);

    Printer::log(M_STATUS, "Test::Lua passed.");
}
