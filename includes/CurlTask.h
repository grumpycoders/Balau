#pragma once

#include <curl/curl.h>
#include <StacklessTask.h>
#include <TaskMan.h>

namespace Balau {

class CurlTask : public StacklessTask {
public:
    CurlTask();
    friend class TaskMan;
  protected:
    CURL * m_curlHandle;
private:
    void curlDone(CURLcode result);

    static  size_t writeFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata);
    virtual size_t writeFunction(char * ptr, size_t size, size_t nmemb) { return size * nmemb; }
    static  size_t readFunctionStatic(void * ptr, size_t size, size_t nmemb, void * userdata);
    virtual size_t readFunction(void * ptr, size_t size, size_t nmemb) { return CURL_READFUNC_ABORT; }
    static  int    debugFunctionStatic(CURL * easy, curl_infotype info, char * str, size_t str_len, void * userdata);
    virtual int    debugFunction(curl_infotype info, char * str, size_t str_len) { return 0; }
};

};
