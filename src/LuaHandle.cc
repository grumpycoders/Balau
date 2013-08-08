#include "LuaHandle.h"
#include "Handle.h"

typedef Balau::IO<Balau::Handle> IOHandle;
typedef IOHandle IOInput;

// Handle exports

enum IOHandle_methods_t {
    IOHANDLE_CLOSE,
    IOHANDLE_READU8,
    IOHANDLE_READU16,
    IOHANDLE_READU32,
    IOHANDLE_READI8,
    IOHANDLE_READI16,
    IOHANDLE_READI32,
    IOHANDLE_WRITEU8,
    IOHANDLE_WRITEU16,
    IOHANDLE_WRITEU32,
    IOHANDLE_WRITEI8,
    IOHANDLE_WRITEI16,
    IOHANDLE_WRITEI32,
};

struct Balau::lua_functypes_t IOHandle_methods[] = {
    { IOHANDLE_CLOSE,           "close",          0, 0, { } },
    { IOHANDLE_READU8,          "readU8",         0, 0, { } },
    { IOHANDLE_READU16,         "readU16",        0, 0, { } },
    { IOHANDLE_READU32,         "readU32",        0, 0, { } },
    { IOHANDLE_READI8,          "readI8",         0, 0, { } },
    { IOHANDLE_READI16,         "readI16",        0, 0, { } },
    { IOHANDLE_READI32,         "readI32",        0, 0, { } },
    { IOHANDLE_WRITEU8,         "writeU8",        1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEU16,        "writeU16",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEU32,        "writeU32",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI8,         "writeI8",        1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI16,        "writeI16",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI32,        "writeI32",       1, 1, { Balau::BLUA_NUMBER } },
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
    case IOHANDLE_READU8:
        {
            Balau::Future<uint8_t> c = h->readU8();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READU16:
        {
            Balau::Future<uint16_t> c = h->readU16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READU32:
        {
            Balau::Future<uint32_t> c = h->readU32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READI8:
        {
            Balau::Future<int8_t> c = h->readI8();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READI16:
        {
            Balau::Future<int16_t> c = h->readI16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READI32:
        {
            Balau::Future<int32_t> c = h->readI32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_WRITEU8:
        {
            Balau::Future<void> c = h->writeU8(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEU16:
        {
            Balau::Future<void> c = h->writeU16(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEU32:
        {
            Balau::Future<void> c = h->writeU32(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI8:
        {
            Balau::Future<void> c = h->writeI8(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI16:
        {
            Balau::Future<void> c = h->writeI16(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI32:
        {
            Balau::Future<void> c = h->writeI32(L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
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
    PUSH_METHOD(IOHandle, IOHANDLE_READU8);
    PUSH_METHOD(IOHandle, IOHANDLE_READU16);
    PUSH_METHOD(IOHandle, IOHANDLE_READU32);
    PUSH_METHOD(IOHandle, IOHANDLE_READI8);
    PUSH_METHOD(IOHandle, IOHANDLE_READI16);
    PUSH_METHOD(IOHandle, IOHANDLE_READI32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU8);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI8);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI32);
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
