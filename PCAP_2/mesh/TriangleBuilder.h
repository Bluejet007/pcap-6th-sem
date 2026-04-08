#pragma once
#include <vector>
#include "../core/types.h"
#include <glm/glm.hpp>

std::vector<Triangle> buildWorldTriangles(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const glm::mat4& model
);