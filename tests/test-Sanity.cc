#include <Main.h>

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Sanity running.");

    Assert(sizeof(off_t) == 8);

    Printer::log(M_STATUS, "Test::Sanity passed.");
}
