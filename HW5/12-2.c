#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>

#define MAX_PATH_LENGTH 400


struct ProcessNode {
    pid_t pid;
    pid_t ppid;
    char command[50];
    
    struct ProcessNode* next; // list pointer
    
    struct ProcessNode* child; // tree pointer, point to child node
    struct ProcessNode* sibling; // list pointer, point to sibling
};

struct ProcessNode* tail = NULL; // node list head
struct ProcessNode* root = NULL; // tree root

// create a new node
void createNode(pid_t pid, pid_t ppid, const char* cmd)
{
	struct ProcessNode* newNode = (struct ProcessNode*)malloc(sizeof(struct ProcessNode));
	
	if(newNode == NULL){
		printf("Maccloc failed\n");
		exit(EXIT_FAILURE);
	}
	
	newNode->pid = pid;
	newNode->ppid = ppid;
    strncpy(newNode->command, cmd, 50);
    // update list
    newNode->next = tail;
    tail = newNode;
    
    newNode->child = NULL;
    newNode->sibling = NULL;
}


// find a node by pid
// return: ProcessNode
struct ProcessNode* find(pid_t pid)
{
	struct ProcessNode* current = tail;
	while(current != NULL)
	{
		if(current->pid == pid) return current;
		
		current = current->next;
	}
	
	return NULL;
}


void deleteNodeList(void)
{
	struct ProcessNode* current = tail;
	while(current != NULL)
	{
		tail = tail->next;
		free(current);
		current = tail;
	}
}


// get digit count of a number
int digitLength(int n)
{
	if(n == 0) return 1;
	
	int count = 0;
	while(n > 0){
		n /= 10;
		count++;
	}
	return count;
}

// print process tree by DFS
void printTree(struct ProcessNode* node, int level)
{
	if(node == NULL) return; // double check, this line can be deleted
	
	printf("%s,[%d]", node->command, node->pid);
	
	// goto child node
	if(node->child != NULL){
		printf("-----");
		int length = strlen(node->command) + 3 + digitLength(node->pid);
		printTree(node->child, level+length+5);
	}
	
	// goto sibling node
	if(node->sibling != NULL){
		printf("\n");
		for(int i = 0; i < level-3; i++) printf(" ");
		if(level > 3) printf("|--");
		
		printTree(node->sibling, level);
	}
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
	
	
	//struct stat buf;
	DIR* dirPtr;
	if( (dirPtr = opendir("/proc")) == NULL){
		perror("opendir");
		exit(EXIT_FAILURE);
	}
	
	
	struct dirent* entry = NULL;
	
	
	// go through every entry, and create a node for it
	// then record the nodes in list
	while((entry = readdir(dirPtr)) && entry != NULL)
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
		char cmdName[50]; // command name
		int ppid; // parent pid
		int count = 0; // use to count the information get and break the while loop 
		
		while(count < 2 && fgets(line, sizeof(line), fp))
		{
			if (strncmp(line, "Name:", 5) == 0) {
		        sscanf(line, "Name:\t%s", cmdName);
		        count++;
		    }
		    else if (strncmp(line, "PPid:", 5) == 0) {
		        sscanf(line, "PPid:\t%d", &ppid);
		        count++;
		    }
		}
		
		if(count == 2) createNode(pid, ppid, cmdName);
		
		fclose(fp);
	}
	closedir(dirPtr);
	
	
	struct ProcessNode* current = tail;
	struct ProcessNode* parent = NULL;
	
	// go through the node list, and find the parent node of each node by its ppid
	while(current != NULL)
	{
		// record the root of tree
		if(current->pid == 1) root = current;
	
		parent = find(current->ppid);
		if(parent != NULL){
			current->sibling = parent->child;
			parent->child = current;
		}
		current = current->next;
	}
	
	// use DFS to print the process tree
	if(root != NULL){
		printTree(root, 0);
		printf("\n");
	}
	else{
		printf("Tree root is NULL!\n");
	}
	
	deleteNodeList();
	return 0;
}
