#pragma once

namespace Balau {

namespace Http {

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
