#include "myUtil.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

int MyUtil::set_non_blocking(int& sfd) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (-1 == flags) {
        fprintf(stderr, "[%s: %d] get file non blocking failed", __FILE__, __LINE__);
    }

    flags = flags | O_NONBLOCK;
    int res = fcntl(sfd, F_SETFL, flags);
    if (-1 == res) {
        fprintf(stderr, "[%s: %d] set file non blocking failed", __FILE__, __LINE__);
    }

    return res;
}

std::string  MyUtil::makeUniqueID() {
    boost::uuids::random_generator rgen;
    boost::uuids::uuid u = rgen();
    std::stringstream ss;
    ss << u;
    std::string id = ss.str();
    id.erase(8, 1);
    id.erase(12, 1);
    id.erase(16, 1);
    id.erase(20, 1);
    return id;
}
