#include "particle.h"
#include <stdlib.h>

int particleCount = 0;
Particle particles[MAX_PARTICLES];

void spawnParticle(float x, float y){
    if(particleCount >= MAX_PARTICLES)return;
    Particle p;
    p.x = x;
    p.y = y;
    p.z = ((rand() % 160) - 80) / 100.0f; // [-0.8, 0.8]
    p.vx = ((rand() % 200) - 100) / 5000.0f;
    p.vy = ((rand() % 200) - 100) / 5000.0f;
    p.vz = ((rand() % 200) - 100) / 5000.0f;
    p.radius = DOT_RADIUS;

    p.r = (rand() % 100) / 100.0f;
    p.g = (rand() % 100) / 100.0f;
    p.b = (rand() % 100) / 100.0f;
    particles[particleCount++] = p;
}
