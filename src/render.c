#include <GL/glut.h>
#include "render.h"
#include "camera.h"
#include "particle.h"
#include <math.h>

void init3D(){
    glMatrixMode(GL_PROJECTION);   // Switch to projection matrix (camera lens)
    glLoadIdentity();              // Reset it

    // Perspective projection:
    // 45° FOV, aspect ratio 1, near plane 0.1, far plane 10
    //gluPerspective(45.0, 1.0, 0.1, 10.0);
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);

    gluPerspective(45.0, (float)width/height, 0.1, 10.0);

    glMatrixMode(GL_MODELVIEW);    // Switch back to model (object space)
}
void drawCircle(float cx, float cy, float radius){
    glBegin(GL_POLYGON);
    for(int i=0; i<100; i++){
        float angle = 2 * PI * i /100;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}

void drawSphere(float x, float y, float z, float r){
    glPushMatrix();              // Save current transform
    glTranslatef(x, y, z);       // Move to particle position
    glutSolidSphere(r, 20, 20);  // Draw sphere
    glPopMatrix();               // Restore transform
}
void drawGrid(){    // Makes grid in the view for better presentaion
    glColor3f(0.3f, 0.3f, 0.3f); // gray

    glBegin(GL_LINES);

    int size = 10;
    float step = 0.2f;

    for(int i = -size; i <= size; i++){
        // lines along X
        glVertex3f(i * step, 0, -size * step);
        glVertex3f(i * step, 0, size * step);

        // lines along Z
        glVertex3f(-size * step, 0, i * step);
        glVertex3f(size * step, 0, i * step);
    }

    glEnd();
}

void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clear BOTH color buffer AND depth buffer
    glLoadIdentity();
    // Must be before gluLookAt
    dirX = cos(yaw * PI/180.0f) * cos(pitch * PI/180.0f);
    dirY = sin(pitch * PI/180.0f);
    dirZ = sin(yaw * PI/180.0f) * cos(pitch * PI/180.0f);
    // camera position
    gluLookAt(
        camX, camY, camZ,   // camera position (eye)
        camX + dirX, camY + dirY, camZ + dirZ,   // where camera looks based on angles
        0.0, 1.0, 0.0    // up direction
    );
    glDisable(GL_DEPTH_TEST);
    drawGrid();
    glEnable(GL_DEPTH_TEST);

    for(int i=0; i<particleCount; i++){
        Particle *p = &particles[i];    // We use a pointer so that we dont make an unnecssary copy

        glColor3f(p->r, p->g, p->b);
        //drawCircle(p->x, p->y, p->radius);
        drawSphere(p->x, p->y, p->z, p->radius);
    }
    glutSwapBuffers();
}
