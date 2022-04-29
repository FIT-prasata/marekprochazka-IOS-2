#include "proj2.h"

int main(int argc, char *argv[])
{
    // Variables
    Tparams params;
    TSemaphores semaphores;
    TSMemory memory;
    TSMemoryVariables memory_variables;

    if (handle_args(argc, argv, &params) != STATUS_OK)
        return STATUS_ERROR;
    if (file_init() != STATUS_OK)
        return STATUS_ERROR;
    if (semaphores_init(&semaphores) != STATUS_OK)
        return STATUS_ERROR;
    if (shm_init(&memory, &memory_variables) != STATUS_OK)
        return STATUS_ERROR;
    if (parent_process(&params, &semaphores, &memory_variables) != STATUS_OK)
        return STATUS_ERROR;
    if (shm_destroy(&memory, &memory_variables) != STATUS_OK)
        return STATUS_ERROR;
    if (semaphores_destroy(&semaphores) != STATUS_OK)
        return STATUS_ERROR;

    fclose(file);
    printf("Program finished successfully\n");
    return STATUS_OK;
}

int handle_args(int argc, char *argv[], Tparams *params)
{
    char *end_NO = "", *end_NH = "", *end_TI = "", *end_TB = "";

    // Check if there are enough arguments
    if (argc < 5)
    {
        fprintf(stderr, "Not enough arguments\n");
        return STATUS_ERROR;
    }
    // Load arguments
    params->NO = strtol(argv[1], &end_NO, 10);
    params->NH = strtol(argv[2], &end_NH, 10);
    params->TI = strtol(argv[3], &end_TI, 10);
    params->TB = strtol(argv[4], &end_TB, 10);

    // Check if arguments are loaded correctly
    if (
        (strcmp(end_NO, "")) ||
        (strcmp(end_NH, "")) ||
        (strcmp(end_TI, "")) ||
        (strcmp(end_TB, "")) ||
        !(params->TI >= 0 && params->TI <= 1000) ||
        !(params->TB >= 0 && params->TB <= 1000))
    {
        fprintf(stderr, "Error in arguments\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

int file_init()
{
    if ((file = fopen("proj2.out", "w")) == NULL)
    {
        fprintf(stderr, "Error opening file\n");
        exit(STATUS_ERROR);
    }
    setbuf(file, NULL);
    return STATUS_OK;
}

int barrier_init(TBarrier *barrier, int n)
{
    barrier->n = n;

    // Mapping barrier semaphores
    barrier->mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier->turnstile = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    barrier->turnstile2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    // Initializing barrier semaphores
    sem_init(barrier->mutex, 1, 1);
    sem_init(barrier->turnstile, 1, 0);
    sem_init(barrier->turnstile2, 1, 1);

    // Check if barrier semaphores are OK
    if (
        (barrier->mutex == SEM_FAILED) ||
        (barrier->turnstile == SEM_FAILED) ||
        (barrier->turnstile2 == SEM_FAILED))
    {
        fprintf(stderr, "Error in barrier_init\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
}
int barrier_destroy(TBarrier *barrier)
{
    // Destroying barrier semaphores
    sem_destroy(barrier->mutex);
    sem_destroy(barrier->turnstile);
    sem_destroy(barrier->turnstile2);
    // Checking if semaphores are destroyed
    if (
        (barrier->mutex == SEM_FAILED) ||
        (barrier->turnstile == SEM_FAILED) ||
        (barrier->turnstile2 == SEM_FAILED))
    {
        fprintf(stderr, "Error in barrier_destroy\n");
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

int semaphores_init(TSemaphores *semaphores)
{
    // Allocating memory for barrier struct inside semaphores struct
    semaphores->barrier = (TBarrier *)malloc(sizeof(TBarrier));
    semaphores->barrier_before_building = (TBarrier *)malloc(sizeof(TBarrier));

    // Initializing barrier
    int err_barrier = barrier_init(semaphores->barrier, 3);
    int err_barrier_before_building = barrier_init(semaphores->barrier_before_building, 3);

    // mapping rest of the semaphores
    semaphores->writing_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->building_mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->oxyQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->hydQueue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);


    // Initializing semaphores
    sem_init(semaphores->writing_mutex, 1, 1);
    sem_init(semaphores->building_mutex, 1, 1);
    sem_init(semaphores->oxyQueue, 1, 0);
    sem_init(semaphores->hydQueue, 1, 0);

    // Check if semaphores are OK
    if (
        semaphores->writing_mutex == SEM_FAILED ||
        semaphores->building_mutex == SEM_FAILED ||
        err_barrier == STATUS_ERROR ||
        semaphores->oxyQueue == SEM_FAILED ||
        semaphores->hydQueue == SEM_FAILED ||
        err_barrier_before_building == STATUS_ERROR)
    {
        fprintf(stderr, "Error in semaphores initialization\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

int semaphores_destroy(TSemaphores *semaphores)
{
    // Destroying semaphores and barrier
    sem_destroy(semaphores->writing_mutex);
    sem_destroy(semaphores->building_mutex);
    sem_destroy(semaphores->oxyQueue);
    sem_destroy(semaphores->hydQueue);

    int err_barrier = barrier_destroy(semaphores->barrier);
    int err_barrier_before_building = barrier_destroy(semaphores->barrier_before_building);

    // Checking if semaphores are destroyed
    if (
        semaphores->writing_mutex == SEM_FAILED ||
        semaphores->building_mutex == SEM_FAILED ||
        semaphores->oxyQueue == SEM_FAILED ||
        semaphores->hydQueue == SEM_FAILED ||
        err_barrier == STATUS_ERROR ||
        err_barrier_before_building == STATUS_ERROR)
    {
        fprintf(stderr, "Error in semaphores destruction\n");
        return STATUS_ERROR;
    }
    // Freeing memory of barrier
    free(semaphores->barrier);
    return STATUS_OK;
}

int shm_init(TSMemory *memory, TSMemoryVariables *memory_variables)
{
    // Alocating memory blocks
    if (
        (memory->oxygen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->hydrogen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->count_outputs_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->count_molecules_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->barrier_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->barrier_before_building_count_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->max_molecules_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->is_building_possilbe_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->o_left_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->h_left_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1)
    {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Mapping memory blocks to pointers
    if (
        (memory_variables->oxygens_in_que = shmat(memory->oxygen_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->hydrogens_in_que = shmat(memory->hydrogen_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->count_outputs = shmat(memory->count_outputs_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->count_molecules = shmat(memory->count_molecules_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->barrier_count = shmat(memory->barrier_count_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->barrier_before_building_count = shmat(memory->barrier_before_building_count_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->max_molecules = shmat(memory->max_molecules_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->is_building_possilbe = shmat(memory->is_building_possilbe_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->o_left = shmat(memory->o_left_id, NULL, 0)) == (void *)-1 ||
        (memory_variables->h_left = shmat(memory->h_left_id, NULL, 0)) == (void *)-1)
    {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Initializing memory blocks
    *(memory_variables->oxygens_in_que) = 0;
    *(memory_variables->hydrogens_in_que) = 0;
    *(memory_variables->count_outputs) = 0;
    *(memory_variables->count_molecules) = 0;
    *(memory_variables->barrier_count) = 0;
    *(memory_variables->barrier_before_building_count) = 0;
    *(memory_variables->max_molecules) = 0;
    *(memory_variables->is_building_possilbe) = 1;
    *(memory_variables->o_left) = 0;
    *(memory_variables->h_left) = 0;

    return STATUS_OK;
}

int shm_destroy(TSMemory *memory, TSMemoryVariables *memory_variables)
{
    // Detaching memory blocks
    if (
        shmdt(memory_variables->oxygens_in_que) == -1 ||
        shmdt(memory_variables->hydrogens_in_que) == -1 ||
        shmdt(memory_variables->count_outputs) == -1 ||
        shmdt(memory_variables->count_molecules) == -1 ||
        shmdt(memory_variables->barrier_count) == -1 ||
        shmdt(memory_variables->barrier_before_building_count) == -1 ||
        shmdt(memory_variables->max_molecules) == -1 ||
        shmdt(memory_variables->is_building_possilbe) == -1 ||
        shmdt(memory_variables->o_left) == -1 ||
        shmdt(memory_variables->h_left) == -1)
    {
        fprintf(stderr, "Error in shm destruction while detaching memory blocks\n");
        return STATUS_ERROR;
    }

    // Destroying memory blocks
    if (
        shmctl(memory->oxygen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->hydrogen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->count_outputs_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->count_molecules_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->barrier_count_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->barrier_before_building_count_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->max_molecules_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->is_building_possilbe_id, IPC_RMID, NULL) == -1)
    {
        fprintf(stderr, "Error in shm destruction while destroying memory blocks\n");
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

void init_max_possible_molecules(TSMemoryVariables *memory_variables, Tparams *params)
{
    // if there are enough atoms to build at least one molecule then proceed the calculation
    if (params->NO >= 1 && params->NH >= 2)
    {

        // How many molecules can be built
        *(memory_variables->max_molecules) = params->NO > params->NH / 2 ? params->NH / 2 : params->NO;
        // How many oxygens and hydrogens will be left
        *(memory_variables->o_left) = params->NO - *(memory_variables->max_molecules);
        *(memory_variables->h_left) = params->NH - *(memory_variables->max_molecules);
    }
    else
    {
        *(memory_variables->max_molecules) = 0;
        *(memory_variables->o_left) = params->NO;
        *(memory_variables->h_left) = params->NH;
        *(memory_variables->is_building_possilbe) = 0;
    }
}

int parent_process(Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    pid_t parent_process, children_O[params->NO], children_H[params->NH], O_process_instance, H_process_instance;

    // Initializing max_possible_molecules, o_left and h_left variables
    init_max_possible_molecules(memory_variables, params);

    // First fork
    parent_process = fork();

    // Oxyde processes factory
    if (parent_process == 0)
    {
        for (int i = 0; i < params->NO; i++)
        {
            O_process_instance = fork();
            if (O_process_instance == 0)
            {
                oxygen_process(i + 1, params, semaphores, memory_variables);
            }
            children_O[i] = O_process_instance;
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NO; i++)
        {
            waitpid(children_O[i], NULL, 0);
        }
    }

    // Hydrogen processes factory
    else
    {
        for (int i = 0; i < params->NH; i++)
        {
            H_process_instance = fork();
            if (H_process_instance == 0)
            {

                hydrogen_process(i + 1, params, semaphores, memory_variables);
            }
            children_H[i] = H_process_instance;
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NH; i++)
        {

            waitpid(children_H[i], NULL, 0);
        }
    }
    if (parent_process == 0)
    {
        fclose(memory_variables->file);
        exit(0);
    }

    return STATUS_OK;
}

void oxygen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{

    // setting seed for pseudo-random number generator
    srand(getpid());
    // process start notification
    atom_start(id, type_O, semaphores, memory_variables);
    // simulating oxygen way to queue
    usleep(rand() % (params->TI + 1) * 1000);

    // taking/waiting for building mutex
    sem_wait(semaphores->building_mutex);

    // taking/waiting for writing mutex
    sem_wait(semaphores->writing_mutex);
    // increasing count oxygen count
    (*memory_variables->oxygens_in_que)++;
    sem_post(semaphores->writing_mutex);

    // if there are enough hydrogens to build molecule
    if (*memory_variables->hydrogens_in_que >= 2)
    {
        // letting go two hydrogens
        sem_post(semaphores->hydQueue);
        sem_post(semaphores->hydQueue);
        // decreasing number of existing hydrogens
        (*memory_variables->hydrogens_in_que) -= 2;
        // letting go one oxygen in que
        sem_post(semaphores->oxyQueue);
        // decreasing number of existing oxygens
        (*memory_variables->oxygens_in_que)--;
    }
    // else letting go of building_mutex
    else
    {
        sem_post(semaphores->building_mutex);
    }

    // process in que notification
    atom_to_queue(id, type_O, semaphores, memory_variables);


    // if building is not possible before going to queue
    // don't even go there and exit
    if (*memory_variables->is_building_possilbe == 0)
    {
        O_not_enough(id, semaphores, memory_variables);
        fclose(memory_variables->file);
        exit(0);
    }

    
    // wating place for oxygen
    sem_wait(semaphores->oxyQueue);

    // if there are no more molecules that are possible to build
    // end the process
    if (*memory_variables->is_building_possilbe == 0)
    {
        O_not_enough(id, semaphores, memory_variables);
        fclose(memory_variables->file);
        exit(0);
    }

    wait_barrier(semaphores->barrier_before_building, memory_variables->barrier_before_building_count);
    // creating molecule
    // process creating molecule notification
    atom_creating_molecule(id, type_O, semaphores, memory_variables);
    // simulating molecule creation
    usleep(rand() % (params->TB + 1) * 1000);

    wait_barrier_phase_1(semaphores->barrier, memory_variables->barrier_count);
    // safely incrementing count of molecules inside of barrier
    inc_molecule_count(semaphores, memory_variables);
    wait_barrier_phase_2(semaphores->barrier, memory_variables->barrier_count);

    // process created molecule notification
    molecule_created(id, type_O, semaphores, memory_variables);

    // letting go of building_mutex
    sem_post(semaphores->building_mutex);

    // if there are no more molecules to build
    if (*memory_variables->max_molecules == *memory_variables->count_molecules)
    {
        // taking/waiting for writing mutex
        sem_wait(semaphores->writing_mutex);
        *memory_variables->is_building_possilbe = 0;
        sem_post(semaphores->writing_mutex);
        // letting go all remaining oxygens from queue
        for (int i = 0; i < *(memory_variables->o_left); i++)
        {
            sem_post(semaphores->oxyQueue);
        }
        // letting go all remaining hydrogens from queue
        for (int i = 0; i < *(memory_variables->h_left); i++)
        {
            sem_post(semaphores->hydQueue);
        }
    }
    fclose(memory_variables->file);
    exit(STATUS_OK);
}
void hydrogen_process(int id, Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    // setting seed for pseudo-random number generator
    srand(getpid());
    // process start notification
    atom_start(id, type_H, semaphores, memory_variables);
    // simulating hydrogen way to queue
    usleep(rand() % (params->TI + 1) * 1000);

    // if building is not possible before going to queue
    // don't even go there and exit
    if (*memory_variables->is_building_possilbe == 0)
    {
        O_not_enough(id, semaphores, memory_variables);
        fclose(memory_variables->file);
        exit(0);
    }

    // increasing number of existing hydrogens
    sem_wait(semaphores->building_mutex);
    (*memory_variables->hydrogens_in_que)++;

    // if there are enough hydrogens and oxygen
    if (*memory_variables->hydrogens_in_que >= 2 && *memory_variables->oxygens_in_que >= 1)
    {
        // letting go two hydrogens
        sem_post(semaphores->hydQueue);
        sem_post(semaphores->hydQueue);
        // decreasing number of existing hydrogens
        (*memory_variables->hydrogens_in_que) -= 2;
        // letting go one oxygen in que
        sem_post(semaphores->oxyQueue);
        // decreasing number of existing oxygens
        (*memory_variables->oxygens_in_que)--;
    }
    else
    {
        sem_post(semaphores->building_mutex);
    }

    // process in que notification
    atom_to_queue(id, type_H, semaphores, memory_variables);

    // wating place for hydrogen
    sem_wait(semaphores->hydQueue);

    // if there are no more molecules that are possible to build
    // end the process
    if (*memory_variables->is_building_possilbe == 0)
    {
        H_not_enough(id, semaphores, memory_variables);
        fclose(memory_variables->file);
        exit(0);
    }
    wait_barrier(semaphores->barrier_before_building, memory_variables->barrier_before_building_count);

    // creating molecule
    // process creating molecule notification
    atom_creating_molecule(id, type_H, semaphores, memory_variables);
    // barrier waiting for other oxygen and hydrogen
    wait_barrier(semaphores->barrier, memory_variables->barrier_count);

    // molecule created notification
    molecule_created(id, type_H, semaphores, memory_variables);

    fclose(memory_variables->file);
    exit(STATUS_OK);
}

void wait_barrier_phase_1(TBarrier *barrier, int *count)
{
    // phase 1
    // solutiotion taken from The Little Book of Semaphores
    sem_wait(barrier->mutex);

    (*count)++;
    if (*count == barrier->n)
    {
        sem_wait(barrier->turnstile2);
        sem_post(barrier->turnstile);
    }
    sem_post(barrier->mutex);

    sem_wait(barrier->turnstile);
    sem_post(barrier->turnstile);
}

void wait_barrier_phase_2(TBarrier *barrier, int *count)
{
    // phase 2
    // solutiotion taken from The Little Book of Semaphores
    sem_wait(barrier->mutex);
    (*count)--;
    if (*count == 0)
    {
        sem_wait(barrier->turnstile);
        sem_post(barrier->turnstile2);
    }
    sem_post(barrier->mutex);
    sem_wait(barrier->turnstile2);
    sem_post(barrier->turnstile2);
}

void wait_barrier(TBarrier *barrier, int *count)
{
    wait_barrier_phase_1(barrier, count);
    wait_barrier_phase_2(barrier, count);
}

// notification functions

void atom_start(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: %c %d: started\n", *(memory_variables->count_outputs), type, id);
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void atom_to_queue(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: %c %d: going to que\n", *(memory_variables->count_outputs), type, id);
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void atom_creating_molecule(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: %c %d: creating molecule %d\n", *(memory_variables->count_outputs), type, id, *(memory_variables->count_molecules) + 1);
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void molecule_created(int id, char type, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: %c %d: molecule %d created\n", *(memory_variables->count_outputs), type, id, *(memory_variables->count_molecules));
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void O_not_enough(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: O %d: not enough H\n", *(memory_variables->count_outputs), id);
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void H_not_enough(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_outputs)++;
    fprintf(file, "%d: H %d: not enough O or H\n", *(memory_variables->count_outputs), id);
    fflush(file);
    sem_post(semaphores->writing_mutex);
}

void inc_molecule_count(TSemaphores *semaphores, TSMemoryVariables *memory_variables)
{
    sem_wait(semaphores->writing_mutex);
    (*memory_variables->count_molecules)++;
    sem_post(semaphores->writing_mutex);
}
