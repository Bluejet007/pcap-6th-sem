#pragma once
#include <vector>
#include "../core/types.h"

class MeshRenderer {
public:
    unsigned int VAO,VBO,EBO;
    int count;

    void init(const std::vector<Vertex>& v,
              const std::vector<unsigned int>& i);

    void draw();
};