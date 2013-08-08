#include <memory>
#include <stdlib.h>
#include "BLua.h"
#include "Printer.h"
#include "Input.h"
#include "Buffer.h"
#include "HelperTasks.h"
#include "StacklessTask.h"
#include "TaskMan.h"

extern "C" {
#include <lualib.h>
#include <luajit.h>
}

#ifndef BUFFERSIZE
#define BUFFERSIZE 2048
#endif

namespace {

class LuaYield : public Balau::GeneralException {
  public:
      LuaYield(Balau::Future<int> & f) : GeneralException(), m_f(f) { }
    Balau::Future<int> m_f;
};

};

namespace Balau {

class LuaStatics {
  public:
    static const char * getF(lua_State * L, void *, size_t *);

    static int callwrap(lua_State * L, lua_CFunction);
    static int collector(lua_State * L);
    static int destructor(lua_State * L);

    static int dumpvars(lua_State * L);

    static int iconv(lua_State * L);
    static int mkdir(lua_State * L);
    static int time(lua_State * L);
    static int getenv(lua_State * L);
    static int setenv(lua_State * L);
    static int unsetenv(lua_State * L);

    static int hex(lua_State * L);

    static int globalindex(lua_State * L);

    static int print(lua_State * L);
};

};

int Balau::LuaStatics::hex(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();
    int x;
    String r;

    if (((n != 1) && (n != 2)) || !L.isnumber(1) || ((n == 2) && !L.isstring(2)))
        L.error("Incorrect arguments to function `hex'");

    x = L.tonumber(1);
    String fmt = n == 2 ? L.tostring() : "%02x";
    r.set(fmt.to_charp(), x);

    L.push(r);

    return 1;
}

int Balau::LuaStatics::dumpvars(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();
    String prefix;
    String varname;

    if ((n > 3) || (n < 2) || !L.isobject(1) ||
        ((n == 2) && !L.isstring(2)) ||
        ((n == 3) && !L.isstring(3) && !L.istable(2) && !L.isstring(2)))
        L.error("Incorrect arguments to function `dumpvars'");

    prefix = L.tostring(n);

    if (n == 3)
        L.pop();

    if (L.isstring(2))
        L.getglobal(L.tostring(2).to_charp());

    IO<Handle> h(L.recast<Balau::Handle>());

    if (h.isA<Buffer>()) {
        L.dumpvars(h, prefix);
    } else {
        IO<Handle> s(new Buffer());
        L.dumpvars(h, prefix);
        Task * t = new CopyTask(s, h);
        std::shared_ptr<Events::TaskEvent> evt(new Events::TaskEvent(t));
        return L.yield(Future<int>([evt, s]() mutable {
            for (;;) {
                if (evt->gotSignal()) {
                    evt->ack();
                    s->close();
                } else {
                    Task::operationYield(evt.get(), Task::INTERRUPTIBLE);
                }
            }
            return 0;
        }));
    }

    return 0;
}

int Balau::LuaStatics::iconv(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();
    String str, from, to;

    if ((n != 3) || !L.isstring(1) || !L.isstring(2) || !L.isstring(3))
        L.error("Incorrect arguments to function `string.iconv'");

    str = L.tostring(1);
    from = L.tostring(2);
    to = L.tostring(3);

    str.iconv(from, to);

    L.push(str);

    return 1;
}

int Balau::LuaStatics::mkdir(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();
    String dirname;

    if (n != 1)
        L.error("Incorrect arguments to function `mkdir'");

    dirname = L.tostring(1);

    Balau::FileSystem::mkdir(dirname.to_charp());

    return 0;
}

int Balau::LuaStatics::time(lua_State * __L) {
    Lua L(__L);

    L.push((lua_Number) ::time(NULL));

    return 1;
}

int Balau::LuaStatics::getenv(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();

    if (n != 1)
        L.error("Incorrect arguments to function `getenv'");

#ifdef _WIN32
    char buffer[BUFSIZ + 1];
    if (GetEnvironmentVariable(L.tostring(1).to_charp(), buffer, BUFSIZ)) {
        L.push(buffer);
    } else {
        L.push();
    }
#else
    char * var = ::getenv(L.tostring(1).to_charp());
    if (var) {
        L.push(var);
    } else {
        L.push();
    }
#endif

    return 1;
}

int Balau::LuaStatics::setenv(lua_State * __L) {
    Lua L(__L);
    int n = L.gettop();

    if (n != 2) {
        L.error("Incorrect arguments to function `setenv'");
    }

#ifdef _WIN32
    SetEnvironmentVariable(L.tostring(1).to_charp(), L.tostring(2).to_charp());
#else
    ::setenv(L.tostring(1).to_charp(), L.tostring(2).to_charp(), 1);
#endif

    return 0;
}

int Balau::LuaStatics::unsetenv(lua_State * __L) {
    Lua L(__L);

    int n = L.gettop();
    if (n != 1)
        L.error("Incorrect arguments to function `unsetenv'");

#ifdef _WIN32
    SetEnvironmentVariable(L.tostring(1).to_charp(), NULL);
#else
    ::unsetenv(L.tostring(1).to_charp());
#endif

    return 0;
}

int Balau::LuaStatics::print(lua_State * __L) {
    Lua L(__L);

    int n = L.gettop();
    int i;
    for (i = 1; i <= n; i++) {
        const char *s;
        s = lua_tostring(__L, i);
        if (s == NULL)
            L.error("`tostring' must return a string to `print'");
        if (i > 1)
            Printer::print("\t");
        Printer::print("%s", s);
        L.pop();
    }
    Printer::print("\n");
    return 0;
}

static char s_signature = 'B';

int Balau::LuaStatics::callwrap(lua_State * __L, lua_CFunction func) {
    Lua L(__L);
    int r = 0;

    try {
        r = func(__L);
    }
    catch (LuaException & e) {
        L.error(String("LuaException: ") + e.getMsg());
    }
    catch (LuaYield & y) {
        Future<int> * f = new Future<int>(y.m_f);
        L.push((void *) f);
        L.push((void *) &s_signature);
        r = L.yield(L.gettop());
    }
    catch (EAgain & e) {
        L.error("The C callback from Lua has thrown an EAgain - you can't do that, you have to wrap it using Lua::yield. The application will now crash, because this task will stop, but the event in the EAgain will later try to wake it up.");
    }
    catch (GeneralException & e) {
        L.processException(e);
    }
// LuaJIT sucks sometime.
//    catch (...) {
//        L.error("Unknown C++ exception");
//    }

    return r;
}

namespace {

class CollectorTask : public Balau::StacklessTask {
  public:
      CollectorTask(Balau::LuaObjectBase * obj) : m_obj(obj) { }
  private:
    virtual const char * getName() const override { return "CollectorTask"; }
    virtual void Do() override { delete m_obj; }
    Balau::LuaObjectBase * m_obj;
};

};

int Balau::LuaStatics::collector(lua_State * __L) {
    Lua L(__L);
    LuaObjectBase * o = (LuaObjectBase *) L.touserdata();
    Task * collector = o->spawnCollector();
    if (collector)
        TaskMan::registerTask(collector);
    return 0;
}

int Balau::LuaStatics::destructor(lua_State * __L) {
    Lua L(__L);
    L.push("__obj");
    L.copy();
    L.gettable(-3, true);
    LuaObjectBase * o = (LuaObjectBase *) L.touserdata();
    L.pop();
    L.push();
    L.settable(-3, true);
    L.pop();
    return L.yield(Future<int>([o]() { o->destroy(); return 0; }));
}

void Balau::Lua::setCallWrap(lua_CallWrapper wrapper) {
    push((void *) wrapper);
    luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
    pop();
}

Balau::Lua::Lua() : L(lua_open()) {
    setCallWrap(LuaStatics::callwrap);
    declareFunc("hex", LuaStatics::hex);
    declareFunc("dumpvars", LuaStatics::dumpvars);
    declareFunc("print", LuaStatics::print);
    push("BLUA_THREADS");
    newtable();
    settable(LUA_REGISTRYINDEX);
}

Balau::Lua & Balau::Lua::operator=(Lua && oL) {
    if (this == &oL)
        return *this;

    AAssert(!L, "Can't assign a Lua VM to another one.");

    L = oL.L;
    oL.L = NULL;

    return *this;
}

void Balau::Lua::close() {
    AAssert(L, "Can't close an already closed VM");

    lua_close(L);
    L = NULL;
}

#define IntPoint(p)  ((unsigned int)(lu_mem)(p))
typedef size_t lu_mem;

void Balau::Lua::weaken() {
    push("BLUA_THREADS");               // -1 = "BLUA_THREADS"
    gettable(LUA_REGISTRYINDEX);        // -1 = BLUA_THREADS
    push((lua_Number) IntPoint(L));     // -2 = BLUA_THREADS, -1 = key-Lt
    push();                             // -3 = BLUA_THREADS, -2 = key-Lt, -1 = nil
    settable();                         // -1 = BLUA_THREADS
    pop();
}

void Balau::Lua::open_base() {
    int n = gettop();
    luaopen_base(L);
    push("mkdir");
    push(LuaStatics::mkdir);
    settable();
    push("time");
    push(LuaStatics::time);
    settable();
    push("getenv");
    push(LuaStatics::getenv);
    settable();
    push("setenv");
    push(LuaStatics::setenv);
    settable();
    push("unsetenv");
    push(LuaStatics::unsetenv);
    settable();
    push("print");
    push(LuaStatics::print);
    settable();
    while (n < gettop()) pop();
    push("mkdir");
    push(LuaStatics::mkdir);
    settable(LUA_GLOBALSINDEX);
    push("time");
    push(LuaStatics::time);
    settable(LUA_GLOBALSINDEX);
    push("getenv");
    push(LuaStatics::getenv);
    settable(LUA_GLOBALSINDEX);
    push("setenv");
    push(LuaStatics::setenv);
    settable(LUA_GLOBALSINDEX);
    push("unsetenv");
    push(LuaStatics::unsetenv);
    settable(LUA_GLOBALSINDEX);
    push("print");
    push(LuaStatics::print);
    settable(LUA_GLOBALSINDEX);
}

void Balau::Lua::open_table() {
    int n = gettop();
    luaopen_table(L);
    while (n < gettop()) pop();
}

void Balau::Lua::open_string() {
    int n = gettop();
    luaopen_string(L);
    push("iconv");
    push(LuaStatics::iconv);
    settable();
    while (n < gettop()) pop();
}

void Balau::Lua::open_math() {
    int n = gettop();
    luaopen_math(L);
    while (n < gettop()) pop();
}

void Balau::Lua::open_debug() {
    int n = gettop();
    luaopen_debug(L);
    while (n < gettop()) pop();
}

void Balau::Lua::open_jit() {
    int n = gettop();
    luaopen_jit(L);
    while (n < gettop()) pop();
}

void Balau::Lua::open_ffi() {
    int n = gettop();
    luaopen_ffi(L);
    while (n < gettop()) pop();
}

void Balau::Lua::open_bit() {
    int n = gettop();
    luaopen_bit(L);
    while (n < gettop()) pop();
}

extern "C" {
int luaopen_lcrypt(lua_State *L);
}

void Balau::Lua::open_lcrypt() {
    int n = gettop();
    luaopen_lcrypt(L);
    while (n < gettop()) pop();
}

void Balau::Lua::declareFunc(const char * name, lua_CFunction f, int i) {
    checkstack(2);
    lua_pushstring(L, name);
    lua_pushcfunction(L, f);
    if ((i < 0) && (i > LUA_REGISTRYINDEX))
        i += 2;
    lua_settable(L, i);
}

void Balau::Lua::call(const char * f, int i, int nargs) {
    checkstack(1);
    lua_pushstring(L, f);
    lua_gettable(L, i);
    lua_insert(L, -1 - nargs);
    call(nargs);
}

void Balau::Lua::settable(int i, bool raw) {
    if (raw) {
        lua_rawset(L, i);
    } else {
        lua_settable(L, i);
    }
}

void Balau::Lua::gettable(int i, bool raw) {
    if (raw) {
        lua_rawget(L, i);
    } else {
        lua_gettable(L, i);
    }
}

void Balau::Lua::getglobal(const char * name) throw (GeneralException) {
    push(name);
    gettable(LUA_GLOBALSINDEX);
}

void Balau::Lua::pushLuaContext() {
    String whole_msg;
    struct lua_Debug ar;
    bool got_error = false;
    int level = 0;

    do {
        if (lua_getstack(L, level, &ar) == 1) {
            if (lua_getinfo(L, "nSl", &ar) != 0) {
                push(String("at ") + ar.source + ":" + ar.currentline + " (" + (ar.name ? ar.name : "[top]") + ")");
            } else {
                got_error = true;
            }
        } else {
            got_error = true;
        }
        level++;
    } while (!got_error);
}

void Balau::Lua::error(const char * msg) throw (GeneralException) {
    push(msg);

    if (yielded()) {
        pushLuaContext();
        showerror();

        throw LuaException("Runtime error while running yielded C code.");
    } else {
        lua_error(L);
    }
}

bool Balau::Lua::isobject(int i) {
    bool r = false;
    if (istable(i)) {
        push("__obj");
        gettable(i);
        r = isuserdata();
        pop();
    } else {
        r = isnil(i);
    }
    return r;
}

Balau::String Balau::Lua::tostring(int i) {
    const char * r = 0;
    size_t l = -1;
    switch (type(i)) {
    case LUA_TNIL:
        r = "(nil)";
        break;
    case LUA_TBOOLEAN:
        r = toboolean(i) ? "true" : "false";
        break;
    case LUA_TNUMBER:
        return String(tonumber(i));
        break;
    default:
        r = lua_tolstring(L, i, &l);
    }
    return String(r ? r : "<lua-NULL>", l);
}

struct LoadF {
    Balau::IO<Balau::Handle> f;
    char buff[BUFFERSIZE];
    Balau::GeneralException * exception = NULL;
};

const char * Balau::LuaStatics::getF(lua_State * L, void * ud, size_t * size) {
    LoadF * lf = (LoadF *)ud;

    try {
        *size = lf->f->read(lf->buff, BUFFERSIZE);
    }
    catch (GeneralException & e) {
        lf->exception = new GeneralException(e);
        AssertHelper("LuaJIT is lame.");
    }
    return (*size > 0) ? lf->buff : NULL;
}

Balau::String Balau::Lua::escapeString(const String & s) {
    String r = "";
    int i;
    for (i = 0; i < s.strlen(); i++) {
        switch(s[i]) {
        case '"': case '\\':
            r += '\\';
            r += s[i];
            break;
        case '\n':
            r += "\\n";
            break;
        case '\r':
            r += "\\r";
            break;
        case '\0':
            r += "\\000";
            break;
        default:
            r += s[i];
        }
    }
    return r;
}

void Balau::Lua::load(IO<Handle> h, bool docall) throw (GeneralException) {
    LoadF lf;
    int status;

    lf.f = h;

    checkstack();
    String name = h->getName();
    IO<Input> i = h;
    if (!i.isNull())
        name = String("@") + i->getFName();
    {
        Task::SimpleContext sc;
        status = lua_load(L, LuaStatics::getF, &lf, name.to_charp());
    }

    if (status) {
        if (lf.exception) {
            GeneralException copy(*lf.exception);
            delete lf.exception;
            throw copy;
        } else {
            pushLuaContext();
            showerror();
            throw LuaException(String("Error loading lua chunk from Handle `") + h->getName() + "'");
        }
    }

    if (docall)
        call();
}

void Balau::Lua::load(const String & s, bool docall) throw (GeneralException) {
    const char * buf = s.to_charp();
    int status;

    status = luaL_loadbuffer(L, buf, s.strlen(), buf);

    if (status) {
        pushLuaContext();
        showerror();
        throw LuaException(String("Error loading lua string `") + s + "'");
    }

    if (docall)
        call();
}

void Balau::Lua::dumpvars(IO<Handle> h, const String & prefix, int i) {
    if (h.isA<Buffer>()) {
        dumpvars_i(h, prefix, i);
    } else {
        Task::SimpleContext sc;
        dumpvars_i(h, prefix, i);
    }
}

void Balau::Lua::dumpvars_i(IO<Handle> h, const String & prefix, int i) {
    h->writeString(prefix);
    h->writeString(" = {\n");
    dumpvars_r(h, i);
    h->writeString("}\n");
}

void Balau::Lua::dumpvars_r(IO<Handle> h, int i, int depth) throw (GeneralException) {
    int j;
    String t;
    bool dump_value;

    if (lua_type(L, i) != LUA_TTABLE)
        throw LuaException("Error dumping variables: variable isn't a table.");

    push();

    if (i < 0)
        i--;

    depth++;

    checkstack();
    while(lua_next(L, i) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING)
            if (String(lua_tostring(L, -2)) == String("_G"))
                continue;
        for (j = 0; j < depth; j++)
            h->writeString("  ");

        dump_value = true;

        // first, let's dump the key.
        switch(lua_type(L, -2)) {
        case LUA_TNONE:
            throw LuaException("Internal error: got invalid index for key while dumpvars.");
        case LUA_TNIL:
            throw LuaException("Internal error: got nil index for key while dumpvars.");
        case LUA_TNUMBER:
            t.set("[%.14g] = ", lua_tonumber(L, -2));
            break;
        case LUA_TBOOLEAN:
            t = String("[") + (lua_toboolean(L, -2) ? "true" : "false") + "] = ";
            break;
        case LUA_TSTRING:
            t = String("[\"") + escapeString(lua_tostring(L, -2)) + String("\"] = ");
            break;
        case LUA_TTABLE:
            t = "-- [a table]\n";
            dump_value = false;
            break;
        case LUA_TFUNCTION:
            t = "-- [function() ... end]\n";
            dump_value = false;
            break;
        case LUA_TUSERDATA:
            t = "-- [userdata]\n";
            dump_value = false;
            break;
        case LUA_TTHREAD:
            t = "-- [thread]\n";
            dump_value = false;
            break;
        default:
            throw LuaException("Internal error: got unknow index for key while dumpvars.");
        }

        // Seems that we can't dump that key.
        if (!dump_value) {
            pop();
            h->writeString(t);
            continue;
        }

        // let's look at the value: if it's a function, a userdata or a thread, we can't dump it.
        if ((lua_type(L, -1) == LUA_TFUNCTION) ||
            (lua_type(L, -1) == LUA_TUSERDATA) ||
            (lua_type(L, -1) == LUA_TTHREAD))
            h->writeString("-- ");

        h->writeString(t);

        // Finally, let's dump the value.
        switch(lua_type(L, -1)) {
        case LUA_TNONE:
            throw LuaException("Internal error: got invalid index for value while dumpvars.");
        case LUA_TNIL:
            h->writeString("nil,\n");
        case LUA_TNUMBER:
            t.set("%.14g,\n", lua_tonumber(L, -1));
            h->writeString(t);
            break;
        case LUA_TBOOLEAN:
            h->writeString(lua_toboolean(L, -1) ? "true" : "false");
            h->writeString(",\n");
            break;
        case LUA_TSTRING:
            h->writeString("\"");
            h->writeString(escapeString(lua_tostring(L, -1)));
            h->writeString("\",\n");
            break;
        case LUA_TTABLE:
            h->writeString("{\n");
            dumpvars_r(h, -1, depth);
            for (j = 0; j < depth; j++)
                h->writeString("  ");
            h->writeString("},\n");
            break;
        case LUA_TFUNCTION:
            h->writeString("function() ... end\n");
            break;
        case LUA_TUSERDATA:
            h->writeString("userdata ...\n");
            break;
        case LUA_TTHREAD:
            h->writeString("thread ...\n");
            break;
        default:
            throw LuaException("Internal error: got unknow index for value while dumpvars.");
        }

        pop();
    }
}

Balau::Lua Balau::Lua::thread(bool saveit) {
    checkstack();
    lua_State * L1 = lua_newthread(L);
    if (saveit) {                               // -1 = thread
        push("BLUA_THREADS");                   // -2 = thread, -1 = "BLUA_THREADS"
        gettable(LUA_REGISTRYINDEX);            // -2 = thread, -1 = BLUA_THREADS
        push((lua_Number) IntPoint(L1));        // -3 = thread, -2 = BLUA_THREADS, -1 = key-Lt
        copy(-3);                               // -4 = thread, -3 = BLUA_THREADS, -2 = key-Lt, -1 = thread
        settable();                             // -2 = thread, -1 = BLUA_THREADS
        pop();                                  // -1 = thread
    }
    return Lua(L1);
}

bool Balau::Lua::resume(int nargs) throw (GeneralException) {
    int r;

    if (resumeC(nargs))
        yieldC();

    r = lua_resume(L, nargs);

    if (r == LUA_YIELD) {
        yieldC();
        return true;
    }

    if (r == 0)
        return false;

    pushLuaContext();
    showerror();

    switch(r) {
    case 0:
        return false;
    case LUA_YIELD:
        return true;
    case LUA_ERRRUN:
        throw LuaException("Runtime error while running LUA code.");
    case LUA_ERRMEM:
        throw LuaException("Memory allocation error while running LUA code.");
    case LUA_ERRERR:
        throw LuaException("Error in Error function.");
    case LUA_ERRSYNTAX:
        throw LuaException("Syntax error in Lua code.");
    default:
        throw LuaException(String("Unknow error while running LUA code (err code: ") + String(r) + ")");
    }
}

bool Balau::Lua::resumeC(int & nargs) {
    if (!yielded())
        return false;

    if (gettop() < 2)
        return false;

    if (!islightuserdata(-1))
        return false;

    if (!islightuserdata(-2))
        return false;

    void * s = touserdata();
    if (s != &s_signature)
        return false;

    pop();

    Future<int> * p = (Future<int> *) touserdata();
    Future<int> f(*p);
    delete p;
    pop();

    try {
        nargs = f.get();
    }
    catch (LuaException & e) {
        error(String("LuaException: ") + e.getMsg());
    }
    catch (EAgain & e) {
        Future<int> * p = new Future<int>(f);
        p->m_evt = e.getEvent();
        push((void *) p);
        push((void *) &s_signature);
    }
    catch (LuaYield & y) {
        Future<int> * p = new Future<int>(y.m_f);
        push((void *) p);
        push((void *) &s_signature);
    }
    catch (Balau::GeneralException & e) {
        processException(e);
    }

    return true;
}

int Balau::Lua::yield(Future<int> f) throw (GeneralException) {
    int r = 0;
    try {
        r = f.get();
    }
    catch (EAgain & e) {
        LuaYield y(f);
        y.m_f.m_evt = e.getEvent();
        throw y;
    }
    return r;
}

void Balau::Lua::yieldC() throw (GeneralException) {
    if (!yielded())
        return;

    if (gettop() < 2)
        return;

    if (!islightuserdata(-1))
        return;

    if (!islightuserdata(-2))
        return;

    void * s = touserdata();
    if (s != &s_signature)
        return;

    Future<int> * p = (Future<int> *) touserdata(-2);

    IAssert(p->m_evt, "There should always be an event here at that point...");
    Task::operationYield(p->m_evt, Task::STACKLESS);
    IAssert(false, "That point shouldn't be reached.");
}

void Balau::Lua::processException(GeneralException & e) {
    const char * details = e.getDetails();
    if (details)
        push(details);
    auto trace = e.getTrace();
    for (String & str : trace)
        push(str);
    error(String("General Exception: ") + e.getMsg());
}

void Balau::Lua::showstack(int level) {
    int n = lua_gettop(L);
    int i;
    String t;

    if (n == 0) {
        Printer::log(level, "Stack empty");
        return;
    }

    for (i = 1; i <= n; i++) {
        switch(lua_type(L, i)) {
        case LUA_TNONE:
            t = "Invalid";
            break;
        case LUA_TNIL:
            t = "(Nil)";
            break;
        case LUA_TNUMBER:
            t.set("(Number) %f", lua_tonumber(L, i));
            break;
        case LUA_TBOOLEAN:
            t = String("(Bool)   ") + (lua_toboolean(L, i) ? "true" : "false");
            break;
        case LUA_TSTRING:
            t = String("(String) ") + lua_tostring(L, i);
            break;
        case LUA_TTABLE:
            t = "(Table)";
            break;
        case LUA_TFUNCTION:
            t = "(Function)";
            break;
        case LUA_TUSERDATA:
            t = "(Userdata)";
            break;
        case LUA_TLIGHTUSERDATA:
            t = "(Lightuserdata)";
            break;
        case LUA_TTHREAD:
            t = "(Thread)";
            break;
        default:
            t = "Unknown";
        }

        Printer::log(level, String(i) + ": " + t);
    }
}

void Balau::Lua::showerror() {
    Printer::log(M_ERROR, "Lua object: Got an LUA error, inspecting stack.");

    showstack(M_ERROR);
}

void Balau::LuaObjectFactory::push(Lua & L) {
    AAssert(!(m_pushed && m_wantsDestruct), "Error: object is owned by the LUA script and can not be pushed.");
    L.newtable();
    pushObjectAndMembers(L);
    m_pushed = true;
}

void Balau::LuaObjectFactory::pushMe(Lua & L, LuaObjectBase * o, const char * objname) {
    L.push("__obj");
    L.insert(-2);
    pushMeta(L, "__gc", LuaStatics::collector);
    L.settable(-3, true);
    if (objname && *objname) {
        L.push("__type");
        L.getglobal(objname);
        L.settable(-3, true);
    }
    pushMethod(L, "destroy", LuaStatics::destructor);
    if (!m_wantsDestruct)
        o->detach();
}

Balau::LuaObjectBase * Balau::LuaObjectFactory::getMeInternal(Lua & L, int i) {
    LuaObjectBase * o;

    if (L.istable(i)) {
        L.push("__obj");
        L.gettable(i, true);
        if (!(o = (LuaObjectBase *) L.touserdata()))
            L.error("Table is not an object.");
        L.pop();
    } else if (L.isnil(i)) {
        o = NULL;
    } else {
        L.error("Not an object (not even a table).");
    }

    return o;
}

void Balau::LuaObjectFactory::pushMethod(Lua & L, const char * s, int strSize, lua_CFunction f, int upvalues) {
    if (upvalues == 0) {
        L.push(s, strSize);
        L.push(f);
    } else {
        L.push(f, upvalues);
        L.push(s, strSize);
        L.insert(-2);
    }
    L.settable(-3, true);
}

void Balau::LuaObjectFactory::pushMeta(Lua & L, const char * s, int strSize, lua_CFunction f, int upvalues) {
    if (!L.getmetatable())
        L.newtable();
    if (upvalues == 0) {
        L.push(s);
        L.push(f);
    } else {
        L.push(f, upvalues);
        L.push(s, strSize);
        L.insert(-2);
    }
    L.settable();
    L.setmetatable();
}

void Balau::LuaObjectFactory::pushDestruct(Lua & L) {
    AAssert(!m_pushed, "Error: can't push destructor, object already pushed");
    m_wantsDestruct = true;
    push(L);
}

void Balau::LuaHelpersBase::validate(const lua_functypes_t & entry, bool method, int n, Lua & L, const char * className) {
    bool invalid = false, arg_valid;
    int i, j, mask;
    int add = method ? 1 : 0;

    if ((n < entry.minargs) || (n > entry.maxargs)) {
        invalid = true;
    } else {
        for (i = 0; i < entry.maxargs && !invalid; i++) {
            if (n >= (i + 1)) {
                arg_valid = false;
                for (j = 0; j < MAX_TYPE && !arg_valid; j++) {
                    mask = 1 << j;
                    if (entry.argtypes[i] & mask) {
                        switch(mask) {
                            case BLUA_OBJECT:
                                if (L.istable(i + 1 + add)) {
                                    L.push("__obj");
                                    L.gettable(i + 1 + add);
                                    arg_valid = L.isuserdata();
                                    L.pop();
                                } else {
                                    arg_valid = L.isnil(i + 1 + add);
                                }
                                break;
                            case BLUA_TABLE:
                                arg_valid = L.istable(i + 1 + add);
                                break;
                            case BLUA_BOOLEAN:
                                arg_valid = L.isboolean(i + 1 + add);
                                break;
                            case BLUA_NUMBER:
                                arg_valid = L.isnumber(i + 1 + add);
                                break;
                            case BLUA_STRING:
                                arg_valid = L.isstring(i + 1 + add);
                                break;
                            case BLUA_FUNCTION:
                                arg_valid = L.isfunction(i + 1 + add);
                                break;
                            case BLUA_NIL:
                                arg_valid = L.isnil(i + 1 + add);
                                break;
                            case BLUA_USERDATA:
                                arg_valid = L.isuserdata(i + 1 + add) || L.islightuserdata(i + 1 + add);
                                break;
                        }
                    }
                }
                invalid = !arg_valid;
            }
        }
    }

    if (invalid) {
        if (method) {
            L.error(String("Invalid arguments to method `") + className + "::" + entry.name + "'");
        } else {
            L.error(String("Invalid arguments to function `") + className + " " + entry.name + "'");
        }
    }
}
