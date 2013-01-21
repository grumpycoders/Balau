#include <Main.h>
#include <Input.h>
#include <Output.h>
#include <Buffer.h>
#include <BStream.h>
#include <ZHandle.h>

#ifdef _WIN32
void ctime_r(const time_t * t, char * str) {
#ifdef _MSVC
    ctime_s(str, 32, t);
#else
    strcpy(str, ctime(t));
#endif
}
#endif

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Handles running.");

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
    char * buf1 = (char *) malloc(i->getSize());
    ssize_t r = i->read(buf1, s + 15);
    Printer::log(M_STATUS, "Read %zi bytes (instead of %" PRIi64 ")", r, s + 15);
    TAssert(i->isEOF())

    char * buf2 = (char *) malloc(i->getSize());
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

    Printer::log(M_STATUS, "Test::Handles passed.");
}
