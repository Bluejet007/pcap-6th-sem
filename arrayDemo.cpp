#include <iostream>
#include "arrayType.hpp"

int main() {
    ArrayND<float, 3> vals[] = {
        ArrayND<float, 3>(2),
        ArrayND<float, 3>(5),
        ArrayND<float, 3>(7)
    };

    vals[0][1] = 3;
    vals[0][2] = 4;
    vals[1][1] = 1;
    vals[1][2] = 0;

    for(auto v: vals)
        std::cout << v << ' ';
    std::cout << std::endl << std::endl;

    float *data = vals[0].values.data();
    for(uint8_t i = 0; i < 9; i++)
        std::cout << *(data + i) << ' ';
    std::cout << std::endl;

    return 0;
}