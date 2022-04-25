
#ifndef MAIN_H
#define MAIN_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/mman.h>

// Exit statuses
#define STATUS_OK 0
#define STATUS_ERROR 1

// Structs
typedef struct {
    int NO;
    int NH;
    int TI;
    int TB;
} Tparams;

typedef struct {
    sem_t *mutex;
    sem_t *barrier;
    sem_t *oxyQue;
    sem_t *hydQue;
} TSemaphores;

// Variables
Tparams params;
TSemaphores semaphores;

int oxygen;
int hydrogen;


// Prototypes

// Save parameters to params struct, if error, save it to error variable
int handle_args(int argc, char *argv[], Tparams *params);

int semaphores_init(TSemaphores *semaphores);

int semaphores_destroy(TSemaphores *semaphores);



#endif