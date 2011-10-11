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
    Printer::log(M_STATUS, "Opened file %s:", i->getName());
    Printer::log(M_STATUS, " - size = %lli", i->getSize());
    char mtimestr[32];
    time_t mtime = i->getMTime();
    ctime_r(&mtime, mtimestr);
    char * nl = strrchr(mtimestr, '\n');
    if (nl)
        *nl = 0;
    Printer::log(M_STATUS, " - mtime = %i (%s)", mtime, mtimestr);
    Printer::log(M_STATUS, "Test::Handles passed.");
}
