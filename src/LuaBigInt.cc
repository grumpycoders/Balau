#include "LuaBigInt.h"

using namespace Balau;

enum BigInt_functions_t {
    BIGINT_CONSTRUCTOR,
};

enum BigInt_methods_t {
    BIGINT_SET,
    BIGINT_SET2EXPT,
    BIGINT_TOSTRING,

    BIGINT_ADD,
    BIGINT_SUB,
    BIGINT_MUL,
    BIGINT_DIV,
    BIGINT_MOD,
    BIGINT_SHL,
    BIGINT_SHR,

    BIGINT_DO_ADD,
    BIGINT_DO_SUB,
    BIGINT_DO_MUL,
    BIGINT_DO_DIV,
    BIGINT_DO_MOD,
    BIGINT_DO_SHL,
    BIGINT_DO_SHR,

    BIGINT_UNM,
    BIGINT_DO_NEG,

    BIGINT_SQRT,
    BIGINT_DO_SQRT,

    BIGINT_GCD,
    BIGINT_LCM,

    BIGINT_EQ,
    BIGINT_LT,
    BIGINT_LE,

    BIGINT_MODADD,
    BIGINT_MODSUB,
    BIGINT_MODMUL,
    BIGINT_MODSQR,
    BIGINT_MODINV,
    BIGINT_MODPOW,

    BIGINT_DO_MODADD,
    BIGINT_DO_MODSUB,
    BIGINT_DO_MODMUL,
    BIGINT_DO_MODSQR,
    BIGINT_DO_MODINV,
    BIGINT_DO_MODPOW,

    BIGINT_ISPRIME,
    BIGINT_ISNEG,
    BIGINT_ISZERO,
    BIGINT_ISPOS,
};

struct lua_functypes_t BigInt_functions[] = {
    { BIGINT_CONSTRUCTOR,     NULL,             0, 2, { BLUA_OBJECT | BLUA_STRING | BLUA_NUMBER, BLUA_NUMBER } },
    { -1, 0, 0, 0, 0 },
};

struct lua_functypes_t BigInt_methods[] = {
    { BIGINT_SET,             "set",            1, 2, { BLUA_STRING | BLUA_NUMBER, BLUA_NUMBER } },
    { BIGINT_SET2EXPT,        "set2expt",       1, 1, { BLUA_NUMBER } },
    { BIGINT_TOSTRING,        "tostring",       0, 1, { BLUA_NUMBER } },

    { BIGINT_ADD,             "add",            1, 1, { BLUA_OBJECT } },
    { BIGINT_SUB,             "sub",            1, 1, { BLUA_OBJECT } },
    { BIGINT_MUL,             "mul",            1, 1, { BLUA_OBJECT } },
    { BIGINT_DIV,             "div",            1, 1, { BLUA_OBJECT } },
    { BIGINT_MOD,             "mod",            1, 1, { BLUA_OBJECT } },
    { BIGINT_SHL,             "shl",            1, 1, { BLUA_NUMBER } },
    { BIGINT_SHR,             "shr",            1, 1, { BLUA_NUMBER } },

    { BIGINT_DO_ADD,          "do_add",         1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_SUB,          "do_sub",         1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_MUL,          "do_mul",         1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_DIV,          "do_div",         1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_MOD,          "do_mod",         1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_SHL,          "do_shl",         1, 1, { BLUA_NUMBER } },
    { BIGINT_DO_SHR,          "do_shr",         1, 1, { BLUA_NUMBER } },

    { BIGINT_UNM,             "unm",            0, 0, { } },
    { BIGINT_DO_NEG,          "do_neg",         0, 0, { } },

    { BIGINT_SQRT,            "sqrt",           0, 0, { } },
    { BIGINT_DO_SQRT,         "do_sqrt",        0, 0, { } },

    { BIGINT_GCD,             "gcd",            1, 1, { BLUA_OBJECT } },
    { BIGINT_LCM,             "lcm",            1, 1, { BLUA_OBJECT } },

    { BIGINT_EQ,              "eq",             1, 1, { BLUA_OBJECT } },
    { BIGINT_LT,              "lt",             1, 1, { BLUA_OBJECT } },
    { BIGINT_LE,              "le",             1, 1, { BLUA_OBJECT } },

    { BIGINT_MODADD,          "modadd",         2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_MODSUB,          "modsub",         2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_MODMUL,          "modmul",         2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_MODSQR,          "modsqr",         1, 1, { BLUA_OBJECT } },
    { BIGINT_MODINV,          "modinv",         1, 1, { BLUA_OBJECT } },
    { BIGINT_MODPOW,          "modpow",         2, 2, { BLUA_OBJECT, BLUA_OBJECT } },

    { BIGINT_DO_MODADD,       "do_modadd",      2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_DO_MODSUB,       "do_modsub",      2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_DO_MODMUL,       "do_modmul",      2, 2, { BLUA_OBJECT, BLUA_OBJECT } },
    { BIGINT_DO_MODSQR,       "do_modsqr",      1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_MODINV,       "do_modinv",      1, 1, { BLUA_OBJECT } },
    { BIGINT_DO_MODPOW,       "do_modpow",      2, 2, { BLUA_OBJECT, BLUA_OBJECT } },

    { BIGINT_ISPRIME,         "isPrime",        0, 0, { } },
    { BIGINT_ISNEG,           "isNeg",          0, 0, { } },
    { BIGINT_ISZERO,          "isZero",         0, 0, { } },
    { BIGINT_ISPOS,           "isPos",          0, 0, { } },

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
            BigInt * a;
            if (L.type() == LUA_TSTRING) {
                a = new BigInt();
                s = L.tostring(1);
                if (n == 2)
                    radix = L.tonumber(-1);
                a->set(s, radix);
            } else if (n == 1) {
                if (L.istable()) {
                    BigInt * b = L.recast<BigInt>();
                    a = new BigInt(*b);
                } else {
                    a = new BigInt();
                    lua_Number f = L.tonumber();
                    a->set(f);
                }
            } else if (n == 0) {
                a = new BigInt();
            } else {
                L.error("Invalid arguments to BigInt:new");
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
    case BIGINT_SET2EXPT:
        a->set2expt(L.tonumber());
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
    case BIGINT_SHL:
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a << L.tonumber();
        r = 1;
        break;
    case BIGINT_SHR:
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = *a >> L.tonumber();
        r = 1;
        break;
    case BIGINT_DO_ADD:
        b = L.recast<BigInt>();
        *a += *b;
        break;
    case BIGINT_DO_SUB:
        b = L.recast<BigInt>();
        *a -= *b;
        break;
    case BIGINT_DO_MUL:
        b = L.recast<BigInt>();
        *a *= *b;
        break;
    case BIGINT_DO_DIV:
        b = L.recast<BigInt>();
        *a /= *b;
        break;
    case BIGINT_DO_MOD:
        b = L.recast<BigInt>();
        *a %= *b;
        break;
    case BIGINT_DO_SHL:
        *a <<= L.tonumber();
        break;
    case BIGINT_DO_SHR:
        *a >>= L.tonumber();
        break;
    case BIGINT_UNM:
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->neg();
        r = 1;
        break;
    case BIGINT_DO_NEG:
        a->do_neg();
        break;
    case BIGINT_SQRT:
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->sqrt();
        r = 1;
        break;
    case BIGINT_DO_SQRT:
        a->do_sqrt();
        break;
    case BIGINT_GCD:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->gcd(*b);
        r = 1;
        break;
    case BIGINT_LCM:
        b = L.recast<BigInt>();
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->lcm(*b);
        r = 1;
        break;
    case BIGINT_EQ:
        b = L.recast<BigInt>();
        L.push(*a == *b);
        r = 1;
        break;
    case BIGINT_LT:
        b = L.recast<BigInt>();
        L.push(*a < *b);
        r = 1;
        break;
    case BIGINT_LE:
        b = L.recast<BigInt>();
        L.push(*a <= *b);
        r = 1;
        break;
    case BIGINT_MODADD:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modadd(*b, *m);
        r = 1;
        break;
    case BIGINT_MODSUB:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modsub(*b, *m);
        r = 1;
        break;
    case BIGINT_MODMUL:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modmul(*b, *m);
        r = 1;
        break;
    case BIGINT_MODSQR:
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modsqr(*m);
        r = 1;
        break;
    case BIGINT_MODINV:
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modinv(*m);
        r = 1;
        break;
    case BIGINT_MODPOW:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        {
            LuaBigIntFactory cf(c = new BigInt());
            cf.pushDestruct(L);
        }
        *c = a->modpow(*b, *m);
        r = 1;
        break;
    case BIGINT_DO_MODADD:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        a->do_modadd(*b, *m);
        break;
    case BIGINT_DO_MODSUB:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        a->do_modsub(*b, *m);
        break;
    case BIGINT_DO_MODMUL:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        a->do_modmul(*b, *m);
        break;
    case BIGINT_DO_MODSQR:
        m = L.recast<BigInt>(-1);
        a->do_modsqr(*m);
        break;
    case BIGINT_DO_MODINV:
        m = L.recast<BigInt>(-1);
        a->do_modinv(*m);
        break;
    case BIGINT_DO_MODPOW:
        b = L.recast<BigInt>(-2);
        m = L.recast<BigInt>(-1);
        a->do_modpow(*b, *m);
        break;
    case BIGINT_ISPRIME:
        L.push(a->isPrime());
        r = 1;
        break;
    case BIGINT_ISNEG:
        L.push(*a < 0);
        r = 1;
        break;
    case BIGINT_ISZERO:
        L.push(*a == 0);
        r = 1;
        break;
    case BIGINT_ISPOS:
        L.push(*a > 0);
        r = 1;
        break;
    }

    return r;
}

#define METAMETHOD_WRAPPER(op) \
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
    METAMETHOD_WRAPPER(add);
    METAMETHOD_WRAPPER(sub);
    METAMETHOD_WRAPPER(div);
    METAMETHOD_WRAPPER(mul);
    METAMETHOD_WRAPPER(mod);
    PUSH_METAMETHOD(BigInt, BIGINT_TOSTRING);
    PUSH_METAMETHOD(BigInt, BIGINT_UNM);
    PUSH_METAMETHOD(BigInt, BIGINT_EQ);
    PUSH_METAMETHOD(BigInt, BIGINT_LT);
    PUSH_METAMETHOD(BigInt, BIGINT_LE);

    L.settable(LUA_REGISTRYINDEX);

    PUSH_CLASS_DONE();
}

void LuaBigIntFactory::pushObjectAndMembers(Lua & L) {
    pushObj(L, m_obj, "BigInt");

    L.push("BIGINT_METAS");
    L.gettable(LUA_REGISTRYINDEX);
    L.setmetatable();

    PUSH_METHOD(BigInt, BIGINT_SET);
    PUSH_METHOD(BigInt, BIGINT_SET2EXPT);
    PUSH_METHOD(BigInt, BIGINT_TOSTRING);

    PUSH_METHOD(BigInt, BIGINT_ADD);
    PUSH_METHOD(BigInt, BIGINT_SUB);
    PUSH_METHOD(BigInt, BIGINT_MUL);
    PUSH_METHOD(BigInt, BIGINT_DIV);
    PUSH_METHOD(BigInt, BIGINT_MOD);
    PUSH_METHOD(BigInt, BIGINT_SHL);
    PUSH_METHOD(BigInt, BIGINT_SHR);

    PUSH_METHOD(BigInt, BIGINT_DO_ADD);
    PUSH_METHOD(BigInt, BIGINT_DO_SUB);
    PUSH_METHOD(BigInt, BIGINT_DO_MUL);
    PUSH_METHOD(BigInt, BIGINT_DO_DIV);
    PUSH_METHOD(BigInt, BIGINT_DO_MOD);
    PUSH_METHOD(BigInt, BIGINT_DO_SHL);
    PUSH_METHOD(BigInt, BIGINT_DO_SHR);

    PUSH_METHOD(BigInt, BIGINT_DO_NEG);

    PUSH_METHOD(BigInt, BIGINT_SQRT);
    PUSH_METHOD(BigInt, BIGINT_DO_SQRT);

    PUSH_METHOD(BigInt, BIGINT_GCD);
    PUSH_METHOD(BigInt, BIGINT_LCM);
}

void Balau::registerLuaBigInt(Lua & L) {
    LuaBigIntFactory::pushStatics(L);
}
