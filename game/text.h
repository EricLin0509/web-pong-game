#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#define FONT_PATH "roboto.ttf"
#define FONT_SIZE 64
#define FONT_COLOR (SDL_Color){255, 255, 255, 255}

typedef struct {
    SDL_Texture *text_texture;
    SDL_FRect text_rect;
} Text;

#endif // TEXT_H