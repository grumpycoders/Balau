#pragma once

namespace Balau {

class AtStart {
  protected:
      AtStart(int priority = 0);
    virtual void doStart() = 0;
  private:
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
    const int m_priority;
    AtExit * m_next;
    static AtExit * s_head;
    friend class Main;
};

};
