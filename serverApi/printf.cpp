#include <iostream>
#include <cstdio>
#include <cstdint>
#include <string>
#include <sstream>

int main()
{
    uint32_t num1 = 0x3fffffff;
    uint64_t num = 0xffffffffffffffff;
    std::printf("%c\n", num1);

    int val = 8888;
    //std::cout << std::to_string(val) << std::endl;

    std::ostringstream oss;
    oss << (val & 0xff);
    oss << ((val >> 8) & 0xff);
    oss << ((val >> 16) & 0xff);
    oss << ((val >> 24) & 0xff);
    std::cout << oss.str() << std::endl;

    uint8_t v = 35;
    char c = v & 0xff;
    std::cout << c  << std::endl;
    return 0;
}
