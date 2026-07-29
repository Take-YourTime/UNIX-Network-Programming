#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define SEM_MUTEX 0
#define SEM_FULL  1

typedef struct {
    pid_t pid;
    int value;
} Data;

typedef struct {
    int stack_size;
    int write_index; // points to the next write position, also represents the number of items in the stack
    Data buffer[];
} SharedStack;

/*
 * Some systems require us to define union semun manually.
 */
union semun {
    int val;
    struct semid_ds* buf;
    unsigned short *array;
};

/* P operation: decrement semaphore */
void sem_wait_op(int semid, int sem_num) {
    struct sembuf op;

    op.sem_num = sem_num;
    op.sem_op = -1; // semophore -1
    op.sem_flg = 0;

    if (semop(semid, &op, 1) == -1) {
        perror("semop wait failed");
        exit(EXIT_FAILURE);
    }
}

/* V operation: increment semaphore */
void sem_signal_op(int semid, int sem_num) {
    struct sembuf op;

    op.sem_num = sem_num;
    op.sem_op = 1; // semophore +1
    op.sem_flg = 0;

    if (semop(semid, &op, 1) == -1) {
        perror("semop signal failed");
        exit(EXIT_FAILURE);
    }
}

void push(SharedStack* stack, int semid, Data data) {
    sem_wait_op(semid, SEM_MUTEX); // wait for lock to get in shared memory

    // get lock!

    if (stack->write_index >= stack->stack_size) {
        fprintf(stderr, "Stack overflow: buffer is full\n");
        sem_signal_op(semid, SEM_MUTEX);
        exit(EXIT_FAILURE);
    }

    stack->buffer[stack->write_index] = data;
    stack->write_index++;

    printf("Producer PID %d pushed value %d\n", data.pid, data.value);
    fflush(stdout);

    sem_signal_op(semid, SEM_MUTEX); // release lock
    sem_signal_op(semid, SEM_FULL); // signal that there's one more item in the stack
}

Data pop(SharedStack* stack, int semid) {
    Data data;

    sem_wait_op(semid, SEM_FULL); // Wait until there's at least one item in the stack
    sem_wait_op(semid, SEM_MUTEX); // Wait for lock to get in shared memory

    // get lock!

    if (stack->write_index <= 0) {
        fprintf(stderr, "Stack underflow: buffer is empty\n");
        sem_signal_op(semid, SEM_MUTEX);
        exit(EXIT_FAILURE);
    }

    stack->write_index--;
    data = stack->buffer[stack->write_index];

    sem_signal_op(semid, SEM_MUTEX); // release lock

    return data;
}

void producer_process(SharedStack* stack, int semid, int N) {
    srand(time(NULL) ^ getpid()); // use xor pid to get different seed for each producer

    for (int i = 0; i < N; i++) {
        Data data;

        data.pid = getpid();
        data.value = rand() % 10 + 1;

        push(stack, semid, data); // push data to stack

        usleep(100000);
    }

    shmdt(stack);
    exit(EXIT_SUCCESS);
}

void consumer_process(SharedStack* stack, int semid, int total_count) {
    int processed = 0; // Number of data points processed
    int sum = 0; // Cumulative sum of values

    while (processed < total_count) {
        Data data = pop(stack, semid);

        processed++;
        sum += data.value;

        printf("Consumer read: PID = %d, Value = %d, Current Sum = %d\n", data.pid, data.value, sum);
        fflush(stdout);
    }

    printf("\nTotal Data Points Processed: %d\n", processed);
    printf("Final Cumulative Sum: %d\n", sum);

    shmdt(stack);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int producer; // M, the number of producers
    int n;        // N, the number of data points each producer generates
    
    int shmid;    // Shared memory ID
    int semid;    // Semaphore ID

    size_t shm_size;

    SharedStack* stack;
    pid_t pid;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s M N\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // commend analysis
    producer = atoi(argv[1]);
    n = atoi(argv[2]);

    if (producer <= 0 || n <= 0) {
        fprintf(stderr, "M and N must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    shm_size = sizeof(SharedStack) + sizeof(Data) * producer * n;

    // Create shared memory segment
    shmid = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    stack = (SharedStack *)shmat(shmid, NULL, 0);
    if (stack == (void *)-1) {
        perror("shmat failed");
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    stack->stack_size = producer * n;
    stack->write_index = 0;
    
    // Create semaphore set with 2 semaphores: 
    // 0 (SEM_MUTEX) -> mutex
    // 1 (SEM_FULL)  -> counting semaphore for full slots
    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600); // 0600 = rw- --- ---
    if (semid == -1) {
        perror("semget failed");
        shmdt(stack);
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    union semun arg;
    
    arg.val = 1; // mutext initialized to 1 (unlocked)
    if (semctl(semid, SEM_MUTEX, SETVAL, arg) == -1) {
        perror("semctl mutex failed");
        shmdt(stack);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        exit(EXIT_FAILURE);
    }
    
    arg.val = 0; // full initialized to 0 (no data in buffer)
    if (semctl(semid, SEM_FULL, SETVAL, arg) == -1) {
        perror("semctl full failed");
        shmdt(stack);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        exit(EXIT_FAILURE);
    }


    // consumer_process
    switch( pid = fork() )
    {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);
        case 0:
            consumer_process(stack, semid, producer * n);
            // if success, following code will never execute
            perror("consumer process failed");
            exit(EXIT_FAILURE);
        default:
            break;
    }


    // producer_process
    for (int i = 0; i < producer; i++) {
        switch ( pid = fork() ) {
            case -1:
                perror("fork producer failed");
                exit(EXIT_FAILURE);
            case 0:
                producer_process(stack, semid, n);
                // if success, following code will never execute
                perror("producer process failed");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    for (int i = 0; i < producer + 1; i++) {
        wait(NULL);
    }


    // Cleanup
    shmdt(stack);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
