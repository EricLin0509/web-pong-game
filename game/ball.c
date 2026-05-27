#include "ball.h"

void ball_init(Ball *ball, float x, float y, int width, int height)
{
    if (ball == NULL) return;

    ball->rect.x = ball->orig_x = x;
    ball->rect.y = ball->orig_y = y;
    ball->rect.w = width;
    ball->rect.h = height;
    
    ball->speed_x = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED_PER_SEC : INITIAL_SPEED_PER_SEC;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED_PER_SEC : INITIAL_SPEED_PER_SEC;
}

void reset_ball(Ball *ball, bool left_serve)
{
    if (ball == NULL) return;

    ball->rect.x = ball->orig_x;
    ball->rect.y = ball->orig_y;
    float speed_x = INITIAL_SPEED_PER_SEC;
    ball->speed_x = (left_serve) ? -speed_x : speed_x;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? INITIAL_SPEED_PER_SEC : -INITIAL_SPEED_PER_SEC;
}

void speed_up_ball(Ball *ball)
{
    if (ball == NULL) return;

    if (ball->speed_x <= MAX_SPEED)
        ball->speed_x *= SPEED_UP_RATE;
    if (ball->speed_y <= MAX_SPEED)
        ball->speed_y *= SPEED_UP_RATE;
}

CollisionType ball_collision(Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                           uint32_t col_min_x, uint32_t col_max_x, uint32_t col_min_y, uint32_t col_max_y, float dt)
{
    if (ball == NULL || left_paddle == NULL || right_paddle == NULL) return COLLISION_NONE;

    // First handle the score points
    if (ball->rect.x < col_min_x)
        return COLLISION_LEFT;
    else if (ball->rect.x > col_max_x - ball->rect.w)
        return COLLISION_RIGHT;

    // Bounce off the top and bottom walls
    float new_y = ball->rect.y + ball->speed_y * dt;
    if (new_y < col_min_y || new_y > (col_max_y - ball->rect.h))
    {
        ball->speed_y *= -1;
        new_y = (new_y < col_min_y) ? col_min_y : (col_max_y - ball->rect.h);
    }

    // Predict the ball's next position
    float new_x = ball->rect.x + ball->speed_x * dt;
    SDL_FRect next_rect = { new_x, new_y, ball->rect.w, ball->rect.h };

    Paddle *chosen_paddle = (new_x < col_max_x / 2) ? left_paddle : right_paddle;

    if (SDL_HasRectIntersectionFloat(&next_rect, &(chosen_paddle->paddle_rect)))
    {
        ball->speed_x = -(ball->speed_x + JITTER_PER_SEC);
        if (ball->speed_x > 0)
            new_x = chosen_paddle->paddle_rect.x + chosen_paddle->paddle_rect.w;
        else
            new_x = chosen_paddle->paddle_rect.x - ball->rect.w;
        speed_up_ball(ball);
    }

    // Apply the new position
    ball->rect.x = new_x;
    ball->rect.y = new_y;

    return COLLISION_NONE;
}

void render_ball(Ball *ball, SDL_Renderer *renderer)
{
    if (ball == NULL || renderer == NULL) return;

    SDL_RenderFillRect(renderer, &(ball->rect));
}