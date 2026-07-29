#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <zmq.h>

#include "common.h"

/*
 * now_ms()
 * 回傳目前時間，單位為毫秒。
 * Worker 會將此數值放入 Task ID，降低不同任務產生相同 ID 的機率。
 */
long long now_ms()
{
    struct timeval tv; /* 保存目前時間的秒數與微秒數。 */
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/*
 * time_string()
 * 將目前時間轉換為 HH:MM:SS.mmm 格式，提供 log 使用。
 */
void time_string(char *out)
{
    struct timeval tv;   /* 保存秒數與微秒數。 */
    struct tm *tm_info;  /* 保存轉換後的本地時間。 */

    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    sprintf(out, "%02d:%02d:%02d.%03ld",
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
            tv.tv_usec / 1000);
}

// output log message to console and file
/*
 * log_msg()
 * 將 Worker 的 log 同時輸出到終端機與 worker.log。
 *
 * worker_name：目前 Worker 的名稱，例如 worker-1。
 * level      ：日誌層級，例如 INFO、WARN、FATAL。
 * task_id    ：Task ID。若當下沒有對應任務，使用 "-"。
 * text       ：要記錄的動作與細節。
 */
void log_msg(const char *worker_name, const char *level,
             const char *task_id, const char *text)
{
    char t[32]; /* 格式化後的時間。 */
    FILE *fp;   /* 指向 worker.log 的檔案指標。 */

    time_string(t);
    printf("[%s] [%s] [%s] [%s] %s\n", t, worker_name, level, task_id, text);
    fflush(stdout);

    fp = fopen("worker.log", "a");
    if (fp != NULL) {
        fprintf(fp, "[%s] [%s] [%s] [%s] %s\n", t, worker_name, level, task_id, text);
        fclose(fp);
    }
}

/*
 * make_task_id()
 * 由 Worker 產生 Task ID。
 * 作業要求 Task ID 必須由 Client 端產生，而且要能區分不同任務。
 *
 * 格式：Worker名稱-完整毫秒時間-PID-流水號
 * 例如：worker-1-1770000000123-8241-3
 *
 * 使用完整時間、Process ID 與 process 內流水號，可避免 Worker 重啟後
 * 或多個 Worker 同時執行時產生相同 Task ID。
 */
void make_task_id(const char *worker_name, char *task_id)
{
    static int seq = 0; /* 此 Worker process 內持續增加的流水號。 */

    seq++;
    snprintf(task_id, TASK_ID_SIZE, "%.10s-%lld-%ld-%d",
             worker_name, now_ms(), (long)getpid(), seq);
}

/*
 * new_req_socket()
 * 建立一個全新的 REQ socket，並連線到 Broker。
 *
 * Simple Pirate 的核心要求：發生 timeout 後，不可以在舊 REQ socket
 * 再次 zmq_send()。必須先關閉舊 socket，再呼叫此函式建立新 socket。
 */
void *new_req_socket(void *context)
{
    void *req = zmq_socket(context, ZMQ_REQ); /* 與 Broker 通訊的 REQ socket。 */

    if (req == NULL)
        return NULL;

    if (zmq_connect(req, BROKER_ADDR) != 0) {
        zmq_close(req);
        return NULL;
    }

    return req;
}

// send READY to Dispatcher
/*
 * send_ready()
 * Worker 啟動後主動向 Dispatcher 發送一次 READY。
 * Dispatcher 收到 READY 後，會將此 Worker 放入 Ready Queue。
 *
 * 注意：Worker 不需要定期重複送出 READY。
 * 任務完成後，Dispatcher 在收到 RESULT 時會自行把 Worker 放回 Ready Queue。
 */
void send_ready(void *dealer, const char *worker_name)
{
    if (zmq_send(dealer, "READY", 5, 0) == -1) {
        log_msg(worker_name, "ERROR", "-", "Failed to send READY to Dispatcher.");
        return;
    }

    log_msg(worker_name, "INFO", "-", "READY sent to Dispatcher.");
}

/*
 * broker_call()
 * 將 RESERVE 或 RELEASE request 傳給 Broker，並依照 Simple Pirate 模式等待回覆。
 *
 * 程式邏輯：
 * 1. 建立 REQ socket 並 connect 到 Broker。
 * 2. 送出 request。
 * 3. 使用 zmq_poll() 最多等待 REQUEST_TIMEOUT 毫秒。
 * 4. 若收到回覆，關閉 REQ socket 並回傳 true。
 * 5. 若 timeout，關閉舊 REQ socket，建立全新 socket，重新 connect。
 * 6. 使用完全相同的 request 重送，因此 Task ID 也保持不變。
 * 7. 最多嘗試 REQUEST_RETRIES 次。
 */
int broker_call(void *context, const char *worker_name,
                const char *task_id, const char *request, char *reply)
{
    int retries_left = REQUEST_RETRIES; /* 剩餘嘗試次數。 */
    void *req = new_req_socket(context); /* 目前使用中的 REQ socket。 */

    if (req == NULL) {
        log_msg(worker_name, "FATAL", task_id, "Cannot create or connect REQ socket.");
        return false;
    }

    while (retries_left > 0) {
        zmq_pollitem_t items[1]; /* 只監聽目前 REQ socket。 */
        int rc;                  /* zmq_poll() 的回傳值。 */

        log_msg(worker_name, "INFO", task_id, request);

        if (zmq_send(req, request, strlen(request), 0) == -1) {
            log_msg(worker_name, "WARN", task_id, "Failed to send request to Broker.");
        }

        items[0].socket = req;
        items[0].fd = 0;
        items[0].events = ZMQ_POLLIN;

        rc = zmq_poll(items, 1, REQUEST_TIMEOUT);

        if (rc > 0 && (items[0].revents & ZMQ_POLLIN)) {
            int n = zmq_recv(req, reply, MESSAGE_SIZE - 1, 0); /* 收到的 reply 長度。 */

            if (n >= 0) {
                reply[n] = '\0';
                zmq_close(req);
                return true;
            }
        }

        /*
         * Timeout：先關閉舊 REQ socket。
         * 這一步不可省略，否則 REQ socket 仍停留在「等待 reply」狀態，
         * 無法在同一個 socket 直接重送 request。
         */
        retries_left--;
        zmq_close(req);

        if (retries_left > 0) {
            log_msg(worker_name, "WARN", task_id,
                    "Timeout. Close old REQ socket, create new REQ socket, reconnect, and resend.");

            req = new_req_socket(context);
            if (req == NULL) {
                log_msg(worker_name, "FATAL", task_id, "Cannot recreate or reconnect REQ socket.");
                return false;
            }
        }
    }

    log_msg(worker_name, "FATAL", task_id, "Broker unreachable. Give up this task.");
    return false;
}

/*
 * main()
 * Worker 主流程：
 * 1. 建立 DEALER socket 並連線到 Dispatcher。
 * 2. 主動傳送一次 READY。
 * 3. 等待 Dispatcher 派發 TASK。
 * 4. Worker 自行建立 Task ID。
 * 5. 使用 REQ socket 向 Broker 送出 RESERVE。
 * 6. 收到 GRANTED 後 sleep() 模擬執行任務。
 * 7. 使用 REQ socket 向 Broker 送出 RELEASE。
 * 8. 將 RESULT 傳回 Dispatcher。
 */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: %s worker_id\n", argv[0]);
        return 0;
    }

    char worker_name[CLIENT_ID_SIZE]; /* Worker 名稱，同時作為 DEALER socket identity。 */
    void *context;                    /* ZeroMQ context。 */
    void *dealer;                     /* 與 Dispatcher 通訊的 DEALER socket。 */

    snprintf(worker_name, sizeof(worker_name), "%s", argv[1]);

    context = zmq_ctx_new();
    dealer = zmq_socket(context, ZMQ_DEALER);

    // set up DEALER socket to communicate with Dispatcher
    if (dealer == NULL) {
        printf("Cannot create Worker DEALER socket: %s\n", zmq_strerror(zmq_errno()));
        zmq_ctx_destroy(context);
        return 1;
    }

    /* Dispatcher 的 ROUTER socket 會使用這個 identity 辨識不同 Worker。 */
    zmq_setsockopt(dealer, ZMQ_IDENTITY, worker_name, strlen(worker_name));

    if (zmq_connect(dealer, DISPATCHER_ADDR) != 0) {
        printf("Cannot connect Worker to Dispatcher: %s\n", zmq_strerror(zmq_errno()));
        zmq_close(dealer);
        zmq_ctx_destroy(context);
        return 1;
    }

    // send READY to Dispatcher
    send_ready(dealer, worker_name);

    while (true) {
        char task_msg[MESSAGE_SIZE]; /* task message received from Dispatcher */
        int n;                       /* the number of bytes received */
        char cmd[32];                /* TASK 指令文字。 */
        char request[MESSAGE_SIZE];  /* 傳給 Broker 的 RESERVE 或 RELEASE request。 */
        char reply[MESSAGE_SIZE];    /* Broker 回覆的 GRANTED、REJECTED 或 ACK。 */
        char result[MESSAGE_SIZE];   /* 傳回 Dispatcher 的 RESULT。 */
        Task task;                   /* structure to hold task details */
        zmq_pollitem_t items[1];     /* 用來監聽 Dispatcher 是否派發任務。 */
        int rc;                      /* zmq_poll() 的回傳值。 */

        items[0].socket = dealer;
        items[0].fd = 0;
        items[0].events = ZMQ_POLLIN;

        /*
         * Worker 只是在等待任務。
         * Timeout 後直接繼續等待，不會重複傳送 READY。
         */
        rc = zmq_poll(items, 1, READY_RETRY_TIMEOUT);

        if (rc == -1)
            break;

        if (!(items[0].revents & ZMQ_POLLIN))
            continue;

        n = zmq_recv(dealer, task_msg, sizeof(task_msg) - 1, 0);
        if (n < 0)
            continue;

        task_msg[n] = '\0';

        // Parse the task message into variables
        if (sscanf(task_msg, "%31s %d %d %d %d",
                   cmd, &task.truck, &task.worker,
                   &task.insurance, &task.duration) != 5 ||
            strcmp(cmd, "TASK") != 0) {
            log_msg(worker_name, "ERROR", "-", "Bad task message from Dispatcher.");
            continue;
        }

        /* 作業要求 Task ID 由 Client / Worker 產生。 */
        make_task_id(worker_name, task.task_id);
        log_msg(worker_name, "INFO", task.task_id, "Task received.");

        // Build the RESERVE message
        snprintf(request, sizeof(request), "RESERVE %s %d %d %d %d",
                 task.task_id, task.truck, task.worker,
                 task.insurance, task.duration);

        // Call the broker and wait for the reply
        if (!broker_call(context, worker_name, task.task_id, request, reply)) {
            /*
             * 最多重試三次後仍沒有收到 RESERVE 回覆，Worker 無法判斷：
             * 1. Broker 根本沒有收到 request；或
             * 2. Broker 已扣除資源，但 GRANTED 回覆遺失。
             *
             * 因此在放棄任務前，再送出一個清理用 RELEASE。
             * Broker 會將 RELEASE 視為冪等操作：有配置就歸還，沒有配置就只回 ACK。
             */
            log_msg(worker_name, "WARN", task.task_id,
                    "RESERVE result unknown. Send cleanup RELEASE before giving up task.");
            snprintf(request, sizeof(request), "RELEASE %s %d %d %d",
                     task.task_id, task.truck, task.worker, task.insurance);

            if (!broker_call(context, worker_name, task.task_id, request, reply)) {
                log_msg(worker_name, "WARN", task.task_id,
                        "Cleanup RELEASE was not confirmed. Broker lease will reclaim resource later.");
            }

            snprintf(result, sizeof(result), "RESULT %s FAILED", task.task_id);
            zmq_send(dealer, result, strlen(result), 0);
            continue;
        }

        // analyze the reply from broker
        if (strncmp(reply, "REJECTED", 8) == 0) {
            /* Fail-Fast 或非法需求。 */
            log_msg(worker_name, "WARN", task.task_id, "Task rejected by Broker.");
            snprintf(result, sizeof(result), "RESULT %s REJECTED", task.task_id);
            zmq_send(dealer, result, strlen(result), 0);
            continue;
        } else if (strncmp(reply, "GRANTED", 7) == 0) {
            /* 取得資源後，使用 sleep() 模擬搬家工作。 */
            log_msg(worker_name, "INFO", task.task_id, "Resource granted. Start moving task.");
            sleep(task.duration);

            snprintf(request, sizeof(request), "RELEASE %s %d %d %d",
                    task.task_id, task.truck, task.worker, task.insurance);

            if (!broker_call(context, worker_name, task.task_id, request, reply)) {
                snprintf(result, sizeof(result), "RESULT %s RELEASE_FAILED", task.task_id);
                zmq_send(dealer, result, strlen(result), 0);
                continue;
            }

            /* RELEASE 成功時，Broker 必須回覆 ACK。 */
            if (strncmp(reply, "ACK", 3) != 0) {
                log_msg(worker_name, "ERROR", task.task_id, "Unexpected RELEASE reply from Broker.");
                snprintf(result, sizeof(result), "RESULT %s RELEASE_FAILED", task.task_id);
                zmq_send(dealer, result, strlen(result), 0);
                continue;
            }

            log_msg(worker_name, "INFO", task.task_id,
                    "Task completed. Result sent to Dispatcher.");
            snprintf(result, sizeof(result), "RESULT %s DONE", task.task_id);
            zmq_send(dealer, result, strlen(result), 0);
        } else {
            /* 收到未知回覆時，不可以假裝任務成功。 */
            log_msg(worker_name, "ERROR", task.task_id, "Unexpected RESERVE reply from Broker.");
            snprintf(result, sizeof(result), "RESULT %s FAILED", task.task_id);
            zmq_send(dealer, result, strlen(result), 0);
        }
    }

    zmq_close(dealer);
    zmq_ctx_destroy(context);
    return 0;
}
