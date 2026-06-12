#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define FONT_COLOR (SDL_Color){255, 255, 255, 255}

typedef struct {
    SDL_Texture *text_texture;
    SDL_FRect text_rect;
} Text;

SDL_IOStream *load_font(void);

bool create_text_texture(Text *text, SDL_IOStream *stream, const char *str, int font_size,
                                                            SDL_Renderer *renderer, SDL_Color color);

void render_text_texture(Text *text, SDL_Renderer *renderer, float x, float y);

void destroy_text_texture(Text *text);

#endif // TEXT_H