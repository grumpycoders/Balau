#pragma once

#include <stdint.h>
#ifndef _WIN32
#include <coro.h>
#endif
#include <ev++.h>
#include <ext/hash_set>
#include <queue>
#include <Async.h>
#include <Threads.h>
#include <Exceptions.h>
#include <Task.h>

namespace gnu = __gnu_cxx;

namespace Balau {

class TaskScheduler;

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
#ifndef _WIN32
    coro_context m_returnContext;
#else
    void * m_fiber;
#endif
    friend class Task;
    friend class TaskScheduler;
    template<class T>
    friend T * createAsyncOp(T * op);
    struct taskHasher { size_t operator()(const Task * t) const { return reinterpret_cast<uintptr_t>(t); } };
    typedef gnu::hash_set<Task *, taskHasher> taskHash_t;
    taskHash_t m_tasks, m_signaledTasks;
    Queue<Task> m_pendingAdd;
    struct ev_loop * m_loop;
    ev::async m_evt;
    std::queue<void *> m_stacks;
    int m_nStacks;
    int m_stopCode = 0;
    bool m_stopped = false;
    bool m_allowedToSignal = false;

      TaskMan(const TaskMan &) = delete;
    TaskMan & operator=(const TaskMan &) = delete;
};

template<class T>
T * createAsyncOp(T * op) { TaskMan::registerAsyncOp(op); return op; }

};
