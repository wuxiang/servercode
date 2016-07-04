#include "process_manager.h"
#include "../util.h"

int
AbortG(int pgid) {
    pgid = 0 - abs(pgid);
    return kill(pgid, SIGABRT);
}

int
Abort(int pid) {
    return kill(pid, SIGABRT);
}

