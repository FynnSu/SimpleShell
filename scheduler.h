#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "pcb.h"

typedef enum // Possible scheduling policies
{
    FCFS,
    SJF,
    RR,
    AGING,
    NONE // Placeholder policy (used by run command)
} sched_mode_t;

void add_process(struct pcb *new_p);
void init_scheduler();
int set_scheduler_mode(sched_mode_t new_mode);
int run_scheduler();
int processes_waiting();
#endif