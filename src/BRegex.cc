#include "BRegex.h"

Balau::Regex::Regex(const char * regex, bool icase) throw (GeneralException) {
    int r = regcomp(&m_regex, regex, REG_EXTENDED | (icase ? REG_ICASE : 0));
    if (r)
        throw GeneralException(getError(r));
}

Balau::Regex::Captures Balau::Regex::match(const char * str) throw (GeneralException) {
    Captures ret;
    regmatch_t * matches = (regmatch_t *) alloca((m_regex.re_nsub + 1) * sizeof(regmatch_t));

    int r = regexec(&m_regex, str, m_regex.re_nsub + 1, matches, 0);

    if (r == REG_NOMATCH)
        return ret;

    if (r != 0)
        throw GeneralException(getError(r));

    for (int i = 0; i < m_regex.re_nsub + 1; i++) {
        if (matches[i].rm_so >= 0) {
            regoff_t begin = matches[i].rm_so;
            regoff_t end = matches[i].rm_eo;
            String t(str + begin, end - begin);
            ret.push_back(t);
        }
    }

    return ret;
}

Balau::Regex::~Regex() {
    regfree(&m_regex);
}

Balau::String Balau::Regex::getError(int err) {
    size_t s;
    char * t;

    s = regerror(err, &m_regex, NULL, 0);
    t = (char *) malloc(s);
    regerror(err, &m_regex, t, s);
    String r(t, s - 1);
    free(t);

    return r;
}

Balau::Regex Balau::Regexes::any(".*");
Balau::Regex Balau::Regexes::empty("^$");
