#include <Main.h>
#include <BLua.h>

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Lua running.");

    Lua L;

    // yeah, they really should be the same thing.
    Assert(sizeof(L) == sizeof(lua_State *));

    L.open_base();
    L.open_table();
    L.open_string();
    L.open_math();
    L.open_debug();
    L.open_bit();
    L.open_jit();

    Assert(L.gettop() == 0);
    L.load("return 42");
    Assert(L.gettop() == 1);
    int r = L.tonumber();
    Assert(r == 42);
    L.pop();
    Assert(L.gettop() == 0);

    Printer::log(M_STATUS, "Test::Lua passed.");
}
