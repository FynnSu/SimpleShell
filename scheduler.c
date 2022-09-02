#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "pcb.h"
#include "shellmemory.h"
#include "shell.h"

#define RR_PREEMPT_FREQ 2 // Number of lines to run before preempt for Round robin policy

struct ll // linked list representing process waiting queue
{
    struct pcb *p;
    int priority;
    struct ll *next;
};

struct scheduler_state // State of Scheduler
{
    int np;                 // Number of processes currently running (includes current process and all processes in queue)
    struct ll *head, *tail; // head and tail pointers of linked list
    struct pcb *cur;        // Current running process (note: this process is popped from queue while it is running)
    int cur_priority;       // Priority of the current process
    sched_mode_t mode;      // Current scheduling policy
} state;

// Error functions
void error_too_many_processes();
void error_process_not_found();
void error_bad_mode_switch();
void error_no_mode_selected();

// private functions
void exec_process();
void run_AGING();
void run_RR();
void run_basic();

// Linked List Funcs
void add_with_priority(struct pcb *data, int priority);
void add_back(struct pcb *data);
void pop_front();
void decr_priorities();

/*
 * Function:  init_scheduler
 * --------------------
 * Initialize the scheduler state.
 *
 * int framenum: frame to find
 */
void init_scheduler()
{
    state.np = 0;
    state.head = NULL;
    state.tail = NULL;
    state.cur = NULL;
    state.cur_priority = 0;
    state.mode = NONE;
}

/*
 * Function:  processes_waiting
 * --------------------
 * Indicates if there are processes waiting to be run by the scheduler
 *
 * returns (int): Indicator (1 if processes are waiting, 0 otherwise)
 */
int processes_waiting()
{
    return state.np > 0;
}

/*
 * Function:  set_scheduler_mode
 * --------------------
 * Attempts to switch the scheduler policy to new policy.
 * There must be no processes running to switch scheduler mode
 *
 * returns (int): Indicator (0 on success, 1 on failure)
 */
int set_scheduler_mode(sched_mode_t new_mode)
{
    if (state.mode == new_mode) // If new mode same as old mode, no change made
        return 0;

    if (state.np != 0) // There is a non-zero number of processes currently running, can't change policy
    {
        error_bad_mode_switch();
        return 1;
    }

    state.mode = new_mode;
    return 0;
}

/*
 * Function:  add_back
 * --------------------
 * Add a new process pcb to the back of the running queue. Creates new linked list
 * struct to store pcb. Used by RR and FCFS policies.
 * Used for regular queue operations.
 * Operation is O(1)
 *
 * Note: should not be used in conjuction with add_with_priority since this function assigns a non-sense priority value
 *
 * struct pcb *data: pcb of process to add to queue
 */
void add_back(struct pcb *data)
{
    if (data == NULL)
    {
        return; // Bad pcb as input (this should never happen)
    }
    struct ll *newNode = malloc(sizeof(struct ll));
    newNode->p = data;
    newNode->next = NULL;
    newNode->priority = -1;
    if (state.head == NULL && state.tail == NULL)
    {
        state.head = newNode;
        state.tail = newNode;
    }
    else if (state.head == NULL || state.tail == NULL)
    {
        // Unstable linked list state (should never happen)
        exit(3);
    }
    else
    {
        state.tail->next = newNode;
        state.tail = newNode;
    }
}

/*
 * Function:  add_with_priority
 * --------------------
 * Adds a new process pcb into it's correct location in the priority running queue. Creates a new linked list
 * struct to store the pcb and set's is priority to the given priority value.
 * Used for priority queue operations.
 * Operation is O(n)
 *
 * Note: should not be used in conjunction with add_back since this function assumes the queue is a sorted priority queue
 *
 * struct pcb *data: pcb of process to add to queue
 * int priority: priority of added process
 */
void add_with_priority(struct pcb *data, int priority)
{
    if (data == NULL)
    {
        printf("What\n");
    }
    struct ll *newNode = malloc(sizeof(struct ll));
    newNode->p = data;
    newNode->next = NULL;
    newNode->priority = priority;

    if (state.head == NULL)
    {
        state.head = newNode;
        return;
    }

    if (priority < state.head->priority)
    {
        newNode->next = state.head;
        state.head = newNode;
        return;
    }

    struct ll *cur = state.head;
    while (cur->next != NULL && cur->next->priority <= priority)
    {
        cur = cur->next;
    }

    newNode->next = cur->next;
    cur->next = newNode;
    if (cur == state.tail)
        state.tail = newNode;
}

/*
 * Function:  decr_priorities
 * --------------------
 * Decrements the priority of all processes in the waiting queue.
 * Operation takes O(n) time.
 */
void decr_priorities()
{
    struct ll *cur = state.head;

    while (cur != NULL)
    {
        if (cur->priority > 0)
            cur->priority--;

        cur = cur->next;
    }
}

/*
 * Function:  pop_front
 * --------------------
 * Removes the head process from the waiting queue and sets it as the current running process.
 * Operation takes O(1) time
 */
void pop_front()
{
    if (state.head == NULL)
    {
        error_process_not_found();
        return;
    }
    struct ll *curHead = state.head;
    state.head = curHead->next;
    state.cur = curHead->p;
    state.cur_priority = curHead->priority;
    if (state.tail == curHead)
        state.tail = NULL;
    free(curHead);
}

void error_process_not_found()
{
    printf("Error: Expected a process on queue but found none!\n");
}

void error_too_many_processes()
{
    printf("Error: Attempted to launch too many concurrent processes.\n");
}

void error_bad_mode_switch()
{
    printf("Error: Attempted to switch mode while processes are running.\n");
}

void error_no_mode_selected()
{
    printf("Error: You must selected a scheduler mode before running processes.\n");
}

/*
 * Function:  add_prcess
 * --------------------
 * Adds a process to the waiting queue, according to the current scheduler policy
 */
void add_process(struct pcb *new_p)
{
    switch (state.mode)
    {
    case FCFS:
    case RR:
        add_back(new_p);
        break;

    case SJF:
    case AGING:
        add_with_priority(new_p, new_p->bound);
        break;

    default:
        error_no_mode_selected();
        return;
    }

    state.np++; // Increase number of processes counter
}

/*
 * Function:  run_basic
 * --------------------
 * Executes basic policy (used by FCFS and SJF). Simply runs processes in the order they are in, in the waiting queue
 */
void run_basic()
{
    exec_process();
}

/*
 * Function:  run_RR
 * --------------------
 * Executes the Round Robin Policy. Runs current process for up to RR_PREEMPT_FREQ iterations and then places it at back of queue
 */
void run_RR()
{
    for (int i = 0; i < RR_PREEMPT_FREQ; ++i)
    {
        if (state.cur == NULL) // Process terminated in less than RR_PREEMPT_FREQ iterations
            break;

        exec_process();
    }

    if (state.cur != NULL) // Check process isn't done
    {
        add_back(state.cur);
        state.cur = NULL;
    }
}

/*
 * Function:  run_AGING
 * --------------------
 * Exectues the AGING Policy. Runs current process for 1 step, then decrements priority of waiting processes.
 * Then checks if new policy has higher priority and swaps if it does
 */
void run_AGING()
{
    exec_process();
    decr_priorities();

    if (state.cur != NULL && state.np > 1 && state.head != NULL && state.head->priority < state.cur_priority) // Current head of waiting queue has higher priority than current running process
    {
        add_with_priority(state.cur, state.cur_priority); // Add current running process back into priority queue (note that it will be added after the head element because of above check)
        state.cur = NULL;
    }
}

/*
 * Function:  run_scheduler
 * --------------------
 * Runs the current selected scheduler policy on tasks in waiting queue
 */
int run_scheduler()
{

    while (state.np > 0)
    {
        if (state.cur == NULL) // No process is currently running
            pop_front();       // Set head of waiting queue as running process

        switch (state.mode)
        {
        case FCFS:
        case SJF:
            run_basic();
            break;
        case RR:
            run_RR();
            break;
        case AGING:
            run_AGING();
            break;
        default:
            error_no_mode_selected();
            return 1;
        }
    }
    return 0;
}

/*
 * Function: exec_process
 * --------------------
 * Executes one instruction from the current running process
 */
void exec_process()
{
    char *instr = read_instruction(state.cur);

    if (instr == NULL)
    {
        // Page fault occurred while reading instruction (read_instruction function handles loading page from backing store)
        // place running process back into queue
        switch (state.mode)
        {
        case FCFS:
        case RR:
            add_back(state.cur);
            break;

        case SJF:
        case AGING:
            add_with_priority(state.cur, state.cur_priority);
            break;

        default:
            error_no_mode_selected();
            return;
        }

        state.cur = NULL;
        return; // Return without executing anything
    }

    // Update pointer and potentially remove process before executing instruction
    // This has better behaviour when the last instruction is itself a run/exec call
    state.cur->pc++;
    if (state.cur->pc >= state.cur->bound)
    {
        free_process(state.cur);
        state.cur = NULL;
        state.np--;

        if (state.np == 0)
        {
            mem_reset_frames(); // All processes done, reset frames
        }
    }

    run_on_buffered_line(instr, 0); // Run instruction line

    free(instr);

    return;
}