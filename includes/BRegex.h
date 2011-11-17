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
    Captures match(const char * str) throw (GeneralException);
  private:
    String getError(int err);
    regex_t m_regex;
};

};
