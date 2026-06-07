#include <stdio.h>

#include "text.h"
#include "roboto.h" // embedded font
#include "ansi-color.h"

SDL_IOStream *load_font(void)
{
    return SDL_IOFromMem(roboto_ttf, roboto_ttf_len);
}

bool create_text_texture(Text *text, SDL_IOStream *stream, const char *str, int font_size,
                                                            SDL_Renderer *renderer, SDL_Color color)
{
    if (stream == NULL) return false; // Check if the stream is valid

    TTF_Font *font = TTF_OpenFontIO(stream, false, font_size);
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

void render_text_texture(Text *text, SDL_Renderer *renderer, float x, float y)
{
    if (text == NULL || renderer == NULL) return;

    text->text_rect.x = x;
    text->text_rect.y = y;

    SDL_RenderTexture(renderer, text->text_texture, NULL, &(text->text_rect));
}

void destroy_text_texture(Text *text)
{
    if (text == NULL) return;

    if (text->text_texture)
        SDL_DestroyTexture(text->text_texture);
}