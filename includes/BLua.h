#pragma once

#include <functional>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <Exceptions.h>
#include <Handle.h>
#include <Task.h>
#include <StacklessTask.h>

namespace Balau {

class Lua;

class LuaObjectBase {
  public:
      virtual ~LuaObjectBase() { }
    virtual void destroy() = 0;
    void detach() { m_detached = true; }
    virtual Task * spawnCollector() = 0;
  protected:
    bool isDetached() { return m_detached; }
  private:
    bool m_detached = false;
};

template<class T>
class DeferredCollector : public StacklessTask {
  public:
      DeferredCollector(T * obj) : m_obj(obj) { }
    virtual const char * getName() const override { return "DeferredCollector"; }
    virtual void Do() override {
        StacklessBegin();
        StacklessOperation(delete m_obj);
        StacklessEnd();
    }
  private:
    T * m_obj;
};

template<class T>
class LuaObject : public LuaObjectBase {
  public:
      LuaObject(T * obj) : m_obj(obj) { }
    virtual void destroy() override { if (!isDetached() && m_obj) delete m_obj; detach(); }
    T * getObj() { return m_obj; }
    virtual Task * spawnCollector() override { return isDetached() ? NULL : new DeferredCollector<T>(m_obj); }
  private:
    T * m_obj;
};

class LuaObjectFactory {
  public:
      LuaObjectFactory() { }
      virtual ~LuaObjectFactory() { }
    virtual void push(Lua & L);
    void pushDestruct(Lua & L);
    template<class T>
    static T * getMe(Lua & L, int idx = 1);
  protected:
    virtual void pushObjectAndMembers(Lua & L) = 0;
    template<class T>
    void pushObj(Lua & L, T * obj, const char * name = NULL);
    static void pushMethod(Lua & L, const String & name, lua_CFunction func, int upvalues = 0) {
        pushMethod(L, name.to_charp(), name.strlen(), func, upvalues);
    }
    static void pushMeta(Lua & L, const String & name, lua_CFunction func, int upvalues = 0) {
        pushMeta(L, name.to_charp(), name.strlen(), func, upvalues);
    }
    template<size_t S>
    static void pushMethod(Lua & L, const char (&str)[S], lua_CFunction func, int upvalues = 0) {
        pushMethod(L, str, S - 1, func, upvalues);
    }
    template<size_t S>
    static void pushMeta(Lua & L, const char (&str)[S], lua_CFunction func, int upvalues = 0) {
        pushMeta(L, str, S - 1, func, upvalues);
    }
    static void pushMethod(Lua & L, const char * name, size_t strSize, lua_CFunction func, int upvalues = 0);
    static void pushMeta(Lua & L, const char * name, size_t strSize, lua_CFunction func, int upvalues = 0);
    static LuaObjectBase * getMeInternal(Lua & L, int idx);
    friend class Lua;
  private:
    void pushMe(Lua & L, LuaObjectBase * o, const char * name = NULL);
    bool m_wantsDestruct = false, m_pushed = false;
    LuaObjectFactory & operator=(const LuaObjectFactory &) = delete;
      LuaObjectFactory(const LuaObjectFactory &) = delete;
};

typedef int (*openlualib_t)(lua_State * L);

class Lua {
  public:
      Lua();
      Lua(lua_State * __L) : L(__L) { }
      Lua(Lua && oL) : L(oL.L) { oL.L = NULL; }
      Lua(const Lua & oL) : L(oL.L) { }

    Lua & operator=(Lua && oL);

    typedef int (*lua_CallWrapper)(lua_State *, lua_CFunction);

    int ref(int t = -2) { return luaL_ref(L, t); }
    void unref(int ref, int t = -1) { luaL_unref(L, t, ref); }

    void close();
    void open_base();
    void open_table();
    void open_string();
    void open_math();
    void open_debug();
    void open_bit();
    void open_jit();
    void open_ffi();
    void open_lcrypt();
    int wrap_open(openlualib_t open) { int n = gettop(); int r = open(L); while (n < gettop()) pop(); return r; }
    void openlib(const String & libname, const struct luaL_reg *l, int nup) { luaL_openlib(L, libname.to_charp(), l, nup); }

    void setCallWrap(lua_CallWrapper wrapper);
    void declareFunc(const char * funcName, lua_CFunction f, int tableIdx = LUA_GLOBALSINDEX);

    void call(const char * funcName, int tableIdx = LUA_GLOBALSINDEX, int nArgs = 0);
    void call(int nArgs = 0) { resume(nArgs); }
    void pcall(int nArgs = 0) throw (GeneralException);

    void push() { checkstack(); lua_pushnil(L); }
    void push(lua_Number n) { checkstack(); lua_pushnumber(L, n); }
    void push(const String & s) { checkstack(); lua_pushlstring(L, s.to_charp(), s.strlen()); }
    void push(bool b) { checkstack(); lua_pushboolean(L, b); }
    template<size_t S>
    void push(const char (&str)[S]) { checkstack(); lua_pushlstring(L, str, S - 1); }
    void push(const char * str, ssize_t size = -1) { if (size < 0) size = strlen(str); checkstack(); lua_pushlstring(L, str, size); }
    void push(void * p) { checkstack(); lua_pushlightuserdata(L, p); }
    void push(lua_CFunction f, int n = 0) { checkstack(); lua_pushcclosure(L, f, n); }
    void pop(int idx = 1) { lua_pop(L, idx); }
    int checkstack(int extra = 1) { return lua_checkstack(L, extra); }

    int next(int t = -2) { return lua_next(L, t); }
    void copy(int i = -1) { checkstack(); lua_pushvalue(L, i); }
    void remove(int i = 1) { lua_remove(L, i); }
    void insert(int i = 1) { checkstack(); lua_insert(L, i); }
    void replace(int i = 1) { lua_replace(L, i); }
    void newtable() { checkstack(); lua_newtable(L); }
    void * newuser(size_t s) { checkstack(); return lua_newuserdata(L, s); }
    void settable(int tableIdx = -3, bool raw = false);
    void gettable(int tableIdx = -2, bool raw = false);
    void rawseti(int idx, int tableIdx = -2) { lua_rawseti(L, tableIdx, idx); }
    void rawgeti(int idx, int tableIdx = -1) { lua_rawgeti(L, tableIdx, idx); }
    void setvar() { lua_settable(L, LUA_GLOBALSINDEX); }
    int gettop() { return lua_gettop(L); }
    void getglobal(const char * name) throw (GeneralException);
    void pushLuaContext();
    void error(const char * msg) throw (GeneralException);
    void error(const String & msg) { error(msg.to_charp()); }

    int type(int i = -1) { return lua_type(L, i); }
    bool isnil(int i = -1) { return lua_isnil(L, i); }
    bool isboolean(int i = -1) { return lua_isboolean(L, i); }
    bool isnumber(int i = -1) { return lua_isnumber(L, i); }
    bool isstring(int i = -1) { return lua_isstring(L, i); }
    bool istable(int i = -1) { return lua_istable(L, i); }
    bool isfunction(int i = -1) { return lua_isfunction(L, i); }
    bool iscfunction(int i = -1) { return lua_iscfunction(L, i); }
    bool isuserdata(int i = -1) { return lua_isuserdata(L, i); }
    bool islightuserdata(int i = -1) { return lua_islightuserdata(L, i); }
    bool isobject(int i = -1);

    int upvalue(int i) { return lua_upvalueindex(i); }

    bool toboolean(int i = -1) { return lua_toboolean(L, i); }
    lua_Number tonumber(int i = -1) { return lua_tonumber(L, i); }
    String tostring(int i = -1);
    lua_CFunction tocfunction(int i = -1) { return lua_tocfunction(L, i); }
    void * touserdata(int i = -1) { return lua_touserdata(L, i); }

    String escapeString(const String &);
    void load(IO<Handle> in, bool docall = true) throw (GeneralException);
    void load(const String &, bool docall = true) throw (GeneralException);
    void dumpvars(IO<Handle> out, const String & prefix, int idx = -1);
    Lua thread(bool saveit = true);
    int yield(int nresults = 0) { return lua_yield(L, nresults); }
    int yield(Future<int>) throw (GeneralException);
    bool yielded() { return lua_status(L) == LUA_YIELD; }
    bool resume(int nargs = 0) throw (GeneralException);
    void showstack(int level = M_INFO);
    void showerror();
    int getmetatable(int i = -1) { checkstack(); return lua_getmetatable(L, i); }
    int setmetatable(int i = -2) { return lua_setmetatable(L, i); }
    int sethook(lua_Hook func, int mask, int count) { return lua_sethook(L, func, mask, count); }
    void weaken();

    template<class T>
    T * recast(int n = -1) {
        LuaObjectBase * b;
        LuaObject<T> * r;

        b = LuaObjectFactory::getMeInternal(*this, n);
        if (!b)
            error("LuaObject base object required; got null.");

        r = dynamic_cast<LuaObject<T> *>(b);

        if (!r)
            error(String("Object not compatible; expecting ") + ClassName(r).c_str() + " but got *" + ClassName(b).c_str() + " instead.");

        return r->getObj();
    }

    lua_State * getState() { return L; }
  protected:

  private:
    void dumpvars_i(IO<Handle> out, const String & prefix, int idx);
    void dumpvars_r(IO<Handle> out, int idx, int depth = 0) throw (GeneralException);
    bool resumeC(int & nargs);
    void yieldC() throw (GeneralException);
    void processException(GeneralException & e);

    lua_State * L;

    friend class LuaStatics;
};

class LuaException : public GeneralException {
  public:
      LuaException(String fn) : GeneralException(fn) { }
  protected:
      LuaException() { }
};

enum Lua_types_t {
    BLUA_OBJECT      = 0x01,
    BLUA_TABLE       = 0x02,
    BLUA_BOOLEAN     = 0x04,
    BLUA_NUMBER      = 0x08,
    BLUA_STRING      = 0x10,
    BLUA_FUNCTION    = 0x20,
    BLUA_NIL         = 0x40,
    BLUA_USERDATA    = 0x80,
    BLUA_ANY         = 0xff,
};

#define MAX_TYPE 8

#define MAXARGS 32

struct lua_functypes_t {
    int number;
    const char * name;
    int minargs, maxargs;
    int argtypes[MAXARGS];
};

#define PUSH_CLASS(classname) \
    bool callPushClassFirst = true; \
{ \
    L.newtable(); \
    L.push("name"); \
    L.push(#classname); \
    L.copy(); \
    L.copy(-4); \
    L.setvar(); \
    L.settable(); \
}

#define PUSH_SUBCLASS(classname, parentname) \
    bool callPushClassFirst = true; \
{ \
    L.newtable(); \
    L.push("name"); \
    L.push(#classname); \
    L.copy(); \
    L.copy(-4); \
    L.setvar(); \
    L.settable(); \
    L.push("parent"); \
    L.getglobal(#parentname); \
    L.settable(); \
}

#define PUSH_METHOD(classname, enumvar) { \
    L.push(String(classname##_methods[enumvar].name)); \
    L.push((lua_Number) enumvar); \
    L.push((void *) sLua_##classname::classname##_proceed); \
    L.push((void *) NULL); \
    L.push((void *) classname##_methods); \
    L.push(true); \
    L.push(LuaHelpers<classname>::method_multiplex_closure, 5); \
    L.settable(); \
}

#define PUSH_METAMETHOD(classname, enumvar) { \
    L.push(String("__") + classname##_methods[enumvar].name); \
    L.push((lua_Number) enumvar); \
    L.push((void *) sLua_##classname::classname##_proceed); \
    L.push((void *) NULL); \
    L.push((void *) classname##_methods); \
    L.push(true); \
    L.push(LuaHelpers<classname>::method_multiplex_closure, 5); \
    L.settable(); \
}

#define PUSH_CONSTRUCTOR(classname, enumvar) { \
    callPushClassFirst = true; \
    L.push("new"); \
    L.push((lua_Number) enumvar); \
    L.push((void *) NULL); \
    L.push((void *) sLua_##classname::classname##_proceed_static); \
    L.push((void *) classname##_functions); \
    L.push(false); \
    L.push(LuaHelpers<classname>::method_multiplex_closure, 5); \
    L.settable(); \
}

#define PUSH_STATIC(classname, enumvar) { \
    callPushClassFirst = true; \
    L.push(classname##_functions[enumvar].name); \
    L.push((lua_Number) enumvar); \
    L.push((void *) NULL); \
    L.push((void *) sLua_##classname::classname##_proceed_static); \
    L.push((void *) classname##_functions); \
    L.push(false); \
    L.push(LuaHelpers<classname>::method_multiplex_closure, 5); \
    L.settable(); \
}

#define PUSH_FUNCTION(classname, enumvar) { \
    L.push(classname##_functions[enumvar].name); \
    L.push((lua_Number) enumvar); \
    L.push((void *) NULL); \
    L.push((void *) sLua_##classname::classname##_proceed_static); \
    L.push((void *) classname##_functions); \
    L.push(false); \
    L.push(LuaHelpers<classname>::method_multiplex_closure, 5); \
    L.settable(LUA_GLOBALSINDEX); \
}

#define PUSH_CLASS_DONE() L.pop()

#define CHECK_METHODS(classname) { \
    int i = 0; \
    while (classname##_methods[i].number != -1) { \
        lua_functypes_t & func = classname##_methods[i]; \
        AAssert(i == func.number, "Mismatched method in class " #classname); \
        i++; \
    } \
}

#define CHECK_FUNCTIONS(classname) { \
    int i = 0; \
    while (classname##_functions[i].number != -1) { \
        lua_functypes_t & func = classname##_functions[i]; \
        AAssert(i == func.number, "Mismatched function in class " #classname); \
        i++; \
    } \
}

template <class T>
T * LuaObjectFactory::getMe(Lua & L, int idx) { return L.recast<T>(idx); }

class LuaHelpersBase {
  protected:
    static void validate(const lua_functypes_t & entry, bool method, int n, Lua & L, const char * className);
};

template <class T>
class LuaHelpers : public LuaHelpersBase {
    typedef int (*proceed_t)(Lua & L, int n, T * obj, int caller);
    typedef int (*proceed_static_t)(Lua & L, int n, int caller);
  public:
    static int method_multiplex_closure(lua_State * __L) {
        Lua L(__L);
        int caller;
        proceed_t proceed;
        proceed_static_t proceed_static;
        lua_functypes_t * tab;
        bool method;

        caller = (int) L.tonumber(L.upvalue(1));
        proceed = (proceed_t) L.touserdata(L.upvalue(2));
        proceed_static = (proceed_static_t) L.touserdata(L.upvalue(3));
        tab = (lua_functypes_t *) L.touserdata(L.upvalue(4));
        method = L.toboolean(L.upvalue(5));

        return method_multiplex(caller, L, proceed, proceed_static, tab, method);
    }

  private:
    static int method_multiplex(int caller, Lua & L, proceed_t proceed, proceed_static_t proceed_static, lua_functypes_t * tab, bool method) {
        int add = method ? 1 : 0;
        int n = L.gettop() - add;
        T * obj = 0;
        static ClassName cn = ClassName((T *) 0);

        if (method)
            obj = LuaObjectFactory::getMe<T>(L);

        validate(tab[caller], method, n, L, cn.c_str());

        if (method) {
            return proceed(L, n, obj, caller);
        } else {
            return proceed_static(L, n, caller);
        }
    }
};

template<class T> T * lua_recast(Lua & L, int n = 1) { return L.recast<T>(n); }

template<class T>
void LuaObjectFactory::pushObj(Lua & L, T * obj, const char * name) {
    pushMe(L, new (L.newuser(sizeof(LuaObject<T>))) LuaObject<T>(obj), name);
}

};
