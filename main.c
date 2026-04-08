#include <GL/glut.h>

#include "camera.h"
#include "input.h"
#include "render.h"
#include "physics.h"

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("Physics Dot(s)-Bouncing");

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    init3D();
    glEnable(GL_DEPTH_TEST);

    glutDisplayFunc(display);
    glutTimerFunc(0, timer, 0);

    glutMouseFunc(mouse);
    glutPassiveMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}