#ifndef FLUID_SIM_H
#define FLUID_SIM_H

#include <cuda_gl_interop.h>

extern "C" void cudaStepSimulation(
    float time,
    cudaGraphicsResource* posRes[2],
    cudaGraphicsResource* velRes[2],
    int& pingPong,
    uint N,
    float mouseDX, float mouseDY,
    float mouseX, float mouseY
);

#endif