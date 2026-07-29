#ifndef COMMON_H
#define COMMON_H

/*
 * common.h
 *
 * 這個標頭檔集中存放三個程式都會使用的設定值與資料結構。
 * 將共用設定放在同一個檔案，可以避免 Dispatcher、Broker 與 Worker
 * 使用不同的 port、資源上限或 timeout 數值。
 */

// Worker 連線到 Dispatcher 時使用的位址
#define DISPATCHER_ADDR      "tcp://localhost:5555"

// Worker 連線到 Broker 時使用的位址
#define BROKER_ADDR          "tcp://localhost:5556"

// Dispatcher 綁定（bind）ROUTER socket 時使用的位址
#define DISPATCHER_BIND_ADDR "tcp://*:5555"

// Broker 綁定（bind）ROUTER socket 時使用的位址
#define BROKER_BIND_ADDR     "tcp://*:5556"

// 可同時管理的 Worker 數量上限
#define MAX_CLIENTS 20

// Dispatcher 等待佇列、執行中任務與 Broker Pending Queue 的容量上限。
#define MAX_TASKS   100

// Broker 用來記錄已完成 RESERVE / RELEASE 的快取容量上限。
#define MAX_CACHE   1000

// 搬家公司擁有的全域資源總量。
#define MAX_TRUCK     5
#define MAX_WORKER    12
#define MAX_INSURANCE 1000

// 常用字串緩衝區大小。
#define TASK_ID_SIZE   64
#define CLIENT_ID_SIZE 50
#define MESSAGE_SIZE   256
#define IDENTITY_SIZE  256

/*
 * Worker 向 Broker 送出 RESERVE 或 RELEASE 後，等待回覆的時間。
 * 單位為毫秒。若超過此時間，Worker 會依照 Simple Pirate 流程重建 REQ socket。
 */
#define REQUEST_TIMEOUT 2500

/* Worker 最多嘗試向 Broker 傳送同一個 request 的次數。 */
#define REQUEST_RETRIES 3

/*
 * Dispatcher 派發任務後，等待 RESULT 的時間上限。
 * 這是 Bonus：Client 崩潰偵測功能使用的 timeout。
 */
#define TASK_TIMEOUT 10000

/*
 * Worker 等待 Dispatcher 任務時，zmq_poll() 每次等待的時間。
 * 目前 timeout 後只會繼續等待，不會重複送出 READY。
 */
#define READY_RETRY_TIMEOUT 2000

/*
 * Broker 對已配置資源設定的額外保留時間。
 * 若 Worker 因為網路異常始終無法送達 RELEASE，Broker 會在
 * 任務執行時間加上此寬限時間後，自動回收資源，避免永久洩漏。
 */
#define LEASE_GRACE_MS 15000

#define true  1
#define false 0

/*
 * Task：保存一個搬家任務所需要的資料。
 *
 * task_id   ：由 Worker 產生的全域唯一任務編號。
 * truck     ：任務需要的卡車數量。
 * worker    ：任務需要的搬家工人數量。
 * insurance ：任務需要的保險單位數量。
 * duration  ：模擬任務執行時間，單位為秒。
 */
typedef struct {
    char task_id[TASK_ID_SIZE];
    int truck;
    int worker;
    int insurance;
    int duration;
} Task;

#endif
