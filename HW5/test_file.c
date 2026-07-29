#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <unistd.h>     // For getpid(), close()

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <要建立並開啟的檔案名稱>\n", argv[0]);
        fprintf(stderr, "範例: %s /tmp/my_test_file.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filepath = argv[1];
    int fd;

    printf("測試程式啟動中...\n");
    printf("我的 PID 是: %d\n", getpid());

    // O_CREAT: 如果檔案不存在就建立它
    // O_RDWR: 以讀寫模式開啟
    // 0644: 設定檔案權限 (擁有者可讀寫，群組和其他人只能讀)
    fd = open(filepath, O_CREAT | O_RDWR, 0644);

    if (fd == -1) {
        perror("無法開啟檔案");
        exit(EXIT_FAILURE);
    }

    printf("成功開啟檔案 '%s'。檔案現在保持開啟狀態。\n\n", filepath);
    printf(">> 請開啟另一個終端機，並執行以下指令來測試:\n");
    printf(">> ./find_opener %s\n\n", filepath);
    printf("完成測試後，請按 Enter 鍵結束此程式...\n");

    // 等待使用者輸入，以便有足夠的時間進行測試
    getchar();

    // 關閉檔案並結束
    close(fd);
    printf("檔案已關閉，測試程式結束。\n");

    return 0;
}
