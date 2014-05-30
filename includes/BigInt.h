#pragma once

#include <stdint.h>
#include <Exceptions.h>
#include <BString.h>

namespace Balau {

class BigInt {
  public:
      BigInt() throw(GeneralException);
      BigInt(const BigInt &) throw (GeneralException);
      BigInt(BigInt &&);
      ~BigInt();
      template<class T>
      BigInt(const T & v) : BigInt() { set(v); }

    BigInt & operator=(const BigInt &) throw (GeneralException);

    void set(uint64_t) throw (GeneralException);
    void set(int64_t);
    void set(uint32_t) throw (GeneralException);
    void set(int32_t);
    void set(double) throw (GeneralException);
    void set(const String &, int radix = 10) throw (GeneralException);
    void set2expt(int i) throw (GeneralException);

    uint64_t to_uint64() const throw (GeneralException);
    int64_t to_int64() const throw (GeneralException);
    uint32_t to_uint32() const throw (GeneralException);
    int32_t to_int32() const throw (GeneralException);

    BigInt operator^(const BigInt &) const throw (GeneralException);
    BigInt operator|(const BigInt &) const throw (GeneralException);
    BigInt operator&(const BigInt &) const throw (GeneralException);
    BigInt operator+(unsigned int) const throw (GeneralException);
    BigInt operator+(const BigInt &) const throw (GeneralException);
    BigInt operator-(unsigned int) const throw (GeneralException);
    BigInt operator-(const BigInt &) const throw (GeneralException);
    BigInt operator*(unsigned int) const throw (GeneralException);
    BigInt operator*(const BigInt &) const throw (GeneralException);
    BigInt operator/(const BigInt &) const throw (GeneralException);
    BigInt operator%(const BigInt &) const throw (GeneralException);
    BigInt operator<<(unsigned int) const throw (GeneralException);
    BigInt operator>>(unsigned int) const;

    BigInt & operator^=(const BigInt &) throw (GeneralException);
    BigInt & operator|=(const BigInt &) throw (GeneralException);
    BigInt & operator&=(const BigInt &) throw (GeneralException);
    BigInt & operator+=(unsigned int) throw (GeneralException);
    BigInt & operator+=(const BigInt &) throw (GeneralException);
    BigInt & operator-=(unsigned int) throw (GeneralException);
    BigInt & operator-=(const BigInt &) throw (GeneralException);
    BigInt & operator*=(unsigned int) throw (GeneralException);
    BigInt & operator*=(const BigInt &) throw (GeneralException);
    BigInt & operator/=(const BigInt &) throw (GeneralException);
    BigInt & operator%=(const BigInt &) throw (GeneralException);
    BigInt & operator<<=(unsigned int) throw (GeneralException);
    BigInt & operator>>=(unsigned int);

    BigInt operator-() const throw (GeneralException);

    BigInt & operator++();
    BigInt operator++(int);
    BigInt & operator--();
    BigInt operator--(int);

    enum comp_t { LT, GT, EQ };
    comp_t comp(const BigInt &) const throw (GeneralException);
    comp_t comp(unsigned int) const throw (GeneralException);

    BigInt neg() const throw (GeneralException);
    BigInt & do_neg() throw (GeneralException);

    BigInt sqrt() const throw (GeneralException);
    BigInt & do_sqrt() throw (GeneralException);

    BigInt gcd(const BigInt &) const throw (GeneralException);
    BigInt lcm(const BigInt &) const throw (GeneralException);

    bool operator==(unsigned int) const;
    bool operator==(const BigInt &) const;
    bool operator!=(unsigned int) const;
    bool operator!=(const BigInt &) const;
    bool operator<=(unsigned int) const;
    bool operator<=(const BigInt &) const;
    bool operator>=(unsigned int) const;
    bool operator>=(const BigInt &) const;
    bool operator<(unsigned int) const;
    bool operator<(const BigInt &) const;
    bool operator>(unsigned int) const;
    bool operator>(const BigInt &) const;

    BigInt modadd(const BigInt & a, const BigInt & m) const throw (GeneralException);
    BigInt modsub(const BigInt & a, const BigInt & m) const throw (GeneralException);
    BigInt modmul(const BigInt & a, const BigInt & m) const throw (GeneralException);
    BigInt modsqr(const BigInt & m) const throw (GeneralException);
    BigInt modinv(const BigInt & m) const throw (GeneralException);
    BigInt modpow(const BigInt & a, const BigInt & m) const throw (GeneralException);

    BigInt & do_modadd(const BigInt & a, const BigInt & m) throw (GeneralException);
    BigInt & do_modsub(const BigInt & a, const BigInt & m) throw (GeneralException);
    BigInt & do_modmul(const BigInt & a, const BigInt & m) throw (GeneralException);
    BigInt & do_modsqr(const BigInt & m) throw (GeneralException);
    BigInt & do_modinv(const BigInt & m) throw (GeneralException);
    BigInt & do_modpow(const BigInt & a, const BigInt & m) throw (GeneralException);

    bool isPrime() const throw (GeneralException);

    size_t exportSize() const;
    void exportBin(void *) const throw (GeneralException);
    void importBin(const void *, size_t) throw (GeneralException);

    size_t exportUSize() const;
    void exportUBin(void *) const throw (GeneralException);
    void importUBin(const void *, size_t) throw (GeneralException);

    String toString(int radix = 10) const;
    char * makeString(int radix = 10) const;

  private:
    void * m_bi = NULL;
};

};
