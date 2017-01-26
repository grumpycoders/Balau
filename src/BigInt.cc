#include <stdlib.h>
#include <math.h>
#include "tommath.h"
#include "BigInt.h"
#include "Main.h"

namespace {

class InitMP : public Balau::AtStart {
  public:
      InitMP() : AtStart(20) { }
    void doStart() {
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

    m_bi = calloc(1, sizeof(mp_int));
    if (m_bi == NULL)
        throw GeneralException("Unable to allocate ");

    if (mp_init((mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_init");
}

Balau::BigInt::BigInt(const BigInt & v) throw (GeneralException) {
    AAssert(s_MP.initialized(), "You can't statically declare a BigInt.");

    m_bi = calloc(1, sizeof(mp_int));
    if (m_bi == NULL)
        throw GeneralException("Unable to allocate ");

    if (mp_init_copy((mp_int *)m_bi, (mp_int *)v.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_init_copy");
}

Balau::BigInt::BigInt(BigInt && v) {
    m_bi = v.m_bi;
    v.m_bi = NULL;
}

Balau::BigInt::~BigInt() {
    if (!m_bi)
        return;
    mp_clear((mp_int *) m_bi);
    free(m_bi);
    m_bi = NULL;
}

Balau::BigInt & Balau::BigInt::operator=(const BigInt & v) throw (GeneralException) {
    if (&v == this)
        return *this;

    if (mp_copy((mp_int *) v.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_init_copy");

    return *this;
}

void Balau::BigInt::set(uint64_t v) throw (GeneralException) {
    uint32_t low  = v & 0xffffffff;
    uint32_t high = v >> 32;
    if (high == 0) {
        if (mp_set_int((mp_int *) m_bi, low) != MP_OKAY)
            throw GeneralException("Error while calling mp_set_init");
    } else {
        if (mp_set_int((mp_int *) m_bi, high) != MP_OKAY)
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
    if (mp_set_int((mp_int *) m_bi, v) != MP_OKAY)
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
    if (mp_set_int((mp_int *) m_bi, 0) != MP_OKAY)
        throw GeneralException("Error while calling mp_set_init");

    for (e--; e > 0; e--) {
        f *= 2.0;
        if (f >= 1.0) {
            operator+=(1);
            f -= 1.0;
        }
        operator*=(2);
    }
}

void Balau::BigInt::set(const String & v, int radix) throw (GeneralException) {
    if (mp_read_radix((mp_int *) m_bi, v.to_charp(), radix) != MP_OKAY)
        throw GeneralException("Error while calling mp_read_radix");
}

void Balau::BigInt::set2expt(int i) throw (GeneralException) {
    if (mp_2expt((mp_int *) m_bi, i) != MP_OKAY)
        throw GeneralException("Error while calling mp_2expt");
}

static unsigned long get_digit(void * a, int n) {
    mp_int * A = (mp_int *) a;
    return (n >= A->used || n < 0) ? 0 : A->dp[n];
}

uint64_t Balau::BigInt::to_uint64() const throw (GeneralException) {
    if (mp_count_bits((mp_int *) m_bi) > 64)
        throw GeneralException("BigInt too big to fit in a uint64");
    uint64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 64) {
        v |= get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return v;
}

int64_t Balau::BigInt::to_int64() const throw (GeneralException) {
    if (mp_count_bits((mp_int *) m_bi) > 63)
        throw GeneralException("BigInt too big to fit in a int64");
    int64_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 63) {
        v |= get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return comp(0) == LT ? -v : v;
}

uint32_t Balau::BigInt::to_uint32() const throw (GeneralException) {
    if (mp_count_bits((mp_int *) m_bi) > 32)
        throw GeneralException("BigInt too big to fit in a uint32");
    uint32_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 32) {
        v |= get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return v;
}

int32_t Balau::BigInt::to_int32() const throw (GeneralException) {
    if (mp_count_bits((mp_int *) m_bi) > 31)
        throw GeneralException("BigInt too big to fit in a int32");
    int32_t v = 0;
    int shift = 0;
    int digit = 0;
    while (shift <= 31) {
        v |= get_digit(m_bi, digit++) << shift;
        shift += MP_DIGIT_BIT;
    }
    return comp(0) == LT ? -v : v;
}

Balau::BigInt Balau::BigInt::operator^(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_xor((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_xor");
    return r;
}

Balau::BigInt Balau::BigInt::operator|(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_or((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_or");
    return r;
}

Balau::BigInt Balau::BigInt::operator&(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_and((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_and");
    return r;
}

Balau::BigInt Balau::BigInt::operator+(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_add_d((mp_int *) m_bi, i, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_add_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator+(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_add((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_add");
    return r;
}

Balau::BigInt Balau::BigInt::operator-(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_sub_d((mp_int *) m_bi, i, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sub_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator-(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_sub((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sub");
    return r;
}

Balau::BigInt Balau::BigInt::operator*(unsigned int i) const throw (GeneralException) {
    BigInt r;
    if (mp_mul_d((mp_int *) m_bi, i, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mul_d");
    return r;
}

Balau::BigInt Balau::BigInt::operator*(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_mul((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mul");
    return r;
}

Balau::BigInt Balau::BigInt::operator/(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_div((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi, NULL) != MP_OKAY)
        throw GeneralException("Error while calling mp_div");
    return r;
}

Balau::BigInt Balau::BigInt::operator%(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_div((mp_int *) m_bi, (mp_int *) a.m_bi, NULL, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_div");
    return r;
}

Balau::BigInt Balau::BigInt::operator<<(unsigned int a) const throw (GeneralException) {
    BigInt r;
    if (mp_mul_d((mp_int *) m_bi, 1 << a, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt Balau::BigInt::operator>>(unsigned int a) const {
    BigInt s;
    s.set2expt(a);
    return operator/(s);
}

Balau::BigInt & Balau::BigInt::operator^=(const BigInt & a) throw (GeneralException) {
    if (mp_xor((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_xor");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator|=(const BigInt & a) throw (GeneralException) {
    BigInt r;
    if (mp_or((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_or");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator&=(const BigInt & a) throw (GeneralException) {
    BigInt r;
    if (mp_and((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_and");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator+=(unsigned int i) throw (GeneralException) {
    if (mp_add_d((mp_int *) m_bi, i, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_add_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator+=(const BigInt & a) throw (GeneralException) {
    if (mp_add((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_add");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator-=(unsigned int i) throw (GeneralException) {
    if (mp_sub_d((mp_int *) m_bi, i, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sub_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator-=(const BigInt & a) throw (GeneralException) {
    if (mp_sub((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sub");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator*=(unsigned int i) throw (GeneralException) {
    if (mp_mul_d((mp_int *) m_bi, i, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mul_d");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator*=(const BigInt & a) throw (GeneralException) {
    if (mp_mul((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mul");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator/=(const BigInt & a) throw (GeneralException) {
    if (mp_div((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m_bi, NULL) != MP_OKAY)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator%=(const BigInt & a) throw (GeneralException) {
    if (mp_div((mp_int *) m_bi, (mp_int *) a.m_bi, NULL, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_div");
    return *this;
}

Balau::BigInt & Balau::BigInt::operator<<=(unsigned int a) throw (GeneralException) {
    if (mp_mul_d((mp_int *) m_bi, 1 << a, (mp_int *) m_bi) != MP_OKAY)
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
    if (mp_neg((mp_int *) m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_neg");
    return r;
}

Balau::BigInt & Balau::BigInt::do_neg() throw (GeneralException) {
    if (mp_neg((mp_int *) m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_neg");
    return *this;
}

Balau::BigInt Balau::BigInt::sqrt() const throw (GeneralException) {
    BigInt r;
    if (mp_sqr((mp_int *) m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sqr");
    return r;
}

Balau::BigInt & Balau::BigInt::do_sqrt() throw (GeneralException) {
    if (mp_sqr((mp_int *) m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sqr");
    return *this;
}

Balau::BigInt Balau::BigInt::gcd(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_gcd((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_gcd");
    return r;
}

Balau::BigInt Balau::BigInt::lcm(const BigInt & a) const throw (GeneralException) {
    BigInt r;
    if (mp_lcm((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_lcm");
    return r;
}

Balau::BigInt::comp_t Balau::BigInt::comp(const BigInt & a) const throw (GeneralException) {
    int r = mp_cmp((mp_int *) m_bi, (mp_int *) a.m_bi);
    switch (r) {
    case MP_LT:
        return LT;
    case MP_GT:
        return GT;
    case MP_EQ:
        return EQ;
    default:
        throw GeneralException("Unknown result from mp_cmp");
    }
}

Balau::BigInt::comp_t Balau::BigInt::comp(unsigned int a) const throw (GeneralException) {
    int r = mp_cmp_d((mp_int *) m_bi, a);
    switch (r) {
    case MP_LT:
        return LT;
    case MP_GT:
        return GT;
    case MP_EQ:
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
    if (mp_addmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_addmod");
    return r;
}

Balau::BigInt Balau::BigInt::modsub(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_submod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_submod");
    return r;
}

Balau::BigInt Balau::BigInt::modmul(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_mulmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mulmod");
    return r;
}

Balau::BigInt Balau::BigInt::modsqr(const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_sqrmod((mp_int *) m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sqrmod");
    return r;
}

Balau::BigInt Balau::BigInt::modinv(const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_invmod((mp_int *) m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_invmod");
    return r;
}

Balau::BigInt Balau::BigInt::modpow(const BigInt & a, const BigInt & m) const throw (GeneralException) {
    BigInt r;
    if (mp_exptmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) r.m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_exptmod");
    return r;
}

Balau::BigInt & Balau::BigInt::do_modadd(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_addmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_addmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modsub(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_submod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_submod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modmul(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_mulmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_mulmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modsqr(const BigInt & m) throw (GeneralException) {
    if (mp_sqrmod((mp_int *) m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_sqrmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modinv(const BigInt & m) throw (GeneralException) {
    if (mp_invmod((mp_int *) m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_invmod");
    return *this;
}

Balau::BigInt & Balau::BigInt::do_modpow(const BigInt & a, const BigInt & m) throw (GeneralException) {
    if (mp_exptmod((mp_int *) m_bi, (mp_int *) a.m_bi, (mp_int *) m.m_bi, (mp_int *) m_bi) != MP_OKAY)
        throw GeneralException("Error while calling mp_exptmod");
    return *this;
}

bool Balau::BigInt::isPrime() const throw (GeneralException) {
    int r = 0;
    if (mp_prime_is_prime((mp_int *) m_bi, 256, &r) != MP_OKAY)
        throw GeneralException("Error while calling mp_prime_is_prime");
    return r == MP_YES;
}

size_t Balau::BigInt::exportSize() const {
    return mp_unsigned_bin_size((mp_int *) m_bi) + 1;
}

size_t Balau::BigInt::exportUSize() const {
    return mp_unsigned_bin_size((mp_int *) m_bi);
}

void Balau::BigInt::exportBin(void * _buf) const throw (GeneralException) {
    unsigned char * buf = (unsigned char *) _buf;
    buf[0] = comp(0) == LT ? 0xff : 0;
    if (mp_to_unsigned_bin((mp_int *) m_bi, buf + 1) != MP_OKAY)
        throw GeneralException("Error while calling mp_to_unsigned_bin");
}

void Balau::BigInt::exportUBin(void * _buf) const throw (GeneralException) {
    unsigned char * buf = (unsigned char *)_buf;
    if (mp_to_unsigned_bin((mp_int *) m_bi, buf) != MP_OKAY)
        throw GeneralException("Error while calling mp_to_unsigned_bin");
}

void Balau::BigInt::importBin(const void * _buf, size_t size) throw (GeneralException) {
    unsigned char * buf = (unsigned char *) _buf;
    AAssert(size < std::numeric_limits<unsigned long>::max(), "BigInt::importBin(%p, %zu): size too big", _buf, size);
    bool isNeg = buf[0] != 0;
    if (mp_read_unsigned_bin((mp_int *) m_bi, buf + 1, (unsigned long) size - 1) != MP_OKAY)
        throw GeneralException("Error while calling mp_read_unsigned_bin");
    if (isNeg)
        do_neg();
}

void Balau::BigInt::importUBin(const void * _buf, size_t size) throw (GeneralException) {
    unsigned char * buf = (unsigned char *)_buf;
    AAssert(size < std::numeric_limits<unsigned long>::max(), "BigInt::importBin(%p, %zu): size too big", _buf, size);
    if (mp_read_unsigned_bin((mp_int *)m_bi, buf, (unsigned long) size) != MP_OKAY)
        throw GeneralException("Error while calling mp_read_unsigned_bin");
}

Balau::String Balau::BigInt::toString(int radix) const {
    char * out = (char *) alloca(mp_count_bits((mp_int *) m_bi) / (radix >= 10 ? 3 : 1) + 3);
    mp_toradix((mp_int *) m_bi, out, radix);
    return String(out);
}

char * Balau::BigInt::makeString(int radix) const {
    char * out = (char *) malloc(mp_count_bits((mp_int *) m_bi) / (radix >= 10 ? 3 : 1) + 3);
    mp_toradix((mp_int *) m_bi, out, radix);
    return out;
}
