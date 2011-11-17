#include <Main.h>
#include <BRegex.h>

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Regex running");

    Regex reg("http://([^/ ]+)/([^? ]+)(\\?([^ ]+))?");
    Regex::Captures c = reg.match("some url: http://www.test.com/uri?var1=val1 that should match");

    Assert(c[0] == "http://www.test.com/uri?var1=val1");
    Assert(c[1] == "www.test.com");
    Assert(c[2] == "uri");
    Assert(c[3] == "?var1=val1");
    Assert(c[4] == "var1=val1");

    Printer::log(M_STATUS, "Test::Regex passed");
}
