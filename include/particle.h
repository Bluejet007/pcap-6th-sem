#ifndef PARTICLE_H
#define PARTICLE_H

#define MAX_PARTICLES 1000
#define DOT_RADIUS 0.05f

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float radius;
    float r, g, b;
} Particle;

extern Particle particles[MAX_PARTICLES];
extern int particleCount;   // Must be defined in only 1 header file else linker error

void spawnParticle(float x, float y);

#endif