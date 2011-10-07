#pragma once

#include <stdlib.h>
#include <coro.h>

namespace Balau {

class TaskMan;

class Task {
  public:
    enum Status {
        STARTING,
        RUNNING,
        IDLE,
        STOPPED,
        FAULTED,
    };
      Task();
      ~Task() { free(stack); free(tls); }
    virtual void Do() = 0;
    virtual const char * getName() = 0;
    Status getStatus() { return status; }
  protected:
    void suspend();
  private:
    size_t stackSize() { return 128 * 1024; }
    void switchTo();
    static void coroutine(void *);
    void * stack;
    coro_context ctx;
    TaskMan * taskMan;
    Status status;
    void * tls;
    friend class TaskMan;
};

};
