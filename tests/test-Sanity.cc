#include <Main.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Sanity running.");

    TAssert(sizeof(off_t) == 8);
    TAssert(sizeof(size_t) == 4);
    TAssert(sizeof(time_t) == 4);

    Printer::log(M_STATUS, "Test::Sanity passed.");
}
