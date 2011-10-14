#pragma once

#include <queue>
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
    pthread_mutex_t m_lock;
    template<class T>
    friend class Queue;
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
  private:
    pthread_t m_thread;
    bool m_joined;

    friend class ThreadHelper;
};

template<class T>
class Queue {
  public:
      Queue() { pthread_cond_init(&m_cond, NULL); }
      ~Queue() { pthread_cond_destroy(&m_cond); }
    void push(T & t) {
        m_lock.enter();
        m_queue.push(t);
        pthread_cond_signal(&m_cond);
        m_lock.leave();
    }
    T pop() {
        m_lock.enter();
        if (m_queue.size() == 0)
            pthread_cond_wait(&m_cond, &m_lock.m_lock);
        T t = m_queue.front();
        m_queue.pop();
        m_lock.leave();
        return t;
    }
  private:
    std::queue<T> m_queue;
    Lock m_lock;
    pthread_cond_t m_cond;
};

};
