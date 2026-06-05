#ifndef PADDLE_H
#define PADDLE_H

#include <stdbool.h>
#include <stdio.h>
#include <SDL3/SDL.h>

#define PADDLE_WIDTH 15
#define PADDLE_HEIGHT 120
#define PADDLE_PADDING 20
#define PADDLE_SPEED_PER_SEC 600

/* The movement direction of the paddle */
typedef enum {
    PADDLE_STOP, // Do nothing
    PADDLE_UP, // Move up
    PADDLE_DOWN // Move down
} PaddleMove;

typedef struct {
    SDL_FRect paddle_rect;
    PaddleMove direction;
    float orig_y;
} Paddle;

void paddle_init(Paddle *paddle, int x, int y, int width, int height);

/* @note: No need to minus the paddle height in the collision check, It already included */
void paddle_move(Paddle *paddle, uint32_t min_y, uint32_t max_y, float speed, float dt);

/* The touch version of the paddle move function */
void paddle_move_touch(Paddle *paddle, uint32_t min_y, uint32_t max_y, float delta_y);

void render_paddle(Paddle *paddle, SDL_Renderer *renderer);

void reset_paddle(Paddle *paddle);

#endif // PADDLE_H