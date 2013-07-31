#include "Exceptions.h"
#include "Threads.h"
#include "Local.h"
#include "Atomic.h"
#include "TaskMan.h"

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
    void * r = NULL;
    bool success = false;
    try {
        void * tls = Local::createTLS();
        g_tlsManager->setTLS(tls);
        Balau::Thread * thread = reinterpret_cast<Balau::Thread *>(arg);
        r = thread->proc();
        free(tls);
        success = true;
    }
    catch (Exit & e) {
        Printer::log(M_ERROR, "We shouldn't have gotten an Exit exception here... exitting anyway");
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_ERROR, "%s", str.to_charp());
    }
    catch (RessourceException & e) {
        Printer::log(M_ERROR | M_ALERT, "The Thread got a ressource problem: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
    }
    catch (GeneralException & e) {
        Printer::log(M_ERROR | M_ALERT, "The Thread caused an exception: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
    }
    catch (...) {
        Printer::log(M_ERROR | M_ALERT, "The Thread caused an unknown exception");
    }

    if (!success)
        TaskMan::stop(-1);

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
