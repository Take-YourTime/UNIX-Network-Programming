/*
 * shell.c  : test harness for parse routine
 */


#define LONGLINE 255

#include <stdio.h>
#include <stdlib.h>
#include "shell.h"

int main() {
    char line[LONGLINE];
    int i;
    char** myArgv = NULL;

    fputs("myshell -> ", stdout);
    while (fgets(line, LONGLINE, stdin)) {

        /* Create argv array based on commandline. */
        myArgv = parse(line);
       	
        if(myArgv != NULL && myArgv[0] != NULL) { // ignore the commands that only have space or '\n'
            if (is_builtin(myArgv[0])) { /* If command is recognized as a builtin, do it. */
                do_builtin(myArgv);         
            } else {					/* Non-builtin command. */
	            run_command(myArgv);
            }

            free_argv(myArgv);			/* Free argv array. */
        }

        fputs("myshell -> ", stdout);
    }
    exit(0);
}
