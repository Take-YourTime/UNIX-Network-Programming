/*
Broker負責管理全域資源︓卡⾞ (5 輛)、搬家⼯人 (12 名)、保險 (1000 單位)。

【資源審核與 Pending Queue】
Broker 收到 Client 的 RESERVE 請求後，若資源充⾜，扣除資源並回傳 GRANTED。
若資源不⾜，Broker 不可丟棄請求，必須將其放⼊內部的 Pending Queue (等待佇列)，
待其他 Client 釋放資源後重新評估

【快速失敗防禦 (Fail-Fast)】
若單⼀任務的資源需求⼤於系統總量上限 (例如︓要求 6 輛卡⾞)，Broker 必須⽴刻回傳 REJECTED 拒絕請求，
不可將其放⼊ Pending Queue 導致死結！


Broker 必須快取已處理過的 Task ID 狀態。

【Broker 狀態持久化與恢復】
Broker 需將資源變動與 Task ID 狀態寫入本地日誌檔 (如 broker_state.log)。
若 Broker 被強制關閉並重啟，必須能讀取日誌恢復剩餘資源與 Pending Queue，
讓 Client 的 Simple Pirate 重傳機制能無縫接軌
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <zmq.h>

#include "common.h"


int truck_left = MAX_TRUCK;         // the number of available trucks
int worker_left = MAX_WORKER;       // the number of available workers
int insurance_left = MAX_INSURANCE; // the number of available insurance units

int drop_rate = 0;

/*
 * Cache：保存已經處理完成的 Task ID 與最終回覆。
 * 當 Worker 因 timeout 重送相同 request 時，Broker 直接補發原本回覆，
 * 不可以再次扣除或加回資源。
 *
 * used    ：這一格是否已經被使用。
 * task_id ：Task ID。
 * reply   ：最終回覆，例如 GRANTED、REJECTED 或 ACK。
 */
typedef struct {
    int used;
    char task_id[TASK_ID_SIZE];
    char reply[MESSAGE_SIZE];
} Cache;

// Cache for recovering from duplicate requests.
//  RESERVE 與 RELEASE 分開保存，避免兩種操作互相覆蓋。
Cache reserve_cache[MAX_CACHE];
Cache release_cache[MAX_CACHE];

/*
 * Pending：保存目前資源不足、仍在等待的 RESERVE request。
 *
 * used          ：這一格是否已經被使用。
 * task_id       ：Task ID。
 * truck         ：需要的卡車數量。
 * worker        ：需要的工人數量。
 * insurance     ：需要的保險單位數量。
 * identity      ：最新一次送出 request 的 REQ socket identity。
 * identity_size ：identity 的實際長度。
 *
 * Simple Pirate timeout 後會建立全新的 REQ socket，因此 identity 可能改變。
 * Broker 收到重送 request 時，要更新 identity，才能把回覆送到新 socket。
 */
typedef struct {
    int used;
    char task_id[TASK_ID_SIZE];
    int truck;
    int worker;
    int insurance;
    int duration;                 // 任務預計執行秒數，供超時回收使用
    char identity[IDENTITY_SIZE];
    int identity_size;
} Pending;


Pending pending[MAX_TASKS]; // Broker 的 Pending Queue

/*
 * Reservation：記錄 Broker 實際扣除過的資源。
 *
 * used       ：此欄位是否已使用。
 * released   ：資源是否已經歸還。
 * task_id    ：對應的 Task ID。
 * truck      ：實際扣除的卡車數量。
 * worker     ：實際扣除的工人數量。
 * insurance  ：實際扣除的保險單位數量。
 * expire_at  ：保留資源的到期時間，單位為毫秒。
 *
 * RELEASE 時不直接相信 Worker 傳來的數值，而是使用這裡保存的數值。
 * 這樣即使訊息重複或內容錯誤，也不會破壞資源總數。
 */
typedef struct {
    int used;
    int released;
    char task_id[TASK_ID_SIZE];
    int truck;
    int worker;
    int insurance;
    long long expire_at;
} Reservation;

//保存每一筆真正取得資源的任務
Reservation reservations[MAX_CACHE];


// 將目前時間轉換為 HH:MM:SS.mmm 格式，提供 log 使用
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

/*
 * 回傳目前時間，單位為毫秒。
 * Broker 使用此數值判斷已配置資源是否超過保留期限。
 */
long long now_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*
 * 將 Broker 的 log 同時輸出到終端機與 broker.log。
 */
void log_msg(const char *level, const char *task_id, const char *text)
{
    char t[32]; /* 格式化後的時間。 */
    FILE *fp;   /* 指向 broker.log 的檔案指標。 */

    time_string(t);
    printf("[%s] [Broker] [%s] [%s] %s\n", t, level, task_id, text);
    fflush(stdout);

    fp = fopen("broker.log", "a");
    if (fp != NULL) {
        fprintf(fp, "[%s] [Broker] [%s] [%s] %s\n", t, level, task_id, text);
        fclose(fp);
    }
}

/*
 * save_state()
 * 將資源異動追加寫入 broker_state.log。
 * 此格式保留統一時間戳記，同時將欄位寫成容易閱讀的 key=value。
 *
 * 注意：目前只有「寫入」功能，尚未實作 Broker 啟動時讀取檔案並恢復狀態。
 * 因此 Bonus 的 Broker 災難復原仍未完整完成。
 */
void save_state(const char *action, const char *task_id,
                int truck, int worker, int insurance, const char *reply)
{
    char t[32]; /* 格式化後的時間。 */
    FILE *fp;   /* 指向 broker_state.log 的檔案指標。 */

    time_string(t);
    fp = fopen("broker_state.log", "a");

    if (fp != NULL) {
        fprintf(fp,
                "[%s] [Broker] [STATE] [%s] action=%s truck=%d worker=%d insurance=%d reply=%s\n",
                t, task_id, action, truck, worker, insurance, reply);
        fclose(fp);
    }
}

/*
 * chaos_drop()
 * 依照 --drop-rate 設定，隨機決定是否丟棄訊息。
 *
 * where   ：丟棄位置，例如 receive request 或 send reply。
 * task_id ：目前訊息所屬的 Task ID。
 * 回傳值  ：需要丟棄訊息回傳 true，否則回傳 false。
 */
int chaos_drop(const char *where, const char *task_id)
{
    char text[128]; /* log 文字。 */
    int r;          /* 0 到 99 的隨機數。 */

    if (drop_rate <= 0)
        return false;

    r = rand() % 100;

    if (r < drop_rate) {
        sprintf(text, "[CHAOS] Dropped message at %s.", where);
        log_msg("WARN", task_id, text);
        return true;
    }

    return false;
}

/*
 * send_reply()
 * 使用 ROUTER socket 回覆 REQ socket。
 * ROUTER -> REQ 的回覆格式必須為：
 * [REQ identity] [空 frame] [實際 reply]
 *
 * 回覆送出前會先執行 Chaos Monkey，模擬 reply 遺失。
 */
void send_reply(void *router, const char *identity, int identity_size,
                const char *reply, const char *task_id)
{
    if (chaos_drop("send reply", task_id))
        return;

    zmq_send(router, identity, identity_size, ZMQ_SNDMORE);
    zmq_send(router, "", 0, ZMQ_SNDMORE);
    zmq_send(router, reply, strlen(reply), 0);
}

/*
 * find_cache()
 * 在指定快取中尋找 Task ID。
 *
 * cache   ：要搜尋的快取陣列。
 * task_id ：要尋找的 Task ID。
 * 回傳值  ：找到時回傳索引；找不到時回傳 -1。
 */
int find_cache(Cache cache[], const char *task_id)
{
    int i; /* 走訪快取陣列使用的索引。 */

    for (i = 0; i < MAX_CACHE; i++) {
        if (cache[i].used && strcmp(cache[i].task_id, task_id) == 0)
            return i;
    }

    return -1;
}

/*
 * add_cache()
 * 將 Task ID 與最終回覆寫入指定快取。
 * 若 Task ID 已經存在，就更新原本的回覆。
 *
 * 回傳值：成功寫入回傳 true；快取已滿回傳 false。
 */
int add_cache(Cache cache[], const char *task_id, const char *reply)
{
    int i = find_cache(cache, task_id); /* 既有紀錄的索引；找不到時為 -1。 */

    if (i == -1) {
        for (i = 0; i < MAX_CACHE; i++) {
            if (!cache[i].used) {
                cache[i].used = true;
                strcpy(cache[i].task_id, task_id);
                break;
            }
        }
    }

    if (i >= 0 && i < MAX_CACHE) {
        strcpy(cache[i].reply, reply);
        return true;
    }

    log_msg("ERROR", task_id, "Idempotency cache is full.");
    return false;
}

/*
 * find_reservation()
 * 尋找指定 Task ID 的資源配置紀錄。
 */
int find_reservation(const char *task_id)
{
    int i;

    for (i = 0; i < MAX_CACHE; i++) {
        if (reservations[i].used && strcmp(reservations[i].task_id, task_id) == 0)
            return i;
    }

    return -1;
}

/*
 * add_reservation()
 * 記錄一筆真正取得資源的任務。
 * 此函式只建立紀錄，不直接扣除資源。
 */
int add_reservation(const char *task_id, int truck, int worker,
                    int insurance, int duration)
{
    int i = find_reservation(task_id);

    if (i != -1)
        return i;

    for (i = 0; i < MAX_CACHE; i++) {
        if (!reservations[i].used) {
            reservations[i].used = true;
            reservations[i].released = false;
            strcpy(reservations[i].task_id, task_id);
            reservations[i].truck = truck;
            reservations[i].worker = worker;
            reservations[i].insurance = insurance;
            reservations[i].expire_at = now_ms() + (long long)duration * 1000 + LEASE_GRACE_MS;
            return i;
        }
    }

    log_msg("ERROR", task_id, "Reservation table is full.");
    return -1;
}

/* grant_resource() 會呼叫下方定義的 can_grant()。 */
int can_grant(int truck, int worker, int insurance);

/*
 * grant_resource()
 * 將資源配置給指定任務。
 *
 * 重要順序：先建立 reservation 與 cache，再扣除資源。
 * 這樣就算 GRANTED 回覆遺失，Worker 重送 RESERVE 時也只會補發回覆，
 * 不會再次扣除相同資源。
 */
int grant_resource(void *router, const char *identity, int identity_size,
                   const char *task_id, int truck, int worker,
                   int insurance, int duration, const char *reason)
{
    char reply[MESSAGE_SIZE];
    char text[MESSAGE_SIZE];
    int reservation_idx;

    if (!can_grant(truck, worker, insurance))
        return false;

    snprintf(reply, sizeof(reply), "GRANTED %s", task_id);

    reservation_idx = add_reservation(task_id, truck, worker, insurance, duration);
    if (reservation_idx == -1)
        return false;

    if (!add_cache(reserve_cache, task_id, reply)) {
        reservations[reservation_idx].used = false;
        return false;
    }

    truck_left -= truck;
    worker_left -= worker;
    insurance_left -= insurance;

    save_state("RESERVE", task_id, truck, worker, insurance, "GRANTED");
    send_reply(router, identity, identity_size, reply, task_id);

    snprintf(text, sizeof(text), "%s Left: truck=%d worker=%d insurance=%d",
             reason, truck_left, worker_left, insurance_left);
    log_msg("INFO", task_id, text);
    return true;
}

/*
 * can_grant()
 * 判斷目前剩餘資源是否足以執行指定任務。
 */
int can_grant(int truck, int worker, int insurance)
{
    return truck <= truck_left &&
           worker <= worker_left &&
           insurance <= insurance_left;
}

/*
 * find_pending()
 * 在 Pending Queue 中尋找指定 Task ID。
 * 回傳值：找到時回傳索引；找不到時回傳 -1。
 */
int find_pending(const char *task_id)
{
    int i; /* 走訪 Pending Queue 使用的索引。 */

    for (i = 0; i < MAX_TASKS; i++) {
        if (pending[i].used && strcmp(pending[i].task_id, task_id) == 0)
            return i;
    }

    return -1;
}

/*
 * remove_pending()
 * 將指定 Task ID 從 Pending Queue 移除。
 */
void remove_pending(const char *task_id)
{
    int i = find_pending(task_id); /* 指定 Task ID 在 Pending Queue 中的位置。 */

    if (i != -1)
        pending[i].used = false;
}

/*
 * add_pending()
 * 將資源不足的 RESERVE request 放入 Pending Queue。
 * 若該 Task ID 已經存在，表示 Worker 因 timeout 重送 request，
 * 此時只需要更新最新的 REQ socket identity。
 *
 * 回傳值：成功加入或更新回傳 true；佇列已滿回傳 false。
 */
int add_pending(const char *task_id, int truck, int worker, int insurance,
                int duration, const char *identity, int identity_size)
{
    int i; /* 走訪 Pending Queue 使用的索引。 */

    if (identity_size > IDENTITY_SIZE)
        identity_size = IDENTITY_SIZE;

    for (i = 0; i < MAX_TASKS; i++) {
        if (pending[i].used && strcmp(pending[i].task_id, task_id) == 0) {
            memcpy(pending[i].identity, identity, identity_size);
            pending[i].identity_size = identity_size;
            return true;
        }
    }

    for (i = 0; i < MAX_TASKS; i++) {
        if (!pending[i].used) {
            pending[i].used = true;
            strcpy(pending[i].task_id, task_id);
            pending[i].truck = truck;
            pending[i].worker = worker;
            pending[i].insurance = insurance;
            pending[i].duration = duration;
            memcpy(pending[i].identity, identity, identity_size);
            pending[i].identity_size = identity_size;
            return true;
        }
    }

    log_msg("ERROR", task_id, "Pending Queue is full.");
    return false;
}

/*
 * try_pending()
 * 每當有 Worker 釋放資源後，重新掃描 Pending Queue。
 * 若某個等待任務現在已經可以取得資源，就扣除資源並回覆 GRANTED。
 */
void try_pending(void *router)
{
    int i; /* 走訪 Pending Queue 使用的索引。 */

    for (i = 0; i < MAX_TASKS; i++) {
        if (pending[i].used &&
            can_grant(pending[i].truck, pending[i].worker, pending[i].insurance)) {

            if (grant_resource(router,
                               pending[i].identity,
                               pending[i].identity_size,
                               pending[i].task_id,
                               pending[i].truck,
                               pending[i].worker,
                               pending[i].insurance,
                               pending[i].duration,
                               "Pending task granted.")) {
                /*
                 * 即使 reply 被 Chaos Monkey 丟棄，也可以移除 Pending。
                 * 因為 GRANTED 已寫入 cache，重送 RESERVE 時只會補發回覆。
                 */
                pending[i].used = false;
            }
        }
    }
}

/*
 * expire_reservations()
 * 回收長時間沒有收到 RELEASE 的資源。
 *
 * 這是處理極端情況的安全網：Broker 可能已經扣除資源，但 GRANTED 回覆
 * 連續遺失，導致 Worker 放棄任務。若沒有自動回收，資源會永久減少。
 */
void expire_reservations(void *router)
{
    int i;
    long long current = now_ms();

    for (i = 0; i < MAX_CACHE; i++) {
        if (reservations[i].used && !reservations[i].released &&
            current >= reservations[i].expire_at) {
            char reply[MESSAGE_SIZE];
            char reject_reply[MESSAGE_SIZE];
            char text[MESSAGE_SIZE];

            truck_left += reservations[i].truck;
            worker_left += reservations[i].worker;
            insurance_left += reservations[i].insurance;
            reservations[i].released = true;

            snprintf(reply, sizeof(reply), "ACK %s", reservations[i].task_id);
            add_cache(release_cache, reservations[i].task_id, reply);

            /* 過期後不允許舊 RESERVE 再次取得資源。 */
            snprintf(reject_reply, sizeof(reject_reply), "REJECTED %s", reservations[i].task_id);
            add_cache(reserve_cache, reservations[i].task_id, reject_reply);

            snprintf(text, sizeof(text),
                     "Lease expired. Resource reclaimed. Left: truck=%d worker=%d insurance=%d",
                     truck_left, worker_left, insurance_left);
            log_msg("WARN", reservations[i].task_id, text);
            save_state("AUTO_RELEASE", reservations[i].task_id,
                       reservations[i].truck, reservations[i].worker,
                       reservations[i].insurance, "ACK");
        }
    }

    /* 回收資源後，等待中的任務可能已經可以執行。 */
    try_pending(router);
}

/*
 * handle_reserve()
 * 處理 Worker 傳來的 RESERVE request。
 *
 * 程式邏輯：
 * 1. 解析訊息。
 * 2. 若 Task ID 已處理過，補發快取回覆，避免重複扣除資源。
 * 3. 若單一任務需求超過系統總量，立即回覆 REJECTED（Fail-Fast）。
 * 4. 若目前資源足夠，扣除資源並回覆 GRANTED。
 * 5. 若目前資源不足，放入 Pending Queue 等待其他任務 RELEASE。
 */
void handle_reserve(void *router, const char *identity, int identity_size,
                    char *buffer)
{
    char cmd[32];                 /* RESERVE 指令文字。 */
    char task_id[TASK_ID_SIZE];   /* Worker 產生的 Task ID。 */
    int truck;                    /* 需要的卡車數量。 */
    int worker;                   /* 需要的工人數量。 */
    int insurance;                /* 需要的保險單位數量。 */
    int duration;                 /* 任務預計執行秒數。 */
    int idx;                      /* 快取索引。 */
    int pending_idx;              /* Pending Queue 索引。 */
    char reply[MESSAGE_SIZE];     /* 傳給 Worker 的回覆。 */

    if (sscanf(buffer, "%31s %63s %d %d %d %d",
               cmd, task_id, &truck, &worker, &insurance, &duration) != 6) {
        log_msg("ERROR", "-", "Bad RESERVE message.");
        return;
    }

    /*
     * 若先前已收到清理用 RELEASE，代表 Worker 已放棄此任務。
     * 此時即使舊 RESERVE 延遲抵達，也不可以重新扣除資源。
     */
    idx = find_cache(release_cache, task_id);
    if (idx != -1) {
        snprintf(reply, sizeof(reply), "REJECTED %s", task_id);
        add_cache(reserve_cache, task_id, reply);
        log_msg("WARN", task_id, "Late RESERVE ignored because task was already closed.");
        send_reply(router, identity, identity_size, reply, task_id);
        return;
    }

    /* 冪等性：若 RESERVE 已完成，不可以再次扣除資源。 */
    idx = find_cache(reserve_cache, task_id);
    if (idx != -1) {
        log_msg("WARN", task_id, "Duplicate RESERVE detected. Resending cached reply.");
        send_reply(router, identity, identity_size, reserve_cache[idx].reply, task_id);
        return;
    }

    /* 防止負數或零資源破壞資源計算。 */
    if (truck <= 0 || worker <= 0 || insurance <= 0 || duration <= 0) {
        snprintf(reply, sizeof(reply), "REJECTED %s", task_id);
        add_cache(reserve_cache, task_id, reply);
        send_reply(router, identity, identity_size, reply, task_id);
        log_msg("WARN", task_id, "Rejected. Resource values and duration must be positive.");
        return;
    }

    /* Fail-Fast：單一任務不可能完成時，不能放入 Pending Queue。 */
    if (truck > MAX_TRUCK || worker > MAX_WORKER || insurance > MAX_INSURANCE) {
        snprintf(reply, sizeof(reply), "REJECTED %s", task_id);
        add_cache(reserve_cache, task_id, reply);
        save_state("RESERVE", task_id, truck, worker, insurance, "REJECTED");
        send_reply(router, identity, identity_size, reply, task_id);
        log_msg("WARN", task_id, "Fail-Fast rejected. Requirement exceeds system limit.");
        return;
    }

    /*
     * 若 Task 已經在 Pending Queue，表示 Worker 因 timeout 使用新 REQ socket 重送。
     * 必須更新 identity，之後 Broker 才能把 GRANTED 傳到最新 socket。
     */
    pending_idx = find_pending(task_id);
    if (pending_idx != -1) {
        if (identity_size > IDENTITY_SIZE)
            identity_size = IDENTITY_SIZE;

        memcpy(pending[pending_idx].identity, identity, identity_size);
        pending[pending_idx].identity_size = identity_size;

        if (!can_grant(truck, worker, insurance)) {
            log_msg("WARN", task_id, "Duplicate pending RESERVE detected. Still waiting.");
            return;
        }

        /* 使用最初放入 Pending Queue 的資源需求，不直接相信重送內容。 */
        truck = pending[pending_idx].truck;
        worker = pending[pending_idx].worker;
        insurance = pending[pending_idx].insurance;
        duration = pending[pending_idx].duration;
        remove_pending(task_id);
    }

    if (can_grant(truck, worker, insurance)) {
        /* 資源足夠：建立冪等性紀錄後，只扣除一次資源。 */
        grant_resource(router, identity, identity_size, task_id,
                       truck, worker, insurance, duration,
                       "Resource granted.");
    } else {
        /* 資源暫時不足：保留 request，不可以直接丟棄。 */
        add_pending(task_id, truck, worker, insurance, duration, identity, identity_size);
        log_msg("WARN", task_id, "Resource not enough. Put into Pending Queue.");
    }
}

/*
 * handle_release()
 * 處理 Worker 傳來的 RELEASE request。
 *
 * 程式邏輯：
 * 1. 若 RELEASE 已處理過，補發 ACK，避免重複加回資源。
 * 2. 確認該 Task 確實曾經取得 GRANTED。
 * 3. 將資源加回。
 * 4. 回覆 ACK。
 * 5. 重新審核 Pending Queue。
 */
void handle_release(void *router, const char *identity, int identity_size,
                    char *buffer)
{
    char cmd[32];                 /* RELEASE 指令文字。 */
    char task_id[TASK_ID_SIZE];   /* Worker 產生的 Task ID。 */
    int truck;                    /* Worker 回報的卡車數量。 */
    int worker;                   /* Worker 回報的工人數量。 */
    int insurance;                /* Worker 回報的保險單位數量。 */
    int idx;                      /* RELEASE 快取索引。 */
    int reservation_idx;          /* 實際配置紀錄索引。 */
    char reply[MESSAGE_SIZE];     /* 傳給 Worker 的回覆。 */
    char reject_reply[MESSAGE_SIZE]; /* 阻止延遲 RESERVE 重新生效的回覆。 */
    char text[MESSAGE_SIZE];      /* log 文字。 */

    if (sscanf(buffer, "%31s %63s %d %d %d",
               cmd, task_id, &truck, &worker, &insurance) != 5) {
        log_msg("ERROR", "-", "Bad RELEASE message.");
        return;
    }

    /* 冪等性：重複 RELEASE 不可以再次加回資源。 */
    idx = find_cache(release_cache, task_id);
    if (idx != -1) {
        log_msg("WARN", task_id, "Duplicate RELEASE detected. Resending cached ACK.");
        send_reply(router, identity, identity_size, release_cache[idx].reply, task_id);
        return;
    }

    snprintf(reply, sizeof(reply), "ACK %s", task_id);
    reservation_idx = find_reservation(task_id);

    if (reservation_idx == -1) {
        /*
         * Worker 在 RESERVE timeout 後不知道 Broker 是否曾經扣除資源，
         * 因此會送出清理用 RELEASE。若其實尚未配置資源，這裡只記錄 ACK，
         * 不增加資源。未來延遲抵達的 RESERVE 也會被拒絕。
         */
        remove_pending(task_id);
        add_cache(release_cache, task_id, reply);
        snprintf(reject_reply, sizeof(reject_reply), "REJECTED %s", task_id);
        add_cache(reserve_cache, task_id, reject_reply);
        send_reply(router, identity, identity_size, reply, task_id);
        log_msg("INFO", task_id, "Cleanup RELEASE acknowledged. No allocated resource to return.");
        return;
    }

    if (reservations[reservation_idx].released) {
        /* allocation 已經歸還，只需要補發 ACK。 */
        add_cache(release_cache, task_id, reply);
        send_reply(router, identity, identity_size, reply, task_id);
        log_msg("WARN", task_id, "Duplicate RELEASE detected from reservation state. Resending ACK.");
        return;
    }

    /*
     * 使用 Broker 自己保存的原始數量歸還資源，不直接相信 RELEASE 訊息中的數值。
     * 這可避免錯誤或惡意內容破壞資源總數。
     */
    if (truck != reservations[reservation_idx].truck ||
        worker != reservations[reservation_idx].worker ||
        insurance != reservations[reservation_idx].insurance) {
        log_msg("WARN", task_id, "RELEASE values mismatch. Use Broker reservation record.");
    }

    truck_left += reservations[reservation_idx].truck;
    worker_left += reservations[reservation_idx].worker;
    insurance_left += reservations[reservation_idx].insurance;
    reservations[reservation_idx].released = true;

    /* 額外保護：剩餘資源不應超過系統總量。 */
    if (truck_left > MAX_TRUCK) truck_left = MAX_TRUCK;
    if (worker_left > MAX_WORKER) worker_left = MAX_WORKER;
    if (insurance_left > MAX_INSURANCE) insurance_left = MAX_INSURANCE;

    add_cache(release_cache, task_id, reply);
    save_state("RELEASE", task_id,
               reservations[reservation_idx].truck,
               reservations[reservation_idx].worker,
               reservations[reservation_idx].insurance, "ACK");
    send_reply(router, identity, identity_size, reply, task_id);

    snprintf(text, sizeof(text), "Resource released. Left: truck=%d worker=%d insurance=%d",
             truck_left, worker_left, insurance_left);
    log_msg("INFO", task_id, text);

    /* 資源釋放後，重新評估等待中的任務。 */
    try_pending(router);
}

/*
 * main()
 * Broker 主流程：
 * 1. 解析 --drop-rate N。
 * 2. 建立 ROUTER socket 並綁定 port 5556。
 * 3. 接收 REQ socket 送來的 multipart message：
 *    [REQ identity] [空 frame] [RESERVE 或 RELEASE 內容]
 * 4. 接收後先執行 Chaos Monkey。
 * 5. 依照指令呼叫 handle_reserve() 或 handle_release()。
 */
int main(int argc, char *argv[])
{
    void *context; /* ZeroMQ context。 */
    void *router;  /* 與 Worker REQ socket 通訊的 ROUTER socket。 */
    int i;         /* 解析命令列參數使用的索引。 */

    srand(time(NULL));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--drop-rate") == 0 && i + 1 < argc) {
            drop_rate = atoi(argv[i + 1]);

            if (drop_rate < 0) drop_rate = 0;
            if (drop_rate > 100) drop_rate = 100;
        }
    }

    context = zmq_ctx_new();
    router = zmq_socket(context, ZMQ_ROUTER);

    if (router == NULL) {
        printf("Cannot create Broker ROUTER socket: %s\n", zmq_strerror(zmq_errno()));
        zmq_ctx_destroy(context);
        return 1;
    }

    if (zmq_bind(router, BROKER_BIND_ADDR) != 0) {
        printf("Cannot bind Broker to %s: %s\n",
               BROKER_BIND_ADDR, zmq_strerror(zmq_errno()));
        printf("Please check whether another Broker is already running.\n");
        zmq_close(router);
        zmq_ctx_destroy(context);
        return 1;
    }

    log_msg("INFO", "-", "Broker started.");

    {
        char text[64]; /* 將 drop-rate 也用統一格式寫入 log。 */
        sprintf(text, "Drop rate = %d%%", drop_rate);
        log_msg("INFO", "-", text);
    }

    /*
     * TODO(Bonus)：目前尚未讀取 broker_state.log。
     * 若要完整取得 Bonus，需要在此處載入日誌並恢復：
     * 1. 剩餘資源。
     * 2. reserve_cache 與 release_cache。
     * 3. Pending Queue。
     */

    while (true) {
        zmq_pollitem_t items[1];        /* 監聽 Worker request，同時定期執行逾時回收。 */
        zmq_msg_t id, empty, msg;       /* ROUTER 收到的三個 frame。 */
        char identity[IDENTITY_SIZE];   /* REQ socket identity。 */
        int identity_size;              /* identity frame 的實際長度。 */
        char buffer[MESSAGE_SIZE];      /* RESERVE 或 RELEASE 訊息內容。 */
        char cmd[32];                   /* 指令文字。 */
        char task_id[TASK_ID_SIZE];     /* 訊息中的 Task ID。 */
        int msg_size;                   /* 訊息 frame 的實際長度。 */
        int rc;

        items[0].socket = router;
        items[0].fd = 0;
        items[0].events = ZMQ_POLLIN;
        rc = zmq_poll(items, 1, 500);

        if (rc == -1)
            break;

        expire_reservations(router);

        if (!(items[0].revents & ZMQ_POLLIN))
            continue;

        zmq_msg_init(&id);
        if (zmq_msg_recv(&id, router, 0) == -1)
            break;

        identity_size = zmq_msg_size(&id);
        if (identity_size > (int)sizeof(identity))
            identity_size = sizeof(identity);

        memcpy(identity, zmq_msg_data(&id), identity_size);

        zmq_msg_init(&empty);
        if (zmq_msg_recv(&empty, router, 0) == -1) {
            zmq_msg_close(&id);
            break;
        }

        zmq_msg_init(&msg);
        if (zmq_msg_recv(&msg, router, 0) == -1) {
            zmq_msg_close(&id);
            zmq_msg_close(&empty);
            break;
        }

        msg_size = zmq_msg_size(&msg);
        if (msg_size >= (int)sizeof(buffer))
            msg_size = sizeof(buffer) - 1;

        memcpy(buffer, zmq_msg_data(&msg), msg_size);
        buffer[msg_size] = '\0';

        if (sscanf(buffer, "%31s %63s", cmd, task_id) != 2) {
            log_msg("ERROR", "-", "Bad request message.");
            zmq_msg_close(&id);
            zmq_msg_close(&empty);
            zmq_msg_close(&msg);
            continue;
        }

        /* Chaos Monkey：模擬 request 在 Broker 收到後直接被丟棄。 */
        if (chaos_drop("receive request", task_id)) {
            zmq_msg_close(&id);
            zmq_msg_close(&empty);
            zmq_msg_close(&msg);
            continue;
        }

        if (strcmp(cmd, "RESERVE") == 0)
            handle_reserve(router, identity, identity_size, buffer);
        else if (strcmp(cmd, "RELEASE") == 0)
            handle_release(router, identity, identity_size, buffer);
        else
            log_msg("ERROR", task_id, "Unknown command.");

        zmq_msg_close(&id);
        zmq_msg_close(&empty);
        zmq_msg_close(&msg);
    }

    zmq_close(router);
    zmq_ctx_destroy(context);
    return 0;
}
