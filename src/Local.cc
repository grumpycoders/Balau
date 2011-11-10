#include "Local.h"
#include "Main.h"

static void * dummyTLS = NULL;

void * Balau::TLSManager::getTLS() {
    return dummyTLS;
}

void * Balau::TLSManager::setTLS(void * val) {
    void * r = dummyTLS;
    dummyTLS = val;
    return r;
}

void * Balau::TLSManager::createTLS() {
    return Local::create();
}

static Balau::TLSManager dummyTLSManager;
Balau::TLSManager * Balau::g_tlsManager = &dummyTLSManager;

int Balau::Local::s_size = 0;
void ** Balau::Local::m_globals = 0;

void Balau::Local::doStart() {
    Assert(Main::status() == Main::STARTING);
    m_idx = s_size++;
    m_globals = reinterpret_cast<void **>(realloc(m_globals, s_size * sizeof(void *)));
    m_globals[m_idx] = 0;
}

class PThreadsTLSManager : public Balau::TLSManager, public Balau::AtStart {
  public:
      PThreadsTLSManager() : AtStart(0) { }
    virtual void * getTLS();
    virtual void * setTLS(void * val);
    virtual void doStart();
  private:
    pthread_key_t m_key;
};

PThreadsTLSManager pthreadsTLSManager;

void PThreadsTLSManager::doStart() {
    int r;

    r = pthread_key_create(&m_key, NULL);
    Assert(r == 0);
    Balau::g_tlsManager = this;
}

void * PThreadsTLSManager::getTLS() {
    return pthread_getspecific(m_key);
}

void * PThreadsTLSManager::setTLS(void * val) {
    void * r = pthread_getspecific(m_key);
    pthread_setspecific(m_key, val);
    return r;
}
