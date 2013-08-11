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

    BigInt & operator=(const BigInt &) throw (GeneralException);

    void set(uint64_t) throw (GeneralException);
    void set(int64_t);
    void set(uint32_t) throw (GeneralException);
    void set(int32_t);
    void set(double) throw (GeneralException);
    void set(const String &, int radix = 10) throw (GeneralException);
    void set2expt(int i) throw (GeneralException);

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

    BigInt & neg() throw (GeneralException);

    BigInt sqrt() const throw (GeneralException);
    BigInt & do_sqrt() throw (GeneralException);

    BigInt gcd(const BigInt &) const throw (GeneralException);
    BigInt lcm(const BigInt &) const throw (GeneralException);

    bool operator==(const BigInt &) const;
    bool operator!=(const BigInt &) const;
    bool operator<=(const BigInt &) const;
    bool operator>=(const BigInt &) const;
    bool operator<(const BigInt &) const;
    bool operator>(const BigInt &) const;

    String toString(int radix = 10) const;
    char * makeString(int radix = 10) const;

  private:
    void * m_bi = NULL;
};

};
