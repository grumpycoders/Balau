#include "Exceptions.h"
#include "Threads.h"
#include "Local.h"

namespace Balau {

class ThreadHelper {
  public:
    static void * threadProc(void * arg);
};

}

Balau::Lock::Lock() {
    int r;
    pthread_mutexattr_t attr;

    r = pthread_mutexattr_init(&attr);
    Assert(r == 0);
    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    Assert(r == 0);
    r = pthread_mutex_init(&m_lock, &attr);
    Assert(r == 0);
}

void * Balau::ThreadHelper::threadProc(void * arg) {
    void * tls = g_tlsManager->createTLS();
    g_tlsManager->setTLS(tls);
    Balau::Thread * thread = reinterpret_cast<Balau::Thread *>(arg);
    void * r = thread->proc();
    free(tls);
    return r;
}

Balau::Thread::~Thread() {
    join();
}

void * Balau::Thread::join() {
    void * r = NULL;
    if (!m_joined) {
        m_joined = true;
        pthread_join(m_thread, &r);
    }
    return r;
}

void Balau::Thread::threadStart() {
    pthread_attr_t attr;
    int r;

    r = pthread_attr_init(&attr);
    Assert(r == 0);
    r = pthread_create(&m_thread, &attr, Balau::ThreadHelper::threadProc, this);
    Assert(r == 0);
    r = pthread_attr_destroy(&attr);
    Assert(r == 0);
}
