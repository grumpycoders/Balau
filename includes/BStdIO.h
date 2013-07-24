#pragma once

#include <Handle.h>
#include <Selectable.h>

namespace Balau {

// these classes might very well need to have their own
// special version for win32; at least from a Handle.
class StdIN : public Selectable {
  public:
      StdIN();
    virtual const char * getName();
    virtual void close() throw (GeneralException);
  private:
    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags);
};

class StdOUT : public Selectable {
  public:
      StdOUT();
    virtual const char * getName();
    virtual void close() throw (GeneralException);
  private:
    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags);
};

class StdERR : public Selectable {
  public:
      StdERR();
    virtual const char * getName();
    virtual void close() throw (GeneralException);
  private:
    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags);
};

};
