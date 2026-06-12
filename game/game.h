#ifndef GAME_H
#define GAME_H

#include <stddef.h>
#include <assert.h>

#include <SDL3/SDL.h>

#include "difficulty.h"
#include "text.h"
#include "ball.h"
#include "snow.h"
#include "themes.h"

#define TOTAL_TEXT 12

#define INFINITE_MODE_MASK 0x01
#define DOUBLE_PLAYER_MASK 0x02

typedef enum {
    GAME_INIT,
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_OVER
} GameState;

#ifndef __EMSCRIPTEN__ // Browser probably not support double tap gesture

typedef struct {
    Uint64 last_touch_tick;
    float last_touch_x;
    float last_touch_y;
    SDL_FingerID last_finger_id;
} LastTap; // Add Double Tap gesture support

#endif

#define CORE_TEXTS \
    TEXT(0, "Welcome to Pong", 64) \
    TEXT(2, "Classic Mode", 48) \
    TEXT(3, "Infinite Mode", 48) \
    TEXT(4, "Single Player", 48) \
    TEXT(5, "Double Player", 48) \
    TEXT(6, "Easy", 48) \
    TEXT(7, "Medium", 48) \
    TEXT(8, "Hard", 48) \
    TEXT(9, "Paused", 64) \
    TEXT(10, "WIN", 72)

#ifdef __EMSCRIPTEN__

#define INFO_TEXTS \
    TEXT(1, "Press Start Button to start", 32) \
    TEXT(11, "Press Restart Button to restart", 28)

#else

#define INFO_TEXTS \
    TEXT(1, "Press [SPACE] to start", 32) \
    TEXT(11, "Press [SPACE] to restart", 28)

#endif

#define LIST_OF_TEXTS \
    CORE_TEXTS \
    INFO_TEXTS

typedef struct {
    SDL_Window *window;
    SDL_FRect window_boarder;
    SDL_Renderer *window_renderer;
    SDL_FRect middle_line;

    Paddle left_paddle;
    Paddle right_paddle;

    Ball ball;

    Snow snow;

    Text texts[TOTAL_TEXT]; // This include all the texts in LIST_OF_TEXTS

    Uint8 mode_flags; // Flags for the game mode

    size_t score_to_win;
    size_t max_score;
    size_t left_score;
    size_t right_score;

    GameState state;
    Uint8 theme_index; // Index of the current theme
    Theme const *theme;
    Difficulty difficulty_index;

#ifndef __EMSCRIPTEN__
    LastTap last_tap;
#endif

    Uint64 last_counter;
    bool is_first_frame;
    float resume_delay_time;

#ifndef BENCHMARK_MODE
    Uint64 last_key_ticks;
#endif
} Game;

#endif // GAME_H