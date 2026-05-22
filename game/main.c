#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

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
void notify_score_points(void);
void notify_game_over(void);
void is_game_running(void);
void report_wasm_frame(void);
void notify_snow_count_change(void);
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

    switch (game->mode)
    {
        case MODE_CLASSIC:
            game->mode = MODE_INFINITE;
            game->score_to_win = INFINITE_WIN_SCORE;
            break;
        case MODE_INFINITE:
            game->mode = MODE_CLASSIC;
            game->score_to_win = CLASSIC_WIN_SCORE;
            break;
    }
}

/* ===== Snow Functions ====== */

static bool init_snow_sprite(Snow *snow, SDL_Renderer *renderer)
{
    if (snow == NULL || renderer == NULL) return false;

    const int size = 32;
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create snow sprite surface: %s\n", SDL_GetError());
        return false;
    }

    // Draw simple circle
    Uint32 *pixels = surface->pixels;
    int pitch = surface->pitch;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            float dx = (x - size/2.0f) / (size/2.0f);
            float dy = (y - size/2.0f) / (size/2.0f);
            float dist = sqrtf(dx*dx + dy*dy);
            Uint8 alpha = 0;
            if (dist < 1.0f) 
                alpha = (Uint8)((1.0f - dist) * 255.0f); // Gradient from 0 to 255

            Uint32 *pixel = (Uint32*)((Uint8*)pixels + y * pitch + x * 4);
            *pixel = (alpha << 24) | (255 << 16) | (255 << 8) | 255;
        }
    }

    snow->snow_sprite = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    surface = NULL;
    if (snow->snow_sprite == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create snow sprite texture: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

static bool init_snow(Snow *snow, SDL_Renderer *renderer)
{
    if (snow == NULL || renderer == NULL) return false;
    if (!init_snow_sprite(snow, renderer)) return false;

    snow->snow_vertices = SDL_malloc(sizeof(SDL_Vertex) * MAX_SNOWFLAKES * 4);
    snow->snow_indices = SDL_malloc(sizeof(int) * MAX_SNOWFLAKES * 6);
    if (!snow->snow_vertices || !snow->snow_indices)
    {
        SDL_free(snow->snow_vertices);
        SDL_free(snow->snow_indices);
        return false;
    }
    // Calculate all snowflake indices because thay are static
    for (int i = 0; i < MAX_SNOWFLAKES; i++)
    {
        int base = i * 4;
        int idx = i * 6;

        /*
            the triangles look like this:
            0 ----- 1
            |        /  |
            |      /    |
            |    /      |
            |  /        |
            2 ----- 3

            the 0, 1, 2, 3 are the vertices

            these vertices are used to build two triangles:
            triangle 1: 0, 1, 2
            triangle 2: 1, 3, 2

            They share the (1->2) diagonal and form a rectangle
            with the same width and height as the snowflake size

            represent in memory: [ [0, 1, 2], [1, 3, 2] ]
        */

        snow->snow_indices[idx + 0] = base + 0;
        snow->snow_indices[idx + 1] = base + 1;
        snow->snow_indices[idx + 2] = base + 2;
        snow->snow_indices[idx + 3] = base + 1;
        snow->snow_indices[idx + 4] = base + 3;
        snow->snow_indices[idx + 5] = base + 2;
    }
    snow->num_vertices = 0;
    snow->num_indices  = 0;
    return true;
}

static void init_snowflake(Snowflake *snowflake)
{
    if (snowflake == NULL) return;

        /* Randomly position */
        snowflake->x = (float)(rand() % WINDOW_WIDTH);
        snowflake->y = (float)(rand() % WINDOW_HEIGHT);

        /* Randomly speed, size, opacity */
        snowflake->speed = 0.5f + (rand() % 100) / 100.0f; // 0.5~1.5
        snowflake->wind = 0.0f;
        snowflake->size = 1.0f + (rand() % 200) / 100.0f; // 1~3
        snowflake->opacity = 0.5f + (rand() % 50) / 100.0f; // 0.5~1.0
}

/* Increment update the snowflake array from start to end */
static void init_snowflake_inc(Snowflake *snowflake_start_ptr, Snowflake *snowflake_end_ptr)
{
    if (snowflake_start_ptr == NULL || snowflake_end_ptr == NULL
        || snowflake_start_ptr >= snowflake_end_ptr) return;

    Snowflake *update_region = snowflake_start_ptr;
    while (update_region <= snowflake_end_ptr)
    {
        init_snowflake(update_region);
        update_region++;
    }
}

static void increase_snow_count(Snow *snow, int increase)
{
    if (snow == NULL || increase == 0) return;

    if (snow->snowflake_count < MAX_SNOWFLAKES && increase > 0)
    {
        int old_count = snow->snowflake_count;

        snow->snowflake_count = (snow->snowflake_count + increase) <= MAX_SNOWFLAKES ?
            snow->snowflake_count + increase : MAX_SNOWFLAKES;

        Snowflake *start = snow->snowflakes + old_count;
        Snowflake *end = snow->snowflakes + snow->snowflake_count - 1;
        init_snowflake_inc(start, end);
    }

    if (snow->snowflake_count > 0 && increase < 0)
        snow->snowflake_count = (snow->snowflake_count + increase) >= 0 ?
            snow->snowflake_count + increase : 0;

    snow->num_vertices = snow->snowflake_count * 4;
    snow->num_indices = snow->snowflake_count * 6;
#ifdef __EMSCRIPTEN__
    notify_snow_count_change();
#endif
}

static void update_snow(Snow *snow, float dt)
{
    static float wind_timer = 0;
    wind_timer += dt;
    float wind = sinf(wind_timer * 0.5f) * 0.3f;

    for (int i = 0; i < snow->snowflake_count; i++)
    {
        Snowflake *s = &snow->snowflakes[i];
        // Basic movement
        s->y += s->speed * 60.0f * dt;
        s->x += wind * 60.0f * dt;
        // horizontal jitter
        s->x += sinf(s->y * 0.1f + i) * 0.2f * dt * 60.0f;
        // boundary loop: roll out of bottom or top, wrap around
        if (s->y > WINDOW_HEIGHT + 10)
        {
            s->y = -10.0f;
            s->x = (float)(rand() % WINDOW_WIDTH);
        }
        if (s->x < -10) s->x += WINDOW_WIDTH + 20;
        if (s->x > WINDOW_WIDTH + 10) s->x -= WINDOW_WIDTH + 20;
    }
}

/* Fun fact: This implemenation is pretty similar to the act of PS2 hardware, Emotion Engine (EE, CPU) + Graphics Synthesizer (GS, GPU) */
static void render_snow(Snow *snow, SDL_Renderer *renderer)
{
    if (renderer == NULL || snow == NULL) return;
    if (snow->snow_sprite == NULL) return;

    const int snow_count = snow->snowflake_count;

    SDL_Vertex *v = snow->snow_vertices;
    for (int i = 0; i < snow_count; i++)
    {
        Snowflake *s = &snow->snowflakes[i];
        float w = s->size;
        float x = s->x, y = s->y;
        Uint8 alpha = (Uint8)(s->opacity * 255);
        SDL_FColor color = { 1.0f, 1.0f, 1.0f, alpha / 255.0f };

        // The positions of the four corners of the snowflake rectangle
        SDL_FPoint positions[4] = {
            { x - w, y - w },
            { x + w, y - w },
            { x - w, y + w },
            { x + w, y + w }
        };

        /*
            The texture coordinates of the vertices
            This defines how the texture is mapped onto rectangle

            vertex 0: (0, 0)
            vertex 1: (1, 0)
            vertex 2: (0, 1)
            vertex 3: (1, 1)

            This makes the actual memory look like: 
                [ [(0, 0), (1, 0), (1, 0)],     // triangle 1
                  [(1, 0), (1, 1), (0, 1)] ]    // triangle 2
        */
        SDL_FPoint uvs[4] = {
            { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 }
        };

        for (int j = 0; j < 4; j++)
        {
            v->position = positions[j];
            v->color = color;
            v->tex_coord = uvs[j];
            v++;
        }
    }

    SDL_RenderGeometry(renderer, snow->snow_sprite,
                       snow->snow_vertices, snow->num_vertices,
                       snow->snow_indices,  snow->num_indices);
}

/* ====== Text Functions ====== */

static bool create_text_texture(Text *text, const char *font_path, const char *str, int font_size,
                                                                    SDL_Renderer *renderer, SDL_Color color)
{
    TTF_Font *font = TTF_OpenFont(font_path, font_size);
    if (font == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to load font: %s\n", SDL_GetError());
        return false;
    }

    SDL_Surface *surface = TTF_RenderText_Blended(font, str, 0, color);
    TTF_CloseFont(font); // Free the font
    font = NULL;
    if (surface == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to render text: %s\n", SDL_GetError());
        return false;
    }

    text->text_rect.w = surface->w;
    text->text_rect.h = surface->h;

    text->text_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface); // Free the surface
    surface = NULL;
    if (text->text_texture == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create texture: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static void render_text_texture(Text *text, SDL_Renderer *renderer, float x, float y)
{
    if (text == NULL || renderer == NULL) return;

    text->text_rect.x = x;
    text->text_rect.y = y;

    SDL_RenderTexture(renderer, text->text_texture, NULL, &(text->text_rect));
}

static void destroy_text_texture(Text *text)
{
    if (text == NULL) return;

    if (text->text_texture)
        SDL_DestroyTexture(text->text_texture);
}

/* ====== Paddle Functions ====== */

static void paddle_init(Paddle *paddle, int x, int y, int width, int height)
{
    if (paddle == NULL) return;
    
    paddle->paddle_rect.x = x;
    paddle->paddle_rect.y = y;
    paddle->paddle_rect.w = width;
    paddle->paddle_rect.h = height;
    paddle->direction = PADDLE_STOP;
}

static void paddle_move(Paddle *paddle, float dt)
{
    if (paddle == NULL) return;

    float actual_distance = PADDLE_SPEED_PER_SEC * dt;

    switch (paddle->direction)
    {
        case PADDLE_UP:
            if (paddle->paddle_rect.y - actual_distance > WINDOW_BORDER_OFFSET)
                paddle->paddle_rect.y -= actual_distance;
            break;
        case PADDLE_DOWN:
            if (paddle->paddle_rect.y + actual_distance < WINDOW_HEIGHT - WINDOW_BORDER_OFFSET - PADDLE_HEIGHT)
                paddle->paddle_rect.y += actual_distance;
            break;
        case PADDLE_STOP:
        default:
            break;
    }
}

static void render_paddle(Paddle *paddle, SDL_Renderer *renderer)
{
    if (paddle == NULL || renderer == NULL) return;

    SDL_RenderFillRect(renderer, &(paddle->paddle_rect));
}

static void reset_paddle(Paddle *paddle)
{
    if (paddle == NULL) return;

    /* Only reset the y position */
    paddle->paddle_rect.y = (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2;

    paddle->direction = PADDLE_STOP;
}

/* ====== Ball Functions ====== */

static void ball_init(Ball *ball, int x, int y, int width, int height)
{
    if (ball == NULL) return;

    ball->rect.x = x;
    ball->rect.y = y;
    ball->rect.w = width;
    ball->rect.h = height;
    
    ball->speed_x = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED_PER_SEC : INITIAL_SPEED_PER_SEC;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED_PER_SEC : INITIAL_SPEED_PER_SEC;
}

static void reset_ball(Ball *ball, bool left_serve)
{
    if (ball == NULL) return;

    ball->rect.x = (WINDOW_WIDTH - BALL_SIZE) / 2;
    ball->rect.y = (WINDOW_HEIGHT - BALL_SIZE) / 2;
    float speed_x = INITIAL_SPEED_PER_SEC;
    ball->speed_x = (left_serve) ? speed_x : -speed_x;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? INITIAL_SPEED_PER_SEC : -INITIAL_SPEED_PER_SEC;
}

static void speed_up_ball(Ball *ball)
{
    if (ball == NULL) return;

    if (ball->speed_x <= MAX_SPEED)
        ball->speed_x *= SPEED_UP_RATE;
    if (ball->speed_y <= MAX_SPEED)
        ball->speed_y *= SPEED_UP_RATE;
}

static CollisionType ball_collision(Ball *ball, Paddle *left_paddle, Paddle *right_paddle, float dt)
{
    if (ball == NULL || left_paddle == NULL || right_paddle == NULL) return COLLISION_NONE;

    // First handle the score points
    if (ball->rect.x < WINDOW_BORDER_OFFSET)
    {
        reset_ball(ball, true);
        return COLLISION_LEFT;
    }
    else if (ball->rect.x > WINDOW_WIDTH - WINDOW_BORDER_OFFSET - BALL_SIZE)
    {
        reset_ball(ball, false);
        return COLLISION_RIGHT;
    }

    // Bounce off the top and bottom walls
    float new_y = ball->rect.y + ball->speed_y * dt;
    if (new_y < WINDOW_BORDER_OFFSET || new_y > WINDOW_HEIGHT - WINDOW_BORDER_OFFSET - BALL_SIZE)
    {
        ball->speed_y *= -1;
        new_y = (new_y < WINDOW_BORDER_OFFSET) ? WINDOW_BORDER_OFFSET 
                : WINDOW_HEIGHT - WINDOW_BORDER_OFFSET - BALL_SIZE;
    }

    // Predict the ball's next position
    float new_x = ball->rect.x + ball->speed_x * dt;
    SDL_FRect next_rect = { new_x, new_y, ball->rect.w, ball->rect.h };

    Paddle *chosen_paddle = (new_x < WINDOW_WIDTH / 2) ? left_paddle : right_paddle;

    if (SDL_HasRectIntersectionFloat(&next_rect, &(chosen_paddle->paddle_rect)))
    {
        ball->speed_x = -(ball->speed_x + JITTER_PER_SEC);
        if (ball->speed_x > 0)
            new_x = chosen_paddle->paddle_rect.x + chosen_paddle->paddle_rect.w;
        else
            new_x = chosen_paddle->paddle_rect.x - BALL_SIZE;
        speed_up_ball(ball);
    }

    // 应用最终位置
    ball->rect.x = new_x;
    ball->rect.y = new_y;

    return COLLISION_NONE;
}

static void render_ball(Ball *ball, SDL_Renderer *renderer)
{
    if (ball == NULL || renderer == NULL) return;

    SDL_RenderFillRect(renderer, &(ball->rect));
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
            increase_snow_count(&game->snow, 500);
            break;
        case SDL_SCANCODE_V:
            increase_snow_count(&game->snow, -500);
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
    {
        game->state = GAME_OVER;

#ifdef __EMSCRIPTEN__
        notify_game_over();
#endif
    }

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
    font_loaded &= create_text_texture(&game.welcome_description, FONT_PATH, "Press [space] to start", FONT_SIZE - 32, game.window_renderer, FONT_COLOR);
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

    update_snow(&game->snow, dt);

    render(game);

#ifdef __EMSCRIPTEN__
    is_game_running();
    report_wasm_frame();
#endif

    /* Resuming the game after pause */
    if (game->state == GAME_RUNNING &&
            game->resume_delay_time > 0)
    {
        game->resume_delay_time -= dt;
        return SDL_APP_CONTINUE;
    }
    else game->resume_delay_time = 0.0f;

    if (game->state != GAME_RUNNING)
        return SDL_APP_CONTINUE;

    paddle_move(&game->left_paddle, dt);
    paddle_move(&game->right_paddle, dt);

    CollisionType collision = ball_collision(&game->ball, &game->left_paddle, &game->right_paddle, dt);
    switch (collision)
    {
        case COLLISION_LEFT:
            score_points(game, false);
            break;
        case COLLISION_RIGHT:
            score_points(game, true);
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

    SDL_free(game->snow.snow_vertices);
    SDL_free(game->snow.snow_indices);
}

/* Emscripten bindings */

#ifdef __EMSCRIPTEN__

EMSCRIPTEN_KEEPALIVE
void pause_game(void)
{
    if (game_ptr == NULL) return;

    if (game_ptr->state == GAME_RUNNING)
        game_ptr->state = GAME_PAUSED;
}

EMSCRIPTEN_KEEPALIVE
void set_snowflake_count(int count)
{
    if (game_ptr == NULL) return;

    game_ptr->last_key_ticks = SDL_GetTicks(); // Update the last key ticks to prevent the game from limiting the FPS

    increase_snow_count(&game_ptr->snow, count - game_ptr->snow.snowflake_count);
}

void notify_score_points(void)
{
    if (game_ptr == NULL) return;
    EM_ASM ({
        if (typeof window.updateScore === "function")
            window.updateScore($0, $1);
    }, game_ptr->left_score, game_ptr->right_score);
}

void notify_game_over(void)
{
    if (game_ptr == NULL) return;
    EM_ASM({
        if (typeof window.onGameOver === 'function')
                window.onGameOver($0, $1);

    }, game_ptr->left_score, game_ptr->right_score);
}

void is_game_running(void)
{
    if (game_ptr == NULL) return;
    EM_ASM ({
        if (typeof window.isGameRunning === "function")
            window.isGameRunning($0);
    }, game_ptr->state == GAME_RUNNING || game_ptr->state == GAME_PAUSED);
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