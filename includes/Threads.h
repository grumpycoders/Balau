#pragma once

#include <AtStartExit.h>
#include <pthread.h>

namespace Balau {

template<class T>
class Queue;

class Lock {
  public:
      Lock();
      ~Lock() { pthread_mutex_destroy(&m_lock); }
    void enter() { pthread_mutex_lock(&m_lock); }
    void leave() { pthread_mutex_unlock(&m_lock); }
  private:
      Lock(const Lock &);
    pthread_mutex_t m_lock;
    template<class T>
    friend class Queue;
};

class ScopeLock {
  public:
      ScopeLock(Lock & lock) : m_lock(lock) { m_lock.enter(); }
      ~ScopeLock() { m_lock.leave(); }
  private:
      ScopeLock(const ScopeLock &);
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
      RWLock(const RWLock &);
    pthread_rwlock_t m_lock;
};

class ScopeLockR {
  public:
      ScopeLockR(Lock & lock) : m_lock(lock) { m_lock.enterR(); }
      ~ScopeLockR() { m_lock.leave(); }
  private:
      ScopeLockR(const ScopeLockR &);
    RWLock & m_lock;
};

class ScopeLockW {
  public:
      ScopeLockW(Lock & lock) : m_lock(lock) { m_lock.enterW(); }
      ~ScopeLockW() { m_lock.leave(); }
  private:
      ScopeLockW(const ScopeLockW &);
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
    pthread_t m_thread;
    volatile bool m_joined;

    friend class ThreadHelper;
};

class GlobalThread : public Thread, public AtStart, public AtExit {
  protected:
      GlobalThread(int startOrder = 10) : AtStart(startOrder), AtExit(1) { }
  private:
    virtual void doStart() { threadStart(); }
    virtual void doExit() { join(); }
};

template<class T>
class Queue {
  public:
      Queue() : m_front(NULL), m_back(NULL) { pthread_cond_init(&m_cond, NULL); }
      ~Queue() { while (!isEmpty()) pop(); pthread_cond_destroy(&m_cond); }
    void push(T * t) {
        ScopeLock sl(m_lock);
        Cell * c = new Cell(t);
        c->m_prev = m_back;
        if (m_back)
            m_back->m_next = c;
        else
            m_front = c;
        m_back = c;
        pthread_cond_signal(&m_cond);
    }
    T * pop() {
        ScopeLock sl(m_lock);
        while (!m_front)
            pthread_cond_wait(&m_cond, &m_lock.m_lock);
        Cell * c = m_front;
        m_front = c->m_next;
        if (m_front)
            m_front->m_prev = NULL;
        else
            m_back = NULL;
        T * t = c->m_elem;
        delete c;
        return t;
    }
    bool isEmpty() {
        ScopeLock sl(m_lock);
        return !m_front;
    }
  private:
    Lock m_lock;
    class Cell {
      public:
          Cell(T * elem) : m_next(NULL), m_prev(NULL), m_elem(elem) { }
        Cell * m_next, * m_prev;
        T * m_elem;
    };
    Cell * volatile m_front;
    Cell * volatile m_back;
    pthread_cond_t m_cond;
};

};
