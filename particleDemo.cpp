#include <iostream>
#include "particle.hpp"

int main() {
    ArrayND<float, 3> pos = ArrayND<float, 3>(3), vel = ArrayND<float, 3>(1, 2, 3);
    std::cout << pos << ' ' << vel << std::endl << std::endl;

    Particle part = Particle(pos, vel);
    std::cout << part << std::endl << std::endl;

    std::cout << ArrayND<uint16_t, 2>(2.5, 7.9) << std::endl;

    std::cout << (vel / 2) << std::endl;

    return 0;
}