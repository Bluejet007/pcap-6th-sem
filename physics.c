#include "physics.h"
#include "particle.h"
#include <GL/glut.h>
#include <math.h>

void handleCollisions(){    // NEEDS to be Optimized and CUDAed!!!
    for(int i=0; i<particleCount; i++){
        for(int j=i+1; j<particleCount; j++){
            Particle *p1 = &particles[i];
            Particle *p2 = &particles[j];

            float dx = p2->x - p1->x;
            float dy = p2->y - p1->y;
            float dz = p2->z - p1->z;

            float dist = sqrt(dx*dx + dy*dy + dz*dz);
            float minDist = p1->radius + p2->radius;    // Can be times 2 but we'll keep it raw for now

            if(dist < minDist && dist > 0.0f){
                // Normal vector
                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;

                // Postion Correction, prevents sticking possible as our if is dist < minDist and not <=
                float overlap = minDist - dist;

                p1->x -= nx * overlap * 0.5f;
                p1->y -= ny * overlap * 0.5f;
                p1->z -= nz * overlap * 0.5f;

                p2->x += nx * overlap * 0.5f;
                p2->y += ny * overlap * 0.5f;
                p2->z += nz * overlap * 0.5f;

                // Relative velocity
                float rvx = p2->vx - p1->vx;
                float rvy = p2->vy - p1->vy;
                float rvz = p2->vz - p1->vz;

                // Velocity along normal
                float velAlongNormal = rvx*nx + rvy*ny + rvz*nz;
                if(velAlongNormal > 0)continue; // That is they're seperating

                // Elasticity
                float e = 1.0f; // 1 = perfect bounce, <1 = energy loss

                // Impluse
                float jImpulse = -(1 + e) * velAlongNormal;
                jImpulse /= 2.0f;   // Both particles have EQUAL mass for now

                // Applying Impluse
                float impulseX = jImpulse * nx;
                float impulseY = jImpulse * ny;
                float impulseZ = jImpulse * nz;

                p1->vx -= impulseX;
                p1->vy -= impulseY;
                p1->vz -= impulseZ;

                p2->vx += impulseX;
                p2->vy += impulseY;
                p2->vz += impulseZ;
            }
        }
    }
}

void update(){
    float radius = DOT_RADIUS;

    for(int i=0; i<particleCount; i++){
        Particle *p = &particles[i];
        p->vy += -0.0008f;   // Gravity

        p->x += p->vx;
        p->y += p->vy;
        p->z += p->vz;

        if(p->y - radius < -0.8f){
            p->y = -0.8f + radius;
            p->vy *= -1.0f;     // We flip the direction and no speed is lost
        }
        if(p->y + radius > 0.8f){
            p->y = 0.8f - radius;
            p->vy *= -1.0f;     
        }
        if(p->x - radius < -0.8f){
            p->x = -0.8f + radius;
            p->vx *= -1.0f;     
        }
        if(p->x + radius > 0.8f){
            p->x = 0.8f - radius;
            p->vx *= -1.0f;     
        }
        if(p->z + DOT_RADIUS > 0.8f){
            p->z = 0.8f - DOT_RADIUS;
            p->vz *= -1.0f;
        }
        if(p->z - DOT_RADIUS < -0.8f){
            p->z = -0.8f + DOT_RADIUS;
            p->vz *= -1.0f;
        }
        handleCollisions();
    }
    glutPostRedisplay();
}

void timer(int value){
    update();
    glutTimerFunc(16, timer, 0);
}