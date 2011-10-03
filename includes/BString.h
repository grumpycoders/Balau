#pragma once

#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include <string>

namespace Balau {

class String : private std::string {
  public:
      String() : std::string() { }
      String(const char * str) : std::string(str) { }
      String(const char * str, size_t n) : std::string(str, n) { }
      String(char c) { set("%c", c); }
      String(int32_t i) { set("%i", i); }
      String(uint32_t i) { set("%u", i); }
      String(int64_t i) { set("%lli", i); }
      String(uint64_t i) { set("%llu", i); }
      String(double d) { set("%g", d); }
      String(const String & s) : std::string(s) { }
      String(const std::string & s) : std::string(s) { }

    void set(const char * fmt, va_list);
    void set(const char * fmt, ...) { va_list ap; va_start(ap, fmt); set(fmt, ap); va_end(ap); }
    void set(const String & fmt, ...) { va_list ap; va_start(ap, fmt); set(fmt.to_charp(), ap); va_end(ap); }

    int scanf(const char * fmt, va_list ap) const { return ::vsscanf(c_str(), fmt, ap); }
    int scanf(const char * fmt, ...) const { va_list ap; va_start(ap, fmt); int r = scanf(fmt, ap); va_end(ap); return r; }
    int scanf(const String & fmt, ...) const { va_list ap; va_start(ap, fmt); int r = scanf(fmt.to_charp(), ap); va_end(ap); return r; }

    const char * to_charp(size_t begin = 0) const { return c_str() + begin; }
    String extract(size_t begin = 0, size_t size = static_cast<size_t>(-1)) const { return substr(begin, size); }
    char * strdup(size_t begin = 0, size_t size = static_cast<size_t>(-1)) const {
        if (begin == 0)
            return ::strdup(to_charp());
        else
            return ::strdup(extract(begin, size).to_charp());
    }

    int to_int(int base = 0) const { return strtol(to_charp(), NULL, base); }
    double to_double() const { return strtod(to_charp(), NULL); }

    size_t strlen() const { return std::string::length(); }
    ssize_t strchr(char c, size_t begin = 0) const { return find(c, begin); }
    ssize_t strrchr(char c) const { return rfind(c); }
    ssize_t strstr(const String & s, size_t begin = 0) const { return find(std::string(s), begin); }
    ssize_t strstr(const char * s, size_t begin = 0) const { return find(std::string(s), begin); }
    int strchrcnt(char) const;

    String ltrim() const { String c = *this; return c.do_ltrim(); }
    String rtrim() const { String c = *this; return c.do_rtrim(); }
    String trim() const { String c = *this; return c.do_trim(); }
    String upper() const { String c = *this; return c.do_upper(); }
    String lower() const { String c = *this; return c.do_lower(); }
    String iconv(const String & from, const String & to) const { String c = *this; c.do_iconv(from.to_charp(), to.to_charp()); return c; }
    String iconv(const char * from, const char * to) const { String c = *this; c.do_iconv(from, to); return c; }

    String & do_ltrim();
    String & do_rtrim();
    String & do_trim() { return do_ltrim().do_rtrim(); }
    String & do_upper();
    String & do_lower();
    String & do_iconv(const String & from, const String & to) { return do_iconv(from.to_charp(), to.to_charp()); }
    String & do_iconv(const char * from, const char * to);

    String & operator=(const String & v) { *((std::string *) this) = std::string::assign(v); return *this; }
    String & operator=(const char * v) { *((std::string *) this) = std::string::assign(v); return *this; }

    String operator+(const String & v) const { String r = *this; r += v; return r; }
    String operator+(const char * v) const { String r = *this; r += v; return r; }
    String & operator+=(const String & v) { *this = append(v); return *this; }
    String & operator+=(const char * v) { *this = append(v); return *this; }

    int compare(const String & v) const { return std::string::compare(v); }
    int compare(const char * v) const { return std::string::compare(v); }

    bool operator==(const String & v) const { return compare(v) == 0; }
    bool operator==(const char * v) const { return compare(v) == 0; }
    bool operator!=(const String & v) const { return compare(v) != 0; }
    bool operator!=(const char * v) const { return compare(v) != 0; }
    bool operator<(const String & v) const { return compare(v) < 0; }
    bool operator<(const char * v) const { return compare(v) < 0; }
    bool operator<=(const String & v) const { return compare(v) <= 0; }
    bool operator<=(const char * v) const { return compare(v) <= 0; }
    bool operator>(const String & v) const { return compare(v) > 0; }
    bool operator>(const char * v) const { return compare(v) > 0; }
    bool operator>=(const String & v) const { return compare(v) >= 0; }
    bool operator>=(const char * v) const { return compare(v) >= 0; }

    const char & operator[](size_t i) const { return at(i); }
    char & operator[](size_t i) { return at(i); }
};

};
