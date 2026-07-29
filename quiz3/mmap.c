/*
【memory map file練習】

透過mmap()，將temp file映射到memory中，

再經由writer寫入資料，reader讀取資料，來達到不同process之間傳送資料的目的。

注意，demo時需使用兩個terminal開啟此程式！

因為此程式的目的為練習使用mmap()來在不同process間傳遞資料，

因此reader使用busy waiting的方式來等待writer傳送資料。

正式場合應使用condition variable或semaphore來提高效能！
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define TEMP_FILE "/tmp/mmap_test"
#define BLOCK_SIZE 4096
#define FILE_SIZE 8192

#define true 1

int main(int argc, char *argv[]){
    int fd;
    char* ptr; // memory pointer for memory map file

    if(argc != 2){
        printf("Usage: %s [write/read]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // create temp file "for memory map file"
    fd = open(TEMP_FILE, O_RDWR | O_CREAT, 0666);
    if(fd < 0){
        perror("open");
        exit(EXIT_FAILURE);
    }

    // 設定檔案大小
    if(ftruncate(fd, FILE_SIZE) == -1){
	    perror("ftruncate");
	    exit(EXIT_FAILURE);
	}

    // mmap
    ptr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(ptr == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }
	
	
	// writer
    if(strcmp(argv[1], "write") == 0){
        char buf[BLOCK_SIZE];

        printf("Writer process started.\n");
        while(true) {
            printf("Input: ");
            fgets(buf, sizeof(buf), stdin);

            // write to shared memory
            strcpy(ptr, buf);
        }
    }
	// reader
    else if(strcmp(argv[1], "read") == 0){
        char last[BLOCK_SIZE] = "";

        printf("Reader process started.\n");
        while(true){
            // 如果內容改變就印出
            if(strcmp(last, ptr) != 0){
                printf("Received: %s", ptr);
                strcpy(last, ptr); // updtae last string
            }
            usleep(100000); // 0.1 秒
        }
    }
	
	unlink(TEMP_FILE);
    munmap(ptr, FILE_SIZE); // process 結束時也會自動呼叫
    close(fd);
    return 0;
}
