#include <iostream>
#include "particleType.hpp"

int main() {
    std::valarray<float> p1 = std::valarray<float>({1, 2, 3}), v1 = std::valarray<float>({3, 3, 3}), p2 = std::valarray<float>({0, 2, 4}), v2 = std::valarray<float>({1, 2, -1});
    Particle part1 = Particle(p1, v1),
    part2 = Particle(p2, v2);

    std::cout << part1 << std::endl << std::endl;
    std::cout << part2 << std::endl << std::endl;
    std::cout << Particle(part1.position + part2.position, part1.velocity * part2.velocity * 10) << std::endl << std::endl;

    return 0;
}