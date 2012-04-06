#pragma once

#include <stdint.h>
#ifndef _WIN32
#include <coro.h>
#endif
#include <ev++.h>
#include <ext/hash_set>
#include <queue>
#include <Threads.h>
#include <Exceptions.h>
#include <Task.h>

namespace gnu = __gnu_cxx;

namespace Balau {

class TaskScheduler;

namespace Events {

class Async;

};

class TaskMan {
  public:
    class TaskManThread : public Thread {
      public:
          virtual ~TaskManThread();
        virtual void * proc();
        void stopMe(int code = 0) { m_taskMan->stopMe(code); }
      private:
        TaskMan * m_taskMan;
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
    bool stopped() { return m_stopped; }
    template<class T>
    static T * createTask(T * t, Task * stick = NULL) { TaskMan::registerTask(t, stick); return t; }

  private:
    static void registerTask(Task * t, Task * stick);
    void * getStack();
    void freeStack(void * stack);
    void addToPending(Task * t);
#ifndef _WIN32
    coro_context m_returnContext;
#else
    void * m_fiber;
#endif
    friend class Task;
    friend class TaskScheduler;
    template<class T>
    friend T * createTask(T * t, Task * stick = NULL);
    struct taskHasher { size_t operator()(const Task * t) const { return reinterpret_cast<uintptr_t>(t); } };
    typedef gnu::hash_set<Task *, taskHasher> taskHash_t;
    taskHash_t m_tasks, m_signaledTasks;
    Queue<Task> m_pendingAdd;
    bool m_stopped;
    struct ev_loop * m_loop;
    bool m_allowedToSignal;
    ev::async m_evt;
    std::queue<void *> m_stacks;
    int m_nStacks;
    int m_stopCode;
};

};
