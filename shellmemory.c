#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"
#include "pcb.h"
#include "backing_store.h"

// Variables defined in makefile
// FRAMESTORESIZE, FRAMESIZE, VARMEMSIZE, NFRAMES, SHELLMEMSIZE

struct memory_struct // Elements that the shellmemory is comprised of
{
	char *var;
	char *value;
};

struct lru_ll // least recently used double linked list definition
{
	int framenum;
	struct lru_ll *next;
	struct lru_ll *prev;
};

struct memory_state // struct containing all important shellmemory state
{
	int cur_var_size;								// Current number of elements stored in the shell variable memory
	int frames_allocated;							// Indicator (1 if frames are allocated, 0 otherwise)
	struct lru_ll *head, *tail;						// head and tail of LRU double linked list
	struct memory_struct shellmemory[SHELLMEMSIZE]; // Shellmemory array (note that SHELLMEMSIZE = VARMEMSIZE + FRAMESTORESIZE)
	struct lru_ll *ll_quick[NFRAMES];				// Arrary of pointers to elements in LRU linked list, allows O(1) access to any frame
} m_state;											// Note that m_state is an instance of the above struct

int get_frame_start(int framenum);
char *create_frame_key(p_t pid, int pagenum);
void mem_full_error();

/*
 * Function:  move_to_back
 * --------------------
 * Moves the indicated frame to the back of the LRU linked list.
 * Note that this operation is O(1).
 *
 * int framenumber: frame to move
 */
void move_to_back(int framenumber)
{
	struct lru_ll *frame = m_state.ll_quick[framenumber]; // directly access pointer to frame's lru_ll element
	if (m_state.tail == frame)
		return; // Already at back
	if (m_state.head == frame)
	{
		m_state.head = frame->next; // currently at front, set head to point to second item
	}
	else
	{
		frame->prev->next = frame->next; // Not at front, update prev's next pointer
	}

	frame->next->prev = frame->prev; // Update next's prev pointer

	frame->next = NULL;
	frame->prev = m_state.tail;
	m_state.tail->next = frame;
	m_state.tail = frame;
}

/*
 * Function:  move_to_front
 * --------------------
 * Moves the indicated frame to the front of the LRU linked list.
 * Note that this operation is O(1).
 *
 * Note: this function is no longer in use (written due to some confusion about what happens to a processes frames when it terminates)
 *
 * int framenumber: frame to move
 */
void move_to_front(int framenumber)
{
	struct lru_ll *frame = m_state.ll_quick[framenumber];
	if (m_state.head == frame)
		return; // Already at front
	if (m_state.tail == frame)
	{
		m_state.tail = frame->prev; // tail points to second last item
	}
	else
	{
		frame->next->prev = frame->prev;
	}

	frame->prev->next = frame->next;

	frame->next = m_state.head;
	frame->prev = NULL;
	m_state.head->prev = frame;
	m_state.head = frame;
}

/*
 * Function:  get_next_frame
 * --------------------
 * Get's the next frame from LRU queue (and moves that frame to back of queue)
 *
 * returns (int): Number of words read (or -1 if in_stream is empty)
 */
int get_next_frame()
{
	int val = m_state.head->framenum;
	move_to_back(val);
	return val;
}

/*
 * Function:  init_memory
 * --------------------
 * Initializes Shell Memory to NULL Pointers.
 *
 * Should only be called once when program starts.
 * Multiple calls may result in memory leakage.
 */
void init_memory()
{
	m_state.cur_var_size = 0;
	m_state.frames_allocated = 0;

	// Build LRU linked list queue
	struct lru_ll *prev;
	for (int i = 0; i < NFRAMES; i++)
	{
		struct lru_ll *new_frame = malloc(sizeof(struct lru_ll));
		if (new_frame == NULL)
		{
			return;
		}
		new_frame->framenum = i;
		m_state.ll_quick[i] = new_frame;

		if (i == 0)
		{
			m_state.head = new_frame;
		}
		else
		{
			prev->next = new_frame;
		}

		prev = new_frame;
	}

	m_state.tail = prev;

	// Set shell memory to NULL
	for (int i = 0; i < SHELLMEMSIZE; i++)
	{
		m_state.shellmemory[i].var = NULL;
		m_state.shellmemory[i].value = NULL;
	}
}

/*
 * Function:  mem_reset_frames
 * --------------------
 * If there are currently allocated frames in main memory,
 * Iterates through and clears the frame memory
 */
void mem_reset_frames()
{
	// Note: first VARMEMSIZE indices are for Variable storage
	// Then remainder of shellmemory is frame storage
	if (!m_state.frames_allocated)
		return;
	for (int i = VARMEMSIZE; i < SHELLMEMSIZE; i++)
	{
		if (m_state.shellmemory[i].var != NULL)
		{
			free(m_state.shellmemory[i].var);
		}
		if (m_state.shellmemory[i].value != NULL)
		{
			free(m_state.shellmemory[i].value);
		}
		m_state.shellmemory[i].var = NULL;
		m_state.shellmemory[i].value = NULL;
	}
	m_state.frames_allocated = 0;
}

/*
 * Function:  remove_process_claims
 * --------------------
 * Removes the indicator from frame memory that "claims" the frame for use by the process
 * More efficient than clearing the entire, simply makes it available for use without triggering an eviction
 *
 * Note: this function is no longer in use (written due to confusion about how to handle finished process frames)
 *
 * struct pcb *pcb: pcb of the process to remove
 */
void remove_process_claims(struct pcb *pcb)
{
	int n_pages = (pcb->bound + FRAMESIZE - 1) / FRAMESIZE;
	for (int i = 0; i < n_pages; ++i)
	{
		int framenumber = pcb->pagetable[i];
		if (framenumber == -1)
			continue;

		char *key = create_frame_key(pcb->pid, i);
		int frame_start = get_frame_start(framenumber);
		if (m_state.shellmemory[frame_start].var != NULL && strcmp(m_state.shellmemory[frame_start].var, key) == 0)
		{
			free(m_state.shellmemory[frame_start].var);
			m_state.shellmemory[frame_start].var = NULL;
			move_to_front(framenumber);
		}
		free(key);
	}
}

/*
 * Function:  get_frame_start
 * --------------------
 * Calculates the index of the start of a frame in shell memory
 *
 * int framenum: frame to find
 *
 * returns (int): index of start of frame
 */
int get_frame_start(int framenum)
{
	return framenum * FRAMESIZE + VARMEMSIZE;
}

/*
 * Function:  create_frame_key
 * --------------------
 * Creates a string key which indicates the running process and page number
 * being stored in a frame
 *
 * p_t pid: process id
 * int pagenum: page number to store
 *
 * returns (char *): char * to frame key. Must be freed after use
 */
char *create_frame_key(p_t pid, int pagenum)
{
	char key_arr[512]; // Totally overkill size
	sprintf(key_arr, "pid_%llu_page_%d", pid, pagenum);

	return strdup(key_arr);
}

/*
 * Function:  check_eviction
 * --------------------
 * Checks if a frame is currently allocated. If it is, evicts the frame
 *
 * int framenum: frame to check
 *
 */
void check_eviction(int framenum)
{
	int start = get_frame_start(framenum);

	if (m_state.shellmemory[start].var == NULL)
	{
		// No Eviction
		return;
	}

	printf("%s\n", "Page fault! Victim page contents:");

	for (int i = 0; i < FRAMESIZE; i++)
	{
		if (m_state.shellmemory[start + i].value != NULL)
		{
			printf("%s", m_state.shellmemory[start + i].value);
			free(m_state.shellmemory[start + i].value);
			m_state.shellmemory[start + i].value = NULL;
		}
	}

	printf("%s\n", "End of victim page contents.");
	free(m_state.shellmemory[start].var);
	m_state.shellmemory[start].var = NULL;
}

/*
 * Function:  load_from_backing_store
 * --------------------
 * Loads a page from backing store into LRU frame.
 *
 * struct pcb *pcb: pcb of process to load from
 * int start_line: line to start loading from (loads lines start_line:start_line+FRAMESIZE)
 *
 * Note that start_line should be a multiple of FRAME_SIZE (or 0)
 *
 * returns (int): frame number the page was loaded into
 */
int load_from_backing_store(struct pcb *pcb, int start_line)
{
	char **frame_refs[FRAMESIZE];

	int framenum = get_next_frame();
	check_eviction(framenum);
	int start = get_frame_start(framenum);
	for (int i = 0; i < FRAMESIZE; ++i)
	{
		frame_refs[i] = &(m_state.shellmemory[start + i].value); // Get references to shellmemory location so that backing store can copy directly into memory
	}

	// Note: only update shellmemory key for first block in frame (unnecessary to update others)
	int pagenum = start_line / FRAMESIZE;
	m_state.shellmemory[start].var = create_frame_key(pcb->pid, pagenum);

	load_into_mem(pcb, start_line, frame_refs);

	m_state.frames_allocated = 1;

	return framenum;
}

/*
 * Function:  clear_shell_mem
 * --------------------
 * Clears all defined shell variables
 *
 */
void clear_shell_mem()
{
	for (int i = 0; i < m_state.cur_var_size; i++)
	{
		if (m_state.shellmemory[i].var)
		{
			free(m_state.shellmemory[i].var);
			m_state.shellmemory[i].var = NULL;
		}
		if (m_state.shellmemory[i].value)
		{
			free(m_state.shellmemory[i].value);
			m_state.shellmemory[i].value = NULL;
		}
	}

	m_state.cur_var_size = 0;
}

/*
 * Function:  read_instruction
 * --------------------
 * Attempts to read the next instruction (line) for the given process.
 * Detects and handles page faults
 *
 * struct pcb *pcb: pcb of process.
 *
 * returns (char *): Returns pointer to copy of instruction (should be freed) or NULL on page fault.
 */
char *read_instruction(struct pcb *pcb)
{
	int pagenum = pcb->pc / FRAMESIZE;
	int offset = pcb->pc % FRAMESIZE;
	int framenumber = pcb->pagetable[pagenum];
	if (framenumber == -1)
	{
		load_page(pcb, pagenum); // page fault
		return NULL;
	}

	char *key = create_frame_key(pcb->pid, pagenum);

	int frame_start = get_frame_start(framenumber);
	if (m_state.shellmemory[frame_start].var == NULL || strcmp(m_state.shellmemory[frame_start].var, key) != 0)
	{
		// Frame not allocated to current process
		load_page(pcb, pagenum); // page fault
		free(key);
		return NULL;
	}

	free(key);

	// Update LRU
	move_to_back(framenumber);

	return strdup(m_state.shellmemory[frame_start + offset].value);
}

/*
 * Function:  mem_set_value
 * --------------------
 * Changes memory variable value to value_in if variable already exists.
 * Creates a new variable otherwise.
 *
 * char *var_in: Name of variable to set
 * char *value_in: Value to set
 */
void mem_set_value(char *var_in, char *value_in)
{
	for (int i = 0; i < m_state.cur_var_size; i++)
	{
		if (!m_state.shellmemory[i].var)
			continue; // Check not NULL before comparing strings
		if (strcmp(m_state.shellmemory[i].var, var_in) == 0)
		{
			free(m_state.shellmemory[i].value); // Free memory of string being replaced
			m_state.shellmemory[i].value = strdup(value_in);
			return;
		}
	}

	// Value does not exist, attempt to add to end

	// Memory Full
	if (m_state.cur_var_size >= VARMEMSIZE)
	{
		mem_full_error();
		return;
	}

	// indices 0 (inclusive) - cur_mem_size (exclusive) already in use
	m_state.shellmemory[m_state.cur_var_size].var = strdup(var_in);
	m_state.shellmemory[m_state.cur_var_size].value = strdup(value_in);
	m_state.cur_var_size++;
}

/*
 * Function:  mem_full_error
 * --------------------
 * Prints message indicating the shell memory is full.
 */
void mem_full_error()
{
	printf("Error: Shell Memory Full, can't set Environment Variable.\n");
}

/*
 * Function:  mem_get_value
 * --------------------
 * Attempts to retrieve stored variable from shell memory.
 *
 * char *var_in: Name of variable to get
 *
 * returns (char *): Pointer to duplicate of value if variable exists. NULL otherwise
 */
char *mem_get_value(char *var_in)
{
	int i;

	for (i = 0; i < m_state.cur_var_size; i++)
	{
		if (!m_state.shellmemory[i].var)
			continue; // Check not NULL before comparing strings
		if (strcmp(m_state.shellmemory[i].var, var_in) == 0)
		{
			return strdup(m_state.shellmemory[i].value);
		}
	}
	return NULL;
}