#include "scheduler.h"
#include <iostream>
#include <queue>

struct TaskOrder {
    bool operator()(const Task& a, const Task& b) const
    {
        return a.trigger_time > b.trigger_time; // The smaller trigger time comes first
    }
};

typedef struct Scheduler {
    std::priority_queue<Task, std::vector<Task>, TaskOrder> task_queue;
} Scheduler;

static inline void run_destroyer(const Task& task)
{
    if (task.destroyer)
        task.destroyer(task.user_data); // Destroy the user data if the destroyer exists
}

Scheduler *scheduler_create(void)
{
    return new Scheduler;
}

void scheduler_destroy(Scheduler **scheduler)
{
    if (!scheduler || !*scheduler) return;

    scheduler_clear_tasks(*scheduler); // Clear all tasks from the scheduler before destroying the scheduler
    delete *scheduler;
}

void scheduler_add_task(Scheduler *scheduler, Uint64 relative_time,
                      void (*callback)(void *arg), 
					  bool (*cancel_checker)(const Task *task), void (*cancel_callback)(void *user_data),
                      void (*destroyer)(void *user_data), void *user_data)
{
    if (!scheduler || !callback) return;

    Uint64 trigger_time = relative_time + SDL_GetTicks() + 1; // Calculate the absolute trigger time
    scheduler->task_queue.push(Task{trigger_time, callback, cancel_checker, cancel_callback, destroyer, user_data});
}

void scheduler_clear_tasks(Scheduler *scheduler)
{
    if (!scheduler) return;

    while (!scheduler->task_queue.empty())
    {
        const Task curr_task = scheduler->task_queue.top(); // Get the top task
        run_destroyer(curr_task); // Destroy the user data if the destroyer exists
        scheduler->task_queue.pop(); // Remove all tasks from the queue
    }
}

void scheduler_process_tasks(Scheduler *scheduler)
{
    if (!scheduler) return;

    while (!scheduler->task_queue.empty() &&
            scheduler->task_queue.top().trigger_time <= SDL_GetTicks())
    {
        const Task curr_task = scheduler->task_queue.top();
        bool should_cancel = curr_task.cancel_checker && curr_task.cancel_checker(&curr_task);
        scheduler->task_queue.pop();

        if (should_cancel)
        {
            if (curr_task.cancel_callback)
                curr_task.cancel_callback(curr_task.user_data); // Execute the cancel callback function
            continue;
        }

        if (curr_task.callback)
            curr_task.callback(curr_task.user_data); // Execute the callback function
    }
}
