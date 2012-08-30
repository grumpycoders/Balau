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

namespace gnu = __gnu_cxx;

namespace Balau {

class Task;
class TaskScheduler;

namespace Events {

class Async;

};

class TaskMan {
  public:
      TaskMan();
      ~TaskMan();
    int mainLoop();
    static TaskMan * getDefaultTaskMan();
    struct ev_loop * getLoop() { return m_loop; }
    void signalTask(Task * t);
    static void stop(int code);
    void stopMe(int code) { m_stopped = true; m_stopCode = code; }
    static Thread * createThreadedTaskMan();
    bool stopped() { return m_stopped; }
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
    bool m_stopped = false;
    struct ev_loop * m_loop;
    bool m_allowedToSignal = false;
    ev::async m_evt;
    std::queue<void *> m_stacks;
    int m_nStacks;
    int m_stopCode = 0;
};

template<class T>
T * createTask(T * t, Task * stick) { TaskMan::registerTask(t, stick); return t; }

};
