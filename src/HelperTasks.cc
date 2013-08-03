#include <algorithm>

#include "HelperTasks.h"

Balau::CopyTask::CopyTask(IO<Handle> s, IO<Handle> d, ssize_t tocopy)
    : m_s(s)
    , m_d(d)
    , m_tocopy(tocopy)
{
    m_name.set("CopyTask from %s to %s", s->getName(), d->getName());
}

void Balau::CopyTask::Do() {
    ssize_t toread, w;

    try {
        for (;;) {
            switch (m_state) {
            case 0:
                toread = m_tocopy >= 0 ? m_tocopy - m_current : COPY_BUFSIZE;
                toread = std::min(toread, (ssize_t) COPY_BUFSIZE);
                m_read = m_s->read(m_buffer, toread);
                AAssert(m_read >= 0, "Error while reading");
                if (m_s->isEOF() || !m_read)
                    return;
                m_written = 0;
                m_state = 1;
            case 1:
                do {
                    w = m_d->write(m_buffer + m_written, m_read - m_written);
                    AAssert(w >= 0, "Error while writing");
                    m_written += w;
                } while (m_read != m_written);
                m_state = 0;
                m_current += m_read;
            }
        }
    }
    catch (EAgain & e) {
        taskSwitch();
    }
}
