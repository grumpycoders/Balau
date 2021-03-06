#include "Async.h"
#include "TaskMan.h"
#include "Task.h"
#include "Main.h"
#include "Local.h"
#include "CurlTask.h"

#include <ares.h>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#undef ERROR

static Balau::AsyncManager s_async;
static CURLSH * s_curlShared = NULL;

namespace {

class AsyncStarter : public Balau::AtStart, Balau::AtExit {
  public:
      AsyncStarter() : AtStart(1000), AtExit(0) { }
    void doStart() {
        s_async.threadStart();
    }
    void doExit() {
        s_async.join();
    }
};

class Stopper : public Balau::Task {
  public:
      Stopper(int code) : m_code(code) { }
  private:
    virtual void Do();
    virtual const char * getName() const;
    int m_code;
};

class CurlAndAresSharedManager : public Balau::AtStart, Balau::AtExit {
  public:
      CurlAndAresSharedManager() : AtStart(0), AtExit(0) { }
    struct SharedLocks {
        Balau::RWLock share, cookie, dns, ssl_session;
    };
    static void lock_function(CURL *handle, curl_lock_data data, curl_lock_access access, void * userptr) {
        SharedLocks * locks = (SharedLocks *) userptr;
        Balau::RWLock * lock = NULL;
        switch (data) {
            case CURL_LOCK_DATA_SHARE: lock = &locks->share; break;
            case CURL_LOCK_DATA_COOKIE: lock = &locks->cookie; break;
            case CURL_LOCK_DATA_DNS: lock = &locks->dns; break;
            case CURL_LOCK_DATA_SSL_SESSION: lock = &locks->ssl_session; break;
            default: Failure("Unknown lock");
        }
        switch (access) {
            case CURL_LOCK_ACCESS_SHARED: lock->enterR(); break;
            case CURL_LOCK_ACCESS_SINGLE: lock->enterW(); break;
            default: Failure("Unknown access");
        } 
    }
    static void unlock_function(CURL *handle, curl_lock_data data, void * userptr) {
        SharedLocks * locks = (SharedLocks *) userptr;
        Balau::RWLock * lock = NULL;
        switch (data) {
            case CURL_LOCK_DATA_SHARE: lock = &locks->share; break;
            case CURL_LOCK_DATA_COOKIE: lock = &locks->cookie; break;
            case CURL_LOCK_DATA_DNS: lock = &locks->dns; break;
            case CURL_LOCK_DATA_SSL_SESSION: lock = &locks->ssl_session; break;
            default: Failure("Unknown lock");
        }
        lock->leave();
    }
    void doStart() {
        curl_global_init(CURL_GLOBAL_ALL);
        static SharedLocks locks;
        s_curlShared = curl_share_init();
        curl_share_setopt(s_curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
        curl_share_setopt(s_curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        curl_share_setopt(s_curlShared, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
        curl_share_setopt(s_curlShared, CURLSHOPT_USERDATA, &locks);
        curl_share_setopt(s_curlShared, CURLSHOPT_LOCKFUNC, lock_function);
        curl_share_setopt(s_curlShared, CURLSHOPT_UNLOCKFUNC, unlock_function);

        ares_library_init(ARES_LIB_INIT_ALL);
    }
    void doExit() {
        ares_library_cleanup();

        curl_share_cleanup(s_curlShared);
        curl_global_cleanup();
    }
};

};

static AsyncStarter s_asyncStarter;
static CurlAndAresSharedManager s_curlAndAresSharedManager;

void Stopper::Do() {
    getTaskMan()->stopMe(m_code);
}

const char * Stopper::getName() const {
    return "Stopper";
}

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

static const int TOO_MANY_STACKS = 1024;

namespace Balau {

class TaskScheduler : public GlobalThread {
  public:
      TaskScheduler() : GlobalThread(100), m_stopping(false) { }
    void registerTask(Task * t);
    virtual void * proc();
    virtual void threadExit();
    void registerTaskMan(TaskMan * t);
    void unregisterTaskMan(TaskMan * t);
    void stopAll(int code);
  private:
    Queue<Task> m_queue;
    std::queue<TaskMan *> m_taskManagers;
    Lock m_lock;
    volatile bool m_stopping;
};

};

static Balau::TaskScheduler s_scheduler;

void Balau::TaskScheduler::registerTask(Task * t) {
    Printer::elog(E_TASK, "TaskScheduler::registerTask with t = %p", t);
    m_queue.push(t);
}

void Balau::TaskScheduler::registerTaskMan(TaskMan * t) {
    ScopeLock sl(m_lock);
    m_taskManagers.push(t);
}

void Balau::TaskScheduler::unregisterTaskMan(TaskMan * t) {
    ScopeLock sl(m_lock);
    TaskMan * p = NULL;
    // yes, this is a potentially dangerous operation.
    // But unregistering task managers shouldn't happen that often.
    while (true) {
        p = m_taskManagers.front();
        m_taskManagers.pop();
        if (p == t)
            break;
        m_taskManagers.push(p);
    }
}

void Balau::TaskScheduler::stopAll(int code) {
    m_stopping = true;
    ScopeLock sl(m_lock);
    std::queue<TaskMan *> altQueue;
    TaskMan * tm;
    while (!m_taskManagers.empty()) {
        tm = m_taskManagers.front();
        m_taskManagers.pop();
        altQueue.push(tm);
        tm->addToPending(new Stopper(code));
    }
    while (!altQueue.empty()) {
        tm = altQueue.front();
        altQueue.pop();
        m_taskManagers.push(tm);
    }
}

void * Balau::TaskScheduler::proc() {
    while (true) {
        Printer::elog(E_TASK, "TaskScheduler waiting for a task to pop");
        Task * t = m_queue.pop();
        if (!t)
            break;
        if (dynamic_cast<Stopper *>(t) || m_stopping)
            break;
        m_lock.enter();
        size_t s = m_taskManagers.size();
        if (s == 0)
            break;
        TaskMan * tm = m_taskManagers.front();
        if (s != 1) {
            m_taskManagers.pop();
            m_taskManagers.push(tm);
        }
        m_lock.leave();
        Printer::elog(E_TASK, "TaskScheduler popped task %s at %p; adding to TaskMan %p", t->getName(), t, tm);
        tm->addToPending(t);
    }
    Printer::elog(E_TASK, "TaskScheduler stopping.");
    return NULL;
}

void Balau::TaskScheduler::threadExit() {
    Task * s = NULL;
    m_queue.push(s);
}

void asyncDummy(ev::async & w, int revents) {
    Balau::Printer::elog(Balau::E_TASK, "TaskMan is getting woken up...");
}

void Balau::TaskMan::stopMe(int code) {
    Task * t = Task::getCurrentTask();
    if (t->getTaskMan() == this) {
        m_stopped = true;
        m_stopCode = code;
    } else {
        addToPending(new Stopper(code));
    }
}

Balau::TaskMan::TaskMan() {
#ifdef _WIN32
    m_fiber = ConvertThreadToFiber(NULL);
    RAssert(m_fiber, "ConvertThreadToFiber returned NULL");
#endif
    TaskMan * global = localTaskMan.getGlobal();
    if (!global) {
        localTaskMan.setGlobal(this);
        m_loop = ev_default_loop(EVFLAG_AUTO);
    } else {
        m_loop = ev_loop_new(EVFLAG_AUTO);
    }
    m_evt.set(m_loop);
    m_evt.set<asyncDummy>();
    m_evt.start();
    s_scheduler.registerTaskMan(this);

    m_nStacks = 0;

    m_curlMulti = curl_multi_init();

    curl_multi_setopt(m_curlMulti, CURLMOPT_SOCKETFUNCTION, reinterpret_cast<curl_socket_callback>(curlSocketCallbackStatic));
    curl_multi_setopt(m_curlMulti, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(m_curlMulti, CURLMOPT_TIMERFUNCTION, reinterpret_cast <curl_multi_timer_callback>(curlMultiTimerCallbackStatic));
    curl_multi_setopt(m_curlMulti, CURLMOPT_TIMERDATA, this);
    curl_multi_setopt(m_curlMulti, CURLMOPT_PIPELINING, 1L);

    m_curlTimer.set(m_loop);
    m_curlTimer.set<TaskMan, &TaskMan::curlMultiTimerEventCallback>(this);

    m_aresTimer.set(m_loop);
    m_aresTimer.set<TaskMan, &TaskMan::aresTimerEventCallback>(this);

    ares_options aresOptions;

    aresOptions.sock_state_cb = aresSocketCallbackStatic;
    aresOptions.sock_state_cb_data = this;

    ares_init_options(&m_aresChannel, &aresOptions, ARES_OPT_SOCK_STATE_CB);

    for (int i = 0; i < ARES_MAX_SOCKETS; i++) {
        m_aresSockets[i] = ARES_SOCKET_BAD;
        m_aresSocketEvents[i] = NULL;
    }
}

#ifdef _WIN32
inline static int fromSocket(SOCKET s) { return _open_osfhandle(s, 0); }
inline static SOCKET toSocket(int fd) { return _get_osfhandle(fd); }
#else
inline static int fromSocket(int s) { return s; }
inline static int toSocket(int fd) { return fd; }
#endif

int Balau::TaskMan::curlSocketCallbackStatic(CURL * easy, curl_socket_t s, int what, void * userp, void * socketp) {
    TaskMan * taskMan = (TaskMan *) userp;
    return taskMan->curlSocketCallback(easy, s, what, socketp);
}

int Balau::TaskMan::curlSocketCallback(CURL * easy, curl_socket_t s, int what, void * socketp) {
    int fd = fromSocket(s);
    ev::io * evt = (ev::io *) socketp;
    if (!evt) {
        if (what == CURL_POLL_REMOVE)
            return 0;
        evt = new ev::io;
        evt->set<TaskMan, &TaskMan::curlSocketEventCallback>(this);
        evt->set(m_loop);
        curl_multi_assign(m_curlMulti, s, evt);
    }

    switch (what) {
    case CURL_POLL_NONE:
        evt->stop();
        break;
    case CURL_POLL_IN:
        evt->stop();
        evt->set(fd, ev::READ);
        evt->start();
        break;
    case CURL_POLL_OUT:
        evt->stop();
        evt->set(fd, ev::WRITE);
        evt->start();
        break;
    case CURL_POLL_INOUT:
        evt->stop();
        evt->set(fd, ev::READ | ev::WRITE);
        evt->start();
        break;
    case CURL_POLL_REMOVE:
        evt->stop();
        curl_multi_assign(m_curlMulti, s, NULL);
        delete evt;
    }

    return 0;
}

void Balau::TaskMan::curlSocketEventCallback(ev::io & w, int revents) {
    int bitmask = 0;
    if (revents & ev::READ)
        bitmask |= CURL_CSELECT_IN;
    if (revents & ev::WRITE)
        bitmask |= CURL_CSELECT_OUT;
    if (revents & ev::ERROR)
        bitmask |= CURL_CSELECT_ERR;
    curl_multi_socket_action(m_curlMulti, toSocket(w.fd), bitmask, &m_curlStillRunning);
}

int Balau::TaskMan::curlMultiTimerCallbackStatic(CURLM * multi, long timeout_ms, void * userp) {
    TaskMan * taskMan = (TaskMan *)userp;
    return taskMan->curlMultiTimerCallback(multi, timeout_ms);
}

int Balau::TaskMan::curlMultiTimerCallback(CURLM * multi, long timeout_ms) {
    m_curlTimer.stop();
    if (timeout_ms >= 0) {
        m_curlTimer.set((ev_tstamp) timeout_ms);
        m_curlTimer.start();
    }
    return 0;
}

void Balau::TaskMan::curlMultiTimerEventCallback(ev::timer & w, int revents) {
    curl_multi_socket_action(m_curlMulti, CURL_SOCKET_TIMEOUT, 0, &m_curlStillRunning);
}

void Balau::TaskMan::aresSocketCallbackStatic(void * data, curl_socket_t s, int read, int write) {
    TaskMan * taskMan = (TaskMan *) data;
    return taskMan->aresSocketCallback(s, read, write);
}

void Balau::TaskMan::aresSocketCallback(curl_socket_t s, int read, int write) {
    int fd = fromSocket(s);
    int i;
    int freeSlot = ARES_MAX_SOCKETS;

    int what = CURL_POLL_NONE;

    for (i = 0; i < ARES_MAX_SOCKETS; i++) {
        if (m_aresSockets[i] == s)
            break;
        if (m_aresSockets[i] == ARES_SOCKET_BAD)
            freeSlot = i;
    }

    if (i == ARES_MAX_SOCKETS)
        i = freeSlot;

    IAssert(i != ARES_MAX_SOCKETS, "ares socket error - please increase ARES_MAX_SOCKETS");

    if (!read && !write) {
        what = CURL_POLL_REMOVE;
    } else if (read && !write) {
        what = CURL_POLL_IN;
    } else if (!read && write) {
        what = CURL_POLL_OUT;
    } else if (read && write) {
        what = CURL_POLL_INOUT;
    }

    struct timeval tv = { 5, 0 };
    ares_timeout(m_aresChannel, &tv, &tv);

    m_aresTimer.stop();
    m_aresTimer.set((ev_tstamp)(tv.tv_sec * 1000 + tv.tv_usec / 1000 + 1));
    m_aresTimer.start();

    ev::io * evt = m_aresSocketEvents[i];
    if (!evt) {
        if (what == CURL_POLL_REMOVE)
            return;
        evt = new ev::io;
        evt->set<TaskMan, &TaskMan::aresSocketEventCallback>(this);
        evt->set(m_loop);
        m_aresSocketEvents[i] = evt;
        m_aresSockets[i] = s;
    }

    switch (what) {
    case CURL_POLL_IN:
        evt->stop();
        evt->set(fd, ev::READ);
        evt->start();
        break;
    case CURL_POLL_OUT:
        evt->stop();
        evt->set(fd, ev::WRITE);
        evt->start();
        break;
    case CURL_POLL_INOUT:
        evt->stop();
        evt->set(fd, ev::READ | ev::WRITE);
        evt->start();
        break;
    case CURL_POLL_REMOVE:
        evt->stop();
        delete evt;
        m_aresSocketEvents[i] = NULL;
        m_aresSockets[i] = ARES_SOCKET_BAD;
    }

    return;
}

void Balau::TaskMan::aresSocketEventCallback(ev::io & w, int revents) {
    ares_socket_t s = toSocket(w.fd);
    ares_process_fd(m_aresChannel, revents & (ev::READ | ev::ERROR) ? s : ARES_SOCKET_BAD, revents & (ev::WRITE | ev::ERROR) ? s : ARES_SOCKET_BAD);
}

void Balau::TaskMan::aresTimerEventCallback(ev::timer & w, int revents) {
    ares_process(m_aresChannel, NULL, NULL);
}

#ifdef _WIN32
namespace {

class WinSocketStartup : public Balau::AtStart {
  public:
      WinSocketStartup() : AtStart(5) { }
    virtual void doStart() {
        WSADATA wsaData;
        int r = WSAStartup(MAKEWORD(2, 0), &wsaData);
        RAssert(r == 0, "WSAStartup returned %i", r);
    }
};

static WinSocketStartup wsa;

};
#endif

Balau::TaskMan * Balau::TaskMan::getDefaultTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    AAssert(localTaskMan.getGlobal() != this, "Don't create / delete a TaskMan directly");
    while (m_stacks.size() != 0) {
        free(m_stacks.front());
        m_stacks.pop();
    }
    s_scheduler.unregisterTaskMan(this);
    // probably way more work to do here in order to clean up tasks from that thread
    m_evt.stop();
    taskHash_t tasks = std::move(m_curlTasks);
    m_curlTasks.clear();
    for (Task * t : tasks) {
        CurlTask * c = dynamic_cast<CurlTask *>(t);
        if (!c)
            continue;
        unregisterCurlHandle(c);
    }
    tasks.clear();
    curl_multi_cleanup(m_curlMulti);
    ares_destroy(m_aresChannel);
    m_curlTimer.stop();
    m_aresTimer.stop();
    if (m_aresSocketEvents[0])
        m_aresSocketEvents[0]->stop();
    m_aresSocketEvents[0] = NULL;
    if (m_aresSocketEvents[1])
        m_aresSocketEvents[1]->stop();
    m_aresSocketEvents[1] = NULL;

    ev_loop_destroy(m_loop);
}

void * Balau::TaskMan::getStack() {
    if (!Task::needsStacks())
        return NULL;
    void * r = NULL;
    if (m_nStacks == 0) {
        r = malloc(Task::stackSize());
    } else {
        r = m_stacks.front();
        m_stacks.pop();
        m_nStacks--;
    }
    return r;
}

void Balau::TaskMan::freeStack(void * stack) {
    if (!stack)
        return;
    if (m_nStacks >= TOO_MANY_STACKS) {
        free(stack);
    } else {
        m_stacks.push(stack);
        m_nStacks++;
    }
}

int Balau::TaskMan::mainLoop() {
    taskHash_t starting, stopped, yielded, yielded2;
    taskHash_t::iterator iH;

    // we start by pushing all of the 'STARTING' tasks into the appropriate queue.
    for (Task * t : m_tasks)
        if (t->getStatus() == Task::STARTING)
            starting.insert(t);

    s_async.setIdleReadyCallback(asyncIdleReady, this);

    do {
        Printer::elog(E_TASK, "TaskMan::mainLoop() at %p with m_tasks.size = %li", this, m_tasks.size());

        // checking "STARTING" tasks, and running them once
        while ((iH = starting.begin()) != starting.end()) {
            Task * t = *iH;
            IAssert(t->getStatus() == Task::STARTING, "Got task at %p in the starting list, but isn't starting.", t);
            t->switchTo();
            IAssert(t->getStatus() != Task::STARTING, "Task at %p got switchedTo, but still is 'STARTING'.", t);
            starting.erase(iH);
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED))
                stopped.insert(t);
            if (t->getStatus() == Task::YIELDED)
                yielded.insert(t);
        }

        // if we begin that loop with any pending task, just don't block, so we can add them immediately.
        bool noWait = !m_pendingAdd.isEmpty() || !yielded.empty() || !stopped.empty();
        bool curlNeedsSpin = (!m_curlTimer.is_active() && m_curlStillRunning != 0) || m_curlGotNewHandles;

        // Process c-ares requests
        m_allowedToSignal = true;
        while (!m_aresRequests.empty()) {
            AresRequest * request = m_aresRequests.front();
            m_aresRequests.pop();
            ares_gethostbyname(m_aresChannel, request->name.to_charp(), request->family, aresHostCallback, request->callback);
            // Because c-ares might call its callbacks immediately, we don't know if we didn't just wake up a few tasks,
            // so we need to spin at least once.
            noWait = true;
            delete request;
        }

        // libev's event "loop". We always runs it once though.
        Printer::elog(E_TASK, "TaskMan at %p Going to libev main loop; stopped = %s", this, m_stopped ? "true" : "false");
        ev_run(m_loop, noWait || curlNeedsSpin || m_stopped ? EVRUN_NOWAIT : EVRUN_ONCE);
        Printer::elog(E_TASK, "TaskMan at %p Getting out of libev main loop", this);

        // calling async's idle
        s_async.idle();

        // process curl events, and signal tasks
        if (m_curlGotNewHandles || curlNeedsSpin)
            curl_multi_socket_all(m_curlMulti, &m_curlStillRunning);
        m_curlGotNewHandles = false;

        CURLMsg * curlMsg = NULL;
        int curlMsgInQueue;

        while ((curlMsg = curl_multi_info_read(m_curlMulti, &curlMsgInQueue))) {
            if (curlMsg->msg != CURLMSG_DONE)
                continue;
            Task * maybeCurlTask = NULL;
            curl_easy_getinfo(curlMsg->easy_handle, CURLINFO_PRIVATE, &maybeCurlTask);
            IAssert(maybeCurlTask, "curl easy handle didn't have any private data...");
            CurlTask * curlTask = dynamic_cast<CurlTask *>(maybeCurlTask);
            IAssert(curlTask, "curl easy handle had corrupted private data...");
            curlTask->curlDone(curlMsg->data.result);
        }

        // let's check what task got stopped, and signal them
        for (Task * t : stopped) {
            IAssert((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED), "Task %p in stopped list but isn't stopped.", t);
            if (t->m_waitedBy.size() != 0)
                for (Events::TaskEvent * e : t->m_waitedBy)
                    e->signal();
        }
        m_allowedToSignal = false;

        // let's check who got signaled, and call them
        for (Task * t : m_signaledTasks) {
            Printer::elog(E_TASK, "TaskMan at %p Switching to task %p (%s - %s) that got signaled somehow.", this, t, t->getName(), ClassName(t).c_str());
            IAssert(t->getStatus() == Task::SLEEPING || t->getStatus() == Task::YIELDED, "We're switching to a non-sleeping/yielded task at %p... ? status = %i", t, t->getStatus());
            bool toRemoveFromYielded = t->getStatus() == Task::YIELDED;
            t->switchTo();
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                stopped.insert(t);
            } else if (t->getStatus() == Task::YIELDED) {
                taskHash_t::iterator i = yielded.find(t);
                if (i == yielded.end())
                    yielded.insert(t);
                toRemoveFromYielded = false;
            }
            if (toRemoveFromYielded) {
                taskHash_t::iterator i = yielded.find(t);
                IAssert(i != yielded.end(), "Task %s of type %s at %p was yielded, but not in yielded list... ?", t->getName(), ClassName(t).c_str(), t);
                yielded.erase(i);
            }
        }
        m_signaledTasks.clear();

        // now let's make a round of yielded tasks
        for (Task * t : yielded) {
            Printer::elog(E_TASK, "TaskMan at %p Switching to task %p (%s - %s) that was yielded.", this, t, t->getName(), ClassName(t).c_str());
            IAssert(t->getStatus() == Task::YIELDED, "Task %s of type %s at %p was in yielded list, but wasn't yielded ?", t->getName(), ClassName(t).c_str(), t);
            t->switchTo();
            if ((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED)) {
                stopped.insert(t);
            } else if (t->getStatus() == Task::YIELDED) {
                yielded2.insert(t);
            }
        }
        yielded = yielded2;
        yielded2.clear();

        // Adding tasks that were added, maybe from other threads
        while (!m_pendingAdd.isEmpty()) {
            Printer::elog(E_TASK, "TaskMan at %p trying to pop a task...", this);
            Task * t = m_pendingAdd.pop();
            Printer::elog(E_TASK, "TaskMan at %p popped task %s of type %s at %p...", this, t->getName(), ClassName(t).c_str(), t);
            IAssert(m_tasks.find(t) == m_tasks.end(), "TaskMan got task %p twice... ?", t);
            ev_now_update(m_loop);
            t->setup(this, t->isStackless() ? NULL : getStack());
            m_tasks.insert(t);
            starting.insert(t);
        }

        // Finally, let's destroy tasks that no longer are necessary.
        bool didDelete;
        do {
            didDelete = false;
            for (auto iH = stopped.begin(); iH != stopped.end(); iH++) {
                Task * t = *iH;
                IAssert((t->getStatus() == Task::STOPPED) || (t->getStatus() == Task::FAULTED), "Task %p in stopped list but isn't stopped.", t);
                t->m_eventLock.enter();
                if (t->m_waitedBy.size() == 0) {
                    freeStack(t->m_stack);
                    stopped.erase(iH);
                    iH = m_tasks.find(t);
                    IAssert(iH != m_tasks.end(), "Task %s of type %s at %p in stopped list but not in m_tasks...", t->getName(), ClassName(t).c_str(), t);
                    m_tasks.erase(iH);
                    IAssert(yielded.find(t) == yielded.end(), "Task %s of type %s at %p is deleted but is in yielded list... ?", t->getName(), ClassName(t).c_str(), t);
                    t->m_eventLock.leave();
                    delete t;
                    didDelete = true;
                    break;
                }
                if (!didDelete)
                    t->m_eventLock.leave();
            }
        } while (didDelete);

    } while (!m_stopped);
    Printer::elog(E_TASK, "TaskManager at %p stopping.", this);
    s_async.setIdleReadyCallback(NULL, NULL);
    return m_stopCode;
}

void Balau::TaskMan::registerCurlHandle(Balau::CurlTask * curlTask) {
    m_curlGotNewHandles = true;
    curl_easy_setopt(curlTask->m_curlHandle, CURLOPT_SHARE, s_curlShared);
    curl_easy_setopt(curlTask->m_curlHandle, CURLOPT_PRIVATE, curlTask);
    curl_easy_setopt(curlTask->m_curlHandle, CURLOPT_NOSIGNAL, 1L);
    curl_multi_add_handle(m_curlMulti, curlTask->m_curlHandle);
    m_curlTasks.insert(curlTask);
}

void Balau::TaskMan::unregisterCurlHandle(Balau::CurlTask * curlTask) {
    void * ptr = NULL;
    curl_easy_getinfo(curlTask->m_curlHandle, CURLINFO_PRIVATE, &ptr);
    if (!ptr)
        return;
    curl_easy_setopt(curlTask->m_curlHandle, CURLOPT_PRIVATE, static_cast<void *>(0));
    curl_multi_remove_handle(m_curlMulti, curlTask->m_curlHandle);
    m_curlTasks.erase(curlTask);
}

void Balau::TaskMan::getHostByName(const Balau::String & name, int family, AresHostCallback callback) {
    AresRequest * request = new AresRequest();
    request->name = name;
    request->family = family;
    request->callback = new AresHostCallback(callback);
    m_aresRequests.push(request);
}

void Balau::TaskMan::aresHostCallback(void * arg, int status, int timeouts, struct hostent * hostent) {
    AresHostCallback * callback = (AresHostCallback *) arg;
    (*callback)(status, timeouts, hostent);
    delete callback;
}

void Balau::TaskMan::iRegisterTask(Balau::Task * t, Balau::Task * stick, Events::TaskEvent * event) {
    if (stick) {
        IAssert(!event, "inconsistent");
        TaskMan * tm = stick->getTaskMan();
        tm->addToPending(t);
    } else {
        if (event)
            event->attachToTask(t);
        s_scheduler.registerTask(t);
    }
}

void Balau::TaskMan::registerAsyncOp(Balau::AsyncOperation * op) {
    s_async.queueOp(op);
}

void Balau::TaskMan::addToPending(Balau::Task * t) {
    m_pendingAdd.push(t);
    m_evt.send();
}

void Balau::TaskMan::signalTask(Task * t) {
    AAssert(m_tasks.find(t) != m_tasks.end(), "Can't signal task %s of type %s at %p that I don't own (me = %p)", t->getName(), ClassName(t).c_str(), t, this);
    AAssert(m_allowedToSignal, "I'm not allowed to signal (me = %p)", this);
    m_signaledTasks.insert(t);
}

void Balau::TaskMan::stop(int code) {
    s_scheduler.stopAll(code);
}

void * Balau::TaskMan::TaskManThread::proc() {
    bool success = false;
    m_taskMan = NULL;
    try {
        m_taskMan = new Balau::TaskMan();
        m_taskMan->mainLoop();
        success = true;
    }
    catch (Exit & e) {
        Printer::log(M_ERROR, "We shouldn't have gotten an Exit exception here... exitting anyway");
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_ERROR, "%s", str.to_charp());
    }
    catch (RessourceException & e) {
        Printer::log(M_ERROR | M_ALERT, "The TaskMan thread got a ressource problem: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
    }
    catch (GeneralException & e) {
        Printer::log(M_ERROR | M_ALERT, "The TaskMan thread caused an exception: %s", e.getMsg());
        const char * details = e.getDetails();
        if (details)
            Printer::log(M_ERROR, "  %s", details);
        auto trace = e.getTrace();
        for (String & str : trace)
            Printer::log(M_DEBUG, "%s", str.to_charp());
    }
    catch (...) {
        Printer::log(M_ERROR | M_ALERT, "The TaskMan thread caused an unknown exception");
    }
    if (!success) {
        if (m_taskMan)
            delete m_taskMan;
        m_taskMan = NULL;
        TaskMan::stop(-1);
    }
    return NULL;
}

Balau::TaskMan::TaskManThread::~TaskManThread() {
    if (m_taskMan)
        delete m_taskMan;
}
