/*
 * builtin.c : check for shell built-in commands
 * structure of file is
 * 1. definition of builtin functions
 * 2. lookup-table
 * 3. definition of is_builtin and do_builtin
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"




/****************************************************************************/
/* builtin function definitions                                             */
/****************************************************************************/

/* "echo" command.  Does not print final <CR> if "-n" encountered. */
static void bi_echo(char **argv) {
  	/* Fill in code. */
  	
	int i; // the index of argv[]
	int newLine; // Flag: print \n or not
	
	// option -n
	if(argv[1] != NULL && strcmp(argv[1], "-n") == 0){
		newLine = 0;
		i = 2;
	}
	else{
		newLine = 1;
		i = 1;
	}
	
	while(argv[i] != NULL){
		printf("%s", argv[i]);
		if(argv[i+1] != NULL) printf(" ");
		
		i++;
	}
	
	if(newLine) printf("\n");
}

/* Fill in code. */

/* "cd" command */
static void bi_cd(char** argv){
    if (argv[1] == NULL){
        // 如果只打 cd，通常回到 HOME
        char *home = getenv("HOME");
        if(home != NULL) chdir(home);
    }
	else{
        if(chdir(argv[1]) == -1){
            perror("cd failed!\n");
        }
    }
}

/* "exit" command */
static void bi_exit(char** argv) {
    exit(EXIT_SUCCESS);
}

/* "help" command */
static void bi_help(char** argv){
	printf("========== Shell Guildline ==========\n");
	printf("This shell is a homework assignment of course, Network Applications Programming, in NSYSU.\n");
	printf("It can run some simple Linux external commands and built-in commands, including \"help\", \"cd\", \"exit\", and \"echo\".\n");
	printf("If there are some commands, such as \"ls -a *.c\", cannot run correctly, it is normal, since this shell is just a simple version.\n");
	printf("Remember to type \"exit\" to end this shell when you want to exit.\n");
	printf("Wish you have fun with it~\n");
	printf("-\n");
	printf("The author is Hsu Yu-Chang, whose student ID number is B113040045.\n");
	printf("Creating date: 2026/03/29\n");
	printf("========== =============== ==========\n");
}


/****************************************************************************/
/* lookup table                                                             */
/****************************************************************************/

static struct cmd {
	char* keyword;				/* When this field is argv[0] ... */
	void (* do_it)(char **);	/* ... this function is executed. */
} inbuilts[] = {

	/* Fill in code. */

	{ "echo", bi_echo },		/* When "echo" is typed, bi_echo() executes.  */
    { "cd",   bi_cd   },
    { "exit", bi_exit },
    { "help", bi_help },
	{ NULL, NULL }				/* NULL terminated. */
};




/****************************************************************************/
/* is_builtin and do_builtin                                                */
/****************************************************************************/

static struct cmd * this; 		/* close coupling between is_builtin & do_builtin */

/* Check to see if command is in the inbuilts table above.
Hold handle to it if it is. */
int is_builtin(char *cmd) {
	struct cmd *tableCommand;
	
	for (tableCommand = inbuilts ; tableCommand->keyword != NULL; tableCommand++)
		if (strcmp(tableCommand->keyword,cmd) == 0) {
			this = tableCommand;
			return 1;
		}
	return 0;
}


/* Execute the function corresponding to the builtin cmd found by is_builtin. */
int do_builtin(char** argv) {
	if(argv == NULL) return 1;
	
	this->do_it(argv);
	return 0;
}
