#pragma once

#include <stdint.h>
#include <curl/curl.h>
#ifndef _WIN32
#include <netdb.h>
#endif
#ifdef __linux
#include <ucontext.h>
#endif
#include <ev++.h>
#ifdef _MSC_VER
#include <hash_set>
#else
#include <ext/hash_set>
#endif
#include <queue>
#include <Async.h>
#include <Threads.h>
#include <Exceptions.h>
#include <Task.h>

#ifndef _MSC_VER
namespace gnu = __gnu_cxx;
#endif

struct ares_channeldata;

namespace Balau {

class TaskScheduler;
class CurlTask;

namespace Events {

class TaskEvent;

};

class TaskMan {
  public:
    class TaskManThread : public Thread {
      public:
          virtual ~TaskManThread();
        virtual void * proc();
        void stopMe(int code = 0) { m_taskMan->stopMe(code); }
      private:
        TaskMan * m_taskMan = NULL;
    };

      TaskMan();
      ~TaskMan();
    int mainLoop();
    static TaskMan * getDefaultTaskMan();
    struct ev_loop * getLoop() { return m_loop; }
    void signalTask(Task * t);
    static void stop(int code);
    void stopMe(int code = 0);
    static TaskManThread * createThreadedTaskMan() {
        TaskManThread * r = new TaskManThread();
        r->threadStart();
        return r;
    }
    static void stopThreadedTaskMan(TaskManThread * tmt) {
        tmt->stopMe(0);
        tmt->join();
        delete tmt;
    }
    bool stopped() { return m_stopped; }
    template<class T>
    static T * registerTask(T * t, Task * stick = NULL) { TaskMan::iRegisterTask(t, stick, NULL); return t; }
    template<class T>
    static T * registerTask(T * t, Events::TaskEvent * event) { TaskMan::iRegisterTask(t, NULL, event); return t; }

    typedef std::function<void(int status, int timeouts, struct hostent * hostent)> AresHostCallback;
    void getHostByName(const Balau::String & name, int family, AresHostCallback callback);

  private:
    static void iRegisterTask(Task * t, Task * stick, Events::TaskEvent * event);
    static void registerAsyncOp(AsyncOperation * op);
    void * getStack();
    void freeStack(void * stack);
    void addToPending(Task * t);
    static void asyncIdleReady(void * param) {
        TaskMan * taskMan = (TaskMan *) param;
        taskMan->asyncIdleReady();
    }
    void asyncIdleReady() {
        m_evt.send();
    }
#if defined(__linux)
    ucontext_t m_returnContext;
#elif defined (_WIN32)
    void * m_fiber;
#elif defined (__APPLE__)
    jmp_buf m_returnContext;
#endif
    friend class Task;
    friend class CurlTask;
    friend class TaskScheduler;
    template<class T>
    friend T * createAsyncOp(T * op);
#ifdef _MSC_VER
    typedef stdext::hash_set<Task *> taskHash_t;
#else
    struct taskHasher { size_t operator()(const Task * t) const { return reinterpret_cast<uintptr_t>(t); } };
    typedef gnu::hash_set<Task *, taskHasher> taskHash_t;
#endif
    taskHash_t m_tasks, m_signaledTasks;
    Queue<Task> m_pendingAdd;
    struct ev_loop * m_loop;
    ev::async m_evt;
    std::queue<void *> m_stacks;
    int m_nStacks;
    int m_stopCode = 0;
    bool m_stopped = false;
    bool m_allowedToSignal = false;

    ev::timer m_curlTimer;
    CURLM * m_curlMulti = NULL;
    int m_curlStillRunning = 0;
    bool m_curlGotNewHandles = false;
    static int curlSocketCallbackStatic(CURL * easy, curl_socket_t s, int what, void * userp, void * socketp);
    int curlSocketCallback(CURL * easy, curl_socket_t s, int what, void * socketp);
    void curlSocketEventCallback(ev::io & w, int revents);
    static int curlMultiTimerCallbackStatic(CURLM * multi, long timeout_ms, void * userp);
    int curlMultiTimerCallback(CURLM * multi, long timeout_ms);
    void curlMultiTimerEventCallback(ev::timer & w, int revents);
    void registerCurlHandle(CurlTask * curlTask);
    void unregisterCurlHandle(CurlTask * curlTask);
    taskHash_t m_curlTasks;

    struct ares_channeldata * m_aresChannel = NULL;
    static const int ARES_MAX_SOCKETS = 2;
    curl_socket_t m_aresSockets[ARES_MAX_SOCKETS];
    ev::io * m_aresSocketEvents[ARES_MAX_SOCKETS];
    ev::timer m_aresTimer;
    static void aresSocketCallbackStatic(void * data, curl_socket_t s, int read, int write);
    void aresSocketCallback(curl_socket_t s, int read, int write);
    void aresSocketEventCallback(ev::io & w, int revents);
    void aresTimerEventCallback(ev::timer & w, int revents);
    static void aresHostCallback(void * arg, int status, int timeouts, struct hostent * hostent);
    struct AresRequest {
        Balau::String name;
        int family;
        AresHostCallback * callback;
    };
    std::queue<AresRequest *> m_aresRequests;

      TaskMan(const TaskMan &) = delete;
    TaskMan & operator=(const TaskMan &) = delete;
};

template<class T>
T * createAsyncOp(T * op) { TaskMan::registerAsyncOp(op); return op; }

};
