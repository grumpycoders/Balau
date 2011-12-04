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
        m_lock.enter();
        Cell * c = new Cell(t);
        c->m_prev = m_back;
        if (m_back)
            m_back->m_next = c;
        else
            m_front = c;
        m_back = c;
        pthread_cond_signal(&m_cond);
        m_lock.leave();
    }
    T * pop() {
        m_lock.enter();
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
        m_lock.leave();
        return t;
    }
    bool isEmpty() {
        bool r;
        m_lock.enter();
        r = !m_front;
        m_lock.leave();
        return r;
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
