#include <Main.h>
#include <Input.h>
#include <Output.h>
#include <Buffer.h>
#include <BStream.h>
#include <ZHandle.h>
#include <TaskMan.h>
#include <StacklessTask.h>

#ifdef _WIN32
void ctime_r(const time_t * t, char * str) {
#ifdef _MSVC
    ctime_s(str, 32, t);
#else
    strcpy(str, ctime(t));
#endif
}
#endif

// this may look weird for a dumb test, but I always
// struggle to remember this formula, so I'm going
// to put it here as a "data generator" and then
// I'll know where to find it if I ever need it.

class DiscreteCos {
  public:
      DiscreteCos() { generate(); }
    static const unsigned int DC_2PI = 2048;
    static const unsigned int DC_PI  = 1024;
    static const unsigned int DC_PI2 = 512;

    int32_t cos(unsigned int t) {
        t %= DC_2PI;
        int32_t r;

        if (t < DC_PI2) {
            r = m_cosTable[t];
        } else if (t < DC_PI) {
            r = -m_cosTable[DC_PI - 1 - t];
        } else if (t < (DC_PI + DC_PI2)) {
            r = -m_cosTable[t - DC_PI];
        } else {
            r = m_cosTable[DC_2PI - 1 - t];
        }

        return r;
    }

    // sin(x) = cos(x - pi / 2)
    int32_t sin(unsigned int t) {
        t %= DC_2PI;

        if (t < DC_PI2)
            return cos(t + DC_2PI - DC_PI2);

        return cos(t - DC_PI2);
    }

  private:
    int32_t m_cosTable[512] = {
        16777216, // 2^24 * cos(0 * 2pi / 2048)
        16777137, // 2^24 * cos(1 * 2pi / 2048) = C = f(1)
    };

    // f(n) = cos(n * 2pi / 2048)
    // f(n) = 2 * f(1) * f(n - 1) - f(n - 2)
    void generate() {
        int64_t C = m_cosTable[1];

        for (int i = 2; i < 512; i++)
            m_cosTable[i] = ((C * m_cosTable[i - 1]) >> 23) - m_cosTable[i - 2];

        m_cosTable[511] = 0;
    }
};

DiscreteCos dc;

class DataGenerator {
  public:
      DataGenerator() { generate(); }
    static const size_t size = 128 * 1024;
    const uint8_t * getData() { return (uint8_t *) m_data; }

  private:
    static const int nEntries = size / 4;
    void generate() {
        for (int i = 0; i < nEntries; i++) {
            int64_t c = dc.cos(i);
            // This version is actually too hardcore, and generates
            // uncompressible data. Fun!
//          c *= ((int64_t(i) * 100) << 24) / nEntries;
            // this one is technically wrong, but at least it produces
            // compressible data, so who cares about maths.
            c *= i % (10 * 1024);
            c >>= 24;
            m_data[i] = c;
        }
    }

    int32_t m_data[nEntries];
};

DataGenerator dg;

using namespace Balau;

class SimpleTaskTest : public Task {
    virtual void Do();
    const char * getName() const { return "SimpleTaskTest"; }
};

void SimpleTaskTest::Do() {
    bool failed = false;
    try {
        IO<Input> i(new Input("SomeInexistantFile.txt"));
        i->open();
    }
    catch (ENoEnt e) {
        failed = true;
    }
    TAssert(failed);
    IO<Input> i(new Input("tests/rtest.txt"));
    i->open();
    Printer::log(M_STATUS, "Opened file %s:", i->getName());
    Printer::log(M_STATUS, " - size = %" PRIi64, i->getSize());

    char mtimestr[32];
    time_t mtime = i->getMTime();
    ctime_r(&mtime, mtimestr);
    char * nl = strrchr(mtimestr, '\n');
    if (nl)
        *nl = 0;
    Printer::log(M_STATUS, " - mtime = %li (%s)", mtime, mtimestr);

    off_t s = i->rtell();
    TAssert(s == 0);

    i->rseek(0, SEEK_END);
    s = i->rtell();
    TAssert(s == i->getSize());

    i->rseek(0, SEEK_SET);
    char * buf1 = (char *) Balau::malloc(i->getSize());
    ssize_t r = i->read(buf1, s + 15);
    Printer::log(M_STATUS, "Read %zi bytes (instead of %" PRIi64 ")", r, s + 15);
    TAssert(i->isEOF())

    char * buf2 = (char *) Balau::malloc(i->getSize());
    i->rseek(0, SEEK_SET);
    TAssert(!i->isEOF());
    TAssert(i->rtell() == 0);
    r = i->read(buf2, 5);
    TAssert(r == 5);
    TAssert(i->rtell() == 5);
    r = i->read(buf2 + 5, s - 5);
    TAssert(r == (s - 5));
    TAssert(memcmp(buf1, buf2, s) == 0);

    IO<Output> o(new Output("tests/out.txt"));
    o->open();
    s = o->wtell();
    TAssert(s == 0);
    s = o->getSize();
    TAssert(s == 0);
    o->writeString("foo\n");

    IO<Handle> b(new Buffer());
    s = b->rtell();
    TAssert(s == 0);
    s = b->wtell();
    TAssert(s == 0);
    b->writeString("foo\n");
    s = b->rtell();
    TAssert(s == 0);
    s = b->wtell();
    TAssert(s == 4);
    b->writeString("bar\r\n");
    s = b->rtell();
    TAssert(s == 0);
    s = b->wtell();
    TAssert(s == 9);
    b->writeString("eof");
    s = b->rtell();
    TAssert(s == 0);
    s = b->wtell();
    TAssert(s == 12);

    IO<BStream> strm(new BStream(b));
    String str;
    str = strm->readString();
    TAssert(str == "foo");
    str = strm->readString();
    TAssert(str == "bar");
    str = strm->readString();
    TAssert(str == "eof");
    s = b->rtell();
    TAssert(s == 12);
    TAssert(b->isEOF());

    {
        IO<Output> o(new Output("tests/out.z"));
        o->open();
        IO<ZStream> z(new ZStream(o));
        z->detach();
        z->writeString("foobar\n");
    }

    {
        IO<Output> o(new Output("tests/out.gz"));
        o->open();
        IO<ZStream> z(new ZStream(o, Z_BEST_COMPRESSION, ZStream::GZIP));
        z->detach();
        z->writeString("foobar\n");
    }

    {
        IO<Output> o(new Output("tests/out.raw"));
        o->open();
        IO<ZStream> z(new ZStream(o, Z_BEST_COMPRESSION, ZStream::RAW));
        z->detach();
        z->writeString("foobar\n");
    }

    {
        IO<Input> i(new Input("tests/out.z"));
        i->open();
        IO<ZStream> z(new ZStream(i));
        IO<BStream> s(new BStream(z));
        z->detach();
        s->detach();
        String f = s->readString();
        TAssert(f == "foobar");
    }

    {
        IO<Input> i(new Input("tests/out.gz"));
        i->open();
        IO<ZStream> z(new ZStream(i, Z_BEST_COMPRESSION, ZStream::GZIP));
        IO<BStream> s(new BStream(z));
        z->detach();
        s->detach();
        String f = s->readString();
        TAssert(f == "foobar");
    }

    {
        IO<Input> i(new Input("tests/out.raw"));
        i->open();
        IO<ZStream> z(new ZStream(i, Z_BEST_COMPRESSION, ZStream::RAW));
        IO<BStream> s(new BStream(z));
        z->detach();
        s->detach();
        String f = s->readString();
        TAssert(f == "foobar");
    }
}

class StacklessTaskTest : public StacklessTask {
    virtual void Do();
    const char * getName() const { return "StacklessTaskTest"; }
    IO<Handle> h, s;
    uint8_t data[dg.size];
    ssize_t r;
};

void StacklessTaskTest::Do() {
    StacklessBegin();

    h = new Output("tests/data.raw");
    StacklessOperation(h.asA<Output>()->open());
    StacklessOperation(r = h->write(dg.getData(), dg.size));
    TAssert(r == dg.size);
    StacklessOperation(h->close());

    h = new Input("tests/data.raw");
    StacklessOperation(h.asA<Input>()->open());
    StacklessOperation(r = h->read(data, dg.size));
    TAssert(r == dg.size);
    StacklessOperation(h->close());

    h = new Output("tests/data.gz");
    StacklessOperation(h.asA<Output>()->open());
    s = new ZStream(h, Z_BEST_COMPRESSION, ZStream::GZIP);
    StacklessOperation(r = s->write(dg.getData(), dg.size));
    TAssert(r == dg.size);
    StacklessOperation(s->close());

    h = new Input("tests/data.gz");
    StacklessOperation(h.asA<Input>()->open());
    s = new ZStream(h, Z_BEST_COMPRESSION, ZStream::GZIP);
    StacklessOperation(r = s->read(data, dg.size));
    TAssert(r == dg.size);
    TAssert(memcmp(dg.getData(), data, dg.size) == 0);
    StacklessOperation(s->close());

    StacklessEnd();
}

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Handles running.");

    Events::TaskEvent taskEvt;

    TaskMan::registerTask(new SimpleTaskTest(), &taskEvt);
    waitFor(&taskEvt);
    TAssert(!taskEvt.gotSignal());
    yield();
    TAssert(taskEvt.gotSignal());
    taskEvt.ack();

    TaskMan::registerTask(new StacklessTaskTest(), &taskEvt);
    waitFor(&taskEvt);
    TAssert(!taskEvt.gotSignal());
    yield();
    TAssert(taskEvt.gotSignal());
    taskEvt.ack();

    Printer::log(M_STATUS, "Test::Handles passed.");
}
