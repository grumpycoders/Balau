#include "CurlTask.h"

Balau::CurlTask::CurlTask() {
    m_curlHandle = curl_easy_init();
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, reinterpret_cast<curl_write_callback>(writeFunctionStatic));
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, reinterpret_cast<curl_read_callback>(readFunctionStatic));
    curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, this);
    curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGFUNCTION, reinterpret_cast<curl_debug_callback>(debugFunctionStatic));
    curl_easy_setopt(m_curlHandle, CURLOPT_DEBUGDATA, this);
}

Balau::CurlTask::~CurlTask() {
    unregisterCurlHandle();
    curl_easy_cleanup(m_curlHandle);
}

size_t Balau::CurlTask::writeFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    return curlTask->writeFunction(ptr, size, nmemb);
}

size_t Balau::CurlTask::readFunctionStatic(void * ptr, size_t size, size_t nmemb, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    return curlTask->readFunction(ptr, size, nmemb);
}

int Balau::CurlTask::debugFunctionStatic(CURL * easy, curl_infotype info, char * str, size_t str_len, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    IAssert(easy == curlTask->m_curlHandle, "Got a debug callback for a handle that isn't our own.");
    return curlTask->debugFunction(info, str, str_len);
}

Balau::DownloadTask::DownloadTask(const Balau::String & url) {
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, url.to_charp());
    m_name.set("DownloadTask(%s)", url.to_charp());
}

void Balau::DownloadTask::Do() {
    if (m_state)
        return;

    m_state = 1;
    registerCurlHandle();
    waitFor(&m_evt);
    yield();
}

void Balau::DownloadTask::curlDone(CURLcode result) {
    m_curlResult = result;
    curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &m_responseCode);
    m_evt.doSignal();
    m_done = true;
}
