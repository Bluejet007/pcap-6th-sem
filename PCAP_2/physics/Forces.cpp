#include "Forces.h"
#include "Collision.h"
#include "../config/Parameters.h"
#include <glm/glm.hpp>
#include <cmath>

// ---------------- MOMENTUM ----------------
Vec3 computeMomentumForce(Particle &p, const Triangle &tri) {

    float distCenter = glm::length(p.pos - tri.center);
    if (distCenter > 0.2f) return Vec3(0.0f);

    float dist = glm::dot(p.pos - tri.v0, tri.normal);
    if (fabs(dist) > 0.05f) return Vec3(0.0f);

    if (fabs(dist) < 0.01f) {
        Vec3 proj = p.pos - dist * tri.normal;

        if (pointInTriangle(proj, tri)) {

            Vec3 vin = p.vel;
            p.vel = p.vel - 2.0f * glm::dot(p.vel, tri.normal) * tri.normal;
            Vec3 vout = p.vel;

            return (Params::PARTICLE_MASS * (vin - vout)) / Params::DT;
        }
    }
    return Vec3(0.0f);
}

// ---------------- PRESSURE ----------------
Vec3 computePressureForce(Particle &p, const Triangle &tri) {

    float distCenter = glm::length(p.pos - tri.center);
    if (distCenter > 0.2f) return Vec3(0.0f);

    float dist = glm::dot(p.pos - tri.v0, tri.normal);
    if (dist > 0 && dist < 0.05f) {

        float area = glm::length(glm::cross(tri.v1 - tri.v0, tri.v2 - tri.v0)) * 0.5f;

        return -p.pressure * tri.normal * (area * 0.1f);
    }
    return Vec3(0.0f);
}

// ---------------- SHEAR ----------------
Vec3 computeShearForce(Particle &p, const Triangle &tri) {

    float distCenter = glm::length(p.pos - tri.center);
    if (distCenter > 0.2f) return Vec3(0.0f);

    float dist = glm::dot(p.pos - tri.v0, tri.normal);
    if (dist > 0 && dist < 0.05f) {

        Vec3 vt = p.vel - glm::dot(p.vel, tri.normal) * tri.normal;

        float shear = Params::VISCOSITY * glm::length(vt) / (dist + 1e-5f);

        return -glm::normalize(vt) * shear * 0.01f;
    }
    return Vec3(0.0f);
}