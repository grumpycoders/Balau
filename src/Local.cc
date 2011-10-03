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
Balau::TLSManager * Balau::tlsManager = &dummyTLSManager;

int Balau::Local::s_size = 0;
void ** Balau::Local::m_globals = 0;

void Balau::Local::doStart() {
    Assert(Main::status() == Main::STARTING);
    m_idx = s_size++;
    m_globals = reinterpret_cast<void **>(realloc(m_globals, s_size * sizeof(void *)));
    m_globals[m_idx] = 0;
}
