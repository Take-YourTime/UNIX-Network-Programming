// D R W Q

#define _POSIX_C_SOURCE 200809L

#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define FRONTEND "tcp://127.0.0.1:5555"   // Producer -> Broker
#define BACKEND  "tcp://127.0.0.1:5556"   // Worker   -> Broker
#define MONITOR  "tcp://127.0.0.1:5557"   // Broker   -> Monitor

#define TASK_SIZE 80
#define MAX_ID_SIZE 64
#define MAX_QUEUE 100000
#define MAX_WORKERS 10000

typedef struct {
    char data[TASK_SIZE];
    int task_id;
    long long created_ms;
} Task;

typedef struct {
    Task *items;
    int head, tail, size, cap;
} TaskQueue;

typedef struct {
    char ids[MAX_WORKERS][MAX_ID_SIZE];
    int head, tail, size;
} WorkerQueue;

static volatile sig_atomic_t running = 1;

static void on_sigterm(int sig) {
    (void)sig;
    running = 0;
}

static long long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static void sleep_ms(int ms) {
    if (ms <= 0) return;

    struct timespec req;
    struct timespec rem;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (long)(ms % 1000) * 1000000L;

    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        if (!running) break;
    }
}

static void sleep_ns(long ns) {
    if (ns <= 0) return;

    struct timespec req;
    struct timespec rem;
    req.tv_sec = ns / 1000000000L;
    req.tv_nsec = ns % 1000000000L;

    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        if (!running) break;
    }
}

static int send_text(void *sock, const char *s, int flags) {
    return zmq_send(sock, s, strlen(s), flags);
}

static int recv_text(void *sock, char *buf, size_t size) {
    int n = zmq_recv(sock, buf, size - 1, 0);
    if (n < 0) return -1;
    buf[n] = '\0';
    return n;
}

static void tq_init(TaskQueue *q, int cap) {
    q->items = calloc((size_t)cap, sizeof(Task));
    if (q->items == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    q->head = q->tail = q->size = 0;
    q->cap = cap;
}

static void tq_free(TaskQueue *q) {
    free(q->items);
}

static int tq_push(TaskQueue *q, const Task *t) {
    if (q->size >= q->cap) return 0;
    q->items[q->tail] = *t;
    q->tail = (q->tail + 1) % q->cap;
    q->size++;
    return 1;
}

static int tq_pop(TaskQueue *q, Task *out) {
    if (q->size <= 0) return 0;
    *out = q->items[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return 1;
}

static void wq_init(WorkerQueue *q) {
    q->head = q->tail = q->size = 0;
}

static int wq_push(WorkerQueue *q, const char *id) {
    if (q->size >= MAX_WORKERS) return 0;
    snprintf(q->ids[q->tail], MAX_ID_SIZE, "%s", id);
    q->tail = (q->tail + 1) % MAX_WORKERS;
    q->size++;
    return 1;
}

static int wq_pop(WorkerQueue *q, char *out) {
    if (q->size <= 0) return 0;
    snprintf(out, MAX_ID_SIZE, "%s", q->ids[q->head]);
    q->head = (q->head + 1) % MAX_WORKERS;
    q->size--;
    return 1;
}

static void producer(int D, int R) {
    signal(SIGTERM, on_sigterm);
    signal(SIGINT, on_sigterm);

    void *ctx = zmq_ctx_new();
    void *sock = zmq_socket(ctx, ZMQ_DEALER);
    zmq_setsockopt(sock, ZMQ_IDENTITY, "producer", 8);
    zmq_connect(sock, FRONTEND);

    for (int i = 1; running && i <= D; i++) {
        char msg[TASK_SIZE];
        char tmp[128];
        long long t = now_ms();

        memset(msg, ' ', sizeof(msg));
        snprintf(tmp, sizeof(tmp), "TASK %d %lld : This is message %d", i, t, i);
        memcpy(msg, tmp, strlen(tmp) < TASK_SIZE ? strlen(tmp) : TASK_SIZE);

        zmq_send(sock, "", 0, ZMQ_SNDMORE);
        zmq_send(sock, msg, TASK_SIZE, 0);

        sleep_ms(R);
    }

    zmq_close(sock);
    zmq_ctx_destroy(ctx);
    exit(0);
}

static void worker(int id) {
    signal(SIGTERM, on_sigterm);
    signal(SIGINT, on_sigterm);

    srand((unsigned int)(time(NULL) ^ getpid()));

    void *ctx = zmq_ctx_new();
    void *sock = zmq_socket(ctx, ZMQ_DEALER);

    char identity[MAX_ID_SIZE];
    snprintf(identity, sizeof(identity), "worker-%d", id);
    zmq_setsockopt(sock, ZMQ_IDENTITY, identity, strlen(identity));
    zmq_connect(sock, BACKEND);

    // Initial ready signal. After each task, DONE also means this worker is ready again.
    zmq_send(sock, "", 0, ZMQ_SNDMORE);
    send_text(sock, "READY", 0);

    while (running) {
        char empty[8];
        char task[TASK_SIZE + 1];

        if (recv_text(sock, empty, sizeof(empty)) < 0) break;
        int n = zmq_recv(sock, task, TASK_SIZE, 0);
        if (n < 0) break;
        task[n] = '\0';

        int task_id = 0;
        long long created_ms = 0;
        sscanf(task, "TASK %d %lld", &task_id, &created_ms);

        // Simulate 1000 ~ 100000 microseconds = 1 ~ 100 ms.
        long random_ns = (long)(1000000 + rand() % 99000001);
        sleep_ns(random_ns);

        char done[128];
        snprintf(done, sizeof(done), "DONE %d %lld", task_id, created_ms);
        zmq_send(sock, "", 0, ZMQ_SNDMORE);
        send_text(sock, done, 0);
    }

    zmq_close(sock);
    zmq_ctx_destroy(ctx);
    exit(0);
}

static void monitor_client(void) {
    signal(SIGTERM, on_sigterm);
    signal(SIGINT, on_sigterm);

    void *ctx = zmq_ctx_new();
    void *sub = zmq_socket(ctx, ZMQ_SUB);
    zmq_setsockopt(sub, ZMQ_SUBSCRIBE, "", 0);
    zmq_connect(sub, MONITOR);

    while (running) {
        char msg[256];
        if (recv_text(sub, msg, sizeof(msg)) < 0) break;
        /* uncomment this to print the running steps */
        // printf("\033[2J\033[H");
        // printf("[Monitor]\n%s\n", msg);
        // fflush(stdout);
    }

    zmq_close(sub);
    zmq_ctx_destroy(ctx);
    exit(0);
}

static void dispatch_tasks(void *backend, TaskQueue *tasks, WorkerQueue *workers) {
    while (tasks->size > 0 && workers->size > 0) {
        Task t;
        char worker_id[MAX_ID_SIZE];

        tq_pop(tasks, &t);
        wq_pop(workers, worker_id);

        send_text(backend, worker_id, ZMQ_SNDMORE);
        zmq_send(backend, "", 0, ZMQ_SNDMORE);
        zmq_send(backend, t.data, TASK_SIZE, 0);
    }
}

static void broker(int D, int R, int W, int Q) {
    void *ctx = zmq_ctx_new();
    void *frontend = zmq_socket(ctx, ZMQ_ROUTER);
    void *backend = zmq_socket(ctx, ZMQ_ROUTER);
    void *pub = zmq_socket(ctx, ZMQ_PUB);

    zmq_bind(frontend, FRONTEND);
    zmq_bind(backend, BACKEND);
    zmq_bind(pub, MONITOR);

    TaskQueue tasks;
    WorkerQueue workers;
    tq_init(&tasks, Q);
    wq_init(&workers);

    long long processed = 0, dropped = 0;
    long long total_latency = 0;
    long long start = now_ms();
    long long last_pub = 0;

    while (processed + dropped < D) {
        zmq_pollitem_t items[] = {
            { frontend, 0, ZMQ_POLLIN, 0 },
            { backend,  0, ZMQ_POLLIN, 0 }
        };
        zmq_poll(items, 2, 20);

        if (items[0].revents & ZMQ_POLLIN) {
            char producer_id[MAX_ID_SIZE];
            char empty[8];
            char body[TASK_SIZE + 1];

            if (recv_text(frontend, producer_id, sizeof(producer_id)) < 0) continue;
            if (recv_text(frontend, empty, sizeof(empty)) < 0) continue;
            int n = zmq_recv(frontend, body, TASK_SIZE, 0);
            if (n < 0) continue;
            body[n] = '\0';

            Task t;
            memset(&t, 0, sizeof(t));
            memcpy(t.data, body, TASK_SIZE);
            sscanf(body, "TASK %d %lld", &t.task_id, &t.created_ms);

            if (!tq_push(&tasks, &t)) {
                dropped++;
            }
        }

        if (items[1].revents & ZMQ_POLLIN) {
            char worker_id[MAX_ID_SIZE];
            char empty[8];
            char body[128];

            if (recv_text(backend, worker_id, sizeof(worker_id)) < 0) continue;
            if (recv_text(backend, empty, sizeof(empty)) < 0) continue;
            if (recv_text(backend, body, sizeof(body)) < 0) continue;

            if (strncmp(body, "READY", 5) == 0) {
                wq_push(&workers, worker_id);
            } else if (strncmp(body, "DONE", 4) == 0) {
                int task_id = 0;
                long long created_ms = 0;
                sscanf(body, "DONE %d %lld", &task_id, &created_ms);
                processed++;
                total_latency += now_ms() - created_ms;
                wq_push(&workers, worker_id);
            }
        }

        dispatch_tasks(backend, &tasks, &workers);

        long long now = now_ms();
        if (now - last_pub >= 1000) {
            char stat[256];
            snprintf(stat, sizeof(stat),
                     "Queue size: %d\nIdle workers: %d\nProcessed: %lld\nDropped: %lld",
                     tasks.size, workers.size, processed, dropped);
            send_text(pub, stat, 0);
            last_pub = now;
        }
    }

    long long elapsed = now_ms() - start;
    double loss_rate = (D > 0) ? (100.0 * dropped / D) : 0.0;
    double avg_latency = (processed > 0) ? ((double)total_latency / processed) : 0.0;
    double throughput = (elapsed > 0) ? (processed * 1000.0 / elapsed) : 0.0;

    printf("\n===== Experiment Result =====\n");
	printf("D=%d R=%d W=%d Q=%d\n", D, R, W, Q);
    printf("Total tasks: %d\n", D);
    printf("Processed tasks: %lld\n", processed);
    printf("Dropped tasks: %lld\n", dropped);
    printf("Loss rate: %.2f%%\n", loss_rate);
    printf("Average latency: %.2f ms\n", avg_latency);
    printf("Throughput: %.2f tasks/sec\n", throughput);

    tq_free(&tasks);
    zmq_close(frontend);
    zmq_close(backend);
    zmq_close(pub);
    zmq_ctx_destroy(ctx);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s [Task Count D] [Task Interval R ms] [Worker Count W] [Queue Size Q]\n", argv[0]);
        return 1;
    }

    int D = atoi(argv[1]);
    int R = atoi(argv[2]);
    int W = atoi(argv[3]);
    int Q = atoi(argv[4]);

    if (D <= 0 || R < 0 || W <= 0 || Q <= 0 || Q > MAX_QUEUE || W > MAX_WORKERS) {
        fprintf(stderr, "Invalid arguments. D>0, R>=0, 0<W<=%d, 0<Q<=%d\n", MAX_WORKERS, MAX_QUEUE);
        return 1;
    }

    pid_t *children = calloc((size_t)W + 2, sizeof(pid_t));
    if (children == NULL) {
        perror("calloc");
        return 1;
    }

    int child_count = 0;

    pid_t broker_pid = fork();
    if (broker_pid == 0) {
        broker(D, R, W, Q);
        exit(0);
    } else if (broker_pid < 0) {
        perror("fork broker");
        free(children);
        return 1;
    }

    sleep_ms(300); // wait for broker bind

    pid_t mon = fork();
    if (mon == 0) monitor_client();
    if (mon > 0) children[child_count++] = mon;

    for (int i = 0; i < W; i++) {
        pid_t p = fork();
        if (p == 0) worker(i + 1);
        if (p > 0) children[child_count++] = p;
    }

    sleep_ms(300); // wait for workers to send READY

    pid_t prod = fork();
    if (prod == 0) producer(D, R);
    if (prod > 0) children[child_count++] = prod;

    waitpid(broker_pid, NULL, 0);

    for (int i = 0; i < child_count; i++) {
        kill(children[i], SIGTERM);
    }
    for (int i = 0; i < child_count; i++) {
        waitpid(children[i], NULL, 0);
    }

    free(children);
    return 0;
}
