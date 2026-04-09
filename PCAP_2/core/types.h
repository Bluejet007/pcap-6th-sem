#pragma once
#include <glm/glm.hpp>

using Vec3 = glm::vec3;

struct Particle {
    Vec3 pos;
    Vec3 vel;
    float density;
    float pressure;
    Vec3 initialPos;
};

struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 normal;
    Vec3 center;
    Vec3 force;  
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};