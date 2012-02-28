#pragma once

#include <map>
#include <BString.h>
#include <Handle.h>

namespace Balau {

namespace Http {

const char * getStatusMsg(int httpStatus);

typedef std::map<String, String> StringMap;
typedef std::map<String, IO<Handle> > FileList;

struct Request {
    int method;
    String host;
    String uri;
    StringMap variables;
    StringMap headers;
    FileList files;
    bool persistent;
    String version;
};

enum {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    OPTIONS,
    CONNECT,
    BREW,
    PROPFIND,
    WHEN,

    REDIRECT = 301,

    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,

    ERROR = 500,
};

};

};
