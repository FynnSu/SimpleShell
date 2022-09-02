#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "backing_store.h"

#define BACKING_STORE_DIR "backing_store"
#define MAX_LINE_LENGTH 1000

void error_copy_failed();
void error_read_from_store_failed();

/*
 * Function:  clear_backing_store
 * --------------------
 * Deletes all files in the backing store, and then the backing store directory itself
 *
 */
void clear_backing_store()
{
    DIR *dir = opendir(BACKING_STORE_DIR);
    if (dir)
    {
        // Directory exists (clear contents)
        struct dirent *next_file;
        char filepath[500];

        while ((next_file = readdir(dir)) != NULL)
        {
            sprintf(filepath, "%s/%s", BACKING_STORE_DIR, next_file->d_name);
            remove(filepath);
        }
        closedir(dir);
        rmdir(BACKING_STORE_DIR);
    }
}

/*
 * Function:  init_backing_store
 * --------------------
 *  Creates backing store directory (clears it if it already exists)
 *
 */
void init_backing_store()
{
    clear_backing_store();

    mkdir(BACKING_STORE_DIR, 0777);
}

/*
 * Function:  errory_copy_failed
 * --------------------
 * Prints error when failed to copy script to backing store
 *
 */
void error_copy_failed()
{
    printf("An error occured while trying to copy script contents to backing store\n");
}

/*
 * Function:  error_read_from_store_failed
 * --------------------
 * Prints error when failed to read from backing store
 *
 */
void error_read_from_store_failed()
{
    printf("An error occured while attempting to read data from the backing store into main menu\n");
}

/*
 * Function:  cp_to_store
 * --------------------
 * Attempts to copy given file (given relative to current directory) into backing store
 *
 * const char *filename: name of script to copy
 * p_t pid: process id of process script is being copied for (used for filename in backing store)
 *
 * returns (int): number of lines in copied file (-1 on failure)
 */
int cp_to_store(const char *filename, p_t pid)
{
    char backing_file_name[500];

    sprintf(backing_file_name, "%s/%llu.process", BACKING_STORE_DIR, pid);

    if (access(filename, F_OK) == -1)
    {
        // File to copy not found
        error_copy_failed();
        return -1;
    }

    if (access(backing_file_name, F_OK) == 0)
    {
        // Backing store already contains a script with given pid
        error_copy_failed();
        return -1;
    }

    FILE *read_file = fopen(filename, "rt");
    FILE *write_file = fopen(backing_file_name, "wt");

    // Copy file
    char c;
    int n_lines = 0;

    while ((c = fgetc(read_file)) != EOF)
    {
        if (c == '\n')
        {
            n_lines++;
        }
        fputc(c, write_file);
    }

    fseek(read_file, -1L, SEEK_END);
    if (fgetc(read_file) != '\n')
    {
        n_lines++;
    }

    fclose(read_file);
    fclose(write_file);

    return n_lines;
}

/*
 * Function:  remove_process_store
 * --------------------
 * Removes given process from backing store
 *
 * struct pcb *pcb: pcb of process to remove
 *
 */
void remove_process_store(struct pcb *pcb)
{
    char backing_file_name[500];
    sprintf(backing_file_name, "%s/%llu.process", BACKING_STORE_DIR, pcb->pid);

    if (access(backing_file_name, F_OK) == -1)
    {
        error_read_from_store_failed(); // File trying to delete doesn't exist (but is expected to)
        return;
    }

    remove(backing_file_name);
}

/*
 * Function:  load_into_mem
 * --------------------
 * Loads up to FRAMESIZE lines from backing store into main memory
 *
 * struct pcb *pcb: pcb of process to read from
 * int start: line to start reading from
 * char **mem_loc[]: array of shell memory locations to write lines into
 *
 */
void load_into_mem(struct pcb *pcb, int start, char **mem_loc[])
{
    char backing_file_name[500];

    sprintf(backing_file_name, "%s/%llu.process", BACKING_STORE_DIR, pcb->pid);

    if (access(backing_file_name, F_OK) == -1)
    {
        error_read_from_store_failed();
        return;
    }

    FILE *process_file = fopen(backing_file_name, "rt");

    if (process_file == NULL)
    {
        error_read_from_store_failed();
        return;
    }

    int cur_line = 0;
    char c = 'a'; // Character not used (but needs initialization because otherwise could == EOF by chance)

    // Move file pointer to current pcb location
    while (cur_line < start && (c = fgetc(process_file)) != EOF)
    {
        if (c == '\n')
        {
            cur_line++;
        }
    }

    if (c == EOF)
    {
        // less than "start" lines exist in file, cannot read from "start" line onwards
        error_read_from_store_failed();
        return;
    }

    int n_lines = FRAMESIZE; // Number of lines to read
    if (start + FRAMESIZE > pcb->bound)
    {
        n_lines = pcb->bound - start; // if near end of file, read remaining lines
    }

    size_t buffer_size = MAX_LINE_LENGTH;

    char *buffer = (char *)malloc(buffer_size * sizeof(char));
    if (buffer == NULL)
        return;

    if (pcb->pc + n_lines > pcb->bound)
    {
        return; // Extra check
    }

    // write instructions from file directly into memory locations
    // mem locations passed as an array of char**
    for (int i = 0; i < n_lines; ++i)
    {
        buffer_size = MAX_LINE_LENGTH;
        getline(&buffer, &buffer_size, process_file);
        if (*mem_loc[i] != NULL) // Clear memory if already in use (I don't think this will ever be the case)
            free(*mem_loc[i]);
        *(mem_loc[i]) = strdup(buffer);
    }

    // Clear extra lines
    for (int i = n_lines; i < FRAMESIZE; ++i)
    {
        if (*mem_loc[i] != NULL) // Again probably will never happen because I already clear these on evictions
            free(*mem_loc[i]);
        *(mem_loc[i]) = NULL;
    }

    free(buffer);
    fclose(process_file);
}