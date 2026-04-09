#include "ParticleRenderer.h"
#include <glad/glad.h>

#include "ParticleRenderer.h"
#include <glad/glad.h>

void ParticleRenderer::init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void ParticleRenderer::draw(const std::vector<Particle>& particles) {

    std::vector<float> data;
    data.reserve(particles.size() * 3);

    for (auto &p : particles) {
        data.push_back(p.pos.x);
        data.push_back(p.pos.y);
        data.push_back(p.pos.z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        data.size()*sizeof(float),
        data.data(),
        GL_DYNAMIC_DRAW);

    glBindVertexArray(VAO);
    glPointSize(3.0f);
    glDrawArrays(GL_POINTS, 0, particles.size());
}