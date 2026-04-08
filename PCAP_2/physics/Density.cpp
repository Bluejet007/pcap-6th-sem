#include "Density.h"
#include "../config/Parameters.h"

float computeDensity(Particle &pi,
                     std::vector<Particle>& particles,
                     SpatialGrid& grid) {

    float density = 0.0f;

    auto neighbors = grid.getNeighbors(pi.pos);

    for (int j : neighbors) {
        Particle &pj = particles[j];

        float r = glm::length(pi.pos - pj.pos);

        if (r < Params::SMOOTHING_RADIUS) {
            float q = (Params::SMOOTHING_RADIUS*Params::SMOOTHING_RADIUS - r*r);
            density += Params::PARTICLE_MASS * q*q*q;
        }
    }

    return density;
}