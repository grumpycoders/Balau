#include <Main.h>
#include <Input.h>
#include <Output.h>
#include <Buffer.h>
#include <BStream.h>

#ifdef _WIN32
void ctime_r(const time_t * t, char * str) {
#ifdef _MSVC
    ctime_s(str, 32, t);
#else
    strcpy(str, ctime(t));
#endif
}
#endif

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Handles running.");

    bool failed = false;
    try {
        IO<Handle> i(new Input("SomeInexistantFile.txt"));
    }
    catch (ENoEnt e) {
        failed = true;
    }
    Assert(failed);
    IO<Handle> i(new Input("tests/rtest.txt"));
    Printer::log(M_STATUS, "Opened file %s:", i->getName());
    Printer::log(M_STATUS, " - size = %lli", i->getSize());

    char mtimestr[32];
    time_t mtime = i->getMTime();
    ctime_r(&mtime, mtimestr);
    char * nl = strrchr(mtimestr, '\n');
    if (nl)
        *nl = 0;
    Printer::log(M_STATUS, " - mtime = %i (%s)", mtime, mtimestr);

    off_t s = i->rtell();
    Assert(s == 0);

    i->rseek(0, SEEK_END);
    s = i->rtell();
    Assert(s == i->getSize());

    i->rseek(0, SEEK_SET);
    char * buf1 = (char *) malloc(i->getSize());
    ssize_t r = i->read(buf1, s + 15);
    Printer::log(M_STATUS, "Read %i bytes (instead of %i)", r, s + 15);
    Assert(i->isEOF())

    char * buf2 = (char *) malloc(i->getSize());
    i->rseek(0, SEEK_SET);
    Assert(!i->isEOF());
    Assert(i->rtell() == 0);
    r = i->read(buf2, 5);
    Assert(r == 5);
    Assert(i->rtell() == 5);
    r = i->read(buf2 + 5, s - 5);
    Assert(r == (s - 5));
    Assert(memcmp(buf1, buf2, s) == 0);

    IO<Handle> o(new Output("tests/out.txt"));
    s = o->wtell();
    Assert(s == 0);
    s = o->getSize();
    Assert(s == 0);
    o->writeString("foo\n");

    IO<Handle> b(new Buffer());
    s = b->rtell();
    Assert(s == 0);
    s = b->wtell();
    Assert(s == 0);
    b->writeString("foo\n");
    s = b->rtell();
    Assert(s == 0);
    s = b->wtell();
    Assert(s == 4);
    b->writeString("bar\r\n");
    s = b->rtell();
    Assert(s == 0);
    s = b->wtell();
    Assert(s == 9);
    b->writeString("eof");
    s = b->rtell();
    Assert(s == 0);
    s = b->wtell();
    Assert(s == 12);

    IO<BStream> strm(new BStream(b));
    String str;
    str = strm->readString();
    Assert(str == "foo");
    str = strm->readString();
    Assert(str == "bar");
    str = strm->readString();
    Assert(str == "eof");
    s = b->rtell();
    Assert(s == 12);
    Assert(b->isEOF());

    Printer::log(M_STATUS, "Test::Handles passed.");
}
