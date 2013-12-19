#include "Async.h"

namespace {

class AsyncStopper : public Balau::AsyncOperation {
  public:
    virtual bool needsSynchronousCallback() { return false; }
    virtual void done() { delete this; }
};

};

void Balau::AsyncManager::setIdleReadyCallback(void (*callback)(void *), void * param) {
    while (!m_ready);
    TLS * tls = getTLS();
    tls->idleReadyCallback = callback;
    tls->idleReadyParam = param;
}

void Balau::AsyncManager::queueOp(AsyncOperation * op) {
    if (m_stopperPushed) {
        Printer::elog(E_ASYNC, "AsyncManager's queue has been stopped; running operation %p on this thread instead.", op);
        op->run();
        op->finalize();
        return;
    }
    while (!m_ready);
    TLS * tls = getTLS();
    Printer::elog(E_ASYNC, "Queuing operation at %p", op);
    if (op->needsSynchronousCallback()) {
        Printer::elog(E_ASYNC, "Operation at %p needs synchronous callback, copying values; idleQueue = %p; idleReadyCallback = %p; idleReadyParam = %p", op, &tls->idleQueue, tls->idleReadyCallback, tls->idleReadyParam);
        op->m_idleQueue = &tls->idleQueue;
        op->m_idleReadyCallback = tls->idleReadyCallback;
        op->m_idleReadyParam = tls->idleReadyParam;
    }
    if (op->needsMainQueue()) {
        m_queue.push(op);
    } else if (op->needsFinishWorker()) {
        m_finished.push(op);
    } else {
        op->run();
        op->finalize();
    }
}

void Balau::AsyncManager::checkIdle() {
    if (m_numFinishersIdle > m_maxIdle)
        killOneFinisher();
    if (m_numFinishersIdle < m_minIdle)
        startOneFinisher();
    joinStoppedFinishers();
}

void Balau::AsyncManager::killOneFinisher() {
    Printer::elog(E_ASYNC, "Too many workers idle (%i / %i), killing one.", m_numFinishersIdle, m_maxIdle);
    m_finished.push(new AsyncStopper());
}

void Balau::AsyncManager::startOneFinisher() {
    AsyncFinishWorker * worker = new AsyncFinishWorker(this, &m_finished);
    Printer::elog(E_ASYNC, "Not enough workers idle (%i / %i), starting one at %p.", m_numFinishersIdle, m_minIdle, worker);
    m_workers.push_back(worker);
    m_numFinishers++;
    worker->threadStart();
}

void Balau::AsyncManager::joinStoppedFinishers() {
    for (auto i = m_workers.begin(); i != m_workers.end(); i++) {
        AsyncFinishWorker * worker = *i;
        if (!worker->stopped())
            continue;
        Printer::elog(E_ASYNC, "Joining stopped worker at %p", worker);
        m_numFinishers--;
        m_workers.erase(i);
        worker->join();
        delete worker;
        break;
    }
}

void * Balau::AsyncManager::proc() {
    Printer::elog(E_ASYNC, "AsyncManager thread starting up");
    m_tlsManager.init();
    m_ready = true;
    while (!m_stopping) {
        checkIdle();
        AsyncOperation * op = m_queue.pop();
        Printer::elog(E_ASYNC, "AsyncManager got an operation at %p", op);
        if (dynamic_cast<AsyncStopper *>(op)) {
            Printer::elog(E_ASYNC, "AsyncManager got a stopper operation");
            m_stopping = true;
        }
        Printer::elog(E_ASYNC, "AsyncManager running operation at %p", op);
        op->run();
        if (op->needsFinishWorker()) {
            Printer::elog(E_ASYNC, "AsyncManager pushing operation at %p in the finisher's queue", op);
            m_finished.push(op);
        } else {
            Printer::elog(E_ASYNC, "AsyncManager finalizing operation at %p", op);
            op->finalize();
        }
    }
    stopAllWorkers();

    Printer::elog(E_ASYNC, "Async thread waits for all idle queues to empty");
    while (m_numTLSes--) {
        TLS * tls = m_TLSes.pop();
        while (!tls->idleQueue.isEmpty());
    }

    Printer::elog(E_ASYNC, "Async thread stopping");
    return NULL;
}

void * Balau::AsyncFinishWorker::proc() {
    Printer::elog(E_ASYNC, "AsyncFinishWorker thread starting up");
    AsyncOperation * op;
    while (!m_stopping) {
        m_async->incIdle();
        op = m_queue->pop();
        m_async->decIdle();
        Printer::elog(E_ASYNC, "AsyncFinishWorker got operation at %p", op);
        if (dynamic_cast<AsyncStopper *>(op)) {
            Printer::elog(E_ASYNC, "AsyncFinishWorker got a stopper operation");
            m_stopping = true;
        }
        if (!op->needsMainQueue())
            op->run();
        op->finalize();
    }

    m_stopped = true;
    Printer::elog(E_ASYNC, "AsyncFinishWorker thread stopping");

    return NULL;
}

void Balau::AsyncOperation::finalize() {
    Printer::elog(E_ASYNC, "AsyncOperation::finalize() is finishing operation %p", this);
    finish();
    if (needsSynchronousCallback()) {
        Printer::elog(E_ASYNC, "AsyncOperation::finalize() is pushing operation %p to its idle queue", this);
        bool wasEmpty = m_idleQueue->isEmpty();
        m_idleQueue->push(this);
        Printer::elog(E_ASYNC, "AsyncOperation::finalize() has pushed operation %p to its idle queue; wasEmpty = %s; callback = %p", this, wasEmpty ? "true" : "false", m_idleReadyCallback);
        if (wasEmpty && m_idleReadyCallback) {
            Printer::elog(E_ASYNC, "AsyncOperation::finalize() is calling ready callback to wake up main loop");
            m_idleReadyCallback(m_idleReadyParam);
        }
    } else {
        Printer::elog(E_ASYNC, "AsyncOperation::finalize() is wrapping up operation %p", this);
        done();
    }
}

void Balau::AsyncManager::idle() {
    Printer::elog(E_ASYNC, "AsyncManager::idle() is running");
    while (!m_ready);
    AsyncOperation * op;
    TLS * tls = getTLS();
    while ((op = tls->idleQueue.pop())) {
        Printer::elog(E_ASYNC, "AsyncManager::idle() is wrapping up operation %p", op);
        op->done();
    }
}

void Balau::AsyncManager::threadExit() {
    Printer::elog(E_ASYNC, "AsyncManager thread is being asked to stop; creating stopper");
    if (!m_stopperPushed.exchange(true))
        m_queue.push(new AsyncStopper());
}

void Balau::AsyncManager::stopAllWorkers() {
    Printer::elog(E_ASYNC, "AsyncManager thread is stopping and joining %i workers", m_numFinishers);
    for (int i = 0; i < m_numFinishers; i++)
        m_finished.push(new AsyncStopper());
    for (auto worker : m_workers)
        worker->join();
}
