#include "main.h"

int main(int argc, char *argv[])
{
    // Variables
    Tparams params;
    TSemaphores semaphores;
    TSMemory memory;
    TSMemoryVariables memory_variables;
    
    if (handle_args(argc, argv, &params) != STATUS_OK) return STATUS_ERROR;
    if (semaphores_init(&semaphores) != STATUS_OK) return STATUS_ERROR;
    if (shm_init(&memory, &memory_variables) != STATUS_OK) return STATUS_ERROR;

    printf("test start \n");
    sem_wait(semaphores.barrier->mutex);
    sem_post(semaphores.barrier->mutex);
    printf("test end \n");

    if (parent_process(&params, &semaphores, &memory_variables) != STATUS_OK) return STATUS_ERROR;
    if (shm_destroy(&memory, &memory_variables) != STATUS_OK) return STATUS_ERROR;
    if (semaphores_destroy(&semaphores) != STATUS_OK) return STATUS_ERROR;
    
    return 0;

}


int handle_args(int argc, char *argv[], Tparams *params) {
    char *end_NO="", *end_NH="", *end_TI="", *end_TB="";

    if (argc < 5) {
        fprintf(stderr, "Not enough arguments\n");
        return STATUS_ERROR;
    }
    
    params->NO = strtol(argv[1], &end_NO, 10);
	params->NH = strtol(argv[2], &end_NH, 10);
	params->TI = strtol(argv[3], &end_TI, 10);
	params->TB = strtol(argv[4], &end_TB, 10);

    if (
        (strcmp(end_NO, "")) ||
        (strcmp(end_NH, "")) ||
        (strcmp(end_TI, "")) ||
        (strcmp(end_TB, "")) ||
        !(params->TI >= 0 && params->TI <= 1000) ||
        !(params->TB >= 0 && params->TB <= 1000) 
    ) {
        fprintf(stderr, "Error in arguments\n");
        return STATUS_ERROR;
    }

    printf("TI = %d\n", params->TI);
    return STATUS_OK;
}

int barrier_init(TBarrier *barrier, int n) {
    barrier->n = n;
    barrier->mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier->turnstile = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier->turnstile2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);                                                                                          

    sem_init(barrier->mutex, 1, 1);
    sem_init(barrier->turnstile, 1, 0);
    sem_init(barrier->turnstile2, 1, 1);

    if (
        (barrier->mutex == SEM_FAILED) ||
        (barrier->turnstile == SEM_FAILED) ||
        (barrier->turnstile2 == SEM_FAILED)
    )
    {
        fprintf(stderr, "Error in barrier_init\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
    
}
int barrier_destroy(TBarrier *barrier){
    sem_destroy(barrier->mutex);
    sem_destroy(barrier->turnstile);
    sem_destroy(barrier->turnstile2);

    if (
        (barrier->mutex == SEM_FAILED) ||
        (barrier->turnstile == SEM_FAILED) ||
        (barrier->turnstile2 == SEM_FAILED)
    ) {
        fprintf(stderr, "Error in barrier_destroy\n");
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

int semaphores_init(TSemaphores *semaphores) {
    semaphores->barrier = (TBarrier *) malloc(sizeof(TBarrier));
    semaphores->barrier2 = (TBarrier *) malloc(sizeof(TBarrier));

    int err_barrier = barrier_init(semaphores->barrier, 3);
    int err_barrier2 = barrier_init(semaphores->barrier2, 3);

    semaphores->writing_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->building_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->oxyQue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->hydQue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    sem_init(semaphores->writing_mutex, 1, 1);
    sem_init(semaphores->building_mutex, 1, 1);

    sem_init(semaphores->oxyQue, 1, 0);
    sem_init(semaphores->hydQue, 1, 0);

    if (
        semaphores->writing_mutex == SEM_FAILED ||
        semaphores->building_mutex == SEM_FAILED ||
        err_barrier == STATUS_ERROR ||
        err_barrier2 == STATUS_ERROR ||
        semaphores->oxyQue == SEM_FAILED ||
        semaphores->hydQue == SEM_FAILED
    ) {
        fprintf(stderr, "Error in semaphores initialization\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

int semaphores_destroy(TSemaphores *semaphores) {
    
        sem_destroy(semaphores->writing_mutex);
        sem_destroy(semaphores->building_mutex);
        sem_destroy(semaphores->oxyQue);
        sem_destroy(semaphores->hydQue);

        int err_barrier = barrier_destroy(semaphores->barrier);
        int err_barrier2 = barrier_destroy(semaphores->barrier2);
    
        if (
            semaphores->writing_mutex == SEM_FAILED ||
            semaphores->building_mutex == SEM_FAILED ||
            semaphores->oxyQue == SEM_FAILED ||
            semaphores->hydQue == SEM_FAILED ||
            err_barrier == STATUS_ERROR ||
            err_barrier2 == STATUS_ERROR
        ) {
            fprintf(stderr, "Error in semaphores destruction\n");
            return STATUS_ERROR;
        }
        free(semaphores->barrier);
        return STATUS_OK;
}

int shm_init(TSMemory *memory, TSMemoryVariables *memory_variables) {
    // Alocate memory blocks 
    if (
        (memory->oxygen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->hydrogen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->count_outputs_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->count_molecules_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->barrier_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->molecules_left_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 
    ) {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Map memory blocks to pointers
    if (
        (memory_variables->oxygen = shmat(memory->oxygen_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->hydrogen = shmat(memory->hydrogen_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->count_outputs = shmat(memory->count_outputs_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->count_molecules = shmat(memory->count_molecules_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->barrier_count = shmat(memory->barrier_count_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->molecules_left = shmat(memory->molecules_left_id, NULL, 0)) == (void *) -1 
    ) {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Initialize memory blocks
    *(memory_variables->oxygen) = 0;
    *(memory_variables->hydrogen) = 0;
    *(memory_variables->count_outputs) = 0;
    *(memory_variables->count_molecules) = 0;
    *(memory_variables->barrier_count) = 0;

    return STATUS_OK;
}

int shm_destroy(TSMemory *memory, TSMemoryVariables *memory_variables) {
     // Detach memory blocks
    if (
        shmdt(memory_variables->oxygen) == -1 ||
        shmdt(memory_variables->hydrogen) == -1 ||
        shmdt(memory_variables->count_outputs) == -1 ||
        shmdt(memory_variables->count_molecules) == -1 ||
        shmdt(memory_variables->barrier_count) == -1 ||
        shmdt(memory_variables->molecules_left) == -1
    ) {
        fprintf(stderr, "Error in shm destruction while detaching memory blocks\n");
        return STATUS_ERROR;
    }
    
    // Destroy memory blocks
    if (
        shmctl(memory->oxygen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->hydrogen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->count_outputs_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->count_molecules_id, IPC_RMID, NULL) == -1 || 
        shmctl(memory->barrier_count_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->molecules_left_id, IPC_RMID, NULL) == -1
    ) {
        fprintf(stderr, "Error in shm destruction while destroying memory blocks\n");
        return STATUS_ERROR;
    }
   
    return STATUS_OK;
}

void init_max_possible_molecules(int *molecules_left, Tparams *params) {
    UNUSED(params);
    *molecules_left = 1;
}


int parent_process(Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables) {
    pid_t parent_process,  O_process_instance, H_process_instance;
    

    init_max_possible_molecules(memory_variables->molecules_left, params);
    global_NH = params->NH;
    global_NO = params->NO;
    global_children_H = malloc(sizeof(pid_t) * params->NH);
    global_children_O = malloc(sizeof(pid_t) * params->NO);
    struct sigaction sa;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sa.sa_mask = mask;
    sa.sa_handler = &handle_signal_to_parent;
    sigaction(SIGUSR2, &sa, NULL);


    parent_process = fork();
    // Oxyde processes
    if (parent_process == 0) {
        for (int i = 0; i < params->NO; i++) {
            O_process_instance = fork();
            if (O_process_instance == 0) {
                oxygen_process(i+1, params,semaphores, memory_variables);
            }
            global_children_O[i] = O_process_instance;
            
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NO; i++) {
            waitpid(global_children_O[i], NULL, 0);
        }
    }

    // Hydrogen processes
    else {
        for (int i = 0; i < params->NH; i++) {
            H_process_instance = fork();
            if (H_process_instance == 0) {
                
                hydrogen_process(i+1, params, semaphores, memory_variables);
            }
            global_children_H[i] = H_process_instance;
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NH; i++) {

            waitpid(global_children_H[i], NULL, 0);
        }
    }
    if (parent_process == 0) {
        exit(0);
    }

    free(global_children_H);
    free(global_children_O);
    return STATUS_OK;
}

void oxygen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables) {
    init_globals(id, type_O, semaphores, memory_variables);
    
    struct sigaction sa;
    sigset_t mask;
    sigfillset(&mask);
    sa.sa_handler = &handle_not_enough_atoms;
    sa.sa_mask = mask;
    sigaction(SIGUSR1, &sa, NULL);

    srand(getpid());
    atom_start(id, type_O, semaphores, memory_variables);
    usleep((rand() % (params->TI + 1)) * 1000);
    atom_to_queue(id, type_O, semaphores, memory_variables);

    // increasing number of existing oxygens
    sem_wait(semaphores->building_mutex);
    (*memory_variables->oxygen)++;
    
    // if there are enough hydrogens 
    if (*memory_variables->hydrogen >= 2) {
        // letting go two hydrogens
        sem_post(semaphores->hydQue);
        sem_post(semaphores->hydQue);
        // decreasing number of existing hydrogens
        (*memory_variables->hydrogen) -= 2;
        // letting go one oxygen in que
        sem_post(semaphores->oxyQue);
        // decreasing number of existing oxygens
        (*memory_variables->oxygen)--;
    }
    else
    {
        sem_post(semaphores->building_mutex);
    }

    // wating place for oxygen (can receive not enough signal)
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    sem_wait(semaphores->oxyQue);
    sigprocmask(SIG_BLOCK, &mask, NULL);



    // creating molecule 

    atom_creating_molecule(id, type_O, semaphores, memory_variables);
    usleep(rand() % (params->TB + 1) * 1000);
    
    
    wait_barrier_phase_1(semaphores->barrier, memory_variables->barrier_count);
    inc_molecule_count(semaphores, memory_variables);
    wait_barrier_phase_2(semaphores->barrier, memory_variables->barrier_count);
    molecule_created(id, type_O, semaphores, memory_variables);
    sem_post(semaphores->building_mutex);
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->molecules_left)--;
    sem_post(semaphores->writing_mutex);

    if (*memory_variables->molecules_left == 0) {
        kill(getppid(), SIGUSR2);
    }

    exit(STATUS_OK);

    
}
void hydrogen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    init_globals(id, type_H, semaphores, memory_variables);
    struct sigaction sa;
    sigset_t mask;
    sigemptyset(&mask);
    sa.sa_mask = mask;
    sa.sa_handler = &handle_not_enough_atoms;
    sigaction(SIGUSR1, &sa, NULL);

    srand(getpid());
    atom_start(id, type_H, semaphores, memory_variables);
    usleep(rand() % (params->TI + 1) * 1000);
    atom_to_queue(id, type_H, semaphores, memory_variables);
  
    // increasing number of existing hydrogens
    sem_wait(semaphores->building_mutex);
    (*memory_variables->hydrogen)++;
    
    // if there are enough hydrogens and oxygen
    if (*memory_variables->hydrogen >= 2 && *memory_variables->oxygen >= 1) {
        // letting go two hydrogens
        sem_post(semaphores->hydQue);
        sem_post(semaphores->hydQue);
        // decreasing number of existing hydrogens
        (*memory_variables->hydrogen) -= 2;
        // letting go one oxygen in que
        sem_post(semaphores->oxyQue);
        // decreasing number of existing oxygens
        (*memory_variables->oxygen)--;
    }
    else
    {
        sem_post(semaphores->building_mutex);
    }

    // wating place for hydrogen
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    sem_wait(semaphores->hydQue);
    sigprocmask(SIG_BLOCK, &mask, NULL);



    atom_creating_molecule(id, type_H, semaphores, memory_variables);

    wait_barrier(semaphores->barrier, memory_variables->barrier_count);
    
    molecule_created(id, type_H, semaphores, memory_variables);

    exit(STATUS_OK);
}

void wait_barrier_phase_1(TBarrier *barrier, int *count){
    sem_wait(barrier->mutex);

    (*count)++;
    if(*count == barrier->n) {
        sem_wait(barrier->turnstile2);
        sem_post(barrier->turnstile);
    }
    sem_post(barrier->mutex);

    sem_wait(barrier->turnstile);
    sem_post(barrier->turnstile);
}

void wait_barrier_phase_2(TBarrier *barrier, int *count){
    // phase 2
    sem_wait(barrier->mutex);
    (*count)--;
    if(*count == 0) {
        sem_wait(barrier->turnstile);
        sem_post(barrier->turnstile2);
    }
    sem_post(barrier->mutex);
    sem_wait(barrier->turnstile2);
    sem_post(barrier->turnstile2);
}

void wait_barrier(TBarrier *barrier, int *count){
    wait_barrier_phase_1(barrier, count);
    wait_barrier_phase_2(barrier, count);
}

void atom_start(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables) {
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    printf("%d: %c %d: started\n", *(memory_variables->count_outputs), type, id);
    sem_post(semaphores->writing_mutex);
}

void atom_to_queue(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    printf("%d: %c %d: going to que\n", *(memory_variables->count_outputs), type, id);
    sem_post(semaphores->writing_mutex);
}

void atom_creating_molecule(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    printf("%d: %c %d: creating molecule %d\n", *(memory_variables->count_outputs), type, id, *(memory_variables->count_molecules)+1);
    sem_post(semaphores->writing_mutex);
}

void molecule_created(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    printf("%d: %c %d: molecule %d created\n", *(memory_variables->count_outputs), type, id, *(memory_variables->count_molecules));
    sem_post(semaphores->writing_mutex);
}

void inc_molecule_count(TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_molecules)++;
    sem_post(semaphores->writing_mutex);
}

void init_globals(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    global_atom_id = id;
    global_atom_type = type;
    global_output_count = memory_variables->count_outputs;
    global_writing_mutex = semaphores->writing_mutex;
}

void handle_signal_to_parent(int sig){
    UNUSED(sig);
    for (int i = 0; i < global_NO; i++) {

            kill(global_children_O[i], SIGUSR1);
        }
    for (int i = 0; i < global_NH; i++) {
            kill(global_children_H[i], SIGUSR1);
        }
}

void handle_not_enough_atoms(int sig) {
    UNUSED(sig);
    sem_wait(global_writing_mutex);
    printf("%d: %c %d: not enough atoms\n", ++(*global_output_count), global_atom_type, global_atom_id);
    sem_post(global_writing_mutex);
    exit(0);
}