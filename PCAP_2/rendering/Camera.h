#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    float yaw=0, pitch=0, zoom=5, height=0.5f;

    glm::mat4 getView();
    glm::mat4 getProj(float w,float h);
};