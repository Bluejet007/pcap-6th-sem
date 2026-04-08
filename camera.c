#include <math.h>
#include <GL/glut.h>
#include "camera.h"
#include "render.h"

float camX = 0.0f, camY = 0.0f, camZ = 2.0f;
float dirX, dirY, dirZ;

float yaw = -90.0f;
float pitch = 0.0f;

float lastX = 400, lastY = 300;
int firstMouse = 1;

void mouseMotion(int x, int y){
    if(firstMouse){
        lastX = x;
        lastY = y;
        firstMouse = 0;
    }

    float sensitivity = 0.1f;
    float offsetX = (x - lastX) * sensitivity;
    float offsetY = (lastY - y) * sensitivity;

    lastX = x;
    lastY = y;

    yaw += offsetX;
    pitch += offsetY;

    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
}