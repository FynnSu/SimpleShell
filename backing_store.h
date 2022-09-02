#ifndef BACKING_STORE_H
#define BACKING_STORE_H

#include <stdio.h>
#include "pcb.h"

void init_backing_store();
int cp_to_store(const char *filename, p_t pid);
void load_into_mem(struct pcb *pcb, int n, char **mem_loc[]);
void clear_backing_store();
void remove_process_store(struct pcb *pcb);

#endif