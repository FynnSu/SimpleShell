# Simple c shell implementation

## Features:

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
