#pragma once

#include <stdint.h>
#ifndef _WIN32
#include <coro.h>
#endif
#include <ev++.h>
#include <ext/hash_set>
#include <vector>
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
    void mainLoop();
    static TaskMan * getDefaultTaskMan();
    struct ev_loop * getLoop() { return m_loop; }
    void signalTask(Task * t);
    static void stop();
    void stopMe() { m_stopped = true; }
  private:
    static void registerTask(Task * t, Task * stick);
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
    Queue<Task *> m_pendingAdd;
    bool m_stopped;
    struct ev_loop * m_loop;
    bool m_allowedToSignal;
    ev::async m_evt;
};

template<class T>
T * createTask(T * t, Task * stick) { TaskMan::registerTask(t, stick); Assert(dynamic_cast<Task *>(t)); return t; }

};
