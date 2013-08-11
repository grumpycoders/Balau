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

    {
        BigInt a, b, m, r, c;

        // random values; c is computed by wolfram alpha.
        a.set("23475098273405987234905872394051923847902348567");
        b.set("3234057230495872034923458723049857203948572039485734598276345987");
        m.set("534287528093746598127364987236459872634587");

        c.set("211833264233843026809053124694679687014311");

        r = a.modpow(b, m);

        TAssert(r == c);
    }

    Printer::log(M_STATUS, "Test::BigInt passed.");
}
