#pragma once

#include <pthread.h>

namespace Balau {

class Lock {
  public:
      Lock();
      ~Lock() { pthread_mutex_destroy(&m_lock); }
    void enter() { pthread_mutex_lock(&m_lock); }
    void leave() { pthread_mutex_unlock(&m_lock); }
  private:
    pthread_mutex_t m_lock;
};

};
