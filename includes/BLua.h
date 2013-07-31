#pragma once

#include <functional>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <Exceptions.h>
#include <Handle.h>
#include <Task.h>

namespace Balau {

class Lua;

class LuaObjectBase {
  public:
      virtual void destroy() { }
    void detach() { m_detached = true; }
  protected:
    bool isDetached() { return m_detached; }
  private:
    bool m_detached = false;
};

template<class T>
class LuaObject : public LuaObjectBase {
  public:
      LuaObject(T * obj) : m_obj(obj) { }
      virtual void destroy() { if (!isDetached() && m_obj) delete m_obj; detach(); }
    T * getObj() { return m_obj; }
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
    static void pushIt(Lua & L, const char * name, lua_CFunction func);
    static void pushMeta(Lua & L, const char * name, lua_CFunction func);
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
    int wrap_open(openlualib_t open) { int n = gettop(); int r = open(L); while (n < gettop()) remove(n); return r; }
    void openlib(const String & libname, const struct luaL_reg *l, int nup) { luaL_openlib(L, libname.to_charp(), l, nup); }

    void setCallWrap(lua_CallWrapper wrapper);
    void declareFunc(const char * funcName, lua_CFunction f, int tableIdx = LUA_GLOBALSINDEX);

    void call(const char * funcName, int tableIdx = LUA_GLOBALSINDEX, int nArgs = 0);
    void call(int nArgs = 0) { resume(nArgs); }

    void push() { checkstack(); lua_pushnil(L); }
    void push(lua_Number n) { checkstack(); lua_pushnumber(L, n); }
    void push(const String & s) { checkstack(); lua_pushlstring(L, s.to_charp(), s.strlen()); }
    void push(bool b) { checkstack(); lua_pushboolean(L, b); }
    void push(const char * str, int size = -1) { if (size < 0) size = strlen(str); checkstack(); lua_pushlstring(L, str, size); }
    void push(void * p) { checkstack(); lua_pushlightuserdata(L, p); }
    void push(lua_CFunction f, int n = 0) { checkstack(); lua_pushcclosure(L, f, n); }
    void pop(int idx = 1) { lua_pop(L, idx); }
    int checkstack(int extra = 1) { return lua_checkstack(L, extra); }

    int next(int t = -2) { return lua_next(L, t); }
    void copy(int n = -1) { checkstack(); lua_pushvalue(L, n); }
    void remove(int n = 1) { lua_remove(L, n); }
    void insert(int n = 1) { checkstack(); lua_insert(L, n); }
    void replace(int n = 1) { lua_replace(L, n); }
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
    void error(const char * msg);
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
    bool yielded() { return lua_status(L) == LUA_YIELD; }
    bool resume(int nargs = 0) throw (GeneralException);
    void showstack(int level = M_INFO);
    void showerror();
    int getmetatable(int i = -1) { checkstack(); return lua_getmetatable(L, i); }
    int setmetatable(int i = -2) { return lua_setmetatable(L, i); }
    int sethook(lua_Hook func, int mask, int count) { return lua_sethook(L, func, mask, count); }
    void weaken();

    template<class T>
    T * recast(int n = 1) {
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
    void dumpvars_r(IO<Handle> out, int idx, int depth = 0) throw (GeneralException);

    lua_State * L;

    friend class LuaStatics;

    Lua & operator=(const Lua &) = delete;
      Lua(const Lua &) = delete;
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

#define DECLARE_METHOD(classname, enumvar) static int method_##enumvar(lua_State * L) { \
    return LuaHelpers<classname>::method_multiplex( \
        enumvar, \
        L, \
        sLua_##classname::classname##_proceed, \
        NULL, \
        classname##_methods, \
        true); \
    }

#define DECLARE_CONSTRUCTOR(classname, enumvar) static int constructor(lua_State * L) { \
    return LuaHelpers<classname>::method_multiplex( \
        enumvar, \
        L, \
        NULL, \
        sLua_##classname::classname##_proceed_statics, \
        classname##_functions, \
        false); \
    }

#define DECLARE_FUNCTION(classname, enumvar) static int function_##enumvar(lua_State * L) { \
    return LuaHelpers<classname>::method_multiplex( \
        enumvar, \
        L, \
        NULL, \
        sLua_##classname::classname##_proceed_statics, \
        classname##_functions, \
        false); \
    }

#define DECLARE_STATIC(classname, enumvar) static int static_##enumvar(lua_State * L) { \
    return LuaHelpers<classname>::method_multiplex( \
        enumvar, \
        L, \
        NULL, \
        sLua_##classname::classname##_proceed_statics, \
        classname##_functions, \
        false); \
    }

#define PUSH_METHOD(classname, enumvar) pushIt( \
    L, \
    classname##_methods[enumvar].name, \
    sLua_##classname::method_##enumvar)

#define PUSH_METAMETHOD(classname, enumvar) pushMeta( \
    L, \
    String("__") + classname##_methods[enumvar].name, \
    sLua_##classname::method_##enumvar)

#define PUSH_CLASS(classname) \
    bool classPushed = true; \
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
    bool classPushed = true; \
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

#define PUSH_CONSTRUCTOR(classname, enumvar) L.declareFunc("new", sLua_##classname::constructor, -1)

#define PUSH_STATIC(classname, enumvar) { \
    AAssert(classPushed, "Please call PUSH_(SUB)CLASS first"); \
    L.declareFunc( \
    classname##_functions[enumvar].name, \
    sLua_##classname::static_##enumvar, \
    -1); \
}

#define PUSH_FUNCTION(classname, enumvar) L.declareFunc( \
    classname##_functions[enumvar].name, \
    sLua_##classname::function_##enumvar)

#define PUSH_SUBFUNCTION(classname, enumvar, array) L.declareFunc( \
    classname##_functions[enumvar].name, \
    sLua_##classname::function_##enumvar, \
    array)

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
  public:
    static bool resume(Lua & L);
    static Events::BaseEvent * getEvent(Lua & L);
  protected:
    static void validate(const lua_functypes_t & entry, bool method, int n, Lua & L, const char * className);
    static void pushContext(Lua & L, std::function<int(Lua & L)> context, Events::BaseEvent * evt);
  private:
      LuaHelpersBase(std::function<int(Lua & L)> context, Events::BaseEvent * evt) : m_context(context), m_evt(evt) { }
    std::function<int(Lua & L)> m_context;
    Events::BaseEvent * m_evt;
};

template <class T>
class LuaHelpers : public LuaHelpersBase {
  public:
    static int method_multiplex(int caller, lua_State * __L, int (*proceed)(Lua & L, int n, T * obj, int caller), int (*proceed_static)(Lua & L, int n, int caller), lua_functypes_t * tab, bool method) {
        Lua L(__L);
        return method_multiplex(caller, L, proceed, proceed_static, tab, method);
    }

  private:
    static int method_multiplex(int caller, Lua & L, int (*proceed)(Lua & L, int n, T * obj, int caller), int (*proceed_static)(Lua & L, int n, int caller), lua_functypes_t * tab, bool method) {
        int r;

        try {
            r = method_multiplex_internal(caller, L, proceed, proceed_static, tab, method);
        }
        catch (EAgain & e) {
            pushContext(L, [caller, proceed, proceed_static, tab, method](Lua & L) -> int { return method_multiplex_resume(caller, L, proceed, proceed_static, tab, method); }, e.getEvent());
            r = L.yield(L.gettop());
        }

        return r;
    }

    static int method_multiplex_resume(int caller, Lua & L, int (*proceed)(Lua & L, int n, T * obj, int caller), int (*proceed_static)(Lua & L, int n, int caller), lua_functypes_t * tab, bool method) {
        int r;

        try {
            r = method_multiplex_internal(caller, L, proceed, proceed_static, tab, method);
        }
        catch (EAgain & e) {
            pushContext(L, [caller, proceed, proceed_static, tab, method](Lua & L) -> int { return method_multiplex_resume(caller, L, proceed, proceed_static, tab, method); }, e.getEvent());
            r = -1;
        }

        return r;
    }

    static int method_multiplex_internal(int caller, Lua & L, int (*proceed)(Lua & L, int n, T * obj, int caller), int (*proceed_static)(Lua & L, int n, int caller), lua_functypes_t * tab, bool method) {
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
