#include <stdio.h>
#include <stdlib.h>

#include "pcb.h"
#include "shellmemory.h"
#include "backing_store.h"

p_t cur_pid = 0; // Simple method to ensure unique pid's for all processes. First process has pid 0, then 1, and so on...

/*
 * Function:  load_script
 * --------------------
 * Load script with given file name. Creates a new process with a new pcb.
 * Copies script into backing store and loads first two pages in frame memory.
 *
 *
 * char *file_name: filename of script to laod (must be a valid file in current dir)
 *
 * returns (struct pcb *): pointer to pcb of new process
 */
struct pcb *load_script(char *file_name)
{
    p_t pid = cur_pid++; // Assign process id

    int n_lines = cp_to_store(file_name, pid); // Copy into backing store

    if (n_lines <= 0) // Copy to backing store failed
        return NULL;

    struct pcb *ret = malloc(sizeof(struct pcb));
    if (ret == NULL)
        return NULL;

    ret->pid = pid;
    ret->bound = n_lines;
    ret->pc = 0;

    int n_pages = (n_lines + FRAMESIZE - 1) / FRAMESIZE;

    // Instatiate pagetable
    ret->pagetable = malloc(n_pages * sizeof(int));

    if (ret->pagetable == NULL)
        return NULL;

    for (int i = 0; i < n_pages; ++i)
    {
        ret->pagetable[i] = -1;
    }

    load_page(ret, 0); // Load first page

    if (n_lines > FRAMESIZE) // Checks script is long enough to require two pages
    {
        load_page(ret, 1); // Load second page
    }

    return ret;
}

/*
 * Function:  free_process
 * --------------------
 * Free's memory for process
 *
 * struct pcb *pcb: pcb to free memory of
 *
 */
void free_process(struct pcb *pcb)
{
    remove_process_store(pcb); // Remove script from backing store
    // remove_process_claims(pcb);

    free(pcb->pagetable);
    free(pcb);
}

/*
 * Function:  load_page
 * --------------------
 * Load page from backing store into frame memory
 *
 * struct pcb *pcb: pcb of process to load
 * int page: page index to load
 *
 */
void load_page(struct pcb *pcb, int page)
{
    int framenum = load_from_backing_store(pcb, page * FRAMESIZE);

    if (framenum == -1)
    {
        return; // Failed to allocated frame
    }

    pcb->pagetable[page] = framenum; // Set frame number in pagetable
}
