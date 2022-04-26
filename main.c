#include "main.h"

int main(int argc, char *argv[])
{
    
    if (handle_args(argc, argv, &params) != STATUS_OK) return STATUS_ERROR;
    if (semaphores_init(&semaphores) != STATUS_OK) return STATUS_ERROR;
    if (shm_init(&memory, &memory_variables) != STATUS_OK) return STATUS_ERROR;
    if (parent_process(&params, &semaphores, &memory_variables) != STATUS_OK) return STATUS_ERROR;
    if (semaphores_destroy(&semaphores) != STATUS_OK) return STATUS_ERROR;
    if (shm_destroy(&memory, &memory_variables) != STATUS_OK) return STATUS_ERROR;
    
    printf("%d %d %d %d\n", params.NO, params.NH, params.TI, params.TB);
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

    return STATUS_OK;
}

int semaphores_init(TSemaphores *semaphores) {

    semaphores->mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->barrier = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->oxyQue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    semaphores->hydQue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    sem_init(semaphores->mutex, 1, 1);
    sem_init(semaphores->barrier, 1, 3);
    sem_init(semaphores->oxyQue, 1, 0);
    sem_init(semaphores->hydQue, 1, 0);

    if (
        semaphores->mutex == SEM_FAILED ||
        semaphores->barrier == SEM_FAILED ||
        semaphores->oxyQue == SEM_FAILED ||
        semaphores->hydQue == SEM_FAILED
    ) {
        fprintf(stderr, "Error in semaphores initialization\n");
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

int semaphores_destroy(TSemaphores *semaphores) {
    
        sem_destroy(semaphores->mutex);
        sem_destroy(semaphores->barrier);
        sem_destroy(semaphores->oxyQue);
        sem_destroy(semaphores->hydQue);
    
        if (
            semaphores->mutex == SEM_FAILED ||
            semaphores->barrier == SEM_FAILED ||
            semaphores->oxyQue == SEM_FAILED ||
            semaphores->hydQue == SEM_FAILED
        ) {
            fprintf(stderr, "Error in semaphores destruction\n");
            return STATUS_ERROR;
        }
        return STATUS_OK;
}

int shm_init(TSMemory *memory, TSMemoryVariables *memory_variables) {
    // Alocate memory blocks 
    if (
        (memory->oxygen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->hydrogen_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1 ||
        (memory->count_outputs_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644)) == -1
    ) {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Map memory blocks to pointers
    if (
        (memory_variables->oxygen = shmat(memory->oxygen_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->hydrogen = shmat(memory->hydrogen_id, NULL, 0)) == (void *) -1 ||
        (memory_variables->count_outputs = shmat(memory->count_outputs_id, NULL, 0)) == (void *) -1
    ) {
        fprintf(stderr, "Error in shm initialization\n");
        return STATUS_ERROR;
    }
    // Initialize memory blocks
    *(memory_variables->oxygen) = 0;
    *(memory_variables->hydrogen) = 0;
    *(memory_variables->count_outputs) = 0;

    return STATUS_OK;
}

int shm_destroy(TSMemory *memory, TSMemoryVariables *memory_variables) {
    printf("%d %d %d\n", *(memory_variables->oxygen), *(memory_variables->hydrogen), *(memory_variables->count_outputs));
     // Detach memory blocks
    if (
        shmdt(memory_variables->oxygen) == -1 ||
        shmdt(memory_variables->hydrogen) == -1 ||
        shmdt(memory_variables->count_outputs) == -1
    ) {
        fprintf(stderr, "Error in shm destruction while detaching memory blocks\n");
        return STATUS_ERROR;
    }
    
    // Destroy memory blocks
    if (
        shmctl(memory->oxygen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->hydrogen_id, IPC_RMID, NULL) == -1 ||
        shmctl(memory->count_outputs_id, IPC_RMID, NULL) == -1
    ) {
        fprintf(stderr, "Error in shm destruction while destroying memory blocks\n");
        return STATUS_ERROR;
    }
   
    return STATUS_OK;
}


int parent_process(Tparams *params, TSemaphores *semaphores, TSMemoryVariables *memory_variables) {
    pid_t parent_process, children_O[params->NO], children_H[params->NH], O_process_instance, H_process_instance;


    parent_process = fork();
    printf("Parent process: %d\n", parent_process);
    // Oxyde processes
    if (parent_process == 0) {
        for (int i = 0; i < params->NO; i++) {
            O_process_instance = fork();
            if (O_process_instance == 0) {
                printf("Oxyde process %d\n", i);
                oxygen_process(i, semaphores, memory_variables);
                printf("Oxyde process %d finished\n", i);
                exit(0);
            }
            children_O[i] = O_process_instance;
            
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NO; i++) {
            waitpid(children_O[i], NULL, 0);
        }
    }

    // Hydrogen processes
    else {
        for (int i = 0; i < params->NH; i++) {
            H_process_instance = fork();
            if (H_process_instance == 0) {
                printf("Hydrogen process %d\n", i);
                hydrogen_process(i, semaphores, memory_variables);
                printf("Hydrogen process %d finished\n", i);
                exit(0);
            }
            children_H[i] = H_process_instance;
        }
        // wait for child processes to exit
        for (int i = 0; i < params->NH; i++) {
            waitpid(children_H[i], NULL, 0);
        }
    }
    if (parent_process == 0) {
        exit(0);
    }
    return STATUS_OK;
}

void oxygen_process(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables) {
    printf("Oxygen process %d started\n", id);
    UNUSED(semaphores);
    UNUSED(memory_variables);

    // while (1) {
    //     sem_wait(semaphores->mutex);
    //     if (*(memory_variables->hydrogen) == 2) {
    //         sem_post(semaphores->mutex);
    //         sem_wait(semaphores->barrier);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->oxygen) += 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->oxyQue);
    //         sem_wait(semaphores->hydQue);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->hydrogen) -= 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->barrier);
    //     }
    //     else {
    //         sem_post(semaphores->mutex);
    //         sem_wait(semaphores->barrier);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->oxygen) += 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->oxyQue);
    //         sem_wait(semaphores->hydQue);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->hydrogen) -= 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->barrier);
    //     }
    // }
}
void hydrogen_process(int id, TSemaphores *semaphores, TSMemoryVariables *memory_variables){
    printf("Hydrogen process %d started\n", id);
    UNUSED(semaphores);
    UNUSED(memory_variables);
    // while (1) {
    //     sem_wait(semaphores->mutex);
    //     if (*(memory_variables->oxygen) == 1) {
    //         sem_post(semaphores->mutex);
    //         sem_wait(semaphores->barrier);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->hydrogen) += 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->hydQue);
    //         sem_wait(semaphores->oxyQue);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->oxygen) -= 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->barrier);
    //     }
    //     else {
    //         sem_post(semaphores->mutex);
    //         sem_wait(semaphores->barrier);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->hydrogen) += 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->hydQue);
    //         sem_wait(semaphores->oxyQue);
    //         sem_wait(semaphores->mutex);
    //         *(memory_variables->oxygen) -= 1;
    //         sem_post(semaphores->mutex);
    //         sem_post(semaphores->barrier);
    //     }
    // }
}