#include <StacklessTask.h>
#include <TaskMan.h>
#include <SmartWriter.h>

namespace {

struct WriteCell {
      ~WriteCell() { free(buffer); }
    void * buffer = NULL;
    uint8_t * ptr;
    size_t size;
    bool stop = false;
    bool close = false;
};

}

class SmartWriterTask : public Balau::StacklessTask {
  public:
      SmartWriterTask(Balau::IO<Balau::Handle> h) : m_h(h) { m_name.set("SmartWriterTask(%s)", h->getName()); }
    void queueWrite(const void * buf, size_t count) {
        WriteCell * cell = new WriteCell();
        uint8_t * copied = (uint8_t *) malloc(count);
        memcpy(copied, buf, count);
        cell->buffer = cell->ptr = copied;
        cell->size = count;
        m_queue.push(cell);
    }
    void stop(bool closeHandle) {
        WriteCell * cell = new WriteCell();
        cell->stop = true;
        cell->close = closeHandle;
        m_queue.push(cell);
    }
    bool gotError() { return m_gotError.load(); }
  private:
      virtual ~SmartWriterTask() { empty(); }
    virtual const char * getName() const override { return m_name.to_charp(); }
    void empty() {
        delete m_current;
        m_current = NULL;
        while (!m_queue.isEmpty())
            delete m_queue.pop();
    }
    virtual void Do() override {
        waitFor(m_queue.getEvent());
        m_queue.getEvent()->resetMaybe();

        bool gotQueue = !m_queue.isEmpty();

        if (gotQueue && !m_current) {
            m_current = m_queue.pop();
            gotQueue = !m_queue.isEmpty();
        }

        if (!gotQueue && !m_current)
            taskSwitch();

        if (m_current->stop) {
            if (m_current->close)
                m_h->close();
            delete m_current;
            m_current = NULL;
            return;
        }

        ssize_t r = 0;

        if (m_gotError.load()) {
            delete m_current;
            m_current = NULL;
            r = -1;
        } else {
            try {
                r = m_h->write(m_current->ptr, m_current->size);
            }
            catch (Balau::EAgain & e) {
                waitFor(e.getEvent());
                taskSwitch();
            }
        }

        if (r < 0) {
            m_gotError.store(true);
        } else {
            m_current->ptr += r;
            m_current->size -= r;
        }

        if (m_current && m_current->size == 0) {
            delete m_current;
            m_current = NULL;
        }

        if (gotQueue || m_current)
            yield();
        else
            taskSwitch();
    }
    Balau::IO<Balau::Handle> m_h;
    Balau::String m_name;
    std::atomic<bool> m_gotError;
    Balau::TQueue<WriteCell> m_queue;
    WriteCell * m_current = NULL;
};

void Balau::SmartWriter::close() throw (GeneralException) {
    if (m_writerTask) {
        m_writerTask->stop(!isDetached());
        m_writerTask = NULL;
    } else {
        Filter::close();
    }
}

ssize_t Balau::SmartWriter::write(const void * _buf, size_t count) throw (Balau::GeneralException) {
    const uint8_t * buf = (const uint8_t *) _buf;
    const ssize_t r = count;
    while (count) {
        if (m_writerTask) {
            if (m_writerTask->gotError())
                return -1;
            m_writerTask->queueWrite(buf, count);
        }

        ssize_t r = 0;
        try {
            r = Filter::write(buf, count);
        }
        catch (EAgain &) {
            m_writerTask = TaskMan::registerTask(new SmartWriterTask(getIO()));
        }
        if (r < 0)
            return r;
        count -= r;
        buf += r;
    }

    return r;
}
