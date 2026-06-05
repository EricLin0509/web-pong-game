#include <math.h>

#include "ball.h"

#define MAX_BOUNCE_ANGLE 300.0f

float initial_speed_easy(void)
{
    return (float)rand() / (float)RAND_MAX * 50.0f + 350.0f; // [350, 400]
}

float initial_speed_medium(void)
{
    return (float)rand() / (float)RAND_MAX * 100.0f + 400.0f; // [400, 500]
}

float initial_speed_hard(void)
{
    return (float)rand() / (float)RAND_MAX * 150.0f + 500.0f; // [500, 650]
}

static const initial_speed_generator generators[3] = {
    initial_speed_easy,
    initial_speed_medium,
    initial_speed_hard
};

void ball_init(Ball *ball, float x, float y, int width, int height)
{
    if (ball == NULL) return;

    ball->rect.x = ball->orig_x = x;
    ball->rect.y = ball->orig_y = y;
    ball->rect.w = width;
    ball->rect.h = height;

    ball->generator = generators[0];
    
    ball->speed_x = (SHOUD_MOVE_REVERSE) ? -(ball->generator()) : (ball->generator());
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -(ball->generator()) + 200.0f : (ball->generator()) - 200.0f;
}

void reset_ball(Ball *ball, bool left_serve)
{
    if (ball == NULL) return;

    ball->rect.x = ball->orig_x;
    ball->rect.y = ball->orig_y;
    ball->speed_x = (left_serve) ? -(ball->generator()) : (ball->generator());
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -(ball->generator()) + 200.0f : (ball->generator()) - 200.0f;
}

void set_ball_difficulty(Ball *ball, Uint8 index)
{
    if (ball == NULL) return;

    if (index < 3)
        ball->generator = generators[index];

    /* Update ball speed based on difficulty */
    ball->speed_x = (SHOUD_MOVE_REVERSE) ? -(ball->generator()) : (ball->generator());
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -(ball->generator()) + 200.0f : (ball->generator()) - 200.0f;
}

CollisionType ball_collision(Ball *ball, Paddle *left_paddle, Paddle *right_paddle,
                           uint32_t col_min_x, uint32_t col_max_x, uint32_t col_min_y, uint32_t col_max_y, float dt)
{
    if (ball == NULL || left_paddle == NULL || right_paddle == NULL) return COLLISION_NONE;

    // Score points
    if (ball->rect.x < col_min_x)
        return COLLISION_LEFT;
    else if (ball->rect.x > col_max_x - ball->rect.w)
        return COLLISION_RIGHT;

    // Wall collision
    float new_y = ball->rect.y + ball->speed_y * dt;
    if (new_y < col_min_y || new_y > (col_max_y - ball->rect.h))
    {
        ball->speed_y *= -1;
        new_y = (new_y < col_min_y) ? col_min_y : (col_max_y - ball->rect.h);
    }

    // Predict next position
    float new_x = ball->rect.x + ball->speed_x * dt;
    SDL_FRect next_rect = { new_x, new_y, ball->rect.w, ball->rect.h };

    Paddle *chosen_paddle = (new_x < col_max_x / 2) ? left_paddle : right_paddle;

    if (SDL_HasRectIntersectionFloat(&next_rect, &(chosen_paddle->paddle_rect)))
    {
        float ball_center_y = ball->rect.y + ball->rect.h / 2.0f;
        float paddle_center_y = chosen_paddle->paddle_rect.y + chosen_paddle->paddle_rect.h / 2.0f;
        float hit_pos = (ball_center_y - paddle_center_y) / (chosen_paddle->paddle_rect.h / 2.0f);
        // Limit the hit position to [-1, 1] for better control
        if (hit_pos > 1.0f) hit_pos = 1.0f;
        if (hit_pos < -1.0f) hit_pos = -1.0f;

        /*
            Calculate the new speed based on the hit position and the paddle's height.
            The hit position is used to determine the angle of the bounce.
            The paddle's height is used to limit the speed of the bounce.
            The horizontal speed is also jittered to make the ball more realistic.
        */
        ball->speed_x = -ball->speed_x;
        // Speed up the horizontal speed
        if (fabsf(ball->speed_x) < MAX_SPEED)
            ball->speed_x = ball->speed_x > 0 ? ball->speed_x + JITTER_PER_SEC : ball->speed_x - JITTER_PER_SEC;

        ball->speed_y = hit_pos * MAX_BOUNCE_ANGLE;

        // Adjust the ball position to prevent stuck in the paddle
        if (ball->speed_x > 0)
            new_x = chosen_paddle->paddle_rect.x + chosen_paddle->paddle_rect.w;
        else
            new_x = chosen_paddle->paddle_rect.x - ball->rect.w;
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