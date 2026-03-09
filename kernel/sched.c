#include "sched.h"

static int task_is_runnable(int id) {
    return task_states[id] == TASK_READY || task_states[id] == TASK_RUNNING;
}

int sched_pick_next(int current) {
    int next = current;

    for (int i = 0; i < MAX_TASKS; i++) {
        next = (next + 1) % MAX_TASKS;
        if (task_is_runnable(next)) {
            return next;
        }
    }

    return -1;
}

void sched_prepare_switch(int next) {
    int prev = current_task;

    current_task = next;
    if (task_states[prev] == TASK_RUNNING) {
        task_states[prev] = TASK_READY;
    }
    task_states[current_task] = TASK_RUNNING;
}
