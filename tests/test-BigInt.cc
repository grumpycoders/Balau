#include <Main.h>
#include <BigInt.h>

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::BigInt running.");

    {
        BigInt a, b;
        uint64_t t = 10000000000000000000;
        String s =  "10000000000000000000";
        a.set(t);
        b.set(s);
        TAssert(a == b);
        TAssert(a.toString() == s);
    }

    Printer::log(M_STATUS, "Test::BigInt passed.");
}
