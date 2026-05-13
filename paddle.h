#ifndef PADDLE_H
#define PADDLE_H

#include <stdbool.h>
#include <SDL3/SDL_main.h>

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define PADDLE_SPEED 12

/* The movement direction of the paddle */
typedef enum {
    PADDLE_STOP, // Do nothing
    PADDLE_UP, // Move up
    PADDLE_DOWN // Move down
} PaddleMove;

typedef struct {
    SDL_FRect paddle_rect;
    PaddleMove direction;
} Paddle;

#endif // PADDLE_H