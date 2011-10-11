#include <Main.h>
#include <Input.h>

BALAU_STARTUP;

using namespace Balau;

void MainTask::Do() {
    Printer::log(M_STATUS, "Test::Handles running.");

    bool failed = false;
    try {
        IO i(new Input("SomeInexistantFile.txt"));
    }
    catch (GeneralException) {
        failed = true;
    }
    Assert(failed);
    IO i(new Input("Makefile"));

    Printer::log(M_STATUS, "Test::Handles passed.");
}
