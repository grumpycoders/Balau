#pragma once

#include <coro.h>

namespace Balau {

class Task;

class TaskMan {
  public:
      TaskMan();
      ~TaskMan();
    void mainLoop();

  private:
    static TaskMan * getTaskMan();
    coro_context returnContext;
    friend class Task;
};

};
