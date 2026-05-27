#ifndef BALL_H
#define BALL_H

#include <stdlib.h>
#include <SDL3/SDL.h>

#include "paddle.h"

#define BALL_SIZE 20.0f
#define INITIAL_SPEED_PER_SEC ((float)rand() / (float)RAND_MAX * 50.0f + 350.0f) // [350, 400]
#define JITTER_PER_SEC ((float)rand() / (float)RAND_MAX * 10.0f + 10.0f) // [10, 20]
#define MAX_SPEED 1000.0f
#define SHOUD_MOVE_REVERSE ((rand() & 1) == 0)

typedef enum {
    COLLISION_NONE, // Hit the paddle, do nothing
    COLLISION_LEFT, // Hit the left wall, right player wins
    COLLISION_RIGHT // Hit the right wall, left player wins
} CollisionType;

typedef struct {
    SDL_FRect rect;
    float orig_x;
    float orig_y;
    float speed_x;
    float speed_y;
} Ball;

void ball_init(Ball *ball, float x, float y, int width, int height);

void reset_ball(Ball *ball, bool left_serve);

void speed_up_ball(Ball *ball);

/* @note: No need to minus the ball size in the collision check, It already included */
CollisionType ball_collision(Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                           uint32_t col_min_x, uint32_t col_max_x, uint32_t col_min_y, uint32_t col_max_y, float dt);

void render_ball(Ball *ball, SDL_Renderer *renderer);

#endif // BALL_H