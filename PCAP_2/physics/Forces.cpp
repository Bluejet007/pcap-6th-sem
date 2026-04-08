#include "Forces.h"
#include "Collision.h"
#include "../config/Parameters.h"   // <-- ADD THIS
#include <glm/glm.hpp>

Vec3 computeMomentumForce(Particle &p, const Triangle &tri) {
    float dist = dot(p.pos - tri.v0, tri.normal);

    if (fabs(dist) < 0.01f) {
        Vec3 proj = p.pos - dist * tri.normal;

        if (pointInTriangle(proj, tri)) {

            Vec3 vin = p.vel;
            p.vel = p.vel - 2.0f * dot(p.vel, tri.normal) * tri.normal;
            Vec3 vout = p.vel;

            return (Params::PARTICLE_MASS * (vin - vout)) / Params::DT;
        }
    }
    return Vec3(0);
}

// ---------------- PRESSURE ----------------
Vec3 computePressureForce(Particle &p, const Triangle &tri) {
    float dist = dot(p.pos - tri.v0, tri.normal);

    if (dist > 0 && dist < 0.05f) {

        float area = length(cross(tri.v1 - tri.v0, tri.v2 - tri.v0)) * 0.5f;

        return -p.pressure * tri.normal * (area * 0.1f); 
        // scaled for stability
    }
    return Vec3(0);
}

// ---------------- SHEAR ----------------
Vec3 computeShearForce(Particle &p, const Triangle &tri) {
    float dist = dot(p.pos - tri.v0, tri.normal);

    if (dist > 0 && dist < 0.05f) {

        Vec3 vt = p.vel - dot(p.vel, tri.normal) * tri.normal;

        float shear = Params::VISCOSITY * length(vt) / (dist + 1e-5f);

        return -normalize(vt) * shear * 0.01f; // damped
    }
    return Vec3(0);
}