#pragma once

#include <coro.h>
#include <ext/hash_set>
#include <stdint.h>

namespace gnu = __gnu_cxx;

namespace Balau {

class Task;

class TaskMan {
  public:
      TaskMan();
      ~TaskMan();
    void mainLoop();
    void stop() { stopped = true; }
    static TaskMan * getTaskMan();

  private:
    void registerTask(Task * t);
    void unregisterTask(Task * t);
    coro_context returnContext;
    friend class Task;
    struct taskHash { size_t operator()(const Task * t) const { return reinterpret_cast<uintptr_t>(t); } };
    typedef gnu::hash_set<Task *, taskHash> taskList;
    taskList tasks, pendingAdd;
    volatile bool stopped;
};

};
