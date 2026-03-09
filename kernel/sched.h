#ifndef SCHED_H
#define SCHED_H

#define TASK_READY    0
#define TASK_RUNNING  1
#define TASK_SLEEPING 2
#define TASK_DEAD     3

#define MAX_TASKS 4

extern int current_task;
extern int task_states[MAX_TASKS];

int sched_pick_next(int current);
void sched_prepare_switch(int next);

#endif
