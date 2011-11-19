#pragma once

#include <map>
#include <BString.h>
#include <Handle.h>

namespace Balau {

namespace Http {

typedef std::map<String, String> StringMap;
typedef std::map<String, IO<Handle> > FileList;

class Request {
  public:
    int method;
    String host;
    String uri;
    StringMap variables;
    StringMap headers;
    FileList files;
    bool persistent;
};

enum {
    GET,
    POST,

    REDIRECT = 301,

    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,

    ERROR = 500,
};

};

};
