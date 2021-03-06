#include <iconv.h>
#include <ctype.h>

#include "Exceptions.h"
#include "Printer.h"
#include "BString.h"

Balau::String & Balau::String::vset(const char * fmt, va_list ap) {
    unsigned int l;
    char * t = NULL;
#ifdef _WIN32
    l = _vscprintf(fmt, ap);
    if (l >= 0) {
        t = (char *) Alloc::malloc(l + 1);
        l = vsnprintf_s(t, l + 1, _TRUNCATE, fmt, ap);
    }
#else
    l = vasprintf(&t, fmt, ap);
#endif
    if (t) {
        assign(t, l);
        free(t);
    } else {
        clear();
    }

    return *this;
}

Balau::String & Balau::String::vappend(const char * fmt, va_list ap) {
    unsigned int l;
    char * t = NULL;
#ifdef _WIN32
    l = _vscprintf(fmt, ap);
    if (l >= 0) {
        t = (char *) Alloc::malloc(l + 1);
        l = vsnprintf_s(t, l + 1, _TRUNCATE, fmt, ap);
    }
#else
    l = vasprintf(&t, fmt, ap);
#endif
    if (t) {
        std::string::append(t, l);
        free(t);
    }

    return *this;
}

int Balau::String::strchrcnt(char c) const {
    size_t l = length();
    int r = 0;
    const char * buffer = data();

    for (unsigned int i = 0; i < l; i++) {
        if (buffer[i] == c) {
            r++;
        }
    }

    return r;
}

Balau::String & Balau::String::do_ltrim() {
    size_t l = length(), s = 0;
    const char * buffer = data();

    for (size_t i = 0; i < l; i++) {
        if (isspace(buffer[i])) {
            s++;
        } else {
            break;
        }
    }

    erase(0, s);

    return *this;
}

Balau::String & Balau::String::do_rtrim() {
    size_t i, l = length(), p = l;
    const char * buffer = data();

    if (l == 0) {
        return *this;
    }

    for (i = l - 1; i > 0; i--) {
        if (isspace(buffer[i])) {
            p--;
        } else {
            break;
        }
    }

    if ((i == 0) && isspace(buffer[0])) {
        clear();
    } else {
        erase(p);
    }

    return *this;
}

Balau::String & Balau::String::do_upper() {
    size_t l = length();

    for (size_t i = 0; i < l; i++)
        (*this)[i] = toupper((*this)[i]);

    return *this;
}

Balau::String & Balau::String::do_lower() {
    size_t l = length();

    for (size_t i = 0; i < l; i++) {
        (*this)[i] = tolower((*this)[i]);
    }

    return *this;
}

Balau::String & Balau::String::do_iconv(const char * from, const char * _to) {
    iconv_t cd;
    const String to = String(_to) + "//TRANSLIT";
    Printer::elog(E_STRING, "Converting a string from %s to %s", from, _to);

    const char * inbuf;
    char * outbuf, * t;
    size_t inleft, outleft;

    if ((cd = iconv_open(to.c_str(), from)) == (iconv_t)(-1)) {
        return *this;
    }

    inleft = length();
    outleft = inleft * 8;
    inbuf = c_str();
    t = outbuf = (char *) Alloc::malloc(outleft + 1);
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

Balau::String::List Balau::String::split(char c) {
    char * d, * p, * f;
    List r;

    d = p = strdup();

    while (true) {
        f = ::strchr(p, c);
        if (!f) break;
        *f = 0;
        r.push_back(p);
        p = f + 1;
    }

    r.push_back(p);
    free(d);
    return r;
}

std::vector<Balau::String> Balau::String::tokenize(const String & delimiters, bool trimEmpty) {
    std::vector<String> tokens;
    size_t pos, lastPos = 0;
    for (;;) {
        pos = find_first_of(delimiters, lastPos);
        if (pos == String::npos) {
            pos = strlen();

            if ((pos != lastPos) || !trimEmpty) {
                tokens.push_back(extract(lastPos, pos - lastPos));
            }

            return tokens;
        } else {
            if ((pos != lastPos) || !trimEmpty) {
                tokens.push_back(extract(lastPos, pos - lastPos));
            }
        }

        lastPos = pos + 1;
    }
}
