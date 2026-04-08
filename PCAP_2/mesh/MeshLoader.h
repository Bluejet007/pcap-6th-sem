#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "../core/types.h"

void loadMesh(const std::string& path,
              std::vector<Vertex>& vertices,
              std::vector<unsigned int>& indices,
              glm::vec3& center);