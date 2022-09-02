#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>

#include "shellmemory.h"
#include "shell.h"
#include "pcb.h"
#include "scheduler.h"
#include "backing_store.h"

#define ECHO_VAR_FLAG '$'

int help();
int quit();
int ls();
int reset_mem();
int my_cmp();
int my_filter();
int echo(char *var);
int badcommand();
int set(char *args[], int n_args);
int print(char *var);
int run(char *script);
int exec(char *args[], int n_args);
int badcommandFileDoesNotExist();
int badcommandTooManyTokens();
int badcommandInvalidMode();
int badcommandDuplicateScript();
int badcommandFailedToLoadScript();
/*
 * Function:  interpreter
 * --------------------
 * Checks command validity and then calls the corresponding command.
 *
 * char* command_args[]: arguments for the command to run
 * int args_size: number of arguments passed
 *
 * returns (int): exit status
 */
int interpreter(char *command_args[], int args_size)
{
	if (args_size < 1)
	{
		return badcommand();
	}

	if (strcmp(command_args[0], "help") == 0)
	{
		// help
		if (args_size != 1)
			return badcommand();
		return help();
	}
	else if (strcmp(command_args[0], "quit") == 0)
	{
		// quit
		if (args_size != 1)
			return badcommand();
		return quit();
	}
	else if (strcmp(command_args[0], "set") == 0)
	{
		// set
		if (args_size < 3)
			return badcommand();
		return set(command_args + 1, args_size - 1); // Pass in pointer to the first relevant arg
	}
	else if (strcmp(command_args[0], "print") == 0)
	{
		// print
		if (args_size != 2)
			return badcommand();
		return print(command_args[1]);
	}
	else if (strcmp(command_args[0], "run") == 0)
	{
		// run
		if (args_size != 2)
			return badcommand();
		return run(command_args[1]);
	}
	else if (strcmp(command_args[0], "exec") == 0)
	{
		// exec
		if (args_size < 3 || args_size > 5)
			return badcommand();
		return exec(command_args + 1, args_size - 1);
	}
	else if (strcmp(command_args[0], "echo") == 0)
	{
		// echo
		if (args_size != 2)
			return badcommand();
		return echo(command_args[1]);
	}
	else if (strcmp(command_args[0], "ls") == 0)
	{
		// ls
		if (args_size != 1)
			return badcommand();
		return ls();
	}
	else if (strcmp(command_args[0], "resetmem") == 0)
	{
		// resetmem
		if (args_size != 1)
			return badcommand();
		return reset_mem();
	}
	else
		return badcommand();
}

/*
 * Function:  help
 * --------------------
 * Prints out shell usage information.
 *
 * returns (int): status
 */
int help()
{

	char help_string[] = "COMMAND					DESCRIPTION\n\n \
help					Displays all the commands\n \
quit					Exits / terminates the shell with “Bye!”\n \
set VAR STRING				Assigns a value to shell memory\n \
print VAR				Displays the STRING assigned to VAR\n \
run SCRIPT.TXT				Executes the file SCRIPT.TXT\n \
exec prog1 [prog2] [prog3] POLICY	Executes the entered scripts using the given policy\n \
echo (STRING || $VAR)			Displays the STRING or the STRING associated with VAR\n \
ls 					Lists all files and directories in the current directory\n \
resetmem				Delete the contents of variable store\n";
	printf("%s\n", help_string);
	return 0;
}

/*
 * Function:  quit
 * --------------------
 * Quits Shell.
 *
 * returns (int): status
 */
int quit()
{
	clear_backing_store();
	printf("%s\n", "Bye!");
	exit(0);
}

/*
 * Function:  badcommand
 * --------------------
 * Indicates that a command was used incorrectly
 *
 * returns (int): status
 */
int badcommand()
{
	printf("%s\n", "Unknown Command");
	return 1;
}

int badcommandFailedToLoadScript()
{
	printf("%s\n", "Failed to Load Script into Memory. Perhaps OOM?");
	return 8;
}

int badcommandDuplicateScript()
{
	printf("Scripts must have unique names when called with exec");
	return 7;
}
/*
 * Function:  badcommandFileDoesNotExist
 * --------------------
 * Indicates that an attempt was made to access a non-existent file
 *
 * returns (int): status
 */
int badcommandFileDoesNotExist()
{
	printf("%s\n", "Bad command: File not found");
	return 3;
}

/*
 * Function:  badcommandInvalidMode
 * --------------------
 * Indicates that the scheduler mode input is not a valid mode
 *
 * returns (int): status
 */
int badcommandInvalidMode()
{
	printf("Bad command: Invalid Scheduler Mode\n");
	return 4;
}

/*
 * Function:  badcommandTooManyTokens
 * --------------------
 * Indicates that a command was given too many arguments as input
 *
 * returns (int): status
 */
int badcommandTooManyTokens()
{
	printf("%s\n", "Bad command: Too many tokens");
	return 5;
}

/*
 * Function:  lsFailed
 * --------------------
 * Indicates that an error occured while trying to retrieve files/dirs in current directory.
 *
 * returns (int): status
 */
int lsFailed()
{
	printf("%s\n", "An error occured while running ls");
	return 6;
}

/*
 * Function:  my_cmp
 * --------------------
 * Custom comparison function for sorting ls entries.
 * Sorts elements according to case-insensitive ascii ordering.
 *
 * const struct dirent **a: First element to compare
 * const struct dirent **a: Second element to compare
 *
 * returns (int): sortorder (neg if a < b, 0 if a == b, pos if a > b)
 */
int my_cmp(const struct dirent **a, const struct dirent **b)
{
	return strcasecmp((*a)->d_name, (*b)->d_name);
}

/*
 * Function:  my_filter
 * --------------------
 * Custom filter function for filtering ls entries.
 * Removes files/directors that start with '.' (i.e. hidden files and special dirs "." and "..")
 *
 * const struct dirent *a: Element to check
 *
 * returns (int): 1 if a's name does not start with '.', 0 otherwise
 */
int my_filter(const struct dirent *a)
{
	return a->d_name[0] != '.';
}

/*
 * Function:  ls
 * --------------------
 * Retreives sorted/filter list of file/dir names and prints them to console.
 * Uses custom comparator and filter: my_sort and my_filter
 *
 * returns (int): status
 */
int ls()
{
	struct dirent **namelist;
	int n = scandir(".", &namelist, my_filter, my_cmp); // Scan current directory, returns number of items found
	if (n == -1)
		return lsFailed();

	for (int i = 0; i < n; i++)
	{
		printf("%s\n", namelist[i]->d_name);
		free(namelist[i]); // free mem
	}
	free(namelist);

	return 0;
}

/*
 * Function:  set
 * --------------------
 * Sets up value of first arg to the next (up to 5) args in shell memory.
 *
 * char* args[]: Arguments to be set.
 * int n_args: Number of arguments to use in set operation. An error is thrown if n_args > 6
 *
 * returns (int): status
 */
int set(char *args[], int n_args)
{

	if (n_args > 6)
	{
		return badcommandTooManyTokens();
	}

	char *var = args[0];

	char *sep = " "; // separate multiple tokens with a space
	char buffer[1000];
	strcpy(buffer, args[1]);

	for (int i = 2; i < n_args; ++i)
	{
		strcat(buffer, sep);
		strcat(buffer, args[i]);
	}

	mem_set_value(var, buffer);

	return 0;
}

/*
 * Function:  echo
 * --------------------
 * Echos input to console, or value of input if input begins with '$'.
 * If input variable is not defined, output a blank line
 *
 * char* key: Value to print or '$' followed by name of variable to print
 *
 * returns (int): status
 */
int echo(char *key)
{
	if (*key == ECHO_VAR_FLAG) // check first char
	{
		char *val = mem_get_value(key + 1);
		if (val != NULL)
		{
			printf("%s\n", val);
			free(val); // mem_get_value creates a copy which goes out of scope here
		}
		else
		{
			printf("\n");
		}
	}
	else
	{
		printf("%s\n", key);
	}
	return 0;
}

/*
 * Function:  print
 * --------------------
 * Prints stored value of input to the console.
 *
 * Produces error message if input variable does not exist in shell memory.
 *
 * char* key: Name of Variable to print
 *
 * returns (int): status
 */
int print(char *key)
{
	char *val = mem_get_value(key);
	if (val != NULL)
	{
		printf("%s\n", val);
		free(val);
	}
	else
	{
		printf("Variable does not exist\n");
	}

	return 0;
}

int reset_mem()
{
	clear_shell_mem();

	return 0;
}

/*
 * Function:  run
 * --------------------
 * Opens file and loads script into memory
 *
 * char* script: filename to open. An error is thrown if the file is not found.
 *
 * returns (int): status
 */
int run(char *script)
{

	// Check if file exists and can be read from
	if (access(script, R_OK) == -1)
		return badcommandFileDoesNotExist();

	// Run shell (from shell.c) with open file stream as input. 0 as a second argument indicates this is a subprocess and not the outer shell loop
	struct pcb *p = load_script(script);

	if (p == NULL)
	{
		return badcommandFailedToLoadScript();
	}

	set_scheduler_mode(FCFS);
	add_process(p);

	return 0;
}

/*
 * Function:  exec
 * --------------------
 * Checks script name and mode validity
 * Loads all scripts into memory and sets scheduling mode as specified
 *
 * Note: exec does not trigger the scheduler to run (this is done in the main shell loop)
 *
 * returns (int): status
 */
int exec(char *args[], int n_args)
{
	for (int i = 0; i < n_args - 1; ++i)
	{
		if (access(args[i], R_OK) == -1)
			return badcommandFileDoesNotExist();

		// Allow Duplicate files in A3
		// for (int j = i + 1; j < n_args - 1; ++j)
		// {
		// 	if (strcmp(args[i], args[j]) == 0)
		// 	{
		// 		return badcommandDuplicateScript();
		// 	}
		// }
	}

	if (strcmp(args[n_args - 1], "FCFS") == 0)
	{
		set_scheduler_mode(FCFS);
	}
	else if (strcmp(args[n_args - 1], "SJF") == 0)
	{
		set_scheduler_mode(SJF);
	}
	else if (strcmp(args[n_args - 1], "RR") == 0)
	{
		set_scheduler_mode(RR);
	}
	else if (strcmp(args[n_args - 1], "AGING") == 0)
	{
		set_scheduler_mode(AGING);
	}
	else
	{
		return badcommandInvalidMode();
	}

	for (int i = 0; i < n_args - 1; ++i)
	{

		struct pcb *p = load_script(args[i]);

		if (p == NULL)
		{
			return badcommandFailedToLoadScript();
		}

		add_process(p);
	}
	return 0;
}