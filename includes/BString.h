#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#ifdef _WIN64
typedef __int64    ssize_t;
#else  /* _WIN64 */
typedef _W64 int   ssize_t;
#endif  /* _WIN64 */
#define printfwarning(a, b)
#else
#define printfwarning(a, b) __attribute__((format(printf, a, b)))
#endif

#if defined(_WIN32) && !defined(_MSC_VER)
int vsscanf(const char *, const char *, va_list);
#endif

namespace Balau {

class String : private std::string {
  public:
      String() : std::string() { }
      String(const char * str) : std::string(str ? str : "") { }
      template<size_t L>
      String(const char (&str)[L]) : std::string(str, L - 1) { }
      String(const char * str, size_t n) : std::string(str ? str : "", str ? n : 0) { }
      String(char c) { set("%c", c); }
      String(int32_t i) { set("%" PRIi32, i); }
      String(uint32_t i) { set("%" PRIu32, i); }
      String(int64_t i) { set("%" PRIi64, i); }
      String(uint64_t i) { set("%" PRIu64, i); }
      String(double d) { set("%g", d); }
      String(const String & s) : std::string(s) { }
      String(const std::string & s) : std::string(s) { }
      String(String && s) : std::string(s) { }

    String & vset(const char * fmt, va_list) printfwarning(2, 0);
    String & set(const char * fmt, ...) printfwarning(2, 3) { va_list ap; va_start(ap, fmt); vset(fmt, ap); va_end(ap); return *this; }
    template<typename... Args>
    String & sset(const String & fmt, Args... args) { set(fmt.to_charp(), args...); return *this; }

    String & vappend(const char * fmt, va_list) printfwarning(2, 0);
    String & append(const char * fmt, ...) printfwarning(2, 3) { va_list ap; va_start(ap, fmt); vappend(fmt, ap); va_end(ap); return *this; }
    template<typename... Args>
    String & sappend(const String & fmt, Args... args) { append(fmt.to_charp(), args...); return *this; }

    int vscanf(const char * fmt, va_list ap) const { return ::vsscanf(c_str(), fmt, ap); }
    int scanf(const char * fmt, ...) const printfwarning(2, 3) { va_list ap; va_start(ap, fmt); int r = vscanf(fmt, ap); va_end(ap); return r; }
    template<typename... Args>
    int sscanf(const String & fmt, Args... args) const { return scanf(fmt.to_charp(), args...); }

    const char * to_charp(size_t begin = 0) const { return c_str() + begin; }
    String extract(size_t begin = 0, size_t size = static_cast<size_t>(-1)) const { return substr(begin, size); }
    char * strdup(size_t begin = 0, size_t size = static_cast<size_t>(-1)) const {
        if (begin == 0)
            return ::strdup(to_charp());
        else
            return ::strdup(extract(begin, size).to_charp());
    }

    int to_int(int base = 0) const { return strtol(to_charp(), NULL, base); }
    unsigned to_uint(int base = 0) const { return strtoul(to_charp(), NULL, base); }
    int64_t to_int64(int base = 0) const { return strtoll(to_charp(), NULL, base); }
    uint64_t to_uint64(int base = 0) const { return strtoull(to_charp(), NULL, base); }
    double to_double() const { return strtod(to_charp(), NULL); }
    typedef std::vector<String> List;
    List split(char c);

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
    String replace_all(char from, char to) const { String c = *this; c.do_replace_all(from, to); return c; }

    String & do_ltrim();
    String & do_rtrim();
    String & do_trim() { return do_ltrim().do_rtrim(); }
    String & do_upper();
    String & do_lower();
    String & do_iconv(const String & from, const String & to) { return do_iconv(from.to_charp(), to.to_charp()); }
    String & do_iconv(const char * from, const char * to);
    String & do_replace_all(char from, char to) { std::replace(begin(), end(), from, to); return *this; }

    String & operator=(const String & v) { *((std::string *) this) = std::string::assign(v); return *this; }
    String & operator=(const char * v) { *((std::string *) this) = std::string::assign(v); return *this; }

    String operator+(const String & v) const { String r = *this; r += v; return r; }
    String operator+(const char * v) const { String r = *this; r += v; return r; }
    String & operator+=(const String & v) { *this = std::string::append(v); return *this; }
    String & operator+=(const char * v) { *this = std::string::append(v); return *this; }

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

    const char & operator[](ssize_t i) const { if (i < 0) i = strlen() + i; return at(i); }
    char & operator[](ssize_t i) { if (i < 0) i = strlen() + i; return at(i); }

    void reserve(size_t s) { std::string::reserve(s); }

    size_t find_first_of(const String & set, size_t offset = 0) const { return std::string::find_first_of(set, offset); }
    size_t find_last_of(const String & set, size_t offset = 0) const { return std::string::find_last_of(set, offset); }
    size_t find_first_not_of(const String & set, size_t offset = 0) const { return std::string::find_first_not_of(set, offset); }
    size_t find_last_not_of(const String & set, size_t offset = 0) const { return std::string::find_last_not_of(set, offset); }

    bool isEmpty() { return std::string::empty(); }

    std::vector<String> tokenize(const String & delimiters = "&", bool trimEmpty = true);


    using std::string::npos;
    using std::string::size_type;
};

};
