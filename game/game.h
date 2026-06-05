#ifndef GAME_H
#define GAME_H

#include <stddef.h>
#include <assert.h>

#include <SDL3/SDL.h>

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

typedef float (*ai_difficulty_func)(void);

typedef struct {
    SDL_Window *window;
    SDL_FRect window_boarder;
    SDL_Renderer *window_renderer;
    SDL_FRect middle_line;

    Paddle left_paddle;
    Paddle right_paddle;

    Ball ball;

    Snow snow;

    Text texts[TOTAL_TEXT];
    /*
        The text array contains the following texts:
        [0]: Welcome to Pong
        [1]: Press [SPACE] to start
        [2]: Classic Mode
        [3]: Infinite Mode
        [4]: Single Player
        [5]: Double Player
        [6]: Simple
        [7]: Medium
        [8]: Hard
        [9]: Paused
        [10]: WIN
        [11]: Press [space] to restart
    */

    Uint8 mode_flags; // Flags for the game mode

    size_t score_to_win;
    size_t max_score;
    size_t left_score;
    size_t right_score;

    GameState state;
    Uint8 theme_index; // Index of the current theme
    Theme const *theme;
    Uint8 difficulty_index;
    ai_difficulty_func difficulty;

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