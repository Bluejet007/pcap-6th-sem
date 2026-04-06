#include <stdlib.h>
#include <time.h>
#include<GL/glut.h>
#include<math.h>
#define MAX_PARTICLES 100
#define DOT_RADIUS 0.05f
#define PI 3.14159265f

float camX = 0.0f, camY = 0.0f, camZ = 2.0f;   // camera position
float dirX, dirY, dirZ; // Direction for the camera to look in

float yaw = -90.0f;   // horizontal angle
float pitch = 0.0f;   // vertical angle

float lastX = 400, lastY = 300; // mouse center
int firstMouse = 1;

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

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float radius;
    float r, g, b;  // Colour
}Particle;

int particleCount = 0;
Particle particles[MAX_PARTICLES];

// Making a dot
// We approximate a circle using many points (polygon)
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

void spawnParticle(float x, float y){
    if(particleCount >= MAX_PARTICLES)return;
    Particle p;
    p.x = x;
    p.y = y;
    p.z = ((rand() % 160) - 80) / 100.0f; // [-0.8, 0.8]
    p.vx = ((rand() % 200) - 100) / 5000.0f;
    p.vy = ((rand() % 200) - 100) / 5000.0f;
    p.vz = ((rand() % 200) - 100) / 5000.0f;
    p.radius = DOT_RADIUS;

    p.r = (rand() % 100) / 100.0f;
    p.g = (rand() % 100) / 100.0f;
    p.b = (rand() % 100) / 100.0f;
    particles[particleCount++] = p;
}


void update(){
    float radius = DOT_RADIUS;

    for(int i=0; i<particleCount; i++){
        Particle *p = &particles[i];
        p->vy += -0.0008f;   // Gravity

        p->x += p->vx;
        p->y += p->vy;
        p->z += p->vz;

        if(p->y - radius < -0.8f){
            p->y = -0.8f + radius;
            p->vy *= -1.0f;     // We flip the direction and no speed is lost
        }
        if(p->y + radius > 0.8f){
            p->y = 0.8f - radius;
            p->vy *= -1.0f;     
        }
        if(p->x - radius < -0.8f){
            p->x = -0.8f + radius;
            p->vx *= -1.0f;     
        }
        if(p->x + radius > 0.8f){
            p->x = 0.8f - radius;
            p->vx *= -1.0f;     
        }
        if(p->z + DOT_RADIUS > 0.8f){
            p->z = 0.8f - DOT_RADIUS;
            p->vz *= -1.0f;
        }
        if(p->z - DOT_RADIUS < -0.8f){
            p->z = -0.8f + DOT_RADIUS;
            p->vz *= -1.0f;
        }
    }
    glutPostRedisplay();
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

        spawnParticle(fx, fy); // create particle at clicked position
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
void mouseMotion(int x, int y){ // For looking around
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
    // prevent flipping
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
    glutPostRedisplay();
}
void timer(int value){
    update();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv){
    glutInit(&argc, argv); // Initialize GLUT (must be first, sets up system)
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // Enable double buffer (smooth), RGB color, and depth buffer (for 3D)
    glutCreateWindow("Physics Dot(s)-Bouncing"); // Create window + OpenGL context (everything after depends on this)

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Set background color (must come after window exists)
    init3D(); // Set perspective projection (defines how 3D is viewed)

    glutDisplayFunc(display); // Register render function (draws every frame)
    glutTimerFunc(0, timer, 0); // Start update loop (~60 FPS via timer recursion)
    glutMouseFunc(mouse); // Register mouse input (click → spawn particle)
    glutPassiveMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard); // To move in 3D space using WASD keys

    glutMainLoop(); // Start infinite event loop (handles rendering + input + updates)
    return 0; // Exit program (never actually reached due to main loop)
}