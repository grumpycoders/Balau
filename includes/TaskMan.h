#pragma once

#include <stdint.h>
#include <coro.h>
#include <ext/hash_set>
#include <vector>

namespace gnu = __gnu_cxx;

namespace Balau {

class Task;

class TaskMan {
  public:
      TaskMan();
      ~TaskMan();
    void mainLoop();
    void stop() { m_stopped = true; }
    static TaskMan * getTaskMan();

  private:
    void registerTask(Task * t);
    void unregisterTask(Task * t);
    coro_context m_returnContext;
    friend class Task;
    struct taskHasher { size_t operator()(const Task * t) const { return reinterpret_cast<uintptr_t>(t); } };
    typedef gnu::hash_set<Task *, taskHasher> taskHash_t;
    typedef std::vector<Task *> taskList_t;
    taskHash_t m_tasks;
    taskList_t m_pendingAdd;
    volatile bool m_stopped;
};

};
