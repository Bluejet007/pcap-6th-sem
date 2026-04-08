#include<stdio.h>

int main(){
    float position = 0.0f;
    float velocity = 0.0f;
    float acceleration = -9.8f;

    float dt = 0.1f;

    for(int i=0; i<100; i++){
        velocity += acceleration * dt;
        position += velocity * dt;

        printf("Time: %.2f | Pos: %.2f | Vel: %.2f\n", i * dt, position, velocity);
    }
    return 0;
}