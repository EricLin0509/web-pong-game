#include "scheduler.h"
#include "game_events.h"

static void game_resume(void *arg)
{
    if (!arg) return;
    Game *game = (Game *)arg;
    game->state = GAME_RUNNING;
}

void game_resuming(void *arg)
{
    if (!arg) return;
    Game *game = (Game *)arg;
    game->state = GAME_RESUMING;

    scheduler_add_task(game->scheduler, 500, game_resume, NULL, NULL, NULL, game);
}

static void insert_idle_task(Game *game)
{
    if (!game) return;

    scheduler_add_task(game->scheduler, 
                    IDLE_TIMEOUT_MS, 
                    idle_timeout, 
                    idle_timeout_cancel, 
                    idle_timeout_cancel_callback, 
                    NULL, 
                    game);
}

void idle_timeout(void *arg)
{
    if (!arg) return;
    Game *game = arg;

    game->mode_flags |= IDLE_MASK; // Set the refresh suspended flag

    /* Add the task back to the scheduler */
    insert_idle_task(game);
}

bool idle_timeout_cancel(const Task *task)
{
    if (!task) return true;
    Game *game = task->user_data;

    bool is_canceled = game->idle_token != game->last_idle_token; // Check if the token matches the current token
    
    return is_canceled;
}

void idle_timeout_cancel_callback(void *arg)
{
    if (!arg) return;

    Game *game = arg;
    game->last_idle_token = game->idle_token; // Update the last idle token

    /* Add the task back to the scheduler */
    insert_idle_task(game);
}
