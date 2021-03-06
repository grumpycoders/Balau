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

static Balau::TLSManager dummyTLSManager;
Balau::TLSManager * Balau::g_tlsManager = &dummyTLSManager;

int Balau::Local::s_size = 0;
void ** Balau::Local::m_globals = 0;

void Balau::Local::doStart() {
    m_idx = s_size++;
    m_globals = reinterpret_cast<void **>(realloc(m_globals, s_size * sizeof(void *)));
    m_globals[m_idx] = 0;
}

class GlobalPThreadsTLSManager : public Balau::PThreadsTLSManager, public Balau::AtStart {
  public:
      GlobalPThreadsTLSManager() : AtStart(0) { }
    void doStart();
};

GlobalPThreadsTLSManager pthreadsTLSManager;

void GlobalPThreadsTLSManager::doStart() {
    init();
    Balau::g_tlsManager = this;
}

void Balau::PThreadsTLSManager::init() {
    int r;

    r = pthread_key_create(&m_key, NULL);
    RAssert(r == 0, "Unable to create a pthtread_key: %i", r);
}

void * Balau::PThreadsTLSManager::getTLS() {
    return pthread_getspecific(m_key);
}

void * Balau::PThreadsTLSManager::setTLS(void * val) {
    void * r = pthread_getspecific(m_key);
    pthread_setspecific(m_key, val);
    return r;
}
