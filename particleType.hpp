#ifndef PARTICLE_H
#define PARTICLE_H

#include <valarray>
#include <initializer_list>

class Particle {
    private:
    
    public:
    std::valarray<float> position, velocity;
    /* Constructors */
    Particle();
    Particle(const std::valarray<float> pos, const std::valarray<float> vel);
    Particle(const std::initializer_list<std::initializer_list<float>> pos, const std::initializer_list<std::initializer_list<float>> vel);

    /* toString() */
    friend std::ostream& operator<<(std::ostream& os, const Particle& p);
};

Particle::Particle(): position(), velocity() {}
Particle::Particle(const std::valarray<float> pos, const std::valarray<float> vel): position(pos), velocity(vel) {}
// Particle::Particle(const std::initializer_list<std::initializer_list<float>> pos, const std::initializer_list<std::initializer_list<float>> vel): position(pos), velocity(vel) {}

std::ostream& operator<<(std::ostream& os, const Particle& p) {
    os << "Pos: [ ";
    for(float val: p.position)
        os << val << ' ';
    
    os << "] | Vel: [ ";
    for(float val: p.velocity)
        os << val << ' ';
    
    return os << ']';
}

#endif