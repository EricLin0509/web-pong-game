#ifndef GAME_EVENTS
#define GAME_EVENTS

#include "game.h"

void game_resuming(void *arg);

void idle_timeout(void *arg);

bool idle_timeout_cancel(const Task *task);

void idle_timeout_cancel_callback(void *user_data);

#endif // GAME_EVENTS