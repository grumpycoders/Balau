#pragma once

namespace Balau {

class Task;
class BootstrapTask;

class AtStart {
  protected:
      AtStart(int priority = 0);
    virtual void doStart() = 0;
  private:
      AtStart(const AtStart &) = delete;
    AtStart & operator=(const AtStart &) = delete;
    const int m_priority;
    AtStart * m_next;
    static AtStart * s_head;
    friend class Main;
};

class AtExit {
  protected:
      AtExit(int priority = 0);
    virtual void doExit() = 0;
  private:
      AtExit(const AtExit &) = delete;
    AtExit & operator=(const AtExit &) = delete;
    const int m_priority;
    AtExit * m_next;
    static AtExit * s_head;
    friend class Main;
};

class AtStartAsTask {
  protected:
      AtStartAsTask(int priority = 0);
    virtual Task * createStartTask() = 0;
  private:
      AtStartAsTask(const AtStartAsTask &) = delete;
    AtStart & operator=(const AtStartAsTask &) = delete;
    const int m_priority;
    AtStartAsTask * m_next;
    static AtStartAsTask * s_head;
    friend class BootstrapTask;
};

class AtExitAsTask {
  protected:
      AtExitAsTask(int priority = 0);
    virtual Task * createExitTask() = 0;
  private:
      AtExitAsTask(const AtExit &) = delete;
    AtExitAsTask & operator=(const AtExitAsTask &) = delete;
    const int m_priority;
    AtExitAsTask * m_next;
    static AtExitAsTask * s_head;
    friend class BootstrapTask;
};

};
