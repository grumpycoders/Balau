#include "LuaBigInt.h"

using namespace Balau;

enum BigInt_functions_t {
    BIGINT_CONSTRUCTOR,
};

enum BigInt_methods_t {
    BIGINT_SET,
    BIGINT_TOSTRING,

    BIGINT_ADD,
    BIGINT_SUB,
    BIGINT_MUL,
    BIGINT_DIV,
    BIGINT_MOD,
};

struct lua_functypes_t BigInt_functions[] = {
    { BIGINT_CONSTRUCTOR,     NULL,             0, 2, { BLUA_STRING | BLUA_NUMBER, BLUA_NUMBER } },
    { -1, 0, 0, 0, 0 },
};

struct lua_functypes_t BigInt_methods[] = {
    { BIGINT_SET,             "set",            1, 2, { BLUA_STRING | BLUA_NUMBER, BLUA_NUMBER } },
    { BIGINT_TOSTRING,        "tostring",       0, 1, { BLUA_NUMBER } },

    { BIGINT_ADD,             "add",            1, 1, { BLUA_OBJECT } },
    { BIGINT_SUB,             "sub",            1, 1, { BLUA_OBJECT } },
    { BIGINT_MUL,             "mul",            1, 1, { BLUA_OBJECT } },
    { BIGINT_DIV,             "div",            1, 1, { BLUA_OBJECT } },
    { BIGINT_MOD,             "mod",            1, 1, { BLUA_OBJECT } },

    { -1, 0, 0, 0, 0 },
};

struct sLua_BigInt {
    static int BigInt_proceed_static(Lua & L, int n, int caller);
    static int BigInt_proceed(Lua & L, int n, BigInt * obj, int caller);
};

int sLua_BigInt::BigInt_proceed_static(Lua & L, int n, int caller) {
    int r = 0;
    String s;
    int radix = 10;

    switch (caller) {
    case BIGINT_CONSTRUCTOR:
        {
            BigInt * a = new BigInt();
            if (L.type() == LUA_TSTRING) {
                s = L.tostring(1);
                if (n == 2)
                    radix = L.tonumber(-1);
                a->set(s, radix);
            } else {
                lua_Number f = L.tonumber();
                a->set(f);
            }
            LuaBigIntFactory o(a);
            o.pushDestruct(L);
            r = 1;
            break;
        }
    }

    return r;
}

int sLua_BigInt::BigInt_proceed(Lua & L, int n, BigInt * a, int caller) {
    int r = 0;
    BigInt * b, * c, * m;
    String s;
    int radix = 10;

    switch (caller) {
    case BIGINT_SET:
        if (L.type() == LUA_TSTRING) {
            s = L.tostring(2);
            if (n == 3)
                radix = L.tonumber(-1);
            a->set(s, radix);
        } else {
            lua_Number f = L.tonumber();
            a->set(f);
        }
        r = 0;
        break;
    case BIGINT_TOSTRING:
        if (n == 3)
            radix = L.tonumber(-1);
        L.push(a->toString(radix));
        r = 1;
        break;
    case BIGINT_ADD:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a + *b;
        r = 1;
        break;
    case BIGINT_SUB:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a - *b;
        r = 1;
        break;
    case BIGINT_MUL:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a * *b;
        r = 1;
        break;
    case BIGINT_DIV:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a / *b;
        r = 1;
        break;
    case BIGINT_MOD:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a % *b;
        r = 1;
        break;
    }

    return r;
}

#define METAMETHOD_WRAPPER(op) \
L.push("__" #op); \
L.load("" \
"return function(bigint) return function(a) \n" \
"    return a:" #op "() \n" \
"end end\n" \
""); \
L.copy(-5); \
L.pcall(1); \
L.settable() \

#define METAMETHOD_WRAPPER2(op) \
L.push("__" #op); \
L.load("" \
"return function(bigint) return function(a, b) \n" \
"    if type(a) ~= 'table' or a.__type ~= bigint then \n" \
"        a = bigint.new(a) \n" \
"    end \n" \
"    if type(b) ~= 'table' or b.__type ~= bigint then \n" \
"        b = bigint.new(b) \n" \
"    end \n" \
"    return a:" #op "(b) \n" \
"end end\n" \
""); \
L.copy(-5); \
L.pcall(1); \
L.settable() \

void LuaBigIntFactory::pushStatics(Lua & L) {
    CHECK_FUNCTIONS(BigInt);
    CHECK_METHODS(BigInt);
    PUSH_CLASS(BigInt);
    PUSH_CONSTRUCTOR(BigInt, BIGINT_CONSTRUCTOR);
    L.push("BIGINT_METAS");
    L.newtable();
    METAMETHOD_WRAPPER2(add);
    METAMETHOD_WRAPPER2(sub);
    METAMETHOD_WRAPPER2(div);
    METAMETHOD_WRAPPER2(mul);
    METAMETHOD_WRAPPER2(mod);
    METAMETHOD_WRAPPER(tostring);
    L.settable(LUA_REGISTRYINDEX);
    PUSH_CLASS_DONE();
}

void LuaBigIntFactory::pushObjectAndMembers(Lua & L) {
    pushObj(L, m_obj, "BigInt");

    L.push("BIGINT_METAS");
    L.gettable(LUA_REGISTRYINDEX);
    L.setmetatable();

    PUSH_METHOD(BigInt, BIGINT_SET);
    PUSH_METHOD(BigInt, BIGINT_TOSTRING);

    PUSH_METHOD(BigInt, BIGINT_ADD);
    PUSH_METHOD(BigInt, BIGINT_SUB);
    PUSH_METHOD(BigInt, BIGINT_MUL);
    PUSH_METHOD(BigInt, BIGINT_DIV);
    PUSH_METHOD(BigInt, BIGINT_MOD);
}

void Balau::registerLuaBigInt(Lua & L) {
    LuaBigIntFactory::pushStatics(L);
}
