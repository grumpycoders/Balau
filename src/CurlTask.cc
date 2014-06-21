#include "CurlTask.h"

Balau::CurlTask::CurlTask() {
    m_curlHandle = curl_easy_init();
    curl_easy_setopt(m_curlHandle, CURLOPT_HEADERFUNCTION, reinterpret_cast<curl_write_callback>(headerFunctionStatic));
    curl_easy_setopt(m_curlHandle, CURLOPT_HEADERDATA, this);
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

size_t Balau::CurlTask::headerFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    return curlTask->headerFunction(ptr, size * nmemb);
}

size_t Balau::CurlTask::writeFunctionStatic(char * ptr, size_t size, size_t nmemb, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    return curlTask->writeFunction(ptr, size * nmemb);
}

size_t Balau::CurlTask::readFunctionStatic(void * ptr, size_t size, size_t nmemb, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    return curlTask->readFunction(ptr, size * nmemb);
}

int Balau::CurlTask::debugFunctionStatic(CURL * easy, curl_infotype info, char * str, size_t str_len, void * userdata) {
    CurlTask * curlTask = (CurlTask *) userdata;
    IAssert(easy == curlTask->m_curlHandle, "Got a debug callback for a handle that isn't our own.");
    return curlTask->debugFunction(info, str, str_len);
}

Balau::String Balau::CurlTask::percentEncode(const String & src) {
    const size_t size = src.strlen();
    String ret;
    ret.reserve(size);

    static char toHex[] = "0123456789ABCDEF";

    for (size_t i = 0; i < size; i++) {
        char c = src[i];
        if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || (c == '-') || (c == '.') || (c == '_') || (c == '~')) {
            ret += c;
        } else {
            ret += '%';
            ret += toHex[c >> 4];
            ret += toHex[c & 15];
        }
    }

    return ret;
}

Balau::String Balau::CurlTask::percentDecode(const String & src) {
    const size_t size = src.strlen();
    String ret;
    ret.reserve(size);

    for (size_t i = 0; i < size; i++) {
        char c = src[i];
        if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || (c == '-') || (c == '.') || (c == '_') || (c == '~')) {
            ret += c;
        } else if ((c == '%') && ((i + 2) < size)) {
            char h1 = src[i + 1];
            char h2 = src[i + 2];
            if ((h1 >= '0') && (h1 <= '9')) {
                c = h1 - '0';
            } else if ((h1 >= 'A') && (h1 <= 'F')) {
                c = h1 - 'A' + 10;
            } else {
                // invalid
                return ret;
            }
            c <<= 4;
            if ((h2 >= '0') && (h2 <= '9')) {
                c |= h2 - '0';
            } else if ((h2 >= 'A') && (h2 <= 'F')) {
                c |= h2 - 'A' + 10;
            } else {
                // invalid
                return ret;
            }
            i += 2;
        } else {
            // invalid
            return ret;
        }
    }

    return ret;
}

std::vector<Balau::String> Balau::CurlTask::tokenize(const String & str, const String & delimiters, bool trimEmpty) {
    std::vector<String> tokens;
    size_t pos, lastPos = 0;
    for (;;) {
        pos = str.find_first_of(delimiters, lastPos);
        if (pos == String::npos) {
            pos = str.strlen();

            if ((pos != lastPos) || !trimEmpty)
                tokens.push_back(str.extract(lastPos, pos));

            return tokens;
        } else {
            if ((pos != lastPos) || !trimEmpty)
                tokens.push_back(str.extract(lastPos, pos));
        }

        lastPos = pos + 1;
    }
}

Balau::DownloadTask::DownloadTask(const Balau::String & url) {
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, url.to_charp());
    m_name.set("DownloadTask(%s)", url.to_charp());
}

void Balau::DownloadTask::Do() {
    if (m_state) {
        downloadDone();
        return;
    }

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

Balau::HttpDownloadTask::~HttpDownloadTask() {
    curl_slist_free_all(m_headers);
    m_headers = NULL;
}

void Balau::HttpDownloadTask::addHeader(const String & header) {
    m_headers = curl_slist_append(m_headers, header.to_charp());
}

void Balau::HttpDownloadTask::prepareRequest() {
    if (m_headers)
        curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, m_headers);
}
