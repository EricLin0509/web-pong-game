#include "paddle.h"

void paddle_init(Paddle *paddle, int x, int y, int width, int height)
{
    if (paddle == NULL) return;
    
    paddle->paddle_rect.x = x;
    paddle->paddle_rect.y = paddle->orig_y = y;
    paddle->paddle_rect.w = width;
    paddle->paddle_rect.h = height;
    paddle->direction = PADDLE_STOP;
}

void paddle_move(Paddle *paddle, uint32_t min_y, uint32_t max_y, float dt)
{
    if (paddle == NULL) return;

    float actual_distance = PADDLE_SPEED_PER_SEC * dt;

    switch (paddle->direction)
    {
        case PADDLE_UP:
            if (paddle->paddle_rect.y - actual_distance > min_y)
                paddle->paddle_rect.y -= actual_distance;
            break;
        case PADDLE_DOWN:
            if (paddle->paddle_rect.y + actual_distance < max_y - paddle->paddle_rect.h)
                paddle->paddle_rect.y += actual_distance;
            break;
        case PADDLE_STOP:
        default:
            break;
    }
}

void render_paddle(Paddle *paddle, SDL_Renderer *renderer)
{
    if (paddle == NULL || renderer == NULL) return;

    SDL_RenderFillRect(renderer, &(paddle->paddle_rect));
}

void reset_paddle(Paddle *paddle)
{
    if (paddle == NULL) return;

    /* Only reset the y position */
    paddle->paddle_rect.y = paddle->orig_y;

    paddle->direction = PADDLE_STOP;
}