/*
 * parse.c : use whitespace to tokenise a line
 * Initialise a vector big enough
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

/* Parse a commandline string into an argv array. */
char** parse(char* line) {

	static char delim[] = " \t\n"; /* SPACE or TAB or NL */
	int count = 0;
	char* token;
	char** newArgv = NULL;
	
	/* Nothing entered. */
	if (line == NULL || strlen(line) == 0) {
		return NULL;
	}

  	/* Init strtok with commandline, then get first token.
     * Return NULL if no tokens in line.
	 *
	 * Fill in code.
     */
    token = strtok(line, delim);
	
	while (token != NULL) {
		/* Create array with room for first token.
	  	 *
		 * Fill in code.
		 */
		 
		// re-allocate the size of newArgv
		// the new size is count+1
	    newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
	    if(newArgv == NULL){ 
			perror("Realloc failed!\n");
			exit(EXIT_FAILURE);
		}
	
	    // allocate memory space to newArgv[count]
	    // the size is the length of "token"
	    newArgv[count] = (char *)malloc(strlen(token) + 1); // the size should +1, since there is a '\0' in c string, but strlen() doesn't count it.
	    if (newArgv[count] == NULL){
			perror("Malloc failured!\n");
			exit(EXIT_FAILURE);
		}
		
		// cpoy string from token to newArgv[count]
	    strcpy(newArgv[count], token);
		
		
		/* While there are more tokens...
		 *
		 *  - Get next token.
		 *	- Resize array.
		 *  - Give token its own memory, then install it.
		 * 
	  	 * Fill in code.
		 */
	    count++;
	    token = strtok(NULL, delim);
	}
	
	
	/* Null terminate the array and return it.
	 *
	 * Fill in code.
	 */
	
	// add NULL to the end of newArgv[], 
	// because if requirement execvp()
	newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
	newArgv[count] = NULL;
	
	return newArgv;
}


/*
 * Free memory associated with argv array passed in.
 * Argv array is assumed created with parse() above.
 */
void free_argv(char **oldArgv) {
	
	if(oldArgv == NULL) return;
	
	int i = 0;

	/* Free each string hanging off the array.
	 * Free the oldArgv array itself.
	 *
	 * Fill in code.
	 */
	
	while(oldArgv[i] != NULL){
        free(oldArgv[i]);
        i++;
    }
	free(oldArgv);
}
