#ifndef PARTICLE_H
#define PARTICLE_H

#include "dataTypes.hpp"

class Particle {
    public:
    /* Constructors */
    Particle();
    Particle(const ArrayND<float, 3> pos, const ArrayND<float, 3> vel);

    /* toString() */
    friend std::ostream& operator<<(std::ostream& os, const Particle& p);

    private:
    ArrayND<float, 3> position, velocity;
};

Particle::Particle(): position(), velocity() {}
Particle::Particle(const ArrayND<float, 3> pos, const ArrayND<float, 3> vel): position(pos), velocity(vel) {}

std::ostream& operator<<(std::ostream& os, const Particle& p) {
    return os << "Pos: " << p.position << " | Vel: " << p.velocity;
}

#endif