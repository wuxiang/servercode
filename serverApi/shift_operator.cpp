#include <iostream>
#include <cstdint>

int main()
{
    uint64_t result = 0;
    result = result | (1 & 0xff);
    std::cout << result << std::endl;

    result = result | (1 << 8) & 0xff00;
    std::cout << result << std::endl;

    result = result | (1 << 16) & 0xff0000;
    std::cout << result << std::endl;

    std::cout << "===========================" << std::endl;
    for(int i = 0; i < 7; ++i)
    {
        result = 1;
        std::cout << ((result << (i * 8) ) & ((uint64_t)(0xff) << (i * 8) )) << std::endl;
        //std::cout << ((result << (i * 8) ) & (0xff << (i * 8) )) << std::endl;
    }

    std::cout << (result << 0) << std::endl;
    return 0;
}
