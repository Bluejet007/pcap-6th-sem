#ifndef PARTICLE_H
#define PARTICLE_H

#include "arrayType.hpp"

class Particle {
    public:
    /* Variables */
    ArrayND<float, 3> pos, vel;

    /* Constructors */
    explicit Particle(const float fillValue = 0);
    Particle(const ArrayND<float, 3> position, const ArrayND<float, 3> velocity);
    Particle(const std::initializer_list<float> position, const std::initializer_list<float> velocity);

    /* Arthmetic Operators */
    inline Particle operator+(const Particle& otherP) const;
    inline Particle operator-(const Particle& otherP) const;
    inline Particle operator/(const Particle& otherP) const;
    inline Particle operator*(const Particle& otherP) const;

    /* Assignment Operators */
    Particle& operator=(const Particle& otherP);
    Particle& operator+=(const Particle& otherP);
    Particle& operator-=(const Particle& otherP);
    Particle& operator*=(const Particle& otherP);
    Particle& operator/=(const Particle& otherP);

    /* toString() */
    friend std::ostream& operator<<(std::ostream& os, const Particle& p);
};


/* Constructors */
Particle::Particle(const std::size_t dimensions): pos(dimensions), vel(dimensions) {}
Particle::Particle(const ArrayND<float, 3> position, const ArrayND<float, 3> velocity): pos(position), vel(velocity) {}
Particle::Particle(const std::initializer_list<float> position, const std::initializer_list<float> velocity): pos(position), vel(velocity) {}


/* Arthmetic Operators */
inline Particle Particle::operator+(const Particle& otherP) const {
    return Particle(pos + otherP.pos, vel + otherP.vel);
}

inline Particle Particle::operator-(const Particle& otherP) const {
    return Particle(pos - otherP.pos, vel - otherP.vel);
}

inline Particle Particle::operator*(const Particle& otherP) const {
    return Particle(pos * otherP.pos, vel * otherP.vel);
}

inline Particle Particle::operator/(const Particle& otherP) const {
    return Particle(pos / otherP.pos, vel / otherP.vel);
}


/* Assignment Operators */
Particle& Particle::operator=(const Particle& otherP) {
    if(this != &otherP) {
        pos = otherP.pos;
        vel = otherP.vel;
    }

    return *this;
}

Particle& Particle::operator+=(const Particle& otherP) {
    pos += otherP.pos;
    vel += otherP.vel;

    return *this;
}

Particle& Particle::operator-=(const Particle& otherP) {
    pos -= otherP.pos;
    vel -= otherP.vel;

    return *this;
}

Particle& Particle::operator*=(const Particle& otherP) {
    pos *= otherP.pos;
    vel *= otherP.vel;

    return *this;
}

Particle& Particle::operator/=(const Particle& otherP) {
    pos /= otherP.pos;
    vel /= otherP.vel;

    return *this;
}


/* toString() */
std::ostream& operator<<(std::ostream& os, const Particle& p) {
    os << "Pos: [ ";
    for(float val: p.pos)
        os << val << ' ';
    
    os << "] | Vel: [ ";
    for(float val: p.vel)
        os << val << ' ';
    
    return os << ']';
}

#endif