#include "GridRenderer.h"
#include <glad/glad.h>
#include <vector>
#include "../core/types.h"

// 🔹 Generate grid lines
static std::vector<float> generateGrid(float size, int divisions) {
    std::vector<float> vertices;
    float step = size / divisions;

    for (int i = 0; i <= divisions; ++i) {
        float pos = -size / 2.0f + i * step;

        // vertical lines (Z direction)
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(-size/2);
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(size/2);

        // horizontal lines (X direction)
        vertices.push_back(-size/2); vertices.push_back(0.0f); vertices.push_back(pos);
        vertices.push_back(size/2);  vertices.push_back(0.0f); vertices.push_back(pos);
    }

    return vertices;
}

// 🔹 Initialize grid buffers
void GridRenderer::init(float size, int divisions) {
    std::vector<float> verts = generateGrid(size, divisions);
    vertexCount = verts.size() / 3;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(float),
                 verts.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// 🔹 Draw grid
void GridRenderer::draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, vertexCount);
    glBindVertexArray(0);
}