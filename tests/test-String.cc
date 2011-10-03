#include <BString.h>
#include <Main.h>

BALAU_STARTUP;

using namespace Balau;

int Application::startup() throw (Balau::GeneralException) {
    Printer::log(M_STATUS, "Test::String running.");

    String x = "foobar";
    Assert(x == "foobar");
    Assert(x != "barfoo");

    String y = "xyz";

    x = "abcdef";        Assert(x < y); Assert(x + y == "abcdefxyz");
    x.set("x:%i", 42);   Assert(x == "x:42");

    x = "foobar";        Assert(x == "foobar");

    y = x.extract(3);    Assert(y == "bar");
    y = x.extract(1, 3); Assert(y == "oob");

    y = " foo bar ";
    x = y; x.do_ltrim(); Assert(x == "foo bar ");
    x = y; x.do_rtrim(); Assert(x == " foo bar");
    x = y; x.do_trim();  Assert(x == "foo bar");

    x = "42";            Assert(x.to_int() == 42);
    x = "0x42";          Assert(x.to_int() == 0x42);
    x = "42";            Assert(x.to_int(16) == 0x42);
    x = "4.2";           Assert(x.to_double() == 4.2);

    x = "foobar";
    Assert(x[0] == 'f');
    Assert(x[5] == 'r');
    Assert(x.strlen() == 6);
    Assert(x.strchr('o') == 1);
    Assert(x.strrchr('o') == 2);
    Assert(x.strchrcnt('o') == 2);
    Assert(x.strstr("bar") == 3);

    x = "\xc3\xa9";
    y = x.iconv("UTF-8", "Latin1");
    Assert(((unsigned char) y[0]) == 0xe9);

    Printer::log(M_STATUS, "Test::String passed.");
    return 0;
}
