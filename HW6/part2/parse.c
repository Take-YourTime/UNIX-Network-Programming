/*
 * parse.c : use whitespace to tokenise a line
 * Initialise a vector big enough
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

/* Parse a commandline string into an argv array. */
/* Return the number of arguments parsed */
char** parse(char* line, int* parse_argc) {

  	static char delim[] = " \t\n"; /* SPACE or TAB or NL */
  	int count = 0;
  	char* token;
	char** newArgv = NULL;

  	/* Nothing entered. */
  	if (line == NULL || strcmp(line,"\n")==0) {
    	*parse_argc = 0;
    	return NULL;
  	}

  	/* Init strtok with commandline, then get first token.
     * Return NULL if no tokens in line.
	 *
	 * Fill in code.
     */
	token = strtok(line, delim);
    if (token == NULL) {
        return NULL;
    }

  	/* While there are more tokens...
	 *
	 *  - Get next token.
	 *	- Resize array.
	 *  - Give token its own memory, then install it.
	 * 
  	 * Fill in code.
	 */
	while (token != NULL) {
        newArgv = (char**)realloc(newArgv, sizeof(char *) * (count + 1));
        if (newArgv == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        newArgv[count] = (char*)malloc(strlen(token) + 1);
        if (newArgv[count] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        strcpy(newArgv[count], token);
        count++;

        token = strtok(NULL, delim);
    }


  	/* Null terminate the array and return it.
	 *
  	 * Fill in code.
	 */
	newArgv = (char**)realloc(newArgv, sizeof(char*) * (count + 1));
    if (newArgv == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    newArgv[count] = NULL;

	*parse_argc = count;
    return newArgv;
}


/*
 * Free memory associated with argv array passed in.
 * Argv array is assumed created with parse() above.
 */
void free_argv(char** oldArgv) {

	int i = 0;

	/* Free each string hanging off the array.
	 * Free the oldArgv array itself.
	 *
	 * Fill in code.
	 */
	if (oldArgv == NULL) {
        return;
    }

    while (oldArgv[i] != NULL) {
        free(oldArgv[i]);
        i++;
    }
    free(oldArgv);
}
