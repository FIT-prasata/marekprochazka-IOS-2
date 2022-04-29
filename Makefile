cc=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread 

all: proj2

# GEN .o files

proj2.o: proj2.c
	$(CC) $(CFLAGS) -c $< -o $@

# BUILD EXECUTABLES
proj2: proj2.o
	$(CC) $(CFLAGS) $^ -o $@

# Delete .o files
clean:
	rm -f *.o