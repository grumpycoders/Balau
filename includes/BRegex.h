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
      Regex(const char * regex, bool icase = false) throw (GeneralException);
      ~Regex();
    Captures match(const char * str) const throw (GeneralException);
  private:
    String getError(int err) const;
    regex_t m_regex;
};

class Regexes {
  public:
    static const Regex any;
    static const Regex empty;
};

};
