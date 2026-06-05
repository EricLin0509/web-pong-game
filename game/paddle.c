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

void paddle_move(Paddle *paddle, uint32_t min_y, uint32_t max_y, float speed, float dt)
{
    if (paddle == NULL) return;

    float actual_distance = speed * dt;

    switch (paddle->direction)
    {
        case PADDLE_UP:
            if (paddle->paddle_rect.y - actual_distance > min_y)
                paddle->paddle_rect.y -= actual_distance;
            else
                paddle->paddle_rect.y = min_y; // Clamp to the minimum y position
            break;
        case PADDLE_DOWN:
            if (paddle->paddle_rect.y + actual_distance < max_y - paddle->paddle_rect.h)
                paddle->paddle_rect.y += actual_distance;
            else
                paddle->paddle_rect.y = max_y - paddle->paddle_rect.h; // Clamp to the maximum y position
            break;
        case PADDLE_STOP:
        default:
            break;
    }
}

void paddle_move_touch(Paddle *paddle, uint32_t min_y, uint32_t max_y, float delta_y)
{
    if (paddle == NULL) return;

    float new_pos = paddle->paddle_rect.y + delta_y; // Calculate the new position based on touch input

    if (new_pos < min_y || new_pos > max_y - paddle->paddle_rect.h) return;

    paddle->paddle_rect.y = new_pos; // Update the paddle's position
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