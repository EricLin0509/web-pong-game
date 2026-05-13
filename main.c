#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "ansi-color.h"

#include "text.h"
#include "paddle.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "Pong"

typedef enum {
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_QUIT
} GameState;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *window_renderer;

    Paddle left_paddle;
    Paddle right_paddle;

    Text paused_text;

    GameState state;
} Game;

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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &(paddle->paddle_rect));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

/*====== Game Logics ======*/

static bool game_init(Game *game)
{
    if (game == NULL) return false;

    SDL_Init(SDL_INIT_VIDEO);

    if (!TTF_Init())
    {
        fprintf(stderr, ERROR_TEXT " Failed to initialize TTF: %s\n", SDL_GetError());
        return false;
    }

    /* Initialize window and renderer */
    game->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (game->window == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create window: %s\n", SDL_GetError());
        return false;
    }

    game->window_renderer = SDL_CreateRenderer(game->window, NULL);
    if (game->window_renderer == NULL)
    {
        fprintf(stderr, ERROR_TEXT " Failed to create renderer: %s\n", SDL_GetError());
        return false;
    }

    /* Load font */
    create_text_texture(&game->paused_text, FONT_PATH, "Paused", FONT_SIZE, game->window_renderer, FONT_COLOR);

    /* Initialize paddles */
    paddle_init(&game->left_paddle, 0, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);
    paddle_init(&game->right_paddle, WINDOW_WIDTH - PADDLE_WIDTH, (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2, PADDLE_WIDTH, PADDLE_HEIGHT);

    game->state = GAME_RUNNING;
    return true;
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

static void handle_key_down(Game *game, SDL_KeyboardEvent key)
{
    if (game == NULL) return;

    switch (key.scancode)
    {
        /* Quit game */
        case SDL_SCANCODE_ESCAPE:
            game->state = GAME_QUIT;
            break;
        /* Pause game */
        case SDL_SCANCODE_SPACE:
            game->state = (game->state == GAME_RUNNING) ?  GAME_PAUSED : GAME_RUNNING;
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

static void handle_events(Game *game)
{
    if (game == NULL) return;

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                game->state = GAME_QUIT;
                break;
            case SDL_EVENT_KEY_DOWN:
                handle_key_down(game, event.key);
                break;
            case SDL_EVENT_KEY_UP:
                handle_key_up(game, event.key);
                break;
            default:
                break;
        }
    }
}

static void render(Game *game)
{
    SDL_RenderClear(game->window_renderer);

    if (game->state == GAME_PAUSED)
        render_text_texture(&game->paused_text, game->window_renderer,
                             (WINDOW_WIDTH - game->paused_text.text_rect.w) / 2,
                             (WINDOW_HEIGHT - game->paused_text.text_rect.h) / 2 - 100);


    render_paddle(&game->left_paddle, game->window_renderer);
    render_paddle(&game->right_paddle, game->window_renderer);

    SDL_RenderPresent(game->window_renderer);
}

static void game_run(Game *game)
{
    if (game->state == GAME_PAUSED)
        return;

    paddle_move(&game->left_paddle);
    paddle_move(&game->right_paddle);
}

static void game_loop(Game *game)
{
    if (game == NULL) return;

    while (game->state != GAME_QUIT)
    {
        handle_events(game);

        game_run(game);

        render(game);

        SDL_Delay(16);
    }
}

static void game_quit(Game *game)
{
    if (game == NULL) return;

    fprintf(stdout, INFO_TEXT "Quitting...\n");

    if (game->window_renderer)
        SDL_DestroyRenderer(game->window_renderer);

    if (game->window)
        SDL_DestroyWindow(game->window);

    destroy_text_texture(&game->paused_text);
    
    SDL_Quit();
}

int main(void) {
    Game game;

    if (!game_init(&game))
        return 1;

    game_loop(&game);

    game_quit(&game);

    return 0;
}