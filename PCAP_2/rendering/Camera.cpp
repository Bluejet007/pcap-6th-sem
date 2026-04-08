#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::getView() {
    float x = zoom*cos(pitch)*sin(yaw);
    float y = zoom*sin(pitch)+height;
    float z = zoom*cos(pitch)*cos(yaw);

    return glm::lookAt(glm::vec3(x,y,z),
                       glm::vec3(0,height,0),
                       glm::vec3(0,1,0));
}

glm::mat4 Camera::getProj(float w,float h) {
    return glm::perspective(glm::radians(45.0f), w/h, 0.1f, 100.0f);
}