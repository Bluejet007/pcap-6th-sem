#include <stdlib.h>
#include <time.h>
#include<GL/glut.h>
#include<math.h>

// Initial State
float posX = 0.0f;
float posY = 0.0f;
float radius = 0.02f;

float velX = 0.005f;
float velY = 0.005f;

float accX = -0.0f;
float accY = -0.0008f;

// DRAW A CIRCLE
// We approximate a circle using many points (polygon)
void drawCircle(float cx, float cy, float radius){
    glBegin(GL_POLYGON);
    for(int i=0; i<100; i++){
        float angle = 2 * 3.14159f * i /100;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}


void update(){
    velY += accY;
    velX += accX;

    posY += velY;
    posX += velX;

    if(posY - radius < -0.8f){   // Collision with floor
        posY = -0.8f + radius;
        velY *= -1.0f; // No energy loss
    }
    if(posY + radius > 0.8f){
        posY = 0.8f - radius;
        velY *= -1.0f;  // No energy loss
    }
    if(posX + radius > 0.8f){
        posX = 0.8f - radius;
        velX *= -1.0f;
    }
    if(posX - radius < -0.8f){
        posX = -0.8f + radius;
        velX *= -1.0f;
    }
    glutPostRedisplay();
}

// Rendering
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawCircle(posX, posY, radius);

    glFlush();
}

void timer(int value) {
    update();
    glutTimerFunc(16, timer, 0); // 16ms ~ 60 FPS
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutCreateWindow("Physics Dot-Bouncing");

    srand(time(NULL));
    velX = ((rand() % 200) - 100) / 5000.0f;
    velY = ((rand() % 200) - 100) / 5000.0f;
    //glPointSize(10);    // Useless ince we make our circle

    glutDisplayFunc(display);   // For rendering
    glutTimerFunc(0, timer, 0); // Updates the display loop

    glutMainLoop(); // Infinite loop
    return 0;
}