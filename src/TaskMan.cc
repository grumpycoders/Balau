#include "TaskMan.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::TaskMan> defaultTaskMan(50);
static Balau::LocalTmpl<Balau::TaskMan> localTaskMan;

Balau::TaskMan::TaskMan() {
    coro_create(&returnContext, 0, 0, 0, 0);
    if (!localTaskMan.getGlobal())
        localTaskMan.setGlobal(this);
}

Balau::TaskMan * Balau::TaskMan::getTaskMan() { return localTaskMan.get(); }

Balau::TaskMan::~TaskMan() {
    Assert(localTaskMan.getGlobal() != this);
}

void Balau::TaskMan::mainLoop() {
}
