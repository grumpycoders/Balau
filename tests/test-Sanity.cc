#include <Main.h>

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Sanity running.");

    Assert(sizeof(off_t) == 8);
    Assert(sizeof(size_t) == 4);
    Assert(sizeof(time_t) == 4);

    Printer::log(M_STATUS, "Test::Sanity passed.");
}
