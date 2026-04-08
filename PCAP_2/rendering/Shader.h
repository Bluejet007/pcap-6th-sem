#pragma once
#include <string>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vs, const char* fs);
    void use();
};