#ifndef GAME_H
#define GAME_H

#include <stddef.h>
#include <assert.h>

#include "text.h"
#include "paddle.h"
#include "ball.h"
#include "snow.h"
#include "themes.h"

#define TOTAL_TEXT (offsetof(Game, game_over_description) \
    - offsetof(Game, welcome_text)) / sizeof(Text) + 1

typedef enum {
    GAME_INIT,
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_OVER
} GameState;

typedef enum {
    MODE_CLASSIC,
    MODE_INFINITE
} GameMode;

typedef struct {
    SDL_Window *window;
    SDL_FRect window_boarder;
    SDL_Renderer *window_renderer;
    SDL_FRect middle_line;

    Paddle left_paddle;
    Paddle right_paddle;

    Ball ball;

    Snow snow;

    Text welcome_text;
    Text welcome_description;
    Text current_mode_classic;
    Text current_mode_infinite;

    Text paused_text;

    Text game_over_text;
    Text game_over_description;

    GameMode mode;

    size_t score_to_win;
    size_t max_score;
    size_t left_score;
    size_t right_score;

    GameState state;
    Theme const *theme;

    Uint64 last_counter;
    bool is_first_frame;
    float resume_delay_time;
#ifndef BENCHMARK_MODE
    Uint64 last_key_ticks;
    Uint64 last_frame_ticks;
#endif
} Game;

/* Make sure the Text fields are continuous */
#define TEXT_FIELDS \
    X(welcome_text, welcome_description) \
    X(welcome_description, current_mode_classic) \
    X(current_mode_classic, current_mode_infinite) \
    X(current_mode_infinite, paused_text) \
    X(paused_text, game_over_text) \
    X(game_over_text, game_over_description)

#define X(curr, next) \
    static_assert(offsetof(Game, next) == \
        (offsetof(Game, curr) + sizeof(Text)), \
        "\'"#curr" -> "#next"\' is NOT continuous");
TEXT_FIELDS
#undef X

#endif // GAME_H