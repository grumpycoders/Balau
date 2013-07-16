#include "StdIO.h"

/** stdin **/
Balau::StdIN::StdIN() {
    setFD(0);
}

const char * Balau::StdIN::getName() {
    return "stdin";
}

void Balau::StdIN::close() throw (GeneralException) {
    ::close(0);
    internalClose();
}

ssize_t Balau::StdIN::recv(int sockfd, void *buf, size_t len, int flags) {
    IAssert(sockfd == 0, "StdIN::recv called, but not on stdin");

    return ::read(0, buf, len);
}

ssize_t Balau::StdIN::send(int sockfd, const void *buf, size_t len, int flags) {
    return 0;
}

/** stdout **/
Balau::StdOUT::StdOUT() {
    setFD(1);
}

const char * Balau::StdOUT::getName() {
    return "stdout";
}

void Balau::StdOUT::close() throw (GeneralException) {
    ::close(1);
    internalClose();
}

ssize_t Balau::StdOUT::recv(int sockfd, void *buf, size_t len, int flags) {
    return 0;
}

ssize_t Balau::StdOUT::send(int sockfd, const void *buf, size_t len, int flags) {
    IAssert(sockfd == 1, "StdOUT::send called, but not on stdout");
    return ::write(1, buf, len);
}

/** stderr **/
Balau::StdERR::StdERR() {
    setFD(2);
}

const char * Balau::StdERR::getName() {
    return "stderr";
}

void Balau::StdERR::close() throw (GeneralException) {
    ::close(2);
    internalClose();
}

ssize_t Balau::StdERR::recv(int sockfd, void *buf, size_t len, int flags) {
    return 0;
}

ssize_t Balau::StdERR::send(int sockfd, const void *buf, size_t len, int flags) {
    IAssert(sockfd == 2, "StdERR::send called, but not on stderr");
    return ::write(2, buf, len);
}

