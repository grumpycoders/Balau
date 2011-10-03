#include "Main.h"

Balau::AtStart * Balau::AtStart::s_head = 0;
Balau::AtExit * Balau::AtExit::s_head = 0;

Balau::AtStart::AtStart(int priority) : m_priority(priority) {
    if (priority < 0)
        return;

    AtStart ** ptr = &s_head;

    m_next = 0;

    for (ptr = &s_head; *ptr && (priority > (*ptr)->m_priority); ptr = &((*ptr)->m_next));

    m_next = *ptr;
    *ptr = this;
}

Balau::AtExit::AtExit(int priority) : m_priority(priority) {
    if (priority < 0)
        return;

    AtExit ** ptr = &s_head;

    m_next = 0;

    for (ptr = &s_head; *ptr && (priority > (*ptr)->m_priority); ptr = &((*ptr)->m_next));

    m_next = *ptr;
    *ptr = this;
}

Balau::Main * Balau::Main::application = NULL;
