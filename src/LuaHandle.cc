#include "LuaHandle.h"
#include "Handle.h"

typedef Balau::IO<Balau::Handle> IOHandle;
typedef IOHandle IOInput;

// Handle exports

enum IOHandle_methods_t {
    IOHANDLE_CLOSE,
};

struct Balau::lua_functypes_t IOHandle_methods[] = {
    { IOHANDLE_CLOSE,           "close",          0, 0, { } },
    { -1, 0, 0, 0, 0 },
};

struct sLua_IOHandle {
    static int IOHandle_proceed(Balau::Lua & L, int n, IOHandle * obj, int caller);
};

int sLua_IOHandle::IOHandle_proceed(Balau::Lua & L, int n, IOHandle * obj, int caller) {
    int r = 0;
    Balau::IO<Balau::Handle> h = *obj;

    switch (caller) {
    case IOHANDLE_CLOSE:
        return L.yield(Balau::Future<int>([h]() mutable { h->close(); return 0; }));
        break;
    }

    return r;
}

void Balau::LuaHandleFactory::pushStatics(Balau::Lua & L) {
    CHECK_METHODS(IOHandle);
    PUSH_CLASS(Handle);
    PUSH_CLASS_DONE();
}

void Balau::LuaHandleFactory::pushObjectAndMembers(Lua & L) {
    pushObj(L, m_obj, "Handle");

    PUSH_METHOD(IOHandle, IOHANDLE_CLOSE);
}


// Input exports

enum IOInput_functions_t {
    IOINPUT_CONSTRUCTOR,
};

enum IOInput_methods_t {
    IOINPUT_OPEN,
};

struct Balau::lua_functypes_t IOInput_functions[] = {
    { IOINPUT_CONSTRUCTOR,      NULL,             1, 1, { Balau::BLUA_STRING } },
    { -1, 0, 0, 0, 0 },
};

struct Balau::lua_functypes_t IOInput_methods[] = {
    { IOINPUT_OPEN,             "open",           0, 0, { } },
    { -1, 0, 0, 0, 0 },
};

struct sLua_IOInput {
    static int IOInput_proceed_static(Balau::Lua & L, int n, int caller);
    static int IOInput_proceed(Balau::Lua & L, int n, IOHandle * obj, int caller);
};

int sLua_IOInput::IOInput_proceed_static(Balau::Lua & L, int n, int caller) {
    int r;

    switch (caller) {
    case IOINPUT_CONSTRUCTOR:
        {
            Balau::LuaInputFactory factory(new Balau::Input(L.tostring()));
            factory.pushDestruct(L);
        }
        r = 1;
        break;
    }

    return r;
}

int sLua_IOInput::IOInput_proceed(Balau::Lua & L, int n, IOInput * obj, int caller) {
    int r;
    Balau::IO<Balau::Input> h = *obj;

    switch (caller) {
    case IOINPUT_OPEN:
        return L.yield(Balau::Future<int>([h]() mutable { h->open(); return 0; }));
        break;
    }

    return r;
}

void Balau::LuaInputFactory::pushStatics(Balau::Lua & L) {
    CHECK_FUNCTIONS(IOInput);
    CHECK_METHODS(IOInput);

    PUSH_SUBCLASS(Input, Handle);
    PUSH_CONSTRUCTOR(IOInput, IOINPUT_CONSTRUCTOR);
    PUSH_CLASS_DONE();
}

void Balau::LuaInputFactory::pushObjectAndMembers(Lua & L) {
    LuaHandleFactory::pushObjectAndMembers(L);

    L.push("__type");
    L.getglobal("Input");
    L.settable(-3, true);

    PUSH_METHOD(IOInput, IOINPUT_OPEN);
}

void Balau::registerLuaHandle(Lua & L) {
    LuaHandleFactory::pushStatics(L);
    LuaInputFactory::pushStatics(L);
}
