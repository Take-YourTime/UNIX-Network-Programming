#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <zmq.h>

#include "common.h"

/*
 * ready_clients：Ready Queue。
 * 每一格保存一個空閒 Worker 的 ZeroMQ identity。
 * 只有在這個佇列裡面的 Worker 才可以接收新任務。
 */
char ready_clients[MAX_CLIENTS][CLIENT_ID_SIZE];

/* ready_count：目前 Ready Queue 中有多少個空閒 Worker。 */
int ready_count = 0;

/*
 * waiting_tasks：尚未派發的任務佇列。
 * 當使用者輸入任務，但沒有空閒 Worker 時，任務會暫存在這裡。
 */
char waiting_tasks[MAX_TASKS][MESSAGE_SIZE];

/* waiting_count：目前 waiting_tasks 中有多少個任務。 */
int waiting_count = 0;

/*
 * RunningTask：記錄已經派發、但尚未收到 RESULT 的任務。
 * 這個結構主要提供 Bonus 的 Client timeout 偵測功能使用。
 *
 * used      ：這一格是否正在使用。
 * client_id ：目前負責該任務的 Worker identity。
 * task_msg  ：原始 TASK 訊息。若 Worker timeout，可重新派發給其他 Worker。
 * start_ms  ：任務開始派發的時間，用來計算是否逾時。
 */
typedef struct {
    int used;
    char client_id[CLIENT_ID_SIZE];
    char task_msg[MESSAGE_SIZE];
    long long start_ms;
} RunningTask;

/* running：最多同時追蹤 MAX_TASKS 個執行中任務。 */
RunningTask running[MAX_TASKS];

/*
 * now_ms()
 * 回傳目前時間，單位為毫秒。
 * Dispatcher 使用此函式計算任務是否超過 TASK_TIMEOUT。
 */
long long now_ms()
{
    struct timeval tv; /* 保存目前時間的秒數與微秒數。 */
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*
 * time_string()
 * 將目前時間轉換為 HH:MM:SS.mmm 格式，提供 log 使用。
 *
 * out：呼叫端提供的字元陣列，用來接收格式化後的時間字串。
 */
void time_string(char *out)
{
    struct timeval tv;    /* 保存秒數與微秒數。 */
    struct tm *tm_info;   /* 保存轉換後的本地時間。 */

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    sprintf(out, "%02d:%02d:%02d.%03ld",
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            tv.tv_usec / 1000);
}

/*
 * log_msg()
 * 將 Dispatcher 的 log 同時輸出到終端機與 dispatcher.log。
 *
 * level   ：日誌層級，例如 INFO、WARN、ERROR。
 * task_id ：任務 ID。若當下沒有對應任務，使用 "-"。
 * text    ：要記錄的動作與細節。
 */
void log_msg(const char *level, const char *task_id, const char *text)
{
    char t[32]; /* 保存格式化後的時間。 */
    FILE *fp;   /* 指向 dispatcher.log 的檔案指標。 */

    time_string(t);
    printf("[%s] [Dispatcher] [%s] [%s] %s\n", t, level, task_id, text);
    fflush(stdout);

    fp = fopen("dispatcher.log", "a");
    if (fp != NULL) {
        fprintf(fp, "[%s] [Dispatcher] [%s] [%s] %s\n", t, level, task_id, text);
        fclose(fp);
    }
}

/*
 * push_ready()
 * 將空閒 Worker 放入 Ready Queue。
 * 若該 Worker 已經在佇列中，就不會重複加入。
 *
 * client_id：Worker 的 ZeroMQ identity。
 */
void push_ready(const char *client_id)
{
    int i; /* 走訪 Ready Queue 使用的索引。 */

    for (i = 0; i < ready_count; i++) {
        if (strcmp(ready_clients[i], client_id) == 0)
            return;
    }

    if (ready_count < MAX_CLIENTS) {
        strcpy(ready_clients[ready_count], client_id);
        ready_count++;
    } else {
        log_msg("ERROR", "-", "Ready Queue is full.");
    }
}

/*
 * pop_ready()
 * 從 Ready Queue 取出最早加入的空閒 Worker。
 * 這是一個簡單的 FIFO queue。
 *
 * client_id：輸出參數，用來接收取出的 Worker identity。
 * 回傳值   ：成功取出回傳 true；佇列為空回傳 false。
 */
int pop_ready(char *client_id)
{
    int i; /* 將陣列內容往前搬移時使用的索引。 */

    if (ready_count == 0)
        return false;

    strcpy(client_id, ready_clients[0]);

    for (i = 1; i < ready_count; i++)
        strcpy(ready_clients[i - 1], ready_clients[i]);

    ready_count--;
    return true;
}

/*
 * send_to_client()
 * 使用 ROUTER socket 將訊息傳給指定 Worker。
 * ROUTER 傳送訊息時，第一個 frame 必須是 Worker identity，
 * 第二個 frame 才是實際訊息內容。
 *
 * router    ：Dispatcher 的 ROUTER socket。
 * client_id ：接收訊息的 Worker identity。
 * msg       ：實際要傳送的訊息。
 */
void send_to_client(void *router, const char *client_id, const char *msg)
{
    zmq_send(router, client_id, strlen(client_id), ZMQ_SNDMORE);
    zmq_send(router, msg, strlen(msg), 0);
}

/*
 * push_waiting_task()
 * 將尚未能派發的任務加入等待佇列。
 *
 * task_msg：完整 TASK 訊息。
 * 回傳值  ：成功加入回傳 true；佇列已滿回傳 false。
 */
int push_waiting_task(const char *task_msg)
{
    if (waiting_count >= MAX_TASKS)
        return false;

    strcpy(waiting_tasks[waiting_count], task_msg);
    waiting_count++;
    return true;
}

/*
 * pop_waiting_task()
 * 從等待佇列取出最早加入的任務。
 *
 * task_msg：輸出參數，用來接收取出的 TASK 訊息。
 * 回傳值  ：成功取出回傳 true；佇列為空回傳 false。
 */
int pop_waiting_task(char *task_msg)
{
    int i; /* 將陣列內容往前搬移時使用的索引。 */

    if (waiting_count == 0)
        return false;

    strcpy(task_msg, waiting_tasks[0]);

    for (i = 1; i < waiting_count; i++)
        strcpy(waiting_tasks[i - 1], waiting_tasks[i]);

    waiting_count--;
    return true;
}

/*
 * add_running()
 * 記錄一個已經派發、尚未完成的任務。
 *
 * client_id：目前負責任務的 Worker identity。
 * task_msg ：原始 TASK 訊息。
 */
void add_running(const char *client_id, const char *task_msg)
{
    int i; /* 尋找空白 running 欄位時使用的索引。 */

    for (i = 0; i < MAX_TASKS; i++) {
        if (!running[i].used) {
            running[i].used = true;
            strcpy(running[i].client_id, client_id);
            strcpy(running[i].task_msg, task_msg);
            running[i].start_ms = now_ms();
            return;
        }
    }

    log_msg("ERROR", "-", "Running task table is full.");
}

/*
 * finish_running_by_client()
 * Dispatcher 收到 RESULT 後，將該 Worker 對應的執行中任務標記為完成。
 *
 * client_id：回報 RESULT 的 Worker identity。
 */
void finish_running_by_client(const char *client_id)
{
    int i; /* 走訪 running 陣列使用的索引。 */

    for (i = 0; i < MAX_TASKS; i++) {
        if (running[i].used && strcmp(running[i].client_id, client_id) == 0) {
            running[i].used = false;
            return;
        }
    }
}

/*
 * dispatch_waiting_tasks()
 * 只要 Ready Queue 與 Waiting Queue 都有資料，就持續配對：
 * 1. 取出一個空閒 Worker。
 * 2. 取出一個等待中的任務。
 * 3. 傳送 TASK 給 Worker。
 * 4. 將任務記錄為執行中。
 */
void dispatch_waiting_tasks(void *router)
{
    char client_id[CLIENT_ID_SIZE]; /* 即將接收任務的 Worker identity。 */
    char task_msg[MESSAGE_SIZE];    /* 即將派發的 TASK 訊息。 */
    char text[MESSAGE_SIZE];        /* log 文字。 */

    while (ready_count > 0 && waiting_count > 0) {
        if (!pop_ready(client_id))
            return;

        if (!pop_waiting_task(task_msg))
            return;

        send_to_client(router, client_id, task_msg);
        add_running(client_id, task_msg);

        sprintf(text, "Waiting task sent to %s.", client_id);
        log_msg("INFO", "-", text);
    }
}

/*
 * check_timeout()
 * Bonus：檢查執行中任務是否長時間沒有收到 RESULT。
 * 若 timeout，嘗試將原始 TASK 訊息重新派發給其他空閒 Worker。
 *
 * 注意：這是簡化版的崩潰恢復。若舊 Worker 其實只是很慢、並未真正崩潰，
 * 舊 Worker 與新 Worker 可能同時執行同一份搬家任務。正式系統通常還需要
 * lease、取消機制或額外的 Job ID。
 */
void check_timeout(void *router)
{
    int i;                     /* 走訪 running 陣列使用的索引。 */
    long long t = now_ms();    /* 目前時間。 */
    char text[MESSAGE_SIZE];   /* log 文字。 */
    char client_id[CLIENT_ID_SIZE]; /* 接手 timeout 任務的新 Worker identity。 */

    for (i = 0; i < MAX_TASKS; i++) {
        if (running[i].used && t - running[i].start_ms > TASK_TIMEOUT) {
            sprintf(text, "Client %s timeout. Re-dispatch task.", running[i].client_id);
            log_msg("WARN", "-", text);

            if (pop_ready(client_id)) {
                send_to_client(router, client_id, running[i].task_msg);
                strcpy(running[i].client_id, client_id);
                running[i].start_ms = now_ms();

                sprintf(text, "Task re-dispatched to %s.", client_id);
                log_msg("INFO", "-", text);
            } else {
                /* 沒有其他空閒 Worker 時，保留任務並延後下一次 timeout 檢查。 */
                running[i].start_ms = now_ms();
                log_msg("WARN", "-", "No ready client. Task waits.");
            }
        }
    }
}

/*
 * main()
 * Dispatcher 主流程：
 * 1. 建立 ROUTER socket 並綁定 port 5555。
 * 2. 使用 zmq_poll() 同時監聽 Worker 訊息與 stdin。
 * 3. 收到 READY 時，將 Worker 放入 Ready Queue。
 * 4. 收到 RESULT 時，將 Worker 重新放回 Ready Queue。
 * 5. 收到使用者輸入時，只把任務派給 Ready Queue 中的 Worker。
 * 6. 每次迴圈檢查是否有 Worker timeout。
 */
int main()
{
    void *context = zmq_ctx_new();               /* ZeroMQ context。 */
    void *router = zmq_socket(context, ZMQ_ROUTER); /* 與 Worker 通訊的 ROUTER socket。 */
    zmq_pollitem_t items[2];                     /* 同時監聽 ROUTER socket 與 stdin。 */

    if (router == NULL) {
        printf("Cannot create Dispatcher ROUTER socket: %s\n", zmq_strerror(zmq_errno()));
        zmq_ctx_destroy(context);
        return 1;
    }

    if (zmq_bind(router, DISPATCHER_BIND_ADDR) != 0) {
        printf("Cannot bind Dispatcher to %s: %s\n",
               DISPATCHER_BIND_ADDR, zmq_strerror(zmq_errno()));
        printf("Please check whether another Dispatcher is already running.\n");
        zmq_close(router);
        zmq_ctx_destroy(context);
        return 1;
    }

    log_msg("INFO", "-", "Dispatcher started.");
    printf("Input format: TRUCK WORKER INSURANCE DURATION\n");
    printf("Example: 2 4 100 3\n");

    /* items[0]：監聽來自 Worker 的 READY 或 RESULT。 */
    items[0].socket = router;
    items[0].fd = 0;
    items[0].events = ZMQ_POLLIN;

    /* items[1]：監聽使用者從終端機輸入的新任務。 */
    items[1].socket = NULL;
    items[1].fd = 0;
    items[1].events = ZMQ_POLLIN;

    while (true) {
        int rc = zmq_poll(items, 2, 500); /* 每次最多等待 500 ms，之後檢查 timeout。 */

        if (rc == -1)
            break;

        /* 處理來自 Worker 的 READY 或 RESULT。 */
        if (items[0].revents & ZMQ_POLLIN) {
            zmq_msg_t identity, msg;          /* ROUTER 收到的 identity frame 與訊息 frame。 */
            char client_id[CLIENT_ID_SIZE];   /* Worker identity。 */
            char buffer[MESSAGE_SIZE];        /* 收到的 READY 或 RESULT 文字。 */
            char task_id[TASK_ID_SIZE];       /* RESULT 中的 Task ID。 */
            char text[MESSAGE_SIZE];          /* log 文字。 */
            int id_size;                      /* identity frame 的實際長度。 */
            int msg_size;                     /* 訊息 frame 的實際長度。 */

            zmq_msg_init(&identity);
            zmq_msg_recv(&identity, router, 0);

            id_size = zmq_msg_size(&identity);
            if (id_size >= (int)sizeof(client_id))
                id_size = sizeof(client_id) - 1;

            memcpy(client_id, zmq_msg_data(&identity), id_size);
            client_id[id_size] = '\0';

            zmq_msg_init(&msg);
            zmq_msg_recv(&msg, router, 0);

            msg_size = zmq_msg_size(&msg);
            if (msg_size >= (int)sizeof(buffer))
                msg_size = sizeof(buffer) - 1;

            memcpy(buffer, zmq_msg_data(&msg), msg_size);
            buffer[msg_size] = '\0';

            if (strcmp(buffer, "READY") == 0) {
                /* Worker 啟動後主動送出 READY，表示可接收任務。 */
                push_ready(client_id);
                sprintf(text, "Client %s is ready.", client_id);
                log_msg("INFO", "-", text);
                dispatch_waiting_tasks(router);
            } else if (strncmp(buffer, "RESULT", 6) == 0) {
                /* Worker 完成或放棄任務後回報 RESULT，再次成為空閒 Worker。 */
                if (sscanf(buffer, "RESULT %63s", task_id) == 1) {
                    finish_running_by_client(client_id);
                    push_ready(client_id);
                    sprintf(text, "Result received from %s.", client_id);
                    log_msg("INFO", task_id, text);
                    dispatch_waiting_tasks(router);
                } else {
                    log_msg("ERROR", "-", "Bad RESULT message from Worker.");
                }
            } else {
                log_msg("ERROR", "-", "Unknown message from Worker.");
            }

            zmq_msg_close(&identity);
            zmq_msg_close(&msg);
        }

        /* 處理使用者從 stdin 輸入的新任務。 */
        if (items[1].revents & ZMQ_POLLIN) {
            char line[MESSAGE_SIZE];          /* 使用者輸入的一整行文字。 */
            char client_id[CLIENT_ID_SIZE];   /* 即將接收任務的 Worker identity。 */
            Task task;                        /* 使用者輸入的任務需求。 */
            char task_msg[MESSAGE_SIZE];      /* 傳給 Worker 的 TASK 訊息。 */
            char text[MESSAGE_SIZE];          /* log 文字。 */

            if (fgets(line, sizeof(line), stdin) != NULL) {
                if (sscanf(line, "%d %d %d %d", &task.truck,
                           &task.worker, &task.insurance, &task.duration) == 4) {

                    /* 防止負數或零資源破壞資源計算。 */
                    if (task.truck <= 0 || task.worker <= 0 ||
                        task.insurance <= 0 || task.duration < 0) {
                        log_msg("ERROR", "-", "Task values must be positive; duration cannot be negative.");
                        continue;
                    }

                    sprintf(task_msg, "TASK %d %d %d %d",
                            task.truck, task.worker, task.insurance, task.duration);

                    if (pop_ready(client_id)) {
                        /* 有空閒 Worker：立即派發。 */
                        send_to_client(router, client_id, task_msg);
                        add_running(client_id, task_msg);
                        sprintf(text, "Task sent to %s.", client_id);
                        log_msg("INFO", "-", text);
                    } else {
                        /* 沒有空閒 Worker：放入等待佇列。 */
                        if (push_waiting_task(task_msg))
                            log_msg("WARN", "-", "No ready client. Task saved in waiting queue.");
                        else
                            log_msg("ERROR", "-", "No ready client and waiting queue is full.");
                    }
                } else {
                    log_msg("ERROR", "-", "Bad input format.");
                }
            }
        }

        /* Bonus：檢查是否有長時間未回覆 RESULT 的 Worker。 */
        check_timeout(router);
    }

    zmq_close(router);
    zmq_ctx_destroy(context);
    return 0;
}
