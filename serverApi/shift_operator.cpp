#include <iostream>
#include <cstdint>

int main()
{
    uint32_t result = 0;
    result = result | (1 & 0xff);
    std::cout << result << std::endl;

    result = result | (1 << 8) & 0xff00;
    std::cout << result << std::endl;

    result = result | (1 << 16) & 0xff0000;
    std::cout << result << std::endl;
    return 0;
}
