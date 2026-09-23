#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "snow.h"
#include "ansi-color.h"

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

bool init_snow(Snow *snow, SDL_Renderer *renderer)
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

static void init_snowflake(Snowflake *snowflake, int idx, uint32_t w, uint32_t h)
{
    if (snowflake == NULL ||
        idx < 0 || idx >= MAX_SNOWFLAKES) return;

        /* Random position */
        snowflake->x[idx] = (float)(rand() % w);
        snowflake->y[idx] = (float)(rand() % h);

        /* Snowflake size, speed and opacity */
        snowflake->size[idx] = 1.0f + (rand() % 200) / 100.0f; // 1~3
        snowflake->speed[idx] = 0.5f + (rand() % 100) / 100.0f; // 0.5~1.5
        snowflake->opacity[idx] = 0.5f + (rand() % 50) / 100.0f; // 0.5~1.0
}

/* Increment update the snowflake array from start to end */
static void init_snowflake_inc(Snowflake *snowflake, int start_idx, int end_idx, uint32_t w, uint32_t h)
{
    if (snowflake == NULL
        || start_idx >= end_idx) return;

    while (start_idx <= end_idx)
    {
        init_snowflake(snowflake, start_idx, w, h);
        start_idx++;
    }
}

void increase_snow_count(Snow *snow, int increment, uint32_t w, uint32_t h)
{
    if (snow == NULL || increment == 0) return;

    if (snow->snowflake_count < MAX_SNOWFLAKES && increment > 0)
    {
        int old_count = snow->snowflake_count;

        snow->snowflake_count = (snow->snowflake_count + increment) <= MAX_SNOWFLAKES ?
            snow->snowflake_count + increment : MAX_SNOWFLAKES;

        init_snowflake_inc(&snow->snowflakes, old_count, snow->snowflake_count - 1, w, h);
    }

    if (snow->snowflake_count > 0 && increment < 0)
        snow->snowflake_count = (snow->snowflake_count + increment) >= 0 ?
            snow->snowflake_count + increment : 0;

    snow->num_vertices = snow->snowflake_count * 4;
    snow->num_indices = snow->snowflake_count * 6;
}

void update_snow(Snow *snow, uint32_t w, uint32_t h, float dt)
{
    static float wind_timer = 0;
    wind_timer += dt;
    float wind = sinf(wind_timer * 0.5f) * 0.3f;

    Snowflake *s = &snow->snowflakes;
    for (int i = 0; i < snow->snowflake_count; i++)
    {
        // Basic movement
        s->x[i] += wind * 60.0f * dt;
        s->y[i] += s->speed[i] * 60.0f * dt;
        // horizontal jitter
        s->x[i] += sinf(s->y[i] * 0.1f + i) * 0.2f * dt * 60.0f;
        // boundary loop: roll out of bottom or top, wrap around
        if (s->y[i] > h + 10)
        {
            s->y[i] = -10.0f;
            s->x[i] = (float)(rand() % w);
        }
        if (s->x[i] < -10) s->x[i] += w + 20;
        if (s->x[i] > w + 10) s->x[i] -= w + 20;
    }
}

/* Fun fact: This implemenation is pretty similar to the act of PS2 hardware, Emotion Engine (EE, CPU) + Graphics Synthesizer (GS, GPU) */
void render_snow(Snow *snow, SDL_Renderer *renderer)
{
    if (renderer == NULL || snow == NULL) return;
    if (snow->snow_sprite == NULL) return;

    SDL_Vertex *v = snow->snow_vertices;
    Snowflake *s = &snow->snowflakes;
    for (int i = 0; i < snow->snowflake_count; i++)
    {
        float w = s->size[i];
        float x = s->x[i], y = s->y[i];
        Uint8 alpha = (Uint8)(s->opacity[i] * 255);
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