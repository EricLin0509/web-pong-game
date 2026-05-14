#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "ansi-color.h"

#include "text.h"
#include "paddle.h"
#include "ball.h"

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 650
#define WINDOW_TITLE "Pong"
#define LINE_WIDTH 2

#define MAX_SCORE_POINTS 11

typedef enum {
    GAME_INIT,
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_OVER
} GameState;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *window_renderer;
    SDL_FRect middle_line;

    Paddle left_paddle;
    Paddle right_paddle;

    Ball ball;

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
} Game;

static Game game = {0};

/* ====== Text Functions ====== */

static void create_text_texture(Text *text, const char *font_path, const char *str, int font_size,
                                                                    SDL_Renderer *renderer, SDL_Color color)
{
    TTF_Font *font = TTF_OpenFont(font_path, font_size);
    if (font == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to load font: %s\n", SDL_GetError());
        return;
    }

    SDL_Surface *surface = TTF_RenderText_Blended(font, str, 0, color);
    TTF_CloseFont(font); // Free the font
    font = NULL;
    if (surface == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to render text: %s\n", SDL_GetError());
        return;
    }

    text->text_rect.w = surface->w;
    text->text_rect.h = surface->h;

    text->text_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface); // Free the surface
    surface = NULL;
    if (text->text_texture == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create texture: %s\n", SDL_GetError());
        return;
    }
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

static void paddle_move(Paddle *paddle)
{
    if (paddle == NULL) return;

    switch (paddle->direction)
    {
        case PADDLE_UP:
            if (paddle->paddle_rect.y > 0)
                paddle->paddle_rect.y -= PADDLE_SPEED;
            break;
        case PADDLE_DOWN:
            if (paddle->paddle_rect.y < WINDOW_HEIGHT - PADDLE_HEIGHT)
                paddle->paddle_rect.y += PADDLE_SPEED;
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
    
    ball->speed_x = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED : INITIAL_SPEED;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? -INITIAL_SPEED : INITIAL_SPEED;
}

static void reset_ball(Ball *ball, bool left_serve)
{
    if (ball == NULL) return;

    ball->rect.x = (WINDOW_WIDTH - BALL_SIZE) / 2;
    ball->rect.y = (WINDOW_HEIGHT - BALL_SIZE) / 2;
    float speed_x = INITIAL_SPEED;
    ball->speed_x = (left_serve) ? speed_x : -speed_x;
    ball->speed_y = (SHOUD_MOVE_REVERSE) ? INITIAL_SPEED : -INITIAL_SPEED;
}

static void speed_up_ball(Ball *ball)
{
    if (ball == NULL) return;

    if (ball->speed_x <= MAX_SPEED)
        ball->speed_x *= SPEED_UP_RATE;
    if (ball->speed_y <= MAX_SPEED)
        ball->speed_y *= SPEED_UP_RATE;
}

static CollisionType ball_collision(Ball *ball, Paddle *left_paddle, Paddle *right_paddle)
{
    if (ball == NULL || left_paddle == NULL || right_paddle == NULL) return COLLISION_NONE;

    if (ball->rect.x < 0)
    {
        reset_ball(ball, true);
        return COLLISION_LEFT;
    }
    else if (ball->rect.x > WINDOW_WIDTH - BALL_SIZE)
    {
        reset_ball(ball, false);
        return COLLISION_RIGHT;
    }

    if (ball->rect.y + ball->speed_y < 0 || ball->rect.y + ball->speed_y > WINDOW_HEIGHT - BALL_SIZE)
        ball->speed_y *= -1;

    Paddle *chosen_paddle = (ball->rect.x < WINDOW_WIDTH / 2) ? left_paddle : right_paddle;

    if (SDL_HasRectIntersectionFloat(&(ball->rect), &(chosen_paddle->paddle_rect)))
    {
        ball->speed_x = (ball->speed_x + JITTER) * -1;
        if (ball->speed_x > 0)
            ball->rect.x =chosen_paddle->paddle_rect.x + chosen_paddle->paddle_rect.w;
        else
            ball->rect.x =chosen_paddle->paddle_rect.x - BALL_SIZE;
        speed_up_ball(ball);
    }

    ball->rect.x += ball->speed_x;
    ball->rect.y += ball->speed_y;

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
    game->rounds = 0;
    game->left_score = 0;
    game->right_score = 0;
    reset_ball(&game->ball, SHOUD_MOVE_REVERSE);
    reset_paddle(&game->left_paddle);
    reset_paddle(&game->right_paddle);
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
            SDL_Delay(256); // Wait for a while before resuming the game
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
                             10,
                             (WINDOW_HEIGHT - game->paused_text.text_rect.h) - 10);
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

    SDL_SetRenderDrawColor(game->window_renderer, 255, 255, 255, 255);

    switch (game->state)
    {
        case GAME_INIT:
            show_welcome_screen(game);
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
    SDL_SetRenderDrawColor(game->window_renderer, 0, 0, 0, 255);

    SDL_RenderPresent(game->window_renderer);
    SDL_Delay(16); // Limit the frame rate to 60 FPS
}

static void score_points(Game *game, bool is_left_score)
{
    if (game == NULL) return;

    game->rounds++;

    size_t *score = (is_left_score) ? &game->left_score : &game->right_score;
    (*score)++;

    if (*score >= game->max_score)
        game->max_score = *score;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    if (!TTF_Init())
    {
        fprintf(stderr, ERROR_TEXT " Failed to initialize TTF: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Initialize window and renderer */
    game.window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
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

    /* Initialize middle line */
    game.middle_line.x = (WINDOW_WIDTH - LINE_WIDTH) / 2;
    game.middle_line.y = 0;
    game.middle_line.w = LINE_WIDTH;
    game.middle_line.h = WINDOW_HEIGHT;

    /* Load font */
    create_text_texture(&game.welcome_text, FONT_PATH, "Welcome to Pong", FONT_SIZE, game.window_renderer, FONT_COLOR);
    create_text_texture(&game.welcome_description, FONT_PATH, "Press [space] to start", FONT_SIZE - 32, game.window_renderer, FONT_COLOR);

    create_text_texture(&game.paused_text, FONT_PATH, "Paused", FONT_SIZE, game.window_renderer, FONT_COLOR);

    create_text_texture(&game.game_over_text, FONT_PATH, "WIN", FONT_SIZE, game.window_renderer, FONT_COLOR);
    create_text_texture(&game.game_over_description, FONT_PATH, "Press [space] to restart", FONT_SIZE - 36, game.window_renderer, FONT_COLOR);

    /* Initialize paddles */
    paddle_init(&game.left_paddle, PADDLE_PADDING, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);
    paddle_init(&game.right_paddle, WINDOW_WIDTH - PADDLE_WIDTH - PADDLE_PADDING, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);

    /* Initialize ball */
    ball_init(&game.ball, (WINDOW_WIDTH - BALL_SIZE) / 2, (WINDOW_HEIGHT - BALL_SIZE) / 2, BALL_SIZE, BALL_SIZE);

    game.state = GAME_INIT;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    render(&game);

    if (game.state != GAME_RUNNING)
        return SDL_APP_CONTINUE;

    paddle_move(&game.left_paddle);
    paddle_move(&game.right_paddle);

    CollisionType collision = ball_collision(&game.ball, &game.left_paddle, &game.right_paddle);
    switch (collision)
    {
        case COLLISION_LEFT:
            score_points(&game, false);
            break;
        case COLLISION_RIGHT:
            score_points(&game, true);
            break;
        case COLLISION_NONE:
        default:
            break;
    }    

    if (game.max_score == MAX_SCORE_POINTS)
        game.state = GAME_OVER;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            handle_key_down(&game, event->key);
            break;
        case SDL_EVENT_KEY_UP:
            handle_key_up(&game, event->key);
            break;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    fprintf(stdout, INFO_TEXT "Quitting...\n");

    if (game.window_renderer)
        SDL_DestroyRenderer(game.window_renderer);

    if (game.window)
        SDL_DestroyWindow(game.window);

    destroy_text_texture(&game.welcome_text);
    destroy_text_texture(&game.welcome_description);
    destroy_text_texture(&game.paused_text);
    destroy_text_texture(&game.game_over_text);
    destroy_text_texture(&game.game_over_description);
}