#include <iconv.h>
#include <ctype.h>

#include "Printer.h"
#include "BString.h"

void Balau::String::set(const char * fmt, va_list ap) {
    unsigned int l;
#ifdef _WIN32
    // Microsoft is stupid.
    char tt[65536];
    l = _vsnprintf(tt, sizeof(tt) - 1, fmt, ap);
    tt[65535] = 0;
    assign(tt, l);
#else
    char * t;
    l = vasprintf(&t, fmt, ap);
    assign(t, l);
    free(t);
#endif
}

int Balau::String::strchrcnt(char c) const {
    unsigned int l = length();
    int r = 0;
    const char * buffer = data();

    for (unsigned int i = 0; i < l; i++)
        if (buffer[i] == c)
            r++;

    return r;
}

Balau::String & Balau::String::do_ltrim() {
    unsigned int l = length(), s = 0;
    const char * buffer = data();

    for (unsigned int i = 0; i < l; i++)
        if (isspace(buffer[i]))
            s++;
        else
            break;

    erase(0, s);

    return *this;
}

Balau::String & Balau::String::do_rtrim() {
    unsigned int l = length(), p = l;
    const char * buffer = data();

    for (unsigned int i = l - 1; i >= 0; i--)
        if (isspace(buffer[i]))
            p--;
        else
            break;

    erase(p);

    return *this;
}

Balau::String & Balau::String::do_upper() {
    unsigned int l = length();

    for (unsigned int i = 0; i < l; i++)
        (*this)[i] = toupper((*this)[i]);

    return *this;
}

Balau::String & Balau::String::do_lower() {
    unsigned int l = length();

    for (unsigned int i = 0; i < l; i++)
        (*this)[i] = tolower((*this)[i]);

    return *this;
}

Balau::String & Balau::String::do_iconv(const char * from, const char * _to) {
    iconv_t cd;
    const String to = String(_to) + "//TRANSLIT";
    Printer::elog(E_STRING, "Converting a string from %s to %s", from, _to);

    const char * inbuf;
    char * outbuf, * t;
    size_t inleft, outleft;

    if ((cd = iconv_open(to.c_str(), from)) == (iconv_t) (-1))
        return *this;

    inleft = length();
    outleft = inleft * 8;
    inbuf = c_str();
    t = outbuf = (char *) malloc(outleft + 1);
    memset(t, 0, outleft + 1);
#ifdef HAVE_PROPER_ICONV
    ::iconv(cd, &inbuf, &inleft, &outbuf, &outleft);
#else
    ::iconv(cd, const_cast<char **>(&inbuf), &inleft, &outbuf, &outleft);
#endif

    assign(t, outbuf - t);
    free(t);

    return *this;
}
