#include "Collision.h"
#include "../config/Parameters.h"

bool pointInTriangle(Vec3 p, Triangle tri) {
    Vec3 v0 = tri.v1 - tri.v0;
    Vec3 v1 = tri.v2 - tri.v0;
    Vec3 v2 = p - tri.v0;

    float d00 = dot(v0,v0);
    float d01 = dot(v0,v1);
    float d11 = dot(v1,v1);
    float d20 = dot(v2,v0);
    float d21 = dot(v2,v1);

    float denom = d00 * d11 - d01 * d01;

    float v = (d11*d20 - d01*d21)/denom;
    float w = (d00*d21 - d01*d20)/denom;
    float u = 1.0f - v - w;

    return (u>=0 && v>=0 && w>=0);
}