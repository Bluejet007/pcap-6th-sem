#include <GL/glut.h>
#include "input.h"
#include "camera.h"
#include "particle.h"

void keyboard(unsigned char key, int x, int y) {
    float speed = 0.1f;
    switch(key) {
        case 'w': // forward
            camX += dirX * speed;
            camY += dirY * speed;
            camZ += dirZ * speed;
            break;

        case 's': // backward
            camX -= dirX * speed;
            camY -= dirY * speed;
            camZ -= dirZ * speed;
            break;

        case 'a': // left (perpendicular)
            camX -= dirZ * speed;
            camZ += dirX * speed;
            break;

        case 'd': // right
            camX += dirZ * speed;
            camZ -= dirX * speed;
            break;
    }

    glutPostRedisplay(); // redraw scene after movement
}
void mouseZoom(int button, int state, int x, int y) {
    float zoomSpeed = 0.2f;

    if(state == GLUT_DOWN) {
        if(button == 3) { // scroll up → zoom in (move camera forward)
            camX += dirX * zoomSpeed;
            camY += dirY * zoomSpeed;
            camZ += dirZ * zoomSpeed;
        }
        if(button == 4) { // scroll down → zoom out (move camera backward)
            camX -= dirX * zoomSpeed;
            camY -= dirY * zoomSpeed;
            camZ -= dirZ * zoomSpeed;
        }
    }

    glutPostRedisplay(); // redraw scene after zoom
}
void mouseClickForParticle(int button, int state, int x, int y){
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        // screen coorindates -> openGL coordinates (1, -1)
        int width = glutGet(GLUT_WINDOW_WIDTH);
        int height = glutGet(GLUT_WINDOW_HEIGHT);

        float fx = (x / (float)width) * 2.0f - 1.0f;
        float fy = -((y / (float)height) * 2.0f - 1.0f);
        spawnParticle(fx, fy);
    }
}
void mouse(int button, int state, int x, int y){
    // LEFT CLICK → spawn particle
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        int width = glutGet(GLUT_WINDOW_WIDTH);
        int height = glutGet(GLUT_WINDOW_HEIGHT);

        float fx = (x / (float)width) * 2.0f - 1.0f;
        float fy = -((y / (float)height) * 2.0f - 1.0f);

        for(int i = 0; i < 50; i++)
            spawnParticle(fx + 2 * (float) rand() / RAND_MAX, fy + 2 * (float) rand() / RAND_MAX); // create particle at clicked position
    }
    float zoomSpeed = 0.2f;
    // SCROLL UP → zoom in
    if(button == 3 && state == GLUT_DOWN){
        camX += dirX * zoomSpeed;
        camY += dirY * zoomSpeed;
        camZ += dirZ * zoomSpeed;
    }

    // SCROLL DOWN → zoom out
    if(button == 4 && state == GLUT_DOWN){
        camX -= dirX * zoomSpeed;
        camY -= dirY * zoomSpeed;
        camZ -= dirZ * zoomSpeed;
    }

    glutPostRedisplay(); // redraw after interaction
}
