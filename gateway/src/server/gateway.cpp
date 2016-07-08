#include <stdio.h>
#include <iostream>

#include "CallGlobal.h"

int main(int argc, char* argv[]) {
    fprintf(stderr, "main\n");
    //std::cout << typeid(1.1f).name() << std::endl;
    //std::cout << typeid(CallGlobal<int>).name() << std::endl;
    CallGlobal  one;
    return 0;
}
