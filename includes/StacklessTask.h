#pragma once

#include <Task.h>

namespace Balau {

class StacklessTask : public Task {
  public:
      StacklessTask() { setStackless(); }
  protected:
    void taskSwitch() throw (GeneralException) { throw TaskSwitch(); }
    unsigned int m_state = 0;
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
        catch (Balau::EAgain &) { \
            taskSwitch(); \
        } \


#define StacklessOperationOrCond(operation, cond) \
        m_state = __LINE__; \
    } \
    case __LINE__: { \
        try { \
            if (!(cond)) { \
                operation; \
            } \
        } \
        catch (Balau::EAgain &) { \
            taskSwitch(); \
        } \


#define StacklessWaitFor(evt) \
        m_state = __LINE__; \
        waitFor(evt); \
        taskSwitch(); \
    } \
    case __LINE__: { \


#define StacklessWaitCond(cond) \
        m_state = __LINE__; \
    } \
    case __LINE__: { \
        if (!(cond)) \
            taskSwitch(); \


#define StacklessYield() \
        m_state = __LINE__; \
        try { \
            yieldNoWait(); \
        } \
        catch (Balau::EAgain &) { \
            taskSwitch(); \
        } \
    } \
    case __LINE__: { \


#define StacklessEnd() \
        break; \
    } \
    default: \
        AssertHelper("unknown state", "State %i is out of range in task %s at %p", m_state, getName(), this); \
    }
