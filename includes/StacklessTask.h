#pragma once

#include <Task.h>

namespace Balau {

class StacklessTask : public Task {
  public:
      StacklessTask() : m_state(0) { setStackless(); }
  protected:
    void taskSwitch() throw (GeneralException) { throw TaskSwitch(); }
    unsigned int m_state;
};

};

#define StacklessBegin() \
    switch(m_state) { \
    case 0: { \


#define StacklessOperation(operation) \
        m_state = __LINE__; \
    } \
    case __LINE__: { \
        try { \
            operation; \
        } \
        catch (Balau::EAgain & e) { \
            auto evt = e.getEvent(); \
            waitFor(evt); \
            taskSwitch(); \
        } \


#define StacklessWaitFor(evt) \
        m_state = __LINE__; \
        waitFor(evt); \
        taskSwitch(); \
    } \
    case __LINE__: { \


#define StacklessWait(cond) \
        m_state = __LINE__; \
    } \
    case __LINE__: { \
        if (!(cond)) \
            taskSwitch(); \


#define StacklessYield() \
        m_state = __LINE__; \
        try { \
            yield(true); \
        } \
        catch (Balau::EAgain & e) { \
            taskSwitch(); \
        } \
    } \
    case __LINE__: { \


#define StacklessEnd() \
    } \
    default: \
        AssertHelper("unknown state", "State %i is out of range in task %s at %p", m_state, getName(), this); \
    }
