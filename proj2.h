
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
#include <time.h>
#include <signal.h>


// Exit statuses
#define STATUS_OK 0
#define STATUS_ERROR 1

#define type_O 'O'
#define type_H 'H'

#define UNUSED(x) (void)(x)

FILE *file;


// Structs
typedef struct {
    int NO;
    int NH;
    int TI;
    int TB;
} Tparams;

typedef struct {
    int n;
    sem_t *mutex;
    sem_t *turnstile;
    sem_t *turnstile2;
} TBarrier;

typedef struct {
    sem_t *writing_mutex; // mutex for writing to shared memory
    sem_t *building_mutex; // mutex for atoms
    TBarrier *barrier; // barrier used ensure atoms of molecule will leave together
    sem_t *oxyQueue; 
    sem_t *hydQueue;
} TSemaphores;

typedef struct { // struct of id's used in initialization of shared memory
    int oxygen_id;
    int hydrogen_id;
    int count_outputs_id;
    int count_molecules_id;
    int barrier_count_id;
    int max_molecules_id;
    int is_building_possilbe_id;
    int o_left_id;
    int h_left_id;
} TSMemory;

typedef struct {
    int *oxygens_in_que; 
    int *hydrogens_in_que; 
    int *count_outputs; 
    int *count_molecules;
    int *barrier_count;
    int *max_molecules;
    int *is_building_possilbe;
    int *o_left; // how many oxygen atoms will be left after all molecules are built
    int *h_left; // how many hydrogen atoms will be left after all molecules are built
} TSMemoryVariables;



// Prototypes

// Save parameters to params struct, if error, save it to error variable
int handle_args(int argc, char *argv[], Tparams *params);

int file_init();

int barrier_init(TBarrier *barrier, int n);

int barrier_destroy(TBarrier *barrier);

int semaphores_init(TSemaphores *semaphores);

int semaphores_destroy(TSemaphores *semaphores);

int shm_init(TSMemory *memory, TSMemoryVariables *memory_variables);

int shm_destroy(TSMemory *memory, TSMemoryVariables *memory_variables);

void init_max_possible_molecules(TSMemoryVariables *memory_variables, Tparams *params);

int parent_process(Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void oxygen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables);
void hydrogen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void wait_barrier_phase_1(TBarrier *barrier, int *count);
void wait_barrier_phase_2(TBarrier *barrier, int *count);
void wait_barrier(TBarrier *barrier, int *count);

void atom_start(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables);
void atom_to_queue(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables);
void atom_creating_molecule(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void molecule_created(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables);

void inc_molecule_count(TSemaphores *semaphores, TSMemoryVariables *memory_variables);


void H_not_enough(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables);
void O_not_enough(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables);



#endif