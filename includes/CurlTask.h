#pragma once

#include <curl/curl.h>
#include <StacklessTask.h>
#include <TaskMan.h>

namespace Balau {

class CurlTask : public StacklessTask {
  public:
      CurlTask();
      ~CurlTask();
    friend class TaskMan;
  protected:
    CURL * m_curlHandle;
    void registerCurlHandle() { getTaskMan()->registerCurlHandle(this); }
    void unregisterCurlHandle() { getTaskMan()->unregisterCurlHandle(this); }
  private:
    virtual size_t writeFunction(char * ptr, size_t size, size_t nmemb) { return size * nmemb; }
    virtual size_t readFunction(void * ptr, size_t size, size_t nmemb) { return CURL_READFUNC_ABORT; }
    virtual int    debugFunction(curl_infotype info, char * str, size_t str_len) { return 0; }
    virtual void   curlDone(CURLcode result) { }

    static  size_t writeFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata);
    static  size_t readFunctionStatic(void * ptr, size_t size, size_t nmemb, void * userdata);
    static  int    debugFunctionStatic(CURL * easy, curl_infotype info, char * str, size_t str_len, void * userdata);
};

class DownloadTask : public CurlTask {
  public:
      DownloadTask(const String & url);
    const String & getData() const { return m_data; }
    bool isDone() { return m_done; }
    long responseCode() { return m_responseCode; }

  protected:
    String m_data;
    CURLcode m_curlResult;
    long m_responseCode;

  private:
    virtual const char * getName() const override { return m_name.to_charp(); }
    virtual void Do() override;
    virtual void curlDone(CURLcode result) override;
    virtual size_t writeFunction(char * ptr, size_t size, size_t nmemb) override { m_data += ptr; return size * nmemb; }
    String m_name;
    Events::Custom m_evt;
    bool m_done = false;
};

};
