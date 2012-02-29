#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <sstream>

/**
 * *number is not char
 * */
template<typename _Tp>
inline std::string to_string(const _Tp& x)
{
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

int main()
{
    uint64_t  num64 = 0x1234567887654321;
    uint32_t  num32 = 0x12345678;

    uint32_t  result32 = (uint32_t)(num64);
    uint64_t  result64 = (uint64_t)(num32);

    std::cout << "num64: " << num64 << std::endl;
    std::cout << "string type: " << to_string(num64) << std::endl;
    std::cout << "from uint64_t to uint32_t: " << result32 << std::endl;
    std::cout << "from uint32_t to uint64_t: " << result64 << std::endl;
    std::cout << "num32: " << num32 << std::endl;

    return 0;
}
