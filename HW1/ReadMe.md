# Homework 1

B113040045 許育菖

### **Q1: Who is the author of bash shell?**

Ans: Its author is Brian Fox, but I can’t use `man bash` to find the author name.

I find the answer from wikipedia - [https://en.wikipedia.org/wiki/Bash_(Unix_shell)](https://en.wikipedia.org/wiki/Bash_(Unix_shell)) .

![Screen shot of `man bash` .](image.png)

Screen shot of `man bash` .

### **Q2: What is the retrun type of the `read()` system call?**

Ans: `ssize_t` , it is a integer.

![Screen shot of `man read` .](image%201.png)

Screen shot of `man read` .

### Q3: Using the man pages, find the names of all of the header files that you would need to include to use the following functions in a program. There might be more than one needed for some of these.

Ans: Use `man` + each function name to get the include library information.

1. `_exit()` : `#include <unistd.h>`
    
    ![image.png](image%202.png)
    
2. `setuid()` : `#include<sys/types.h>`  `#include <unistd.h>`
    
    ![image.png](image%203.png)
    
3. `fstat()` : `#include<sys/types.h>`  `#include <sys/stat.h>`  `#include <unistd.h>`  
    
    ![image.png](image%204.png)
    

### Q4: If your current working directory is */usr/share/gcc/pyhton*, what is the shortest relative pathname of the file */usr/lib32/libc.so.6* ?

Ans: *../../../lib32/libc.so.6* 

### Q5: What command can be used to print the creation date of a file?

Ans: `stat <filename>` or `stat -c %w <filename>`, the creation date of file will be show behind “Birth” text.

However, some filesystems don’t support recording the creation date of files, if the filesystem doesn’t support this function, it will show “-” behind “Birth” text.

![Use `stat` command to show the creation date of a file.](image%205.png)

Use `stat` command to show the creation date of a file.