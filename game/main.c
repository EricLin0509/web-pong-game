#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "ansi-color.h"
#include "game.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_BORDER_OFFSET 10
#define WINDOW_TITLE "Pong"
#define LINE_WIDTH 2

#define CLASSIC_WIN_SCORE 11
#define INFINITE_WIN_SCORE SIZE_MAX // Use SIZE_MAX to represent infinity

#ifndef BENCHMARK_MODE // Disable benchmark mode by default
#define IDLE_TIMEOUT_MS 2000 // 2 seconds idle timeout
#define IDLE_FPS 20
#endif

#ifdef __EMSCRIPTEN__
Game *game_ptr = NULL; // Expose the game struct to WebAssembly

void pause_game(void);
void set_snowflake_count(int count);
void notify_snow_count_change(void);
void notify_score_points(void);
void notify_game_state(void);
void report_wasm_frame(void);
#endif

/* ===== Theme Functions ====== */

static void set_theme(Game *game)
{
    if (game == NULL) return;

    int theme_index = rand() % THEME_COUNT;

    game->theme = (game->theme != themes + theme_index) ?
        themes + theme_index : themes + (theme_index + 1) % THEME_COUNT;

    /* Due to the Text fields are continuous, we can set the color of all Text fields at once */
    Text *text_region = &game->welcome_text;
    for (int i = 0; text_region <= &game->game_over_description && i < TOTAL_TEXT; i++)
    {
        bool is_colored = SDL_SetTextureColorMod(text_region->text_texture,
            game->theme->foreground[0],
            game->theme->foreground[1],
            game->theme->foreground[2]
        );

        if (!is_colored)
            fprintf(stderr, WARNING_TEXT " Failed to set color of text texture: %s\n", SDL_GetError());
        text_region++;
    }
}

/* ===== Mode switch functions ====== */

static void switch_mode(Game *game)
{
    if (game == NULL) return;
    if (game->state != GAME_INIT) return; // The game mode can only be switched when the game is in the init state

    game->mode ^= MODE_INFINITE; // Toggle between classic and infinite mode

    game->score_to_win = (game->mode == MODE_INFINITE) ? INFINITE_WIN_SCORE : CLASSIC_WIN_SCORE;
}

/*====== Game Logics ======*/

static void game_reset(Game *game)
{
    if (game == NULL) return;

    game->max_score = 0;
    game->left_score = 0;
    game->right_score = 0;
    reset_ball(&game->ball, SHOUD_MOVE_REVERSE);
    reset_paddle(&game->left_paddle);
    reset_paddle(&game->right_paddle);

#ifdef __EMSCRIPTEN__
    notify_score_points();
#endif
}

static void handle_key_up(Game *game, SDL_KeyboardEvent key)
{
    if (game == NULL) return;

    switch (key.scancode)
    {
        /* Right player paddle */
        case SDL_SCANCODE_I:
            game->right_paddle.direction = PADDLE_STOP;
            break;
        case SDL_SCANCODE_K:
            game->right_paddle.direction = PADDLE_STOP;
            break;

        /* Left player paddle */
        case SDL_SCANCODE_W:
            game->left_paddle.direction = PADDLE_STOP;
            break;
        case SDL_SCANCODE_S:
            game->left_paddle.direction = PADDLE_STOP;
            break;

        default:
            break;
    }
}

/* The space key has many functions in Pong, so we need to handle it separately */
static void handle_space_key(Game *game)
{
    if (game == NULL) return;

    switch (game->state)
    {
        case GAME_INIT:
            game->state = GAME_RUNNING;
            break;
        case GAME_RUNNING:
            game->state = GAME_PAUSED;
            break;
        case GAME_PAUSED:
            game->resume_delay_time = 0.25f;
            game->state = GAME_RUNNING;
            break;
        case GAME_OVER:
            game_reset(game);
            game->state = GAME_RUNNING;
            break;
        default:
            break;
    }
}

static void handle_key_down(Game *game, SDL_KeyboardEvent key)
{
    if (game == NULL) return;

    switch (key.scancode)
    {
        /* Go back to the main menu */
        case SDL_SCANCODE_ESCAPE:
            game_reset(game);
            game->state = GAME_INIT;
            break;
        /* Handle the space key */
        case SDL_SCANCODE_SPACE:
            handle_space_key(game);
            break;
        /* Change the theme */
        case SDL_SCANCODE_C:
            set_theme(game);
            break;
        /* Switch the mode */
        case SDL_SCANCODE_M:
            switch_mode(game);
            break;

        /* Increase or decrease the snowflake count */
        case SDL_SCANCODE_B:
            increase_snow_count(&game->snow, 500, WINDOW_WIDTH, WINDOW_HEIGHT);
#ifdef __EMSCRIPTEN__
            notify_snow_count_change();
#endif
            break;
        case SDL_SCANCODE_V:
            increase_snow_count(&game->snow, -500, WINDOW_WIDTH, WINDOW_HEIGHT);
#ifdef __EMSCRIPTEN__
            notify_snow_count_change();
#endif
            break;

        /* Right player paddle */
        case SDL_SCANCODE_I:
            game->right_paddle.direction = PADDLE_UP;
            break;
        case SDL_SCANCODE_K:
            game->right_paddle.direction = PADDLE_DOWN;
            break;

        /* Left player paddle */
        case SDL_SCANCODE_W:
            game->left_paddle.direction = PADDLE_UP;
            break;
        case SDL_SCANCODE_S:
            game->left_paddle.direction = PADDLE_DOWN;
            break;

        default:
            break;
    }
}

static void show_middle_line(Game *game)
{
    if (game == NULL) return;

    SDL_RenderFillRect(game->window_renderer, &(game->middle_line));
}

static void show_mode_screen(Game *game)
{
    if (game == NULL) return;
    Text *mode_text = (game->mode == MODE_CLASSIC) ? &game->current_mode_classic : &game->current_mode_infinite;

    render_text_texture(mode_text, game->window_renderer,
                             20,
                             15);
}

static void show_welcome_screen(Game *game)
{
    if (game == NULL) return;

    render_text_texture(&game->welcome_text, game->window_renderer,
                         (WINDOW_WIDTH - game->welcome_text.text_rect.w) / 2,
                         (WINDOW_HEIGHT - game->welcome_text.text_rect.h) / 2 - 50);

    render_text_texture(&game->welcome_description, game->window_renderer,
                         (WINDOW_WIDTH - game->welcome_description.text_rect.w) / 2,
                         (WINDOW_HEIGHT - game->welcome_description.text_rect.h) / 2 + 50);
}

static void show_paused_screen(Game *game)
{
    if (game == NULL) return;

    render_text_texture(&game->paused_text, game->window_renderer,
                             20,
                             (WINDOW_HEIGHT - game->paused_text.text_rect.h) - 15);
}

static void show_game_screen(Game *game)
{
    if (game == NULL) return;

    render_paddle(&game->left_paddle, game->window_renderer);
    render_paddle(&game->right_paddle, game->window_renderer);
    render_ball(&game->ball, game->window_renderer);
}

static void show_game_over_screen(Game *game)
{
    if (game == NULL) return;

    float msg_pos_x = 0;
    float desc_pos_x = 0;
    if (game->left_score > game->right_score)
    {
        msg_pos_x = (WINDOW_WIDTH - game->game_over_text.text_rect.w) / 5;
        desc_pos_x = (WINDOW_WIDTH - game->game_over_description.text_rect.w) / 8;
    }
    else
    {
        msg_pos_x = (WINDOW_WIDTH - game->game_over_text.text_rect.w) * 4 / 5;
        desc_pos_x = (WINDOW_WIDTH - game->game_over_description.text_rect.w) * 7 / 8;
    }

    render_text_texture(&game->game_over_text, game->window_renderer,
                         msg_pos_x,
                         (WINDOW_HEIGHT - game->game_over_text.text_rect.h) / 2 - 50);

    render_text_texture(&game->game_over_description, game->window_renderer,
                         desc_pos_x,
                         (WINDOW_HEIGHT - game->game_over_description.text_rect.h) / 2 + 50);
}

static void render(Game *game)
{
    SDL_RenderClear(game->window_renderer);

    render_snow(&game->snow, game->window_renderer);

    SDL_SetRenderDrawColor(game->window_renderer,
        game->theme->foreground[0],
        game->theme->foreground[1],
        game->theme->foreground[2],
        255);

    SDL_RenderRect(game->window_renderer, &(game->window_boarder));

    switch (game->state)
    {
        case GAME_INIT:
            show_welcome_screen(game);
            show_mode_screen(game);
            break;
        case GAME_PAUSED:
            show_paused_screen(game);
        case GAME_RUNNING:
            show_middle_line(game);
            show_game_screen(game);
            break;
        case GAME_OVER:
            show_middle_line(game);
            show_game_screen(game);
            show_game_over_screen(game);
            break;
        default:
            break;
    }
    SDL_SetRenderDrawColor(game->window_renderer,
        game->theme->background[0],
        game->theme->background[1],
        game->theme->background[2],
        255);

    SDL_RenderPresent(game->window_renderer);
}

static float calculate_delta_time(Game *game)
{
    Uint64 now = SDL_GetPerformanceCounter();
    if (game->is_first_frame)
    {
        game->last_counter = now;
        game->is_first_frame = false;
    }
    float dt = (float)(now - game->last_counter) / (float)SDL_GetPerformanceFrequency();
    game->last_counter = now;

    // limit the delta time
    if (dt > 0.1f) dt = 0.1f;

    return dt;
}

#ifndef BENCHMARK_MODE

/* Limit the FPS to save energy */
static void handle_idle(Game *game)
{
    if (game == NULL) return;
    if (game->state == GAME_RUNNING) return;

    /* Calculate the idle time */
    Uint64 now = SDL_GetTicks();
    Uint64 idle_ms = now - game->last_key_ticks;
    if (idle_ms > IDLE_TIMEOUT_MS)
    {
        Uint64 target_frame_ms = 1000 / IDLE_FPS; // Target frame time in milliseconds
        Uint64 elapsed = now - game->last_frame_ticks; // Elapsed time since last frame
        if (elapsed < target_frame_ms)
            SDL_Delay(target_frame_ms - elapsed); // Wait for the remaining time
    }

    game->last_frame_ticks = SDL_GetTicks();
}

#endif

static void score_points(Game *game, bool is_left_score)
{
    if (game == NULL) return;

    size_t *score = (is_left_score) ? &game->left_score : &game->right_score;
    (*score)++;

    if (*score >= game->max_score)
        game->max_score = *score;

    if (*score == game->score_to_win)
        game->state = GAME_OVER;

#ifdef __EMSCRIPTEN__
    notify_score_points();
#endif
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    srand(time(NULL));

    static Game game = {0}; // Use static variable to keep the memory allocation in program lifetime
    *appstate = &game;

#ifdef __EMSCRIPTEN__
    game_ptr = *appstate; // Expose the game struct to WebAssembly
#endif

    SDL_Init(SDL_INIT_VIDEO);

    if (!TTF_Init())
    {
        fprintf(stderr, ERROR_TEXT " Failed to initialize TTF: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Initialize boarder and middle line */
    game.window_boarder = (SDL_FRect) {
        WINDOW_BORDER_OFFSET,
        WINDOW_BORDER_OFFSET,
        WINDOW_WIDTH - 2 * WINDOW_BORDER_OFFSET,
        WINDOW_HEIGHT - 2 * WINDOW_BORDER_OFFSET,
    };

    game.middle_line = (SDL_FRect) {
        (WINDOW_WIDTH - LINE_WIDTH) / 2,
        WINDOW_BORDER_OFFSET,
        LINE_WIDTH,
        WINDOW_HEIGHT - 2 * WINDOW_BORDER_OFFSET
    };

    /* Initialize window and renderer */
    /* Emscripten no need to set logical presentation */
#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas"); // Limit the keyboard focus to the canvas only
    game.window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
#else
    game.window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
#endif

    if (game.window == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create window: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    game.window_renderer = SDL_CreateRenderer(game.window, NULL);
    if (game.window_renderer == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

#ifndef __EMSCRIPTEN__
    if (!SDL_SetRenderLogicalPresentation(game.window_renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        fprintf(stderr, ERROR_TEXT " Failed to set logical presentation: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
#endif

    /* Load font */
    bool font_loaded = true;

    font_loaded &= create_text_texture(&game.welcome_text, FONT_PATH, "Welcome to Pong", FONT_SIZE, game.window_renderer, FONT_COLOR);

#ifdef __EMSCRIPTEN__
    font_loaded &= create_text_texture(&game.welcome_description, FONT_PATH, "Press SPACE or click start button", FONT_SIZE - 32, game.window_renderer, FONT_COLOR);
#else
    font_loaded &= create_text_texture(&game.welcome_description, FONT_PATH, "Press [SPACE] to start", FONT_SIZE - 32, game.window_renderer, FONT_COLOR);
#endif
    font_loaded &= create_text_texture(&game.current_mode_classic, FONT_PATH, "Classic Mode", FONT_SIZE - 16, game.window_renderer, FONT_COLOR);
    font_loaded &= create_text_texture(&game.current_mode_infinite, FONT_PATH, "Infinite Mode", FONT_SIZE - 16, game.window_renderer, FONT_COLOR);

    font_loaded &= create_text_texture(&game.paused_text, FONT_PATH, "Paused", FONT_SIZE, game.window_renderer, FONT_COLOR);

    font_loaded &= create_text_texture(&game.game_over_text, FONT_PATH, "WIN", FONT_SIZE, game.window_renderer, FONT_COLOR);
    font_loaded &= create_text_texture(&game.game_over_description, FONT_PATH, "Press [space] to restart", FONT_SIZE - 36, game.window_renderer, FONT_COLOR);

    if (!font_loaded)
        return SDL_APP_FAILURE;

    /* Initialize paddles */
    paddle_init(&game.left_paddle, PADDLE_PADDING, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);
    paddle_init(&game.right_paddle, WINDOW_WIDTH - PADDLE_WIDTH - PADDLE_PADDING, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);

    /* Initialize ball */
    ball_init(&game.ball, (WINDOW_WIDTH - BALL_SIZE) / 2, (WINDOW_HEIGHT - BALL_SIZE) / 2, BALL_SIZE, BALL_SIZE);

    /* Initialize snow sprite */
    if (!init_snow(&game.snow, game.window_renderer))
        return SDL_APP_FAILURE;

    /* Initialize theme */
    set_theme(&game);

    /* Notify the initial score */
#ifdef __EMSCRIPTEN__
    notify_game_state();
    notify_score_points();
#endif

    /* Initialize game mode to classic mode */
    game.mode = MODE_CLASSIC;
    game.score_to_win = CLASSIC_WIN_SCORE;

    game.state = GAME_INIT;
    game.resume_delay_time = 0.0f;
#ifndef BENCHMARK_MODE
    game.last_key_ticks = SDL_GetTicks();
    game.last_frame_ticks = SDL_GetTicks();
#endif
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    Game *game = (Game *)appstate;

#ifndef BENCHMARK_MODE
    handle_idle(game);
#endif

    float dt = calculate_delta_time(game);

    update_snow(&game->snow, WINDOW_WIDTH, WINDOW_HEIGHT, dt);

    render(game);

#ifdef __EMSCRIPTEN__
    notify_game_state();
    report_wasm_frame();
#endif

    /* Resuming the game after pause */
    if (game->state == GAME_RUNNING &&
            game->resume_delay_time > 0)
    {
        game->resume_delay_time -= dt;
        return SDL_APP_CONTINUE;
    }

    if (game->state != GAME_RUNNING)
        return SDL_APP_CONTINUE;

    paddle_move(&game->left_paddle, WINDOW_BORDER_OFFSET, WINDOW_HEIGHT - WINDOW_BORDER_OFFSET, dt);
    paddle_move(&game->right_paddle, WINDOW_BORDER_OFFSET, WINDOW_HEIGHT - WINDOW_BORDER_OFFSET, dt);

    CollisionType collision = ball_collision(&game->ball, &game->left_paddle, &game->right_paddle,
                                WINDOW_BORDER_OFFSET, WINDOW_WIDTH - WINDOW_BORDER_OFFSET,
                                WINDOW_BORDER_OFFSET, WINDOW_HEIGHT - WINDOW_BORDER_OFFSET,
                                dt);

    switch (collision)
    {
        case COLLISION_LEFT:
            score_points(game, false);
            reset_ball(&game->ball, false);
            break;
        case COLLISION_RIGHT:
            score_points(game, true);
            reset_ball(&game->ball, true);
            break;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    Game *game = (Game *)appstate;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
#ifndef BENCHMARK_MODE
            game->last_key_ticks = SDL_GetTicks();
#endif
            handle_key_down(game, event->key);
            break;
        case SDL_EVENT_KEY_UP:
#ifndef BENCHMARK_MODE
            game->last_key_ticks = SDL_GetTicks();
#endif
            handle_key_up(game, event->key);
            break;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    Game *game = (Game *)appstate;

    fprintf(stdout, INFO_TEXT " Quitting...\n");

    if (game->window_renderer)
        SDL_DestroyRenderer(game->window_renderer);

    if (game->window)
        SDL_DestroyWindow(game->window);

    if (game->snow.snow_sprite)
        SDL_DestroyTexture(game->snow.snow_sprite);

    destroy_text_texture(&game->welcome_text);
    destroy_text_texture(&game->welcome_description);
    destroy_text_texture(&game->paused_text);
    destroy_text_texture(&game->game_over_text);
    destroy_text_texture(&game->game_over_description);

    TTF_Quit();

    SDL_free(game->snow.snow_vertices);
    SDL_free(game->snow.snow_indices);
}

/* Emscripten bindings */

#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
void start_game(void)
{
    if (game_ptr && game_ptr->state == GAME_INIT)
        game_ptr->state = GAME_RUNNING;
}

EMSCRIPTEN_KEEPALIVE
void pause_game(void)
{
    if (game_ptr == NULL) return;

    if (game_ptr->state == GAME_RUNNING)
        game_ptr->state = GAME_PAUSED;
}

EMSCRIPTEN_KEEPALIVE
void resume_game(void)
{
    if (game_ptr == NULL) return;

    if (game_ptr->state != GAME_PAUSED) return;

    game_ptr->resume_delay_time = 0.25f;
    game_ptr->state = GAME_RUNNING;
}

EMSCRIPTEN_KEEPALIVE
void restart_game(void)
{
    if (game_ptr && game_ptr->state == GAME_OVER)
    {
        game_reset(game_ptr);
        game_ptr->state = GAME_RUNNING;
    }
}

EMSCRIPTEN_KEEPALIVE
void goto_menu(void)
{
    if (game_ptr == NULL) return;

    game_reset(game_ptr);
    game_ptr->state = GAME_INIT;
}

EMSCRIPTEN_KEEPALIVE
void toggle_mode(void)
{
    if (game_ptr && game_ptr->state == GAME_INIT)
        switch_mode(game_ptr);
}

EMSCRIPTEN_KEEPALIVE
void set_snowflake_count(int count)
{
    if (game_ptr == NULL) return;

#ifndef BENCHMARK_MODE
    game_ptr->last_key_ticks = SDL_GetTicks(); // Update the last key ticks to prevent the game from limiting the FPS
#endif

    increase_snow_count(&game_ptr->snow, count - game_ptr->snow.snowflake_count, WINDOW_WIDTH, WINDOW_HEIGHT);
    notify_snow_count_change();
}

EMSCRIPTEN_KEEPALIVE
int get_game_state(void)
{
    return game_ptr ? game_ptr->state : -1;
}

void notify_score_points(void)
{
    if (game_ptr == NULL) return;
    EM_ASM ({
        if (typeof window.updateScore === "function")
            window.updateScore($0, $1);
    }, game_ptr->left_score, game_ptr->right_score);
}

void notify_game_state(void)
{
    if (game_ptr == NULL) return;

    EM_ASM({
        if (typeof window.onGameStateChange === 'function')
            window.onGameStateChange($0);
    }, game_ptr->state);
}

void report_wasm_frame(void)
{
    if (game_ptr == NULL) return;
    EM_ASM ({
        if (typeof window.reportWasmFrame === "function")
            window.reportWasmFrame(performance.now());
    });
}

void notify_snow_count_change(void)
{
    if (game_ptr == NULL) return;
    EM_ASM ({
        if (typeof window.updateSnowflakeCount === "function")
            window.updateSnowflakeCount($0);
    }, game_ptr->snow.snowflake_count);
}

#endif // __EMSCRIPTEN__