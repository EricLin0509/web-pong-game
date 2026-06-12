#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include <stdlib.h>

typedef float (*ai_difficulty_func) (void);
typedef float (*initial_speed_generator) (void);

/*
  The difficulty levels are defined here
  {id, ai_min, ai_max, ball_min, ball_max, paddle_scale}
*/
#define LIST_OF_DIFFICULTIES \
    DIFFICULTY(EASY, -80, 80, 350, 400, 0.8f) \
    DIFFICULTY(MEDIUM, -40, 40, 400, 500, 1.0f) \
    DIFFICULTY(HARD, -10, 10, 500, 650, 1.2f)

/* The enum of difficulty levels */
#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) DIFFICULTY_##id,
typedef enum {
    LIST_OF_DIFFICULTIES
    NUM_DIFFICULTIES
} Difficulty;
#undef DIFFICULTY

/* The paddle move scales for each difficulty level */
#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) paddle_scale,
static const float paddle_move_scales[NUM_DIFFICULTIES] = {
    LIST_OF_DIFFICULTIES
};
#undef DIFFICULTY

/* The ai difficulty functions for each difficulty level */
#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) \
    static inline float ai_##id##_difficulty(void) { \
        return ai_min + ((float)rand() / (float)RAND_MAX) * (ai_max - ai_min); \
    }
LIST_OF_DIFFICULTIES
#undef DIFFICULTY

#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) ai_##id##_difficulty,
static const ai_difficulty_func ai_difficulties[NUM_DIFFICULTIES] = {
    LIST_OF_DIFFICULTIES
};
#undef DIFFICULTY

/* The ball initial speed generator for each difficulty level */
#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) \
    static inline float ball_##id##_generator(void) { \
        return ball_min + ((float)rand() / (float)RAND_MAX) * (ball_max - ball_min); \
    }
LIST_OF_DIFFICULTIES
#undef DIFFICULTY

#define DIFFICULTY(id, ai_min, ai_max, ball_min, ball_max, paddle_scale) ball_##id##_generator,
static const initial_speed_generator ball_speed_generators[NUM_DIFFICULTIES] = {
    LIST_OF_DIFFICULTIES
};
#undef DIFFICULTY

#endif // DIFFICULTY_H