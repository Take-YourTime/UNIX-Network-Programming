#include <stdio.h>
#include <unistd.h>     // for fork(), sleep(), getpid(), getppid()
#include <stdlib.h>     // for exit()
#include <sys/wait.h>   // for wait()
#include <sys/prctl.h>  // for prctl(), PR_SET_NAME

// 一個子行程的任務：
// 1. 設定自己的名稱
// 2. 印出自己的 PID 和 PPID
// 3. 睡眠
void become_child(const char *name, int sleep_time) {
    // 設定行程名稱 (這樣你的程式才能讀到)
    prctl(PR_SET_NAME, name);
    
    printf("  [%s, PID: %d, PPID: %d] 正在睡眠 %d 秒...\n", 
           name, getpid(), getppid(), sleep_time);
    
    // 睡眠，讓我們有時間去執行 pstree
    sleep(sleep_time);
    
    printf("  [%s, PID: %d] 睡醒並離開。\n", name, getpid());
    exit(0);
}

int main() {
    pid_t pid_b, pid_c, pid_d, pid_e, pid_f;
    
    // 設定主行程 (A) 的名稱
    prctl(PR_SET_NAME, "proc_A");
    printf("[proc_A, PID: %d] 啟動中...\n", getpid());
    printf("將在 60 秒內產生一個行程樹。請開啟新終端機執行您的 pstree 程式。\n");

    if ((pid_b = fork()) == 0) {
        // --- 子行程 B ---
        prctl(PR_SET_NAME, "proc_B");
        
        if ((pid_d = fork()) == 0) {
            // --- 子行程 D ---
            become_child("proc_D", 60); // 葉節點，睡眠
        }
        
        // B 等待 D 結束
        wait(NULL); 
        exit(0);
    }

    if ((pid_c = fork()) == 0) {
        // --- 子行程 C ---
        prctl(PR_SET_NAME, "proc_C");

        if ((pid_e = fork()) == 0) {
            // --- 子行程 E ---
            become_child("proc_E", 60); // 葉節點，睡眠
        }

        if ((pid_f = fork()) == 0) {
            // --- 子行程 F ---
            become_child("proc_F", 60); // 葉節點，睡眠
        }
        
        // C 等待 E 和 F 結束
        wait(NULL); 
        wait(NULL); 
        exit(0);
    }

    // --- 主行程 A ---
    // A 等待 B 和 C 結束
    printf("[proc_A] 正在等待 B 和 C 結束...\n");
    wait(NULL); // 等待 B
    wait(NULL); // 等待 C
    
    printf("[proc_A] 所有子行程皆已結束。程式離開。\n");
    return 0;
}