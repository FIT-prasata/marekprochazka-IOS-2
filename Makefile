CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra  -pedantic -pthread

all: proj2

run: proj2
	./proj2 3 5 100 100
	cat proj2.out

# GENERATE .o FILES

proj2.o: proj2.c
	$(CC) $(CFLAGS) -c $< -o $@

# BUILD EXECUTABLES

proj2: proj2.o
	$(CC) $(CFLAGS) $< -o $@

# DELETE ALL .o FILES AND EXECUTABLE

clean:
	rm *.o proj2 proj2.out