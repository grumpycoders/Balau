#include <Main.h>
#include <BLua.h>

using namespace Balau;

static int objGotDestroyed = 0;

static int callCount = 0;

class ObjectTest {
  public:
      ObjectTest() { Printer::log(M_DEBUG, "ObjectTest at %p created.", this); }
      ~ObjectTest() { Printer::log(M_DEBUG, "ObjectTest at %p destroyed.", this); objGotDestroyed++; }
    void someMethod1() { Printer::log(M_DEBUG, "ObjectTest::someMethod1() called on %p.", this); callCount++; }
    int someMethod2(int p) { Printer::log(M_DEBUG, "ObjectTest::someMethod2() called on %p.", this); callCount++; return p * 2; }
    static void someFunction() { Printer::log(M_DEBUG, "ObjectTest::someFunction() called."); callCount++; }
};

enum ObjectTest_methods_t {
    OBJECTTEST_SOMEMETHOD1,
    OBJECTTEST_SOMEMETHOD2,
};

enum ObjectTest_functions_t {
    OBJECTTEST_CREATEOBJECTTEST,
    OBJECTTEST_SOMEFUNCTION,
    OBJECTTEST_YIELDTEST,
};

struct lua_functypes_t ObjectTest_methods[] = {
    { OBJECTTEST_SOMEMETHOD1,           "someMethod1",          0, 0, { } },
    { OBJECTTEST_SOMEMETHOD2,           "someMethod2",          1, 1, { BLUA_NUMBER } },
    { -1, 0, 0, 0, 0 },
};

struct lua_functypes_t ObjectTest_functions[] = {
    { OBJECTTEST_CREATEOBJECTTEST,      "createObjectTest",     0, 0, { } },
    { OBJECTTEST_SOMEFUNCTION,          "ObjectTestFunction",   0, 0, { } },
    { OBJECTTEST_YIELDTEST,             "yieldTest",            1, 1, { BLUA_NUMBER } },
    { -1, 0, 0, 0, 0 },
};

class sLua_ObjectTest {
  public:
    DECLARE_METHOD(ObjectTest, OBJECTTEST_SOMEMETHOD1);
    DECLARE_METHOD(ObjectTest, OBJECTTEST_SOMEMETHOD2);

    DECLARE_FUNCTION(ObjectTest, OBJECTTEST_CREATEOBJECTTEST);
    DECLARE_FUNCTION(ObjectTest, OBJECTTEST_SOMEFUNCTION);
    DECLARE_FUNCTION(ObjectTest, OBJECTTEST_YIELDTEST);
  private:
    static int ObjectTest_proceed(Lua & L, int n, ObjectTest * obj, int caller);
    static int ObjectTest_proceed_statics(Lua & L, int n, int caller) throw (GeneralException);
};

class LuaObjectTestFactory : public LuaObjectFactory {
  public:
      LuaObjectTestFactory(ObjectTest * obj) : m_obj(obj) { }
    static void pushStatics(Lua & L) {
        CHECK_METHODS(ObjectTest);
        CHECK_FUNCTIONS(ObjectTest);

        PUSH_FUNCTION(ObjectTest, OBJECTTEST_CREATEOBJECTTEST);
        PUSH_FUNCTION(ObjectTest, OBJECTTEST_SOMEFUNCTION);
        PUSH_FUNCTION(ObjectTest, OBJECTTEST_YIELDTEST);
    }
  private:
    void pushObjectAndMembers(Lua & L) {
        pushObj(L, m_obj, "ObjectTest");

        PUSH_METHOD(ObjectTest, OBJECTTEST_SOMEMETHOD1);
        PUSH_METHOD(ObjectTest, OBJECTTEST_SOMEMETHOD2);
    }
    ObjectTest * m_obj;
};

int sLua_ObjectTest::ObjectTest_proceed(Lua & L, int n, ObjectTest * obj, int caller) {
    switch (caller) {
    case OBJECTTEST_SOMEMETHOD1:
        obj->someMethod1();
        break;

    case OBJECTTEST_SOMEMETHOD2:
        L.push((lua_Number) obj->someMethod2(L.tonumber(-1)));
        return 1;
        break;
    }

    return 0;
}

Events::Timeout * evt = NULL;

int sLua_ObjectTest::ObjectTest_proceed_statics(Lua & L, int n, int caller) throw (GeneralException) {
    int y;

    switch (caller) {
    case OBJECTTEST_CREATEOBJECTTEST:
        {
            ObjectTest * ot = new ObjectTest;
            LuaObjectTestFactory factory(ot);
            factory.pushDestruct(L);
        }
        return 1;
        break;

    case OBJECTTEST_SOMEFUNCTION:
        ObjectTest::someFunction();
        break;

    case OBJECTTEST_YIELDTEST:
        y = L.tonumber();
        L.remove();
        L.push((lua_Number) y + 1);
        Printer::log(M_STATUS, "yield %i", y);
        if (evt)
            delete evt;
        evt = NULL;
        if (y < 5) {
            evt = new Events::Timeout(1.0f);
            throw EAgain(evt);
        }
        break;
    }

    return 0;
}

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
    L.open_ffi();

    LuaObjectTestFactory::pushStatics(L);

    TAssert(L.gettop() == 0);
    L.load("return 42");
    TAssert(L.gettop() == 1);
    int r = L.tonumber();
    TAssert(r == 42);
    L.pop();
    TAssert(L.gettop() == 0);

    L.push("obj");
    {
        ObjectTest * ot = new ObjectTest;
        LuaObjectTestFactory factory(ot);
        factory.pushDestruct(L);
    }
    L.settable(LUA_GLOBALSINDEX);

    L.load("return type(obj)");
    TAssert(L.gettop() == 1);
    String t = L.tostring();
    TAssert(t == "table");
    L.pop();

    TAssert(callCount == 0);

    L.load("obj:someMethod1()");
    TAssert(L.gettop() == 0);

    TAssert(callCount == 1);

    L.load("return obj:someMethod2(21)");
    TAssert(L.gettop() == 1);
    TAssert(L.tonumber() == 42);
    L.pop();

    TAssert(callCount == 2);

    L.load("ObjectTestFunction()");
    TAssert(L.gettop() == 0);

    TAssert(callCount == 3);

    L.load("yieldTest(0)");
    while (L.yielded()) {
        waitFor(LuaHelpersBase::getEvent(L));
        yield();
        LuaHelpersBase::resume(L);
    }
    TAssert(L.gettop() == 0);

    TAssert(objGotDestroyed == 0);
    L.load("obj2 = createObjectTest() obj2:destroy()");
    TAssert(objGotDestroyed == 1);
    L.load("createObjectTest() collectgarbage('collect')");
    TAssert(objGotDestroyed == 2);
    L.close();
    TAssert(objGotDestroyed == 3);

    Printer::log(M_STATUS, "Test::Lua passed.");
}
