#ifndef PARTICLE_H
#define PARTICLE_H

#define MAX_SNOWFLAKES  80000
#define DEFAULT_SNOWFLAKES 8000

typedef struct {
    float x, y; // position
    float speed; // fall speed
    float wind; // wind speed
    float size; // size
    float opacity; // opacity
} Snowflake;

typedef struct {
    Snowflake snowflakes[MAX_SNOWFLAKES];
    int snowflake_count;
    float global_speed; // global speed
    SDL_Texture *snow_texture;
} Snow;

#endif // PARTICLE_H