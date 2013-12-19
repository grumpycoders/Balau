#pragma once

#include <atomic>
#include <AtStartExit.h>
#include <pthread.h>

namespace Balau {

class QueueBase;

class Lock {
  public:
      Lock();
      ~Lock() { pthread_mutex_destroy(&m_lock); }
    void enter() { pthread_mutex_lock(&m_lock); }
    void leave() { pthread_mutex_unlock(&m_lock); }
  private:
      Lock(const Lock &) = delete;
    Lock & operator=(const Lock &) = delete;
    pthread_mutex_t m_lock;
    friend class QueueBase;
};

class ScopeLock {
  public:
      ScopeLock(Lock & lock) : m_lock(lock) { m_lock.enter(); }
      ~ScopeLock() { m_lock.leave(); }
  private:
      ScopeLock(const ScopeLock &) = delete;
    ScopeLock & operator=(const ScopeLock &) = delete;
    Lock & m_lock;
};

class RWLock {
  public:
      RWLock();
      ~RWLock() { pthread_rwlock_destroy(&m_lock); }
    void enterR() { pthread_rwlock_rdlock(&m_lock); }
    void enterW() { pthread_rwlock_wrlock(&m_lock); }
    void leave() { pthread_rwlock_unlock(&m_lock); }
  private:
      RWLock(const RWLock &) = delete;
    RWLock & operator=(const ScopeLock &) = delete;
    pthread_rwlock_t m_lock;
};

class ScopeLockR {
  public:
      ScopeLockR(RWLock & lock) : m_lock(lock) { m_lock.enterR(); }
      ~ScopeLockR() { m_lock.leave(); }
  private:
      ScopeLockR(const ScopeLockR &) = delete;
    ScopeLockR & operator=(const ScopeLockR &) = delete;
    RWLock & m_lock;
};

class ScopeLockW {
  public:
      ScopeLockW(RWLock & lock) : m_lock(lock) { m_lock.enterW(); }
      ~ScopeLockW() { m_lock.leave(); }
  private:
      ScopeLockW(const ScopeLockW &) = delete;
    ScopeLockW & operator=(const ScopeLockW &) = delete;
    RWLock & m_lock;
};

class ThreadHelper;

class Thread {
  public:
      virtual ~Thread();
    void threadStart();
    void * join();
  protected:
      Thread() : m_joined(false) { }
    virtual void * proc() = 0;
    virtual void threadExit() { };
  private:
      Thread(const Thread &) = delete;
    Thread & operator=(const Thread &) = delete;
    pthread_t m_thread;
    std::atomic<bool> m_joined;

    friend class ThreadHelper;
};

class GlobalThread : public Thread, public AtStart, public AtExit {
  protected:
      GlobalThread(int startOrder = 10) : AtStart(startOrder), AtExit(1) { }
  private:
      GlobalThread(const GlobalThread &) = delete;
    GlobalThread & operator=(const GlobalThread &) = delete;
    virtual void doStart() { threadStart(); }
    virtual void doExit() { join(); }
};

};
