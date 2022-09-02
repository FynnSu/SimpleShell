#ifndef SHELL_MEMORY_H
#define SHELL_MEMORY_H

#include "pcb.h"

void init_memory();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
char *read_instruction(struct pcb *p);
int load_from_backing_store(struct pcb *pcb, int start_line);
void remove_process_claims(struct pcb *pcb);
void mem_reset_frames();
void clear_shell_mem();

#endif
