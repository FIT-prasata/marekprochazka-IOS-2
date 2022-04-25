#include "main.h"

int main(int argc, char *argv[])
{
    if (
        (handle_args(argc, argv, &params) != STATUS_OK) ||
        (semaphores_init(&semaphores) != STATUS_OK) ||
        (semaphores_destroy(&semaphores) != STATUS_OK)
    )
    {
        return STATUS_ERROR;
    }
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