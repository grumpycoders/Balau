#pragma once

#include <Atomic.h>
#include <Exceptions.h>
#include <Local.h>
#include <Threads.h>

namespace Balau {

class AsyncManager;
class AsyncFinishWorker;

typedef void (*IdleReadyCallback_t)(void *);

class AsyncOperation {
  protected:
    virtual void run() { }
    virtual void finish() { }
    virtual void done() { }
    virtual bool needsMainQueue() { return true; }
    virtual bool needsFinishWorker() { return false; }
    virtual bool needsSynchronousCallback() { return true; }
  protected:
      virtual ~AsyncOperation() { }
  private:
    Queue<AsyncOperation> * m_idleQueue = NULL;
    IdleReadyCallback_t m_idleReadyCallback = NULL;
    void * m_idleReadyParam = NULL;
    void finalize();

    friend class AsyncManager;
    friend class AsyncFinishWorker;
};

class AsyncFinishWorker : public Thread {
  public:
      AsyncFinishWorker(AsyncManager * async, Queue<AsyncOperation> * queue) : m_async(async), m_queue(queue) { }
    virtual void * proc();
    AsyncManager * m_async;
    Queue<AsyncOperation> * m_queue;
    bool m_stopping = false;
    volatile bool m_stopped = false;
};

class AsyncManager : public Thread {
  public:
    void setFinishers(int minIdle, int maxIdle) {
        AAssert(minIdle < maxIdle, "Minimum number of threads needs to be less than maximum number of threads.");
        m_minIdle = minIdle;
        m_maxIdle = maxIdle;
    }
    void setIdleReadyCallback(IdleReadyCallback_t idleReadyCallback, void * param);
    void queueOp(AsyncOperation * op);
    void idle();
    bool isReady() { return m_ready; }

  protected:
    virtual void threadExit();

  private:
    void checkIdle();
    void killOneFinisher();
    void startOneFinisher();
    void joinStoppedFinishers();
    void stopAllWorkers();
    virtual void * proc();
    struct TLS {
        Queue<AsyncOperation> idleQueue;
        IdleReadyCallback_t idleReadyCallback;
        void * idleReadyParam;
    };
    TLS * getTLS() {
        TLS * tls = (TLS *) m_tlsManager.getTLS();
        if (!tls) {
            tls = new TLS();
            m_tlsManager.setTLS(tls);
            m_TLSes.push(tls);
            Atomic::Increment(&m_numTLSes);
        }
        return tls;
    }
    Queue<AsyncOperation> m_queue;
    Queue<AsyncOperation> m_finished;
    Queue<TLS> m_TLSes;
    volatile int m_numTLSes = 0;
    PThreadsTLSManager m_tlsManager;
    std::list<AsyncFinishWorker *> m_workers;
    int m_numFinishers = 0;
    int m_numFinishersIdle = 0;
    int m_minIdle = 1;
    int m_maxIdle = 4;
    bool m_stopping = false;
    volatile bool m_ready = false;
    volatile bool m_stopperPushed = false;

    void incIdle() { Atomic::Increment(&m_numFinishersIdle); }
    void decIdle() { Atomic::Decrement(&m_numFinishersIdle); }

    friend class AsyncFinishWorker;
};

};
