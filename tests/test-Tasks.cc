#include <Main.h>
#include <Task.h>
#include <TaskMan.h>

BALAU_STARTUP;

using namespace Balau;

int Application::startup() throw (Balau::GeneralException) {
    Printer::log(M_STATUS, "Test::Tasks running.");
    Printer::log(M_STATUS, "Test::Tasks passed.");
    return 0;
}
