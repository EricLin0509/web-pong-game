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

    Text paused_text;

    Text game_over_text;
    Text game_over_description;

    size_t rounds;
    size_t max_score;

    size_t left_score;
    size_t right_score;

    GameState state;
    Theme const *theme;
} Game;

/* Make sure the Text fields are continuous */
#define TEXT_FIELDS \
    X(welcome_text, welcome_description) \
    X(welcome_description, paused_text) \
    X(paused_text, game_over_text) \
    X(game_over_text, game_over_description)

#define X(curr, next) \
    static_assert(offsetof(Game, next) == \
        (offsetof(Game, curr) + sizeof(Text)), \
        "\'"#curr" -> "#next"\' is NOT continuous");
TEXT_FIELDS
#undef X

#endif // GAME_H