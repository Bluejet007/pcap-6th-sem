#include "ParticleRenderer.h"
#include <glad/glad.h>

void ParticleRenderer::draw(const std::vector<Particle>& particles) {
    std::vector<float> data;

    for (auto &p:particles) {
        data.push_back(p.pos.x);
        data.push_back(p.pos.y);
        data.push_back(p.pos.z);
    }

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,
        data.size()*sizeof(float), &data[0], GL_DYNAMIC_DRAW);

    glDrawArrays(GL_POINTS,0,particles.size());
}