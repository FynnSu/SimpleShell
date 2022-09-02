SHELL=/bin/bash

# Default values if non entered
# Size of shell variable store
ifndef varmemsize
	varmemsize=10
endif

# Size of frame store
ifndef framesize
	framesize=18
endif

# Size of each frame (note that framesize must be a multiple of singlesize)
ifndef singlesize
	singlesize=3
endif

# Calculate size of shell memory and nframes so that they can be accessed as macros within code
# Note that further checks on these values are performed when the shell is launched
shellmemsize=$$(( $(framesize) + $(varmemsize) ))
nframes=$$(( $(framesize) / $(singlesize) ))

mysh: shell.c interpreter.c shellmemory.c pcb.c scheduler.c backing_store.c
	gcc -D NFRAMES=$(nframes) \
		-D SHELLMEMSIZE=$(shellmemsize) \
		-D FRAMESTORESIZE=$(framesize) \
		-D FRAMESIZE=$(singlesize) \
		-D VARMEMSIZE=$(varmemsize) \
		-c shell.c interpreter.c shellmemory.c pcb.c scheduler.c backing_store.c
	gcc -o mysh shell.o interpreter.o shellmemory.o pcb.o scheduler.o backing_store.o

clean: 
	rm *.o; rm mysh;

debug: shell.c interpreter.c shellmemory.c pcb.c scheduler.c backing_store.c
	gcc -g -Wall -D NFRAMES=$(nframes) \
		-D SHELLMEMSIZE=$(shellmemsize) \
		-D FRAMESTORESIZE=$(framesize) \
		-D FRAMESIZE=$(singlesize) \
		-D VARMEMSIZE=$(varmemsize) \
		-c shell.c interpreter.c shellmemory.c pcb.c scheduler.c backing_store.c
	gcc -g -o mysh shell.o interpreter.o shellmemory.o pcb.o scheduler.o backing_store.o
