# Homework 6

B113040045 許育菖

### Problem Definition

依照作業要求，使用 C 語言完成數個與 system programming、pipe、simple shell 以及 fixed-length record dictionary lookup 相關的程式。

---

### Objectives and Solutions

本次作業共分為三個部分，以下分別說明各部分的目標與實作方式。

#### **Part 1**

此部分主要練習基本的 system call 與 pipe 操作，需完成 `hostinfo`、`mydate`、`printdir`、`mycat` 以及 `pipe_ls`。

- **hostinfo**

    **目標**：印出目前 system 的 hostname、kernel release 與 hostid。

    **實做方法**：使用 `uname()` 取得系統資訊，並取出其中的 `nodename` 與 `release` 欄位，再搭配 `gethostid()` 取得 hostid，最後依照指定格式輸出。

    **資料結構與函數：**

    ```c
    struct utsname {
        char sysname[];   // 作業系統名稱
        char nodename[];  // 主機名稱（hostname）
        char release[];   // Kernel Release
        char version[];   // Kernel Version
        char machine[];   // 硬體架構，e.g. x86_64
    };
    ```

- **mydate**

    **目標**：印出目前日期與時間。

    **實做方法**：使用 `time()` 取得目前時間，並透過 `localtime()` 轉換成當地時間結構，再利用 `strftime()` 將日期與時間格式化為題目要求的輸出形式。

    **資料結構與函數：**

    ```c
    struct tm {
        int tm_sec;    // 秒
        int tm_min;    // 分
        int tm_hour;   // 時
    
        int tm_mday;   // 日期
    
        int tm_mon;    // 月份(0~11)
        int tm_year;   // 自1900起算
        
        int tm_wday;   // 星期(0~6)
        int tm_yday;   // 一年中的第幾天
    
        int tm_isdst;  // 日光節約時間
    };
    ```

    ```c
    // 獲取當前的時間，為自1970-01-01 00:00:00到現今的秒數
    time_t now;
    time(&now);
    	// or
    time_t now = time(NULL);
    
    // 將 Unix Timestamp (time_t) 轉成 struct tm
    struct tm* localtime(const time_t* timer);
    
    // 把 struct tm 轉成特定的字串來表示
    size_t strftime(
        char* output_buffer,
        size_t output_buffer_size,
        const char* format,
        const struct tm* tm);
    ```

    `strftime()` 輸出常用的格式符號：

    | 格式符號 | 意思 | 範例 |
    | --- | --- | --- |
    | `%Y` | 四位數年份 | 2026 |
    | `%y` | 兩位數年份 | 26 |
    | `%m` | 月份 | 06 |
    | `%d` | 日期 | 08 |
    | `%H` | 小時(24hr) | 14 |
    | `%M` | 分鐘 | 35 |
    | `%S` | 秒 | 20 |
    | `%a` | 星期縮寫 | Mon |
    | `%A` | 星期全名 | Monday |
    | `%b` | 月份縮寫 | Jun |
    | `%B` | 月份全名 | June |

- **printdir**

    **目標**：印出目前工作目錄。

    **實做方法**：使用 `getcwd()` 取得目前工作目錄，並採用動態配置的方式取得足夠大小的 buffer，而不是直接呼叫 `pwd()`。

- **mycat**

    **目標**：實作簡化版的 `cat`，輸入一個檔案名稱並將其內容印出。

    **實做方法**：先檢查 command line argument 是否正確，接著建立 fd 並使用 read() 讀取指定檔案，再使用 write 將讀取內容輸出到 `STDOUT_FILENO`

- **pipe_ls**

    **目標**：練習 `pipe()` 與 `dup()` / `dup2()`，並將 `ls` 的輸出透過 pipe 傳回程式本身再印到螢幕。

    **實做方法**：先建立 pipe，之後使用 `fork()` 建立子程序。子程序將標準輸出重新導向到 pipe 的寫入端，接著使用 `exec()` 執行 `ls`；父程序則從 pipe 的讀取端讀取資料，再寫到自己的標準輸出。

    **Note — 使用 `read()` 讀取 `pipe()` 的不同情況：**

    | pipe 狀態 | `read()` 回傳結果 |
    | --- | --- |
    | pipe 有資料 | 讀取資料並回傳讀到的 byte 數 |
    | pipe 沒資料，但還有 write end 開著 | blocking 等待資料 |
    | pipe 沒資料，而且所有 write end 都關閉 | 回傳 `0`，代表 EOF |
    | 發生錯誤 | 回傳 `-1` |

#### **Part 2**

此部分主要是在題目提供的 simple shell 架構上，完成 `cd`、`pwd`、`id`、`hostname`和`builtin` …等等 builtin commands，並透過修改 `redirect_in.c` 、`redirect_out.c` 、`pipe_command.c` 、`pipe_present.c` 來實現指令中的 redirect `<`, `>` 和 pipe 功能 — `|`

*( e.g. `ls -la | grep "HW”` 用於擷取當前資料夾中，名稱帶有「HW」的檔案。)*

- **run_command.c**

    **目標**：統整 external command 的執行流程。

    **實做方法**：先透過 `is_background.c` 檢查是否為 background command，接著使用 `fork()` 建立子程序，如果剛剛檢查結果是 background command，那父程序不需要等待子程序回傳結果；否則使用 `waitpid()` 等待子程序結束。

    我們在子程序中分別構過`redirect_in.c`、`redirect_out.c`、`pipe_command.c`依序處理 input redirection、output redirection 與 pipe，細節詳見各檔案。

- **parse.c**
- **is_background.c**

    **實做方法**：與 Homework 4 相同，請見 Homework 4 的 `parse.c` 和 `is_background.c`。

- **builtin.c**

    **目標**：讓 shell 能辨識並執行 `cd`、`pwd`、`id`、`hostname` 與 `builtin` 等內建指令。

    **實做方法**：建立 builtin command lookup table，當輸入指令的第一個 token 與 table 中的 keyword 相符時，即呼叫對應函式執行。

    `cd` 使用 `chdir()` 改變目錄；

    `pwd` 使用 `getcwd()` 印出目前路徑；

    `id` 透過 `getuid()`、`getgid()` 與對應資料結構取得使用者與群組資訊；

    `hostname` 使用 `uname()` 取得主機名稱；

    `builtin` 則用來判斷某一指令是否為 shell 內建功能；若 builtin 後沒有待判斷的指令 *(即參數數量為1)*，則印出所有的 builtin-commands。

- **redirect_in.c**

    **目標**：實作輸入重導向 `<`，能將`<`後的檔案作為輸入，*e.g. `./shell < test.txt`* 。

    **實做方法**：檢查 argv 中是否存在 `<`，若有則開啟其後指定的檔案，並以 `dup2()` 將其重新導向到標準輸入，最後將 `<` 與檔名從 argv 中移除。

- **redirect_out.c**

    **目標**：實作輸出重導向 `>`，能將指令執行結果輸出到`>`後的檔案，*e.g. `ls > output.txt`* 。

    **實做方法**：搜尋 argv 中是否存在 `>`，若有則建立或截斷目標檔案，並以 `dup2()` 將其重新導向到標準輸出，最後將 `>` 與檔名從 argv 的中移除。

- **pipe_present.c**

    **目標**：檢查 command line 中是否包含 pipe symbol `|`。

    **實做方法**：逐一掃描 argv，若找到 `|` 則回傳其 index；若出現在不合法的位置 *(例如：一開始或最後)*，則回傳錯誤狀態。

- **pipe_command.c**

    **目標**：支援 pipe command。

    **實做方法**：先透過`pipe_present.c`偵測command中是否有`|`，當 argv 中存在 `|` 時，先將指令切成左右兩部分，再建立 pipe 並 `fork()` left pid 與 right pid。left pid 將輸出導向 pipe write-end；right pid 則將輸入接到 pipe read-end，並遞迴執行剩餘的 command，以完成 `cmd1 | cmd2 | cmd3` 的效果。

#### **Part 3**

本部分主要實作 dictionary lookup project，先將原始 dictionary 轉成 fixed-length record file，再以 linear search 的方式查詢單字。

- **convert.c**

    **目標**：將可編輯的 variable-length dictionary file 轉成 fixed-length record format。

    **實做方法**：逐筆讀取 dictionary 檔案，先讀入單字作為 record 的 `word` 欄位，再持續讀取該單字的 definition 直到空白行為止，並將內容存入 `text` 欄位。每筆資料都以固定大小的 `Dictrec` 寫入輸出檔，使輸出檔成為 fixed-length records。
- **lookup1.c**

    **目標**：對 fixed-length record file 進行 simple linear search。

    **實做方法**：第一次呼叫時先開啟指定的 resource file，之後每次查詢都從檔案開頭開始，以 `fread()` 一筆一筆讀取 `Dictrec`，並用 `strcmp()` 比對 `word` 欄位。若找到相符單字，便將其 definition 複製回查詢結構並回傳 `FOUND`；若掃描完整個檔案仍找不到，則回傳 `NOTFOUND`。

---

### Execution Result

#### Part 1

![part1.png](demo%20image/part1.png)

#### Part 2

![part2-1](demo%20image/part2-1.png)

![part2-2.png](demo%20image/part2-2.png)

#### Part 3

![part3.png](demo%20image/part3.png)

---

### Testing Steps

1. Open terminal
2. Enter the corresponding part directory
3. Type `make` to compile the source files

- **Part 1**
    1. Type `./hostinfo`
    2. Type `./mydate`
    3. Type `./printdir`
    4. Type `./mycat 123`
    5. Type `./pipe_ls`
- **Part 2**
    1. `make`
    2. Type `./myshell` to start the simple shell
    3. Or type`./myshell < cmd` to show the auto execution result
- **Part 3**
    1. Type `make test`
    2. Check whether `./convert dict myfixrec` creates the fixed-length record file correctly
    3. Check whether `./file_lookup myfixrec` can search words correctly and return `Not Found!` for missing words
