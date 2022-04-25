cc=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread

all: main

# GEN .o files

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# BUILD EXECUTABLES
main: main.o
	$(CC) $(CFLAGS) $^ -o $@

# Delete .o files
clean:
	rm -f *.o