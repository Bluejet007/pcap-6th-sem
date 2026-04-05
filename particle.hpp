#include "dataTypes.hpp"

class Particle {
    public:
    /* Constructors */
    Particle();
    Particle(const Float3D pos, const Float3D vel);

    /* toString() */
    friend std::ostream& operator<<(std::ostream& os, const Particle& p);

    private:
    Float3D position, velocity;
};

Particle::Particle(): position(), velocity() {}
Particle::Particle(const Float3D pos, const Float3D vel): position(pos), velocity(vel) {}

std::ostream& operator<<(std::ostream& os, const Particle& p) {
    return os << "Pos: " << p.position << ", Vel: " << p.velocity;
}