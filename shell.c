
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "interpreter.h"
#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"
#include "backing_store.h"

#define MAX_INPUT_LEN 1000
#define MAX_WORD_LEN 200
#define MAX_WORDS 100

int refresh_prompt = 1; // Indicates if the shell prompt ("$") should be printed again

void handleErrorCode(int code);
int readInput(char *words[MAX_WORDS], char *buffer, int *buff_pos);
int main_loop();
int error_invalid_frame_settings();

/*
 * Function:  main
 * --------------------
 * Main Shell Function. Initializes Shell and starts main loop.
 *
 * returns (int): exit status
 */
int main()
{
	// Check Macro values set correctly
	if (FRAMESTORESIZE % FRAMESIZE != 0 || NFRAMES < 2)
	{
		return error_invalid_frame_settings();
	}

	printf("%s\n", "Shell version 3.0 \nCreated March, 2022 by Fynn Schmitt-Ulms (Id: 260844168)");
	printf("Frame Store Size = %d; Variable Store Size = %d\n\n", FRAMESTORESIZE, VARMEMSIZE);
	help();

	// init shell memory
	init_memory();
	init_scheduler();
	init_backing_store();

	return main_loop();
}

/*
 * Function:  main_loop
 * --------------------
 * Contains the main loop for the shell. Repeatedly reads lines into buffer and call run_on_buffered_line function.
 * Note that before executing the current line it first checks if there are processes waiting to be run, and starts the
 * scheduler if needed.
 *
 * returns (int): exit status
 */
int main_loop()
{
	char shell_prompt = '$';
	char *buffer;
	size_t buffer_size = MAX_INPUT_LEN;
	int n_read = 0;

	buffer = (char *)malloc(buffer_size * sizeof(char));
	if (buffer == NULL)
	{
		perror("Unable to allocate buffer");
		exit(1);
	}

	while (1)
	{
		buffer_size = MAX_INPUT_LEN;

		if (processes_waiting())
		{
			run_scheduler(); // Run/Exec functions do not start the scheduler, but instead only add the scheduled tasks to
			// the waiting queue. The tasks are then started on the next iteration of the main loop (here).
			// It's a little weird but done this way so that recursive run/exec calls do not attempt to repeatedly start the scheduler.
			// Instaed there is only at most one run_scheduler call executing at a time (preventing stackoverflow exceptions from over recursing)
			continue;
		}
		if (isatty(fileno(stdin)))
		{
			printf("%c ", shell_prompt); // print prompt only if the shell is not being run from a file
		}

		n_read = getline(&buffer, &buffer_size, stdin); // Read line of input

		if (n_read == -1)
		{
			freopen("/dev/tty", "rw", stdin); // If running from a file (i.e. mysh < file) and the file has ended, switch to user shell input
			continue;
		}

		run_on_buffered_line(buffer, 1); // Run line (the 1 indicates the caller is the main loop)
	}

	return 0;
}

/*
 * Function:  error_invalid_frame_settings
 * -------------------------------------------
 * Prints out an error when the shell is run after being compiled with invalid frame size settings
 *
 * returns (int): exit status
 */
int error_invalid_frame_settings()
{
	printf("Invalid Frame size or Frame store size. Frame store must be a multiple of Frame size and must be large enough to contain at least 2 frames\n\n");
	return -2;
}

/*
 * Function:  run_on_buffered_line
 * -------------------------------------------
 * Contains another loop that reads the next command (to handle multi-command lines)
 *
 * char *buffer: buffered line of input to use
 * int in_main_loop: indicator flag (should be 1 if called from main_loop, 0 otherwise)
 *
 */
void run_on_buffered_line(char *buffer, int in_main_loop)
{
	int w;
	char *words[MAX_WORDS];
	int code;
	int buff_pos = 0;

	while (1)
	{
		if (in_main_loop) // Only executes if called with input from the main shell loop (and not from running process)
		{
			if (processes_waiting())
			{
				run_scheduler();
				continue;
			}
		}
		// Read words for next command (if multiple commands in one line, only reads args for first)
		w = readInput(words, buffer, &buff_pos);

		if (w == -1)
		{
			// Reached end of buffered line
			return;
		}

		// Execute command
		code = interpreter(words, w);

		// Free stored words
		while (w--)
			free(words[w]);

		handleErrorCode(code);
	}

	return;
}

/*
 * Function:  readInput
 * --------------------
 * Improved input parsing fucntion that can handle extra tabs and spaces between input words
 * Reads in words from buffer and stores pointers to them in words.
 * Will read up until '\n', or ';'
 *
 * char *words[MAX_WORDS]: Array of words pointers to fill. words[i] will point to a copy of the i-th entered word when function returns
 * char *buffer: a buffer to read from
 * int *buff_pos: current position in buffer to read at
 *
 * returns (int): Number of words read (or -1 if in_stream is empty)
 */
int readInput(char *words[MAX_WORDS], char *buffer, int *buff_pos)
{
	int w = 0;
	char tmp[MAX_WORD_LEN];
	char *tmpi = tmp;
	char *end = tmp + MAX_WORD_LEN - 1;
	char c;

	// Indicate the end of the buffer has been reached
	if ((c = buffer[*buff_pos]) == '\0')
	{
		return -1;
	}

	// Ignore leading whitespace
	while (c == ' ' || c == '\t')
		c = buffer[++*buff_pos];

	// Each iter reads a word
	do
	{
		while (c != '\0' && c != ' ' && c != '\n' && c != ';' && tmpi < end)
		{
			// Each iter reads a char of a word
			*tmpi++ = c;
			c = buffer[++*buff_pos];
		}
		*tmpi = '\0';

		words[w++] = strdup(tmp); // Duplicate word in tmp and set word pointer
		tmpi = tmp;				  // Reset tmpi to point to the start of the tmp array

		// Ignore trailing whitespace (between words/after last word)
		while (c == ' ' || c == '\t')
			c = buffer[++*buff_pos];

	} while (c != '\0' && c != '\n' && c != ';');

	if (c != '\0')
	{
		++*buff_pos;
	}

	return w;
}

/*
 * Function:  handleErrorCode
 * --------------------
 * Checks error code and reacts accordingly.
 *
 */
void handleErrorCode(int code)
{
	if (code == -1)
		exit(99); // ignore all other errors
}
