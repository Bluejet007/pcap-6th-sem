#pragma once
#include "../core/types.h"

Vec3 computeMomentumForce(Particle&, const Triangle&);
Vec3 computePressureForce(Particle&, const Triangle&);
Vec3 computeShearForce(Particle&, const Triangle&);