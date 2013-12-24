#include <map>

#include "Http.h"

template<typename I>
const char * getMap(const std::map<I, const char *> & map, I idx, const char * defStr) {
    auto t = map.find(idx);
    if (t == map.end())
        return defStr;
    return t->second;
}


static const std::map<int, const char *> s_statusMap {
    std::make_pair(100, "Continue"),
    std::make_pair(101, "Switching Protocols"),
    std::make_pair(200, "OK"),
    std::make_pair(201, "Created"),
    std::make_pair(202, "Accepted"),
    std::make_pair(203, "Non-Authoritative Information"),
    std::make_pair(204, "No Content"),
    std::make_pair(205, "Reset Content"),
    std::make_pair(206, "Partial Content"),
    std::make_pair(300, "Multiple Choices"),
    std::make_pair(301, "Moved Permanently"),
    std::make_pair(302, "Found"),
    std::make_pair(303, "See Other"),
    std::make_pair(304, "Not Modified"),
    std::make_pair(305, "Use Proxy"),
    std::make_pair(307, "Temporary Redirect"),
    std::make_pair(400, "Bad Request"),
    std::make_pair(401, "Unauthorized"),
    std::make_pair(402, "Payment Required"),
    std::make_pair(403, "Forbidden"),
    std::make_pair(404, "Not Found"),
    std::make_pair(405, "Method Not Allowed"),
    std::make_pair(406, "Not Acceptable"),
    std::make_pair(407, "Proxy Authentication Required"),
    std::make_pair(408, "Request Timeout"),
    std::make_pair(409, "Conflict"),
    std::make_pair(410, "Gone"),
    std::make_pair(411, "Length Required"),
    std::make_pair(412, "Precondition Failed"),
    std::make_pair(413, "Request Entity Too Large"),
    std::make_pair(414, "Request-URI Too Long"),
    std::make_pair(415, "Unsupported Media Type"),
    std::make_pair(416, "Requested Range Not Satisfiable"),
    std::make_pair(417, "Expectation Failed"),
    std::make_pair(418, "I'm a teapot"),
    std::make_pair(500, "Internal Error"),
    std::make_pair(501, "Not Implemented"),
    std::make_pair(502, "Bad Gateway"),
    std::make_pair(503, "Service Unavailable"),
    std::make_pair(504, "Gateway Timeout"),
    std::make_pair(505, "HTTP Version Not Supported"),
};

const char * Balau::Http::getStatusMsg(int httpStatus) {
    return getMap(s_statusMap, httpStatus, "Unknown HTTP code");
}


static const std::map<const Balau::String, const char *> s_mimeMap {
    std::make_pair("css",  "text/css"),
    std::make_pair("html", "text/html"),
    std::make_pair("js",   "application/javascript"),
    std::make_pair("json", "application/json"),
    std::make_pair("png",  "image/png"),
    std::make_pair("gif",  "image/gif"),
};

const char * Balau::Http::getContentType(const String & extension) {
    return getMap<const Balau::String>(s_mimeMap, extension, "application/octet-stream");
}
