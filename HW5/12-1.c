#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>

#define MAX_PATH_LENGTH 400


// input: user name
// return user id
int userIdFromName(const char *name)
{
	struct passwd* pwd; // 用來存放從系統資料庫 (/etc/passwd) 抓回來的資訊
	uid_t u;
	char* endptr;
	
	// On NULL or empty string, return an error
	if(name == NULL || *name == '\0') return -1;
	
	// "name" is a number
	u = strtol(name, &endptr, 10);
	if(*endptr == '\0') return u; // just retunr it as uid
	
	
	// "name" is a user name
	pwd = getpwnam(name);
	if(pwd == NULL) return -1;
	
	return pwd -> pw_uid;
}



int main(int argc, char* argv[])
{
	int opt; // options
	
	// get options
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
		    default:
		    	break;
		}
	}
	
	// check remaining arguments
	if(optind >= argc){
		printf("Usage: %s <user name>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	
	//struct stat buf;
	DIR* dir_ptr;
	if( (dir_ptr = opendir("/proc")) == NULL){
		perror("opendir");
		exit(EXIT_FAILURE);
	}
	
	// the objective uid
	const int key_uid = userIdFromName(argv[1]);
	if(key_uid == -1){
		printf("Can't find user: %s\n", argv[1]);
		exit(EXIT_SUCCESS);
	}
	
	
	printf("[pid] [command name]\n");
	
	struct dirent* entry = NULL;
	
	while((entry = readdir(dir_ptr)) && entry != NULL)
	{
		// check if the name of current entry is pid
 		char* endptr;
		pid_t pid = strtol(entry->d_name, &endptr, 10);
		if(*endptr != '\0') continue; // not pid
		
		// get the path of status file 
		char path[MAX_PATH_LENGTH];
		snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
		
		// open files
		FILE* fp = fopen(path, "r");
		if(fp == NULL){
			// Handling the situation where a folder suddenly disappears
        	// In this case, just ignore it
			continue;
		}
		
		
		char line[1024]; // read buffer
		char cmd_name[50]; // command name
		int uid; // user id
		int is_found_uid = 0; // flag: find uid or not
		int count = 0; // use to count the information get and break the while loop 
		
		// get command name
		while(count < 2 && fgets(line, sizeof(line), fp))
		{
			if (strncmp(line, "Name:", 5) == 0) {
		        sscanf(line, "Name:\t%s", cmd_name);
		        count++;
		    }
		    else if (strncmp(line, "Uid:", 4) == 0) {
		        sscanf(line, "Uid:\t%d", &uid);
		        is_found_uid = 1;
		        count++;
		    }
		}
		
		fclose(fp);
		
		// output
		if(is_found_uid && uid == key_uid){
			printf("%s  %s\n", entry->d_name, cmd_name);
		}
	}
	
	
	closedir(dir_ptr);
	return 0;
}
