#include <Main.h>
#include <BRegex.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Regex running");

    Regex reg("http://([^/ ]+)/([^? ]+)(\\?([^ ]+))?");
    Regex::Captures c = reg.match("some url: http://www.test.com/uri?var1=val1 that should match");

    TAssert(c[0] == "http://www.test.com/uri?var1=val1");
    TAssert(c[1] == "www.test.com");
    TAssert(c[2] == "uri");
    TAssert(c[3] == "?var1=val1");
    TAssert(c[4] == "var1=val1");

    Printer::log(M_STATUS, "Test::Regex passed");
}
