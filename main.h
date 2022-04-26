
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
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>


// Exit statuses
#define STATUS_OK 0
#define STATUS_ERROR 1

#define type_O 'O'
#define type_H 'H'

#define UNUSED(x) (void)(x)


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

typedef struct {
    int oxygen_id;
    int hydrogen_id;
    int count_outputs_id;
} TSMemory;

typedef struct {
    int *oxygen;
    int *hydrogen;
    int *count_outputs;
} TSMemoryVariables;

// Variables
Tparams params;
TSemaphores semaphores;
TSMemory memory;
TSMemoryVariables memory_variables;

// Prototypes

// Save parameters to params struct, if error, save it to error variable
int handle_args(int argc, char *argv[], Tparams *params);

int semaphores_init(TSemaphores *semaphores);

int semaphores_destroy(TSemaphores *semaphores);

int shm_init(TSMemory *memory, TSMemoryVariables *memory_variables);

int shm_destroy(TSMemory *memory, TSMemoryVariables *memory_variables);

int parent_process(Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void oxygen_process(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables);
void hydrogen_process(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void atom_start(int id, char type);
void atom_to_queue(int id, char type);
void atom_creating_molecule(int id, char type);

void H_not_enough(int id);
void O_not_enough(int id);



#endif