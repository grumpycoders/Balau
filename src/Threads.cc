#include "Exceptions.h"
#include "Threads.h"
#include "Local.h"
#include "Atomic.h"

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
    RAssert(r == 0, "Couldn't initialize mutex attribute; r = %i", r);
    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    RAssert(r == 0, "Couldn't set mutex attribute; r = %i", r);
    r = pthread_mutex_init(&m_lock, &attr);
    RAssert(r == 0, "Couldn't initialize mutex; r = %i", r);
    r = pthread_mutexattr_destroy(&attr);
    RAssert(r == 0, "Couldn't destroy mutex attribute; r = %i", r);
}

Balau::RWLock::RWLock() {
    int r;
    pthread_rwlockattr_t attr;

    r = pthread_rwlockattr_init(&attr);
    RAssert(r == 0, "Couldn't initialize rwlock attribute; r = %i", r);
    r = pthread_rwlock_init(&m_lock, &attr);
    RAssert(r == 0, "Couldn't initialize mutex; r = %i", r);
    r = pthread_rwlockattr_destroy(&attr);
    RAssert(r == 0, "Couldn't destroy rwlock attribute; r = %i", r);
}

void * Balau::ThreadHelper::threadProc(void * arg) {
    void * tls = Local::createTLS();
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
    if (Atomic::CmpXChgBool(&m_joined, true, false)) {
        threadExit();
        pthread_join(m_thread, &r);
    }
    return r;
}

void Balau::Thread::threadStart() {
    pthread_attr_t attr;
    int r;

    r = pthread_attr_init(&attr);
    RAssert(r == 0, "Couldn't initialize pthread attribute; r = %i", r);
    r = pthread_create(&m_thread, &attr, Balau::ThreadHelper::threadProc, this);
    RAssert(r == 0, "Couldn't create pthread; r = %i", r);
    r = pthread_attr_destroy(&attr);
    RAssert(r == 0, "Couldn't destroy pthread attribute; r = %i", r);
}
