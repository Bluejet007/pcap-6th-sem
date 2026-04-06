#include <iostream>
#include "particleType.hpp"

int main() {
    Particle part1 = Particle({1, 2, 3}, {3, 3, 3}),
    part2 = Particle({0, 2, 4}, {1, 2, -1});
    
    std::cout << "P0- " << Particle() << std::endl << std::endl;
    std::cout << "P1- " << part1 << std::endl << std::endl;
    std::cout << "P2- " << part2 << std::endl << std::endl;
    
    Particle part3 = part1 + part2;
    part3.vel *= 10;
    std::cout << "P3- " << part3 << std::endl;

    return 0;
}