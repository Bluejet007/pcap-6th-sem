// nvcc -std=c++17 -O2 fluid_renderer.cpp fluid_sim.cu -o fluid_renderer -lGL -lGLEW -lglut
// __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./fluid_renderer

#include <thrust/sort.h>
#include <thrust/execution_policy.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <cmath>
#include <iostream>
#include "helper_math.h"

#define GRID_SIZE 65536
#define CELL_SIZE 0.05f

#define H              0.1f
#define H2             (H * H)

#define MASS           1.0f
#define REST_DENSITY   0.5f
#define K              .04f
#define VISC           0.2f
#define DT              0.01f
#define MOUSE_RADIUS    0.75f
#define MOUSE_STRENGTH  200.0f
#define GRAVITY         3.18f
#define MAX_DIST        1.0f
#define REPULSE_FORCE   0.01f

bool firstStep = true;
uint* d_particleHash;
uint* d_particleIndex;
uint* d_cellStart;
uint* d_cellEnd;
float* d_density;

__device__ int3 calcGridPos(float3 p) {
    return make_int3(
        floorf(p.x / CELL_SIZE),
        floorf(p.y / CELL_SIZE),
        floorf(p.z / CELL_SIZE)
    );
}

__device__ uint calcGridHash(int3 gridPos) {
    const uint p1 = 73856093;
    const uint p2 = 19349663;
    const uint p3 = 83492791;

    return ((gridPos.x * p1) ^ (gridPos.y * p2) ^ (gridPos.z * p3)) % GRID_SIZE;
}

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

__global__ void computeHash(
    uint* particleHash,
    uint* particleIndex,
    float4* pos,
    uint N
) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id >= N) return;

    float3 p = make_float3(pos[id]);
    int3 gridPos = calcGridPos(p);
    uint hash = calcGridHash(gridPos);

    particleHash[id] = hash;
    particleIndex[id] = id;
}

__global__ void buildGrid(
    uint* cellStart,
    uint* cellEnd,
    uint* particleHash,
    uint* particleIndex,
    uint N
) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;

    if (id >= N) return;

    uint hash = particleHash[id];

    if (id == 0) {
        cellStart[hash] = id;
    } else {
        uint prevHash = particleHash[id - 1];
        if (hash != prevHash) {
            cellStart[hash] = id;
            cellEnd[prevHash] = id;
        }
    }

    if (id == N - 1) {
        cellEnd[hash] = N;
    }
}

__global__ void computeDensity(
    float* density,
    float4* pos,
    uint* particleIndex,
    uint* cellStart,
    uint* cellEnd,
    uint N
) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id >= N) return;

    uint i = particleIndex[id];
    float3 pi = make_float3(pos[i]);

    float d = 0.0f;

    int3 gridPos = calcGridPos(pi);

    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {

        int3 neighborPos = gridPos + make_int3(x,y,z);
        uint hash = calcGridHash(neighborPos);

        uint start = cellStart[hash];
        uint end   = cellEnd[hash];

        for (uint j = start; j < end; j++) {
            uint pjIndex = particleIndex[j];
            float3 pj = make_float3(pos[pjIndex]);

            float3 rij = pi - pj;
            float r2 = dot(rij, rij);

            if (r2 < H2) {
                d += MASS * poly6(r2, H2);
            }
        }
    }

    density[i] = d;
}

__global__ void simulateKernel(
    float4* posIn,
    float4* posOut,
    float4* velIn,
    float4* velOut,
    float* density,
    uint* particleIndex,
    uint* cellStart,
    uint* cellEnd,
    uint N,
    float time,
    float mouseDX,
    float mouseDY,
    float mouseX,
    float mouseY
) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id >= N) return;

    uint i = particleIndex[id];

    float3 pi = make_float3(posIn[i]);
    float3 vi = make_float3(velIn[i]);

    float rho_i = density[i];
    float pressure_i = K * (rho_i - REST_DENSITY);

    float3 force = make_float3(0,0,0);

    int3 gridPos = calcGridPos(pi);

    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {

        int3 neighborPos = gridPos + make_int3(x,y,z);
        uint hash = calcGridHash(neighborPos);

        uint start = cellStart[hash];
        uint end   = cellEnd[hash];

        for (uint j = start; j < end; j++) {
            uint pjIndex = particleIndex[j];
            if (pjIndex == i) continue;

            float3 pj = make_float3(posIn[pjIndex]);
            float3 vj = make_float3(velIn[pjIndex]);

            float3 rij = pi - pj;
            float r = length(rij);

            if (r < H && r > 1e-5f) {

                float rho_j = density[pjIndex];
                float pressure_j = K * (rho_j - REST_DENSITY);

                float3 dir = rij / r;

                // Pressure
                float grad = spikyGrad(r, H);
                force += -dir * MASS *
                    (pressure_i + pressure_j) /
                    (2.0f * rho_j + 1e-5f) * grad;

                // Viscosity
                float lap = viscosityLaplacian(r, H);
                force += VISC * (vj - vi) * lap;
            }
        }
    }

    // Gravity
    force.y -= GRAVITY;

    // Mouse force
    float3 mouse = make_float3(mouseX, mouseY, 0);
    float3 toMouse = mouse - pi;

    float dist2 = dot(toMouse, toMouse);
    if (dist2 < MOUSE_RADIUS * MOUSE_RADIUS) {
        float falloff = expf(-dist2 / (0.5f * MOUSE_RADIUS * MOUSE_RADIUS));

        float speed = sqrtf(mouseDX*mouseDX + mouseDY*mouseDY);
        float3 dir = make_float3(mouseDX, mouseDY, 0);

        if (speed > 1e-5f) dir /= speed;

        force += dir * speed * MOUSE_STRENGTH * falloff;
    }

    // Turbulence (kept)
    float n0 = hash(i * 3u + (uint32_t)(time * 30));
    float n1 = hash(i * 7u + (uint32_t)(time * 30) + 1);
    float n2 = hash(i * 13u + (uint32_t)(time * 30) + 2);

    force += (make_float3(n0,n1,n2) - 0.5f) * 0.0002f;

    // Integrate
    vi += force * DT;
    pi += vi * DT;

    // Bounds
    const float B = 1.0f;

    if (pi.x > B) { pi.x = B; vi.x *= -0.5f; }
    if (pi.x < -B){ pi.x = -B; vi.x *= -0.5f; }

    if (pi.y > B) { pi.y = B; vi.y *= -0.5f; }
    if (pi.y < -B){ pi.y = -B; vi.y *= -0.5f; }

    if (pi.z > B) { pi.z = B; vi.z *= -0.5f; }
    if (pi.z < -B){ pi.z = -B; vi.z *= -0.5f; }

    posOut[i] = make_float4(pi, 0);
    velOut[i] = make_float4(vi, 0);
}

extern "C" void cudaStepSimulation(float time, cudaGraphicsResource* posRes[2], cudaGraphicsResource* velRes[2], int& pingPong, int N, float mouseDX, float mouseDY, float mouseX, float mouseY) {
    if(firstStep) {
        cudaMalloc(&d_particleHash,  N * sizeof(uint));
        cudaMalloc(&d_particleIndex, N * sizeof(uint));
        cudaMalloc(&d_density,       N * sizeof(float));

        cudaMalloc(&d_cellStart, GRID_SIZE * sizeof(uint));
        cudaMalloc(&d_cellEnd,   GRID_SIZE * sizeof(uint));

        firstStep = false;
    }

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

    // -----------------------------
    // 1. Compute hash + index
    // -----------------------------
    computeHash<<<gridSize, blockSize>>>(
        d_particleHash,
        d_particleIndex,
        posIn,
        N
    );

    // -----------------------------
    // 2. Sort by hash
    // -----------------------------
    thrust::sort_by_key(
        thrust::device,
        d_particleHash,
        d_particleHash + N,
        d_particleIndex
    );

    // -----------------------------
    // 3. Reset grid
    // -----------------------------
    cudaMemset(d_cellStart, 0xffffffff, GRID_SIZE * sizeof(uint));
    cudaMemset(d_cellEnd,   0,          GRID_SIZE * sizeof(uint));

    // -----------------------------
    // 4. Build grid
    // -----------------------------
    buildGrid<<<gridSize, blockSize>>>(
        d_cellStart,
        d_cellEnd,
        d_particleHash,
        d_particleIndex,
        N
    );

    // -----------------------------
    // 5. Density pass (PASS 1)
    // -----------------------------
    computeDensity<<<gridSize, blockSize>>>(
        d_density,
        posIn,
        d_particleIndex,
        d_cellStart,
        d_cellEnd,
        N
    );

    // -----------------------------
    // 6. Force + integrate (PASS 2)
    // -----------------------------
    simulateKernel<<<gridSize, blockSize>>>(
        posIn,
        posOut,
        velIn,
        velOut,
        d_density,
        d_particleIndex,
        d_cellStart,
        d_cellEnd,
        N,
        time,
        mouseDX,
        mouseDY,
        mouseX,
        mouseY
    );

    cudaDeviceSynchronize();

    cudaGraphicsUnmapResources(1, &posRes[read]);
    cudaGraphicsUnmapResources(1, &posRes[write]);
    cudaGraphicsUnmapResources(1, &velRes[read]);
    cudaGraphicsUnmapResources(1, &velRes[write]);

    // pingPong = write;
}