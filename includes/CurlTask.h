#pragma once

#include <curl/curl.h>
#include <Task.h>
#include <TaskMan.h>

namespace Balau {

class CurlTask : public Task {
    friend class TaskMan;
  protected:
    CURL * m_curlHandle;
};

};
