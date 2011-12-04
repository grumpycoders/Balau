#include <BString.h>
#include <Main.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::String running.");

    String x = "foobar";
    TAssert(x == "foobar");
    TAssert(x != "barfoo");

    String y = "xyz";

    x = "abcdef";        TAssert(x < y); TAssert(x + y == "abcdefxyz");
    x.set("x:%i", 42);   TAssert(x == "x:42");

    x = "foobar";        TAssert(x == "foobar");

    y = x.extract(3);    TAssert(y == "bar");
    y = x.extract(1, 3); TAssert(y == "oob");

    y = " foo bar ";
    x = y; x.do_ltrim(); TAssert(x == "foo bar ");
    x = y; x.do_rtrim(); TAssert(x == " foo bar");
    x = y; x.do_trim();  TAssert(x == "foo bar");

    y = "        ";
    x = y; x.do_ltrim(); TAssert(x == "");
    x = y; x.do_rtrim(); TAssert(x == "");
    x = y; x.do_trim();  TAssert(x == "");

    x = "42";            TAssert(x.to_int() == 42);
    x = "0x42";          TAssert(x.to_int() == 0x42);
    x = "42";            TAssert(x.to_int(16) == 0x42);
    x = "4.2";           TAssert(x.to_double() == 4.2);

    x = "foobar";
    TAssert(x[0] == 'f');
    TAssert(x[5] == 'r');
    TAssert(x.strlen() == 6);
    TAssert(x.strchr('o') == 1);
    TAssert(x.strrchr('o') == 2);
    TAssert(x.strchrcnt('o') == 2);
    TAssert(x.strstr("bar") == 3);

    x = "\xc3\xa9";
    y = x.iconv("UTF-8", "Latin1");
    TAssert(((unsigned char) y[0]) == 0xe9);

    Printer::log(M_STATUS, "Test::String passed.");
}
