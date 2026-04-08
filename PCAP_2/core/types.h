#pragma once
#include <glm/glm.hpp>

using Vec3 = glm::vec3;

struct Particle {
    Vec3 pos;
    Vec3 vel;
    float density;
    float pressure;
};

struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 normal;
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};