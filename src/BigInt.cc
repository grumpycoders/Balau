#include <malloc.h>
#include <math.h>
#include "tomcrypt.h"
#include "BigInt.h"
#include "Main.h"

namespace {

class InitMP : public Balau::AtStart {
  public:
      InitMP() : AtStart(20) { }
    void doStart() {
        ltc_mp = ltm_desc;
        IAssert(!m_initialized, "doStart should only be called once.");
        m_initialized = true;

        static Balau::BigInt s2p32;
        s2p32.set2expt(32);
        m_2p32 = &s2p32;
    }
    const Balau::BigInt & get2p32() {
        return *m_2p32;
    }
    bool initialized() { return m_initialized; }
  private:
    Balau::BigInt * m_2p32 = NULL;
    bool m_initialized = false;
};

static InitMP s_MP;

};

Balau::BigInt::BigInt() throw (GeneralException) {
    AAssert(s_MP.initialized(), "You can't statically declare a BigInt.");
    if (mp_init(&m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_init");
}

Balau::BigInt::BigInt(const BigInt & v) throw (GeneralException) {
    AAssert(s_MP.initialized(), "You can't statically declare a BigInt.");
    if (mp_init_copy(&m_bi, v.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_init_copy");
}

Balau::BigInt::BigInt(BigInt && v) {
    m_bi = v.m_bi;
    v.m_bi = NULL;
}

Balau::BigInt::~BigInt() {
    if (!m_bi)
        return;
    mp_clear(m_bi);
    m_bi = NULL;
}

Balau::BigInt & Balau::BigInt::operator=(const BigInt & v) throw (GeneralException) {
    if (&v == this)
        return *this;

    if (mp_copy(v.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_init_copy");

    return *this;
}

void Balau::BigInt::set(uint64_t v) throw (GeneralException) {
    uint32_t low  = v & 0xffffffff;
    uint32_t high = v >> 32;
    if (high == 0) {
        if (mp_set_int(m_bi, low) != CRYPT_OK)
            throw GeneralException("Error while calling mp_set_init");
    } else {
        if (mp_set_int(m_bi, high) != CRYPT_OK)
            throw GeneralException("Error while calling mp_set_init");
        operator*=(s_MP.get2p32());
        operator+=(low);
    }
}

void Balau::BigInt::set(int64_t v) {
    if (v >= 0) {
        set((uint64_t) v);
    } else {
        v = -v;
        set((uint64_t) v);
        do_neg();
    }
}

void Balau::BigInt::set(uint32_t v) throw (GeneralException) {
    if (mp_set_int(m_bi, v) != CRYPT_OK)
        throw GeneralException("Error while calling mp_set_init");
}

void Balau::BigInt::set(int32_t v) {
    if (v >= 0) {
        set((uint32_t) v);
    } else {
        v = -v;
        set((uint32_t) v);
        do_neg();
    }
}

void Balau::BigInt::set(double v) throw (GeneralException) {
    double f, i;
    f = modf(v, &i);
    AAssert(f == 0.0, "Can't set a BigInt with a double that has a fractional value");

    int e;

    f = frexp(v, &e);
    if (mp_set_int(m_bi, 0) != CRYPT_OK)
        throw GeneralException("Error while calling mp_set_init");

    for (e -= 1.0; e > 0.0; e -= 1.0) {
        f *= 2.0;
        if (f >= 1.0) {
            operator+=(1);
            f -= 1.0;
        }
        operator*=(2);
    }
}

void Balau::BigInt::set(const String & v, int radix) throw (GeneralException) {
    if (mp_read_radix(m_bi, v.to_charp(), radix) != CRYPT_OK)
        throw GeneralException("Error while calling mp_read_radix");
}

void Balau::BigInt::set2expt(int i) throw (GeneralException) {
    if (mp_2expt(m_bi, i) != CRYPT_OK)
        throw GeneralException("Error while calling mp_2expt");
}

uint64_t Balau::BigInt::to_uint64() const throw (GeneralException) {
    if (mp_count_bits(m_bi) > 64)
        throw GeneralException("BigInt too big to fit in a uint64");
    uint64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 64) {
        v |= mp_get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return v;
}

int64_t Balau::BigInt::to_int64() const throw (GeneralException) {
    if (mp_count_bits(m_bi) > 63)
        throw GeneralException("BigInt too big to fit in a int64");
    int64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 63) {
        v |= mp_get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return comp(0) == LT ? -v : v;
}

uint32_t Balau::BigInt::to_uint32() const throw (GeneralException) {
    if (mp_count_bits(m_bi) > 32)
        throw GeneralException("BigInt too big to fit in a uint32");
    uint64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 32) {
        v |= mp_get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return v;
}

int32_t Balau::BigInt::to_int32() const throw (GeneralException) {
    if (mp_count_bits(m_bi) > 31)
        throw GeneralException("BigInt too big to fit in a uint32");
    int64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 31) {
        v |= mp_get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return comp(0) == LT ? -v : v;
}

Balau::BigInt Balau::BigInt::operator+(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_add_d(m_bi, i, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_add_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator+(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_add(m_bi, a.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_add");
    return r;
}

Balau::BigInt Balau::BigInt::operator-(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_sub_d(m_bi, i, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sub_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator-(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_sub(m_bi, a.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sub");
    return r;
}

Balau::BigInt Balau::BigInt::operator*(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_mul_d(m_bi, i, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mul_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator*(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_mul(m_bi, a.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mul");
    return r;
}

Balau::BigInt Balau::BigInt::operator/(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_div(m_bi, a.m_bi, r.m_bi, NULL) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return r;
}

Balau::BigInt Balau::BigInt::operator%(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_div(m_bi, a.m_bi, NULL, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return r;
}

Balau::BigInt Balau::BigInt::operator<<(unsigned int a) const throw (GeneralException) {
    BigInt r;
    if (mp_mul_d(m_bi, 1 << a, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt Balau::BigInt::operator>>(unsigned int a) const {
    BigInt s;
    s.set2expt(a);
    return operator/(s);
}

Balau::BigInt & Balau::BigInt::operator+=(unsigned int i) throw (GeneralException) {
    if (mp_add_d(m_bi, i, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_add_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator+=(const BigInt & a) throw (GeneralException) {
    if (mp_add(m_bi, a.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_add");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator-=(unsigned int i) throw (GeneralException) {
    if (mp_sub_d(m_bi, i, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sub_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator-=(const BigInt & a) throw (GeneralException) {
    if (mp_sub(m_bi, a.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sub");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator*=(unsigned int i) throw (GeneralException) {
    if (mp_mul_d(m_bi, i, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mul_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator*=(const BigInt & a) throw (GeneralException) {
    if (mp_mul(m_bi, a.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mul");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator/=(const BigInt & a) throw (GeneralException) {
    if (mp_div(m_bi, a.m_bi, m_bi, NULL) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator%=(const BigInt & a) throw (GeneralException) {
    if (mp_div(m_bi, a.m_bi, NULL, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator<<=(unsigned int a) throw (GeneralException) {
    if (mp_mul_d(m_bi, 1 << a, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator>>=(unsigned int a) {
    BigInt s;
    s.set2expt(a);
    return operator/=(s);
}

Balau::BigInt Balau::BigInt::operator-() const throw (GeneralException) {
    return neg();
}

Balau::BigInt & Balau::BigInt::operator++() {
    operator+=(1);
    return *this;
}

Balau::BigInt Balau::BigInt::operator++(int) {
    BigInt r(*this);
    operator+=(1);
    return r;
}

Balau::BigInt & Balau::BigInt::operator--() {
    operator-=(1);
    return *this;
}

Balau::BigInt Balau::BigInt::operator--(int) {
    BigInt r(*this);
    operator-=(1);
    return r;
}

Balau::BigInt Balau::BigInt::neg() const throw (GeneralException) {
    BigInt r;
    if (mp_neg(m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_neg");
    return r;
}

Balau::BigInt & Balau::BigInt::do_neg() throw (GeneralException) {
    if (mp_neg(m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_neg");
    return *this;
}

Balau::BigInt Balau::BigInt::sqrt() const throw (GeneralException) {
    BigInt r;
    if (mp_sqr(m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sqr");
    return r;
}

Balau::BigInt & Balau::BigInt::do_sqrt() throw (GeneralException) {
    if (mp_sqr(m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sqr");
    return *this;
}

Balau::BigInt Balau::BigInt::gcd(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_gcd(m_bi, a.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_gcd");
    return r;
}

Balau::BigInt Balau::BigInt::lcm(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_lcm(m_bi, a.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_lcm");
    return r;
}

Balau::BigInt::comp_t Balau::BigInt::comp(const BigInt & a) const throw (GeneralException) {
    int r = mp_cmp(m_bi, a.m_bi);
    switch (r) {
    case LTC_MP_LT:
        return LT;
    case LTC_MP_GT:
        return GT;
    case LTC_MP_EQ:
        return EQ;
    default:
        throw GeneralException("Unknown result from mp_cmp");
    }
}

Balau::BigInt::comp_t Balau::BigInt::comp(unsigned int a) const throw (GeneralException) {
    int r = mp_cmp_d(m_bi, a);
    switch (r) {
    case LTC_MP_LT:
        return LT;
    case LTC_MP_GT:
        return GT;
    case LTC_MP_EQ:
        return EQ;
    default:
        throw GeneralException("Unknown result from mp_cmp_d");
    }
}

bool Balau::BigInt::operator==(const BigInt & a) const {
    comp_t r = comp(a);
    return r == EQ;
}

bool Balau::BigInt::operator!=(const BigInt & a) const {
    comp_t r = comp(a);
    return r != EQ;
}

bool Balau::BigInt::operator<=(const BigInt & a) const {
    comp_t r = comp(a);
    return r == LT || r == EQ;
}

bool Balau::BigInt::operator>=(const BigInt & a) const {
    comp_t r = comp(a);
    return r == GT || r == EQ;
}

bool Balau::BigInt::operator<(const BigInt & a) const {
    comp_t r = comp(a);
    return r == LT;
}

bool Balau::BigInt::operator>(const BigInt & a) const {
    comp_t r = comp(a);
    return r == GT;
}

bool Balau::BigInt::operator==(unsigned int a) const {
    comp_t r = comp(a);
    return r == EQ;
}

bool Balau::BigInt::operator!=(unsigned int a) const {
    comp_t r = comp(a);
    return r != EQ;
}

bool Balau::BigInt::operator<=(unsigned int a) const {
    comp_t r = comp(a);
    return r == LT || r == EQ;
}

bool Balau::BigInt::operator>=(unsigned int a) const {
    comp_t r = comp(a);
    return r == GT || r == EQ;
}

bool Balau::BigInt::operator<(unsigned int a) const {
    comp_t r = comp(a);
    return r == LT;
}

bool Balau::BigInt::operator>(unsigned int a) const {
    comp_t r = comp(a);
    return r == GT;
}

Balau::BigInt Balau::BigInt::modadd(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_addmod(m_bi, a.m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_addmod");
    return r;
}

Balau::BigInt Balau::BigInt::modsub(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_submod(m_bi, a.m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_submod");
    return r;
}

Balau::BigInt Balau::BigInt::modmul(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_mulmod(m_bi, a.m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mulmod");
    return r;
}

Balau::BigInt Balau::BigInt::modsqr(const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_sqrmod(m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sqrmod");
    return r;
}

Balau::BigInt Balau::BigInt::modinv(const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_invmod(m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_invmod");
    return r;
}

Balau::BigInt Balau::BigInt::modpow(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_exptmod(m_bi, a.m_bi, m.m_bi, r.m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_exptmod");
    return r;
}

Balau::BigInt & Balau::BigInt::do_modadd(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_addmod(m_bi, a.m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_addmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modsub(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_submod(m_bi, a.m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_submod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modmul(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_mulmod(m_bi, a.m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_mulmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modsqr(const BigInt & m) throw (GeneralException) {
    if (mp_sqrmod(m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_sqrmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modinv(const BigInt & m) throw (GeneralException) {
    if (mp_invmod(m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_invmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modpow(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_exptmod(m_bi, a.m_bi, m.m_bi, m_bi) != CRYPT_OK)
        throw GeneralException("Error while calling mp_exptmod");
    return *this;
}

bool Balau::BigInt::isPrime() const throw (GeneralException) {
    int r = 0;
    if (mp_prime_is_prime(m_bi, NULL, &r) != CRYPT_OK)
        throw GeneralException("Error while calling mp_prime_is_prime");
    return r == LTC_MP_YES;
}

size_t Balau::BigInt::exportSize() const {
    return mp_unsigned_bin_size(m_bi) + 1;
}

void Balau::BigInt::exportBin(void * _buf) const throw (GeneralException) {
    unsigned char * buf = (unsigned char *) _buf;
    buf[0] = comp(0) == LT ? 0xff : 0;
    if (mp_to_unsigned_bin(m_bi, buf + 1) != CRYPT_OK)
        throw GeneralException("Error while calling mp_to_unsigned_bin");
}

void Balau::BigInt::importBin(const void * _buf, size_t size) throw (GeneralException) {
    unsigned char * buf = (unsigned char *) _buf;
    bool isNeg = buf[0] != 0;
    if (mp_read_unsigned_bin(m_bi, buf + 1, size - 1) != CRYPT_OK)
        throw GeneralException("Error while calling mp_read_unsigned_bin");
    if (isNeg)
        do_neg();
}

Balau::String Balau::BigInt::toString(int radix) const {
    char * out = (char *) alloca(mp_count_bits(m_bi) / (radix >= 10 ? 3 : 1) + 3);
    mp_toradix(m_bi, out, radix);
    return String(out);
}

char * Balau::BigInt::makeString(int radix) const {
    char * out = (char *) malloc(mp_count_bits(m_bi) / (radix >= 10 ? 3 : 1) + 3);
    mp_toradix(m_bi, out, radix);
    return out;
}
