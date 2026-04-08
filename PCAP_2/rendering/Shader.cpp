#include "Shader.h"
#include <glad/glad.h>

Shader::Shader(const char* vs, const char* fs) {
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v,1,&vs,NULL);
    glCompileShader(v);

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f,1,&fs,NULL);
    glCompileShader(f);

    ID = glCreateProgram();
    glAttachShader(ID,v);
    glAttachShader(ID,f);
    glLinkProgram(ID);

    glDeleteShader(v);
    glDeleteShader(f);
}

void Shader::use() {
    glUseProgram(ID);
}