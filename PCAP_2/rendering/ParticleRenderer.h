#pragma once
#include <vector>
#include "../core/types.h"

class ParticleRenderer {
public:
    unsigned int VAO, VBO;

    void init();
    void draw(const std::vector<Particle>& particles);
};