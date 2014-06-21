#pragma once

#include <curl/curl.h>
#include <StacklessTask.h>
#include <TaskMan.h>

namespace Balau {

class CurlTask : public StacklessTask {
  public:
      CurlTask();
      virtual ~CurlTask();
    friend class TaskMan;
  
    virtual void prepareRequest() { }

  protected:
    CURL * m_curlHandle;
    void registerCurlHandle() { prepareRequest(); getTaskMan()->registerCurlHandle(this); }
    void unregisterCurlHandle() { getTaskMan()->unregisterCurlHandle(this); }

  private:
    virtual size_t headerFunction(char * ptr, size_t size) { return size; }
    virtual size_t writeFunction(char * ptr, size_t size) { return size; }
    virtual size_t readFunction(void * ptr, size_t size) { return CURL_READFUNC_ABORT; }
    virtual int    debugFunction(curl_infotype info, char * str, size_t str_len) { return 0; }
    virtual void   curlDone(CURLcode result) { }

    static  size_t headerFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata);
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
    virtual void downloadDone() { }
    virtual size_t writeFunction(char * ptr, size_t size) override { m_data += ptr; return size; }
    String m_name;
    Events::Custom m_evt;
    bool m_done = false;
};

class HttpDownloadTask : public DownloadTask {
  public:
      HttpDownloadTask(const String & url) : DownloadTask(url) { }
      virtual ~HttpDownloadTask();
    void addHeader(const String & header);
    virtual void prepareRequest() override;

  private:
    struct curl_slist * m_headers = NULL;
};

};
