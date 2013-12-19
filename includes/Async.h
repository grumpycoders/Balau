#pragma once

#include <atomic>
#include <Exceptions.h>
#include <Local.h>
#include <Threads.h>

namespace Balau {

class AsyncManager;
class AsyncFinishWorker;

typedef void (*IdleReadyCallback_t)(void *);

class AsyncOperation {
  protected:
      AsyncOperation() { }
    virtual void run() { }
    virtual void finish() { }
    virtual void done() { }
    virtual bool needsMainQueue() { return true; }
    virtual bool needsFinishWorker() { return false; }
    virtual bool needsSynchronousCallback() { return true; }
  protected:
      virtual ~AsyncOperation() { }
  private:
    CQueue<AsyncOperation> * m_idleQueue = NULL;
    IdleReadyCallback_t m_idleReadyCallback = NULL;
    void * m_idleReadyParam = NULL;
    void finalize();
      AsyncOperation(const AsyncOperation &) = delete;
    AsyncOperation & operator=(const AsyncOperation &) = delete;

    friend class AsyncManager;
    friend class AsyncFinishWorker;
};

class AsyncFinishWorker : public Thread {
  public:
      AsyncFinishWorker(AsyncManager * async, Queue<AsyncOperation> * queue) : m_async(async), m_queue(queue), m_stopped(false) { }
    bool stopped() { return m_stopped; }
  private:
      AsyncFinishWorker(const AsyncFinishWorker &) = delete;
    AsyncOperation & operator=(const AsyncFinishWorker &) = delete;
    virtual void * proc();
    AsyncManager * m_async;
    Queue<AsyncOperation> * m_queue;
    bool m_stopping = false;
    std::atomic<bool> m_stopped;
};

class AsyncManager : public Thread {
  public:
      AsyncManager() : m_numTLSes(0), m_ready(false), m_stopperPushed(false) { }
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
      AsyncManager(const AsyncManager &) = delete;
      AsyncManager & operator=(const AsyncManager &) = delete;
    void checkIdle();
    void killOneFinisher();
    void startOneFinisher();
    void joinStoppedFinishers();
    void stopAllWorkers();
    virtual void * proc();
    struct TLS {
        CQueue<AsyncOperation> idleQueue;
        IdleReadyCallback_t idleReadyCallback;
        void * idleReadyParam;
    };
    TLS * getTLS() {
        TLS * tls = (TLS *) m_tlsManager.getTLS();
        if (!tls) {
            tls = new TLS();
            m_tlsManager.setTLS(tls);
            m_TLSes.push(tls);
            ++m_numTLSes;
        }
        return tls;
    }
    Queue<AsyncOperation> m_queue;
    Queue<AsyncOperation> m_finished;
    Queue<TLS> m_TLSes;
    std::atomic<int> m_numTLSes;
    PThreadsTLSManager m_tlsManager;
    std::list<AsyncFinishWorker *> m_workers;
    int m_numFinishers = 0;
    int m_numFinishersIdle = 0;
    int m_minIdle = 1;
    int m_maxIdle = 4;
    bool m_stopping = false;
    std::atomic<bool> m_ready;
    std::atomic<bool> m_stopperPushed;

    void incIdle() { m_numFinishersIdle++; }
    void decIdle() { m_numFinishersIdle--; }

    friend class AsyncFinishWorker;
};

};
