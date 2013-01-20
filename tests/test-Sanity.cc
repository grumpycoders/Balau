#include <Main.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Sanity running.");

    TAssert(sizeof(off_t) == 8);
#ifdef __x86_64__
    TAssert(sizeof(size_t) == 8);
    TAssert(sizeof(time_t) == 8);
#else
    TAssert(sizeof(size_t) == 4);
    TAssert(sizeof(time_t) == 4);
#endif

    Printer::log(M_STATUS, "Test::Sanity passed.");
}
