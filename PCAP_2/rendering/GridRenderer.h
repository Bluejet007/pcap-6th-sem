#pragma once
#include <vector>

class GridRenderer {
public:
    unsigned int VAO, VBO;
    int vertexCount;

    void init(float size, int divisions);
    void draw();
};