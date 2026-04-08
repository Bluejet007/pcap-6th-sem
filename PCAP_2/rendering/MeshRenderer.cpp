#include "MeshRenderer.h"
#include <glad/glad.h>

void MeshRenderer::init(const std::vector<Vertex>& v,
                        const std::vector<unsigned int>& i) {

    count = i.size();

    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,
        v.size()*sizeof(Vertex), &v[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        i.size()*sizeof(unsigned int), &i[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,
        sizeof(Vertex),(void*)offsetof(Vertex,Normal));
    glEnableVertexAttribArray(1);
}

void MeshRenderer::draw() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_INT,0);
}