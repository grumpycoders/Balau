#include <Exceptions.h>
#include <Threads.h>

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
