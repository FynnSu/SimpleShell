# Simple c shell implementation

This is an implementation of a basic shell written in c. To run, cd into the directory and run `make`. You can modify shell features by running 

`make mysh varmemsize=10 framesize=18 singlesize=3`

to change the size of the variable store, the size of the frame store, and the size of the single frame. See the Makefile for more details. 

Then running `./mysh` will run the shell.

## Program Files
* Makefile: Code for how to correctly compile shell program

* backing_store.c: Implementation of backing store. Includes methods to create, reset/delete, copy scripts into store, and load instructions from store into shellmemory

* interpreter.c: Interprets commands and contains implementations of commands

* pcb.h: Contains definition of pcb struct
* pcb.c: Contains functions to load scripts (creating a new process + it's pcb), load pages, and free pcb memory

* scheduler.h: Contains definition of Scheduler mode (policy) enum
* scheduler.c: Contains logic for maintaining current state of ready queue and executing current process according to the set policy

* shell.c: Contains main function, main shell loops, and method to parse instructions into words

* shellmemory.c: Contains implementation of shell memory.
