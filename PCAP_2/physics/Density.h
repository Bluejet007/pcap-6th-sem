#pragma once
#include "../core/types.h"
#include "SpatialGrid.h"

float computeDensity(Particle &pi,
                     std::vector<Particle>& particles,
                     SpatialGrid& grid);