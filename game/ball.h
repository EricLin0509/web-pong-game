#ifndef BALL_H
#define BALL_H

#include <stdlib.h>
#include <SDL3/SDL_main.h>

#define BALL_SIZE 20.0f
#define INITIAL_SPEED ((float)rand() / (float)RAND_MAX * 1.0f + 2.0f)
#define JITTER ((float)rand() / (float)RAND_MAX * 0.1f - 0.05f) // [-0.05, 0.05]
#define MAX_SPEED 8.0f
#define SPEED_UP_RATE 1.05f
#define SHOUD_MOVE_REVERSE ((rand() & 1) == 0)

typedef enum {
    COLLISION_NONE, // Hit the paddle, do nothing
    COLLISION_LEFT, // Hit the left wall, right player wins
    COLLISION_RIGHT // Hit the right wall, left player wins
} CollisionType;

typedef struct {
    SDL_FRect rect;
    float speed_x;
    float speed_y;
} Ball;

#endif // BALL_H