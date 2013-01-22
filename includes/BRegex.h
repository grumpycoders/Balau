#pragma once

#include <vector>
#include <sys/types.h>
#include <regex.h>
#include <BString.h>
#include <Exceptions.h>

namespace Balau {

class Regex {
  public:
    typedef std::vector<String> Captures;
      Regex(const String & regex, bool icase = false) : Regex(regex.to_charp(), icase) { }
      Regex(const Regex & regex) : Regex(regex.m_regexStr, regex.m_icase) { }
      Regex(Regex && regex) { regex.m_moved = true; m_regex = regex.m_regex; }
      Regex(const char * regex, bool icase = false) throw (GeneralException);
      ~Regex() { if (!m_moved) regfree(&m_regex); }
    Captures match(const char * str) const throw (GeneralException);
  private:
      Regex & operator=(const Regex &) = delete;
    String getError(int err) const;
    String m_regexStr;
    regex_t m_regex;
    bool m_icase, m_moved = false;
};

class Regexes {
  public:
    static const Regex any;
    static const Regex empty;
};

};
