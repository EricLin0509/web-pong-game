#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "ansi-color.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "Pong"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *window_renderer;
    bool is_running;
} Game;

static bool game_init(Game *game)
{
    if (game == NULL) return false;

    SDL_Init(SDL_INIT_VIDEO);

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

    game->is_running = true;
    return true;
}

static void handle_key_down(Game *game, SDL_Event event)
{
    if (game == NULL) return;

    switch (event.key.scancode)
    {
        /* Quit game */
        case SDL_SCANCODE_ESCAPE:
            game->is_running = false;
            break;
        /* Pause game */
        case SDL_SCANCODE_SPACE:
            break;

        /* Right player paddle */
        case SDL_SCANCODE_I:
            break;
        case SDL_SCANCODE_K:
            break;

        /* Left player paddle */
        case SDL_SCANCODE_W:
            break;
        case SDL_SCANCODE_S:
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
                game->is_running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                handle_key_down(game, event);
                break;
            default:
                break;
        }
    }
}

static void render(Game *game)
{
    SDL_RenderClear(game->window_renderer);
    SDL_RenderPresent(game->window_renderer);
}

static void game_loop(Game *game)
{
    if (game == NULL) return;

    while (game->is_running)
    {
        handle_events(game);

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