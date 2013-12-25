#pragma once

#include <HttpServer.h>

namespace Balau {

class HttpActionStatic : public HttpServer::Action {
  public:
      // the regex needs a capture on the file name.
      HttpActionStatic(const String & base, Regex & url) : Action(url), m_base(base) { }
  private:
    virtual bool Do(HttpServer * server, Http::Request & req, HttpServer::Action::ActionMatch & match, IO<Handle> out) throw (GeneralException);
    String m_base;
};

};
