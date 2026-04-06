#ifndef INPUT_H
#define INPUT_H

extern float lastX, lastY, yaw, pitch;
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void mouseMotion(int x, int y);

#endif