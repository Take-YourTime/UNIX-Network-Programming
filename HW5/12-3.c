#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_LINK_LENGTH 1024
#define MAX_PATH_LENGTH 512
int main(int argc, char* argv[])
{
	// too less arguments
    if(argc < 2){
        printf("Usage: %s <file pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
	
    DIR* proc_dir_ptr;
    if((proc_dir_ptr = opendir("/proc")) == NULL){
        perror("opendir /proc");
        exit(EXIT_FAILURE);
    }
    
    
    // turn the target file path into real path
    char target_realpath[MAX_LINK_LENGTH]; // target file's real path
    if (realpath(argv[1], target_realpath) == NULL) {
        perror("realpath");
        exit(EXIT_FAILURE);
    }
    printf("Finding file with path %s\n", target_realpath);


    struct dirent* proc_entry = NULL;

    // go through all the entry in /proc
    while((proc_entry = readdir(proc_dir_ptr)) != NULL)
    {
        // check if the entry is a pid (number)
        char* endptr;
        pid_t pid = strtol(proc_entry->d_name, &endptr, 10);
        if(*endptr != '\0') continue; // not pid, justignore

        // get path "proc/PID/fd"
        char fd_dir_path[MAX_PATH_LENGTH];
        snprintf(fd_dir_path, sizeof(fd_dir_path), "/proc/%s/fd", proc_entry->d_name);

        
        DIR* fd_dir_ptr = opendir(fd_dir_path);
        if(fd_dir_ptr == NULL) {
            // permission deny
            perror("opendir");
            printf("Failed to open path : %s\n", fd_dir_path);
            continue;
        }
		
		
        struct dirent* fd_entry = NULL;
        
        // scan all the file at /proc/PID/fd
        while((fd_entry = readdir(fd_dir_ptr)) != NULL)
        {
            // ignore "." and ".."
            if (strcmp(fd_entry->d_name, ".") == 0 || strcmp(fd_entry->d_name, "..") == 0)
                continue;

            // concat path "proc/PID/fd/n"
            char link_path[MAX_LINK_LENGTH];
            snprintf(link_path, sizeof(link_path), "%s/%s", fd_dir_path, fd_entry->d_name);

            // use readlink to get the true link
            char real_path[MAX_LINK_LENGTH];
            ssize_t len = readlink(link_path, real_path, sizeof(real_path) - 1);
            
            // match the link
            if (len != -1) {
                real_path[len] = '\0'; // add '\0', since readlink() will not add '\0' automatically

                if (strcmp(real_path, target_realpath) == 0) {
                    printf("Found! PID: [%s] opened %s\n", proc_entry->d_name, target_realpath);
                }
            }
        }
        closedir(fd_dir_ptr);
    }

    closedir(proc_dir_ptr);
    return 0;
}
