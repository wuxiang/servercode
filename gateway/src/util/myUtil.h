#ifndef MYUTIL_H_
#define MYUTIL_H_
#include <stdio.h>
#include <fcntl.h>

#include <string>

class MyUtil {
public:
    static int set_non_blocking(int& sfd);
    static std::string  makeUniqueID();
};

#endif //MYUTIL_H_

