#include "LuaHandle.h"
#include "LuaBigInt.h"
#include "Handle.h"

typedef Balau::IO<Balau::Handle> IOHandle;
typedef IOHandle IOInput;

// Handle exports

enum IOHandle_methods_t {
    IOHANDLE_CLOSE,
    IOHANDLE_READU8,
    IOHANDLE_READU16,
    IOHANDLE_READU32,
    IOHANDLE_READU64,
    IOHANDLE_READI8,
    IOHANDLE_READI16,
    IOHANDLE_READI32,
    IOHANDLE_READI64,
    IOHANDLE_READLEU16,
    IOHANDLE_READLEU32,
    IOHANDLE_READLEU64,
    IOHANDLE_READLEI16,
    IOHANDLE_READLEI32,
    IOHANDLE_READLEI64,
    IOHANDLE_READBEU16,
    IOHANDLE_READBEU32,
    IOHANDLE_READBEU64,
    IOHANDLE_READBEI16,
    IOHANDLE_READBEI32,
    IOHANDLE_READBEI64,
    IOHANDLE_WRITEU8,
    IOHANDLE_WRITEU16,
    IOHANDLE_WRITEU32,
    IOHANDLE_WRITEU64,
    IOHANDLE_WRITEI8,
    IOHANDLE_WRITEI16,
    IOHANDLE_WRITEI32,
    IOHANDLE_WRITEI64,
    IOHANDLE_WRITELEU16,
    IOHANDLE_WRITELEU32,
    IOHANDLE_WRITELEU64,
    IOHANDLE_WRITELEI16,
    IOHANDLE_WRITELEI32,
    IOHANDLE_WRITELEI64,
    IOHANDLE_WRITEBEU16,
    IOHANDLE_WRITEBEU32,
    IOHANDLE_WRITEBEU64,
    IOHANDLE_WRITEBEI16,
    IOHANDLE_WRITEBEI32,
    IOHANDLE_WRITEBEI64,
};

struct Balau::lua_functypes_t IOHandle_methods[] = {
    { IOHANDLE_CLOSE,           "close",          0, 0, { } },
    { IOHANDLE_READU8,          "readU8",         0, 0, { } },
    { IOHANDLE_READU16,         "readU16",        0, 0, { } },
    { IOHANDLE_READU32,         "readU32",        0, 0, { } },
    { IOHANDLE_READU64,         "readU64",        0, 0, { } },
    { IOHANDLE_READI8,          "readI8",         0, 0, { } },
    { IOHANDLE_READI16,         "readI16",        0, 0, { } },
    { IOHANDLE_READI32,         "readI32",        0, 0, { } },
    { IOHANDLE_READI64,         "readI64",        0, 0, { } },
    { IOHANDLE_READLEU16,       "readLEU16",      0, 0, { } },
    { IOHANDLE_READLEU32,       "readLEU32",      0, 0, { } },
    { IOHANDLE_READLEU64,       "readLEU64",      0, 0, { } },
    { IOHANDLE_READLEI16,       "readLEI16",      0, 0, { } },
    { IOHANDLE_READLEI32,       "readLEI32",      0, 0, { } },
    { IOHANDLE_READLEI64,       "readLEI64",      0, 0, { } },
    { IOHANDLE_READBEU16,       "readBEU16",      0, 0, { } },
    { IOHANDLE_READBEU32,       "readBEU32",      0, 0, { } },
    { IOHANDLE_READBEU64,       "readBEU64",      0, 0, { } },
    { IOHANDLE_READBEI16,       "readBEI16",      0, 0, { } },
    { IOHANDLE_READBEI32,       "readBEI32",      0, 0, { } },
    { IOHANDLE_READBEI64,       "readBEI64",      0, 0, { } },
    { IOHANDLE_WRITEU8,         "writeU8",        1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEU16,        "writeU16",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEU32,        "writeU32",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEU64,        "writeU64",       1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
    { IOHANDLE_WRITEI8,         "writeI8",        1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI16,        "writeI16",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI32,        "writeI32",       1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEI64,        "writeI64",       1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
    { IOHANDLE_WRITELEU16,      "writeLEU16",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITELEU32,      "writeLEU32",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITELEU64,      "writeLEU64",     1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
    { IOHANDLE_WRITELEI16,      "writeLEI16",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITELEI32,      "writeLEI32",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITELEI64,      "writeLEI64",     1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
    { IOHANDLE_WRITEBEU16,      "writeBEU16",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEBEU32,      "writeBEU32",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEBEU64,      "writeBEU64",     1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
    { IOHANDLE_WRITEBEI16,      "writeBEI16",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEBEI32,      "writeBEI32",     1, 1, { Balau::BLUA_NUMBER } },
    { IOHANDLE_WRITEBEI64,      "writeBEI64",     1, 1, { Balau::BLUA_NUMBER | Balau::BLUA_OBJECT | Balau::BLUA_STRING } },
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
    case IOHANDLE_READU64:
        {
            Balau::Future<uint64_t> c = h->readU64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                uint64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
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
    case IOHANDLE_READI64:
        {
            Balau::Future<int64_t> c = h->readI64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                int64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
        }
        break;
    case IOHANDLE_READLEU16:
        {
            Balau::Future<uint16_t> c = h->readLEU16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READLEU32:
        {
            Balau::Future<uint32_t> c = h->readLEU32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READLEU64:
        {
            Balau::Future<uint64_t> c = h->readLEU64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                uint64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
        }
        break;
    case IOHANDLE_READLEI16:
        {
            Balau::Future<int16_t> c = h->readLEI16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READLEI32:
        {
            Balau::Future<int32_t> c = h->readLEI32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READLEI64:
        {
            Balau::Future<int64_t> c = h->readLEI64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                int64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
        }
        break;
    case IOHANDLE_READBEU16:
        {
            Balau::Future<uint16_t> c = h->readBEU16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READBEU32:
        {
            Balau::Future<uint32_t> c = h->readBEU32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READBEU64:
        {
            Balau::Future<uint64_t> c = h->readBEU64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                uint64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
        }
        break;
    case IOHANDLE_READBEI16:
        {
            Balau::Future<int16_t> c = h->readBEI16();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READBEI32:
        {
            Balau::Future<int32_t> c = h->readBEI32();
            return L.yield(Balau::Future<int>([L, c]() mutable { L.push((lua_Number) c.get()); return 1; }));
        }
        break;
    case IOHANDLE_READBEI64:
        {
            Balau::Future<int64_t> c = h->readBEI64();
            return L.yield(Balau::Future<int>([L, c]() mutable {
                int64_t v = c.get();
                Balau::LuaBigIntFactory f(new Balau::BigInt(v));
                f.pushDestruct(L);
                return 1;
            }));
        }
        break;
    case IOHANDLE_WRITEU8:
        {
            Balau::Future<void> c = h->writeU8((uint8_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEU16:
        {
            Balau::Future<void> c = h->writeU16((uint16_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEU32:
        {
            Balau::Future<void> c = h->writeU32((uint32_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEU64:
        {
            uint64_t v;
            if (L.istable()) {
                Balau::BigInt * b = L.recast<Balau::BigInt>();
                v = b->to_uint64();
            } else if (L.type() == LUA_TSTRING) {
                Balau::BigInt b(L.tostring());
                v = b.to_uint64();
            } else {
                v = (uint64_t) L.tonumber();
            }
            Balau::Future<void> c = h->writeU64(v);
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI8:
        {
            Balau::Future<void> c = h->writeI8((int8_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI16:
        {
            Balau::Future<void> c = h->writeI16((int16_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI32:
        {
            Balau::Future<void> c = h->writeI32((int32_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEI64:
        {
            int64_t v;
            if (L.istable()) {
                Balau::BigInt * b = L.recast<Balau::BigInt>();
                v = b->to_int64();
            } else if (L.type() == LUA_TSTRING) {
                Balau::BigInt b(L.tostring());
                v = b.to_int64();
            } else {
                v = (int64_t) L.tonumber();
            }
            Balau::Future<void> c = h->writeI64(v);
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEU16:
        {
            Balau::Future<void> c = h->writeLEU16((uint16_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEU32:
        {
            Balau::Future<void> c = h->writeLEU32((uint32_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEU64:
        {
            uint64_t v;
            if (L.istable()) {
                Balau::BigInt * b = L.recast<Balau::BigInt>();
                v = b->to_uint64();
            } else if (L.type() == LUA_TSTRING) {
                Balau::BigInt b(L.tostring());
                v = b.to_uint64();
            } else {
                v = (uint64_t) L.tonumber();
            }
            Balau::Future<void> c = h->writeLEU64(v);
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEI16:
        {
            Balau::Future<void> c = h->writeLEI16((int16_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEI32:
        {
            Balau::Future<void> c = h->writeLEI32((int32_t) L.tonumber());
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITELEI64:
        {
            int64_t v;
            if (L.istable()) {
                Balau::BigInt * b = L.recast<Balau::BigInt>();
                v = b->to_int64();
            } else if (L.type() == LUA_TSTRING) {
                Balau::BigInt b(L.tostring());
                v = b.to_int64();
            } else {
                v = (int64_t) L.tonumber();
            }
            Balau::Future<void> c = h->writeLEI64(v);
            return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
        }
        break;
    case IOHANDLE_WRITEBEU16:
    {
        Balau::Future<void> c = h->writeBEU16((uint16_t)L.tonumber());
        return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
    }
        break;
    case IOHANDLE_WRITEBEU32:
    {
        Balau::Future<void> c = h->writeBEU32((uint32_t)L.tonumber());
        return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
    }
        break;
    case IOHANDLE_WRITEBEU64:
    {
        uint64_t v;
        if (L.istable()) {
            Balau::BigInt * b = L.recast<Balau::BigInt>();
            v = b->to_uint64();
        } else if (L.type() == LUA_TSTRING) {
            Balau::BigInt b(L.tostring());
            v = b.to_uint64();
        } else {
            v = (uint64_t)L.tonumber();
        }
        Balau::Future<void> c = h->writeBEU64(v);
        return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
    }
        break;
    case IOHANDLE_WRITEBEI16:
    {
        Balau::Future<void> c = h->writeBEI16((int16_t)L.tonumber());
        return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
    }
        break;
    case IOHANDLE_WRITEBEI32:
    {
        Balau::Future<void> c = h->writeBEI32((int32_t)L.tonumber());
        return L.yield(Balau::Future<int>([L, c]() mutable { c.run(); return 0; }));
    }
        break;
    case IOHANDLE_WRITEBEI64:
    {
        int64_t v;
        if (L.istable()) {
            Balau::BigInt * b = L.recast<Balau::BigInt>();
            v = b->to_int64();
        } else if (L.type() == LUA_TSTRING) {
            Balau::BigInt b(L.tostring());
            v = b.to_int64();
        } else {
            v = (int64_t)L.tonumber();
        }
        Balau::Future<void> c = h->writeBEI64(v);
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
    PUSH_METHOD(IOHandle, IOHANDLE_READU64);
    PUSH_METHOD(IOHandle, IOHANDLE_READI8);
    PUSH_METHOD(IOHandle, IOHANDLE_READI16);
    PUSH_METHOD(IOHandle, IOHANDLE_READI32);
    PUSH_METHOD(IOHandle, IOHANDLE_READI64);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEU64);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEI32);
    PUSH_METHOD(IOHandle, IOHANDLE_READLEI64);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEU64);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEI32);
    PUSH_METHOD(IOHandle, IOHANDLE_READBEI64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU8);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEU64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI8);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEI64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEU64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEI32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITELEI64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEU16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEU32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEU64);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEI16);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEI32);
    PUSH_METHOD(IOHandle, IOHANDLE_WRITEBEI64);
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
    Balau::IO<Balau::Input> h = *obj;

    switch (caller) {
    case IOINPUT_OPEN:
        return L.yield(Balau::Future<int>([h]() mutable { h->open(); return 0; }));
        break;
    }

    return 0;
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
