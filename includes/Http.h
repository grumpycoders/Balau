#pragma once

#include <map>
#include <BString.h>
#include <Handle.h>

#ifdef _MSC_VER
// SERIOUSLY ?!
#undef DELETE
#undef ERROR
#endif

namespace Balau {

namespace Http {

const char * getStatusMsg(int httpStatus);
const char * getContentType(const String & extension);
String percentEncode(const String & src);
String percentDecode(const String & src);

typedef std::map<String, String> StringMap;
typedef std::multimap<String, String> StringMultiMap;
typedef std::map<String, IO<Handle> > FileList;

struct Request {
    int method;
    String host;
    String uri;
    StringMap variables;
    StringMap cookies;
    StringMap headers; // this needs to become a StringMultiMap
    FileList files;
    bool persistent;
    bool upgrade;
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
