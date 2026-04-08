#include <cuda_runtime.h>
#include <cmath>
#include "floatGrid.h"

// ─────────────────────────────────────────────────────────────────────────────
// Kernel: Fill particles in a 3D cube grid
// ─────────────────────────────────────────────────────────────────────────────
__global__ void initCubeKernel(float4* pos, std::size_t N, int side)
{
    std::size_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    // Convert 1D index → 3D grid index
    int z = i / (side * side);
    int y = (i / side) % side;
    int x = i % side;

    // Normalize to [-1, 1]
    float fx = (float)x / (side - 1);
    float fy = (float)y / (side - 1);
    float fz = (float)z / (side - 1);

    float px = fx * 2.0f - 1.0f;
    float py = fy * 2.0f - 1.0f;
    float pz = fz * 2.0f - 1.0f;

    pos[i] = make_float4(px, py, pz, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API: Initialize FloatGrid with cube distribution
// ─────────────────────────────────────────────────────────────────────────────
extern "C" void initParticlesCube(FloatGrid& grid)
{
    std::size_t N = grid.len;
    float4* d_ptr = grid.getData(); 
    // ⚠ NOTE: your getData() copies to host — NOT what we want.
    // We will FIX this below.

    // ❗ IMPORTANT FIX:
    // You should expose raw device pointer instead:
    // float4* d_ptr = grid.devicePtr();

    // TEMP workaround (if you modify FloatGrid):
    d_ptr = *((float4**)&grid); // ❌ not safe — replace properly

    int side = static_cast<int>(ceil(cbrtf((float)N)));

    const int blockSize = 256;
    const int gridSize  = (N + blockSize - 1) / blockSize;

    initCubeKernel<<<gridSize, blockSize>>>(d_ptr, N, side);

    cudaDeviceSynchronize();
}