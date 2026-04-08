// nvcc -std=c++17 -O2     fluid_renderer.cpp fluid_sim.cu     -o fluid_renderer     -lGL -lGLEW -lglut
// __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./fluid_renderer

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <cmath>
#include <iostream>
#include "helper_math.h"
#include "floatGrid.hu"

#define DT              0.01666667f
#define DAMPING         0.03f
#define MOUSE_RADIUS    0.35f
#define MOUSE_STRENGTH  0.5f
#define GRAVITY         5.18f
#define MAX_DIST        1.0f
#define REPULSE_FORCE   0.01f

__device__ float hash(uint32_t n)
{
    n = (n << 13u) ^ n;
    n = n * (n * n * 15731u + 0x789221u) + 0x13763125u;
    return float(n & 0x7fffffffu) / float(0x7fffffff);
}

__device__ float poly6(float r2, float h2) {
    float diff = h2 - r2;
    return (diff > 0.0f) ? diff * diff * diff : 0.0f;
}

__device__ float spikyGrad(float r, float h) {
    float diff = h - r;
    return (diff > 0.0f) ? diff * diff : 0.0f;
}

__device__ float viscosityLaplacian(float r, float h) {
    return (r < h) ? (h - r) : 0.0f;
}

__global__ void simulateKernel(
    float4* posIn,
    float4* posOut,
    float4* velIn,
    float4* velOut,
    uint N,
    float time,
    float mouseDX,
    float mouseDY,
    float mouseX,
    float mouseY
) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id >= N) return;

    float3 pos = make_float3(posIn[id]);
    float3 vel = make_float3(velIn[id]);

    const float h = 0.1f;             // smoothing radius
    const float h2 = h * h;
    const float restDensity = .02f;
    const float k = 0.5f;             // pressure stiffness
    const float viscosity = 0.2f;
    const float mass = 1.0f;

    float density = 0.0f;

    // -------------------------
    // 1. Density computation
    // -------------------------
    for (uint j = 0; j < N; j++) {
        float3 pj = make_float3(posIn[j]);
        float3 rij = pos - pj;
        float r2 = dot(rij, rij);

        if (r2 < h2) {
            density += mass * poly6(r2, h2);
        }
    }

    float pressure = k * (density - restDensity);

    float3 force = make_float3(0.0f);

    // -------------------------
    // 2. Pressure + viscosity
    // -------------------------
    for (uint j = 0; j < N; j++) {
        if (j == id) continue;

        float3 pj = make_float3(posIn[j]);
        float3 vj = make_float3(velIn[j]);

        float3 rij = pos - pj;
        float r = length(rij);

        if (r < h && r > 1e-5f) {

            // Neighbor density
            float density_j = 0.0f;
            float r2 = dot(rij, rij);
            if (r2 < h2) {
                density_j = mass * poly6(r2, h2);
            }

            float pressure_j = k * (density_j - restDensity);

            float3 dir = rij / r;

            // Pressure force
            float grad = spikyGrad(r, h);
            force += -dir * mass * (pressure + pressure_j) * grad / (2.0f * density_j + 1e-5f);

            // Viscosity
            float lap = viscosityLaplacian(r, h);
            force += viscosity * (vj - vel) * lap;
        }
    }

    // -------------------------
    // 3. External forces
    // -------------------------

    // Gravity
    force.y -= GRAVITY;

    // Mouse force (same as yours)
    float3 mouse = make_float3(mouseX, mouseY, 0.0f);
    float3 toMouse = mouse - pos;

    float dist2 = dot(toMouse, toMouse);
    float r2 = MOUSE_RADIUS * MOUSE_RADIUS;

    if (dist2 < r2) {
        float falloff = expf(-dist2 / (0.5f * r2));

        float speed = sqrtf(mouseDX*mouseDX + mouseDY*mouseDY);
        float3 dir = make_float3(mouseDX, mouseDY, 0.0f);

        if (speed > 0.0001f)
            dir /= speed;

        force += dir * speed * MOUSE_STRENGTH * falloff;
    }

    // Turbulence (retained)
    float n0 = hash(id * 3u  + (uint32_t)(time * 30.0f));
    float n1 = hash(id * 7u  + (uint32_t)(time * 30.0f) + 1u);
    float n2 = hash(id * 13u + (uint32_t)(time * 30.0f) + 2u);

    force += (make_float3(n0, n1, n2) - 0.5f) * 0.0002f;

    // -------------------------
    // 4. Integrate
    // -------------------------
    vel += force * DT;
    pos += vel * DT;

    // -------------------------
    // 5. Boundary conditions
    // -------------------------
    const float B = 1.0f;

    if (pos.x >  B) { pos.x =  2*B - pos.x; vel.x *= -DAMPING; }
    if (pos.x < -B) { pos.x = -2*B - pos.x; vel.x *= -DAMPING; }

    if (pos.y >  B) { pos.y =  2*B - pos.y; vel.y *= -DAMPING; }
    if (pos.y < -B) { pos.y = -2*B - pos.y; vel.y *= -DAMPING; }

    if (pos.z >  B) { pos.z =  2*B - pos.z; vel.z *= -DAMPING; }
    if (pos.z < -B) { pos.z = -2*B - pos.z; vel.z *= -DAMPING; }

    // mild damping
    vel *= 0.995f;

    float speed = length(vel);

    posOut[id] = make_float4(pos, speed);
    velOut[id] = make_float4(vel, 0.0f);
}

extern "C" void cudaStepSimulation(float time, cudaGraphicsResource* posRes[2], cudaGraphicsResource* velRes[2], int& pingPong, int N, float mouseDX, float mouseDY, float mouseX, float mouseY) {
    int read  = pingPong;
    int write = 1 - pingPong;

    cudaGraphicsMapResources(1, &posRes[read]);
    cudaGraphicsMapResources(1, &posRes[write]);
    cudaGraphicsMapResources(1, &velRes[read]);
    cudaGraphicsMapResources(1, &velRes[write]);

    float4 *posIn, *posOut, *velIn, *velOut;
    size_t size;

    cudaGraphicsResourceGetMappedPointer((void**)&posIn,  &size, posRes[read]);
    cudaGraphicsResourceGetMappedPointer((void**)&posOut, &size, posRes[write]);
    cudaGraphicsResourceGetMappedPointer((void**)&velIn,  &size, velRes[read]);
    cudaGraphicsResourceGetMappedPointer((void**)&velOut, &size, velRes[write]);

    int blockSize = 256;
    int gridSize  = (N + blockSize - 1) / blockSize;

    simulateKernel<<<gridSize, blockSize>>>(
        posIn,
        posOut,
        velIn,
        velOut,
        N, time,
        mouseDX, mouseDY,
        mouseX, mouseY
    );

    cudaDeviceSynchronize();

    cudaGraphicsUnmapResources(1, &posRes[read]);
    cudaGraphicsUnmapResources(1, &posRes[write]);
    cudaGraphicsUnmapResources(1, &velRes[read]);
    cudaGraphicsUnmapResources(1, &velRes[write]);

    // pingPong = write;
}