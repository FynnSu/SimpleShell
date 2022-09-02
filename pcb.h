#ifndef PCB_H
#define PCB_H

#include <stdio.h>

typedef unsigned long long p_t;

struct pcb
{
    p_t pid;
    int bound;
    int pc;
    int *pagetable;
};

struct pcb *load_script(char *script);
void load_page(struct pcb *pcb, int page);
void free_process(struct pcb *pcb);

#endif